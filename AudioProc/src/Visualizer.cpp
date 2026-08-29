#include "Visualizer.hpp"

#include <exception>
#include <string>
#include <format>
#include <cmath>
#include <algorithm>
#include <print>
#include <numbers>

Visualizer::Visualizer(const VisualizerConfig_t& config): m_config(config), m_buffer(config.fftSize),
    m_captureThreadPtr(), m_fftLock(), m_shouldShutdown(false), m_rep(), m_fftOutSize(config.fftSize/2 + 1)
{
    Status_t status;
    
    // Cheap trick to check if FFT size is a power of 2.
    if(m_config.fftSize == 0 || (m_config.fftSize & (m_config.fftSize-1)) != 0) {
        throw std::runtime_error(std::format("Invalid FFT size{}! FFT size must be a power of 2!", m_config.fftSize));
    }

    m_fftOut = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_fftOutSize);
    m_fftOutCp = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_fftOutSize);
    
    if(m_fftOut == NULL || m_fftOutCp == NULL) {
        throw std::runtime_error(std::format("Failed to allocate memory for FFT output; FFT size: {}", m_config.fftSize));
    }

    m_fftPlan = fftwf_plan_dft_r2c_1d(m_config.fftSize, m_buffer.m_data.data(), m_fftOut, FFTW_MEASURE);
    if(m_fftPlan == NULL) {
        throw std::runtime_error("Failed to create FFT plan!");
    }
    
    m_stream = AllocAudioStream(&status);
    if(status != STATUS_OK) {
        const Error_t err = status.value();
        throw std::runtime_error(std::format("Failed to create audio stream! Error code: {}\r\nMsg: {}",
            static_cast<int>(err.Code), err.Message
        ));
    }
    m_sampleRate = m_stream->GetSampleRate();

    // Validate if all bar configurations are valid
    float startFreq;
    float endFreq;
    for(const auto& bar : m_config.barConfig) {
        startFreq = std::get<0>(bar);
        endFreq = std::get<1>(bar);
        if(startFreq > m_sampleRate/2 || endFreq > m_sampleRate/2 || 
            startFreq < 0.0f || endFreq < 0.0f ||
            startFreq > endFreq)
        {
            throw std::runtime_error(std::format("Invalid start and end frequencies"
                "in bar configuration! Start: {} | End: {} | Sample rate: {}", startFreq, endFreq, m_sampleRate));
        }
    }

    m_binWidth = static_cast<float>(m_sampleRate)/(m_config.fftSize);
    m_heights.resize(m_config.barConfig.size());
    m_prevHeights.resize(m_config.barConfig.size());
    float windowCoeff;
    const double w = 2.0 * std::numbers::pi/ (m_config.fftSize-1);
    for(int i = 0; i < m_config.fftSize; i++) {
        windowCoeff = static_cast<float>(0.5 * (1.0 - cos(w*i)));
        m_windowCoeffs.push_back(windowCoeff);
    }

    std::fill(m_prevHeights.begin(), m_prevHeights.end(), 0.0f);
    m_captureThreadPtr = std::make_unique<std::thread>(&Visualizer::_CaptureWorker, this);
}

void Visualizer::_CalculateSpectrum() {
    Status_t status;
    status = m_stream->UpdateAudioBuffer(&m_buffer);
    if(status != STATUS_OK) {
        m_rep.Report(status.value());
        return;
    }
    const auto& sz = m_config.fftSize; // Same as buffer size.
    for(int i = 0; i < sz; i++) {
        m_buffer[i] *= m_windowCoeffs[i];
    }
    fftwf_execute(m_fftPlan);
    {
        std::lock_guard lk(m_fftLock);
        // Copy fft output for processing.
        std::memcpy(m_fftOutCp, m_fftOut, m_fftOutSize * sizeof(fftwf_complex));
    }
}

void Visualizer::_CalculateHeight() {
    int i = 0;
    unsigned int index, endIndex;
    float multiplier;
    float real, imag;
    {
        std::lock_guard lk(m_fftLock);
        for(const auto& bar : m_config.barConfig) {
            index = static_cast<unsigned int>(std::floor(std::get<0>(bar)/m_binWidth));
            endIndex = static_cast<unsigned int>(std::ceil(std::get<1>(bar)/m_binWidth));
            multiplier = std::get<2>(bar);
            multiplier *= 10.0f;
            m_prevHeights[i] = m_heights[i];
            m_heights[i] = 0.0f;

            for(; index < endIndex + 1; index++) {
                real = m_fftOutCp[index][0];
                imag = m_fftOutCp[index][1];
                m_heights[i] += multiplier * std::log10f(real*real + imag*imag);
            }

            // Slow fall
            m_heights[i] = ((m_prevHeights[i] - m_heights[i]) > m_config.decayConstant) ? 
                (m_prevHeights[i]-m_config.decayConstant) : m_heights[i];
            
            i++;
        }
    }
}

void Visualizer::_CaptureWorker() {
    while(!m_shouldShutdown) {
        _CalculateSpectrum();
    }
}

void Visualizer::Update() {
    _CalculateHeight();
}

void Visualizer::FetchBarHeights(float* heights) {
    std::copy(m_heights.begin(), m_heights.end(), heights);
}

Visualizer::~Visualizer() {
    m_shouldShutdown = true;
    m_captureThreadPtr->join();
    fftwf_free(m_fftOut);
    fftwf_free(m_fftOutCp);
    delete m_stream;
}