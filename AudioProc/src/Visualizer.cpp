#include "Visualizer.hpp"

#include "yaml-cpp/yaml.h"

#include <exception>
#include <string>
#include <format>
#include <cmath>
#include <algorithm>
#include <print>
#include <numbers>
#include <fstream>

std::vector<BarConfig_t> VisualizerConfig_t::GenNConfigs(float start, float end, size_t n, const float multiplier) {
    std::vector<BarConfig_t> ret;
    const float width = (end-start)/n;
    while(n > 0) {
        ret.emplace_back(start, start+width, multiplier);
        n--;
        start += width;
    }
    return ret;
}

Status_t VisualizerConfig_t::ParseYamlFile(const std::string& yamlFile, VisualizerConfig_t& cfg) {
    YAML::Node config;
    cfg = {};
    try {
        config = YAML::LoadFile(yamlFile);

        const auto decayConstantN = config["decayConstant"];
        const auto decayThresholdN = config["decayThreshold"];
        const auto decayVelocityN = config["decayVelocity"];
        const auto barConfigN = config["barConfig"];
        const auto heightCalcFuncN = config["heightCalcFunc"];

        if(!heightCalcFuncN) {
            return GenError(ERROR_CODE_MISSING_PARAMS,
                "Missing 'heightCalcFunc' key in YAML file!"
            );
        } else {
            const auto heightCalcFuncName = heightCalcFuncN.Scalar();
            if(!heightCalcFuncName.compare("DecayHeight")) {
                cfg.heightCalcFunc = DecayHeightCalc;
            } else if (!heightCalcFuncName.compare("ConstantFallVelocity")) {
                cfg.heightCalcFunc = ConstantFallVelocity;
            }
        }
        cfg.fftSize = config["fftSize"].as<size_t>(2048);
        if(decayConstantN) {
            cfg.decayConstant = decayConstantN.as<float>(0.0f);
        }
        if(decayThresholdN) {
            cfg.decayThreshold = decayThresholdN.as<float>(0.0f);
        }
        if(decayVelocityN) {
            cfg.decayVelocity = decayVelocityN.as<float>(0.0f);
        }

        if(!barConfigN) {
            return GenError(ERROR_CODE_MISSING_PARAMS,
                "No 'barConfig' found in YAML config!"
            );
        }
        else {
            for(const auto& bc : barConfigN) {
                const auto type = bc["type"];
                const auto typeStr = type.Scalar();
                if(!type) {
                    return GenError(ERROR_CODE_MISSING_PARAMS,
                        std::format("Missing key 'type' in a bar in YAML config!")
                    );
                }

                const auto startN = bc["start"];
                const auto endN = bc["end"];
                const auto mutliplierN = bc["multiplier"];

                if(!startN || !endN || !mutliplierN) {
                    return GenError(ERROR_CODE_MISSING_PARAMS,
                        "Missing start, end or multiplier parameters in YAML bar config!"
                    );
                }

                if(!typeStr.compare("multi")) {
                    const auto countN = bc["count"];
                    cfg.AppendConfigs(
                        GenNConfigs(
                            startN.as<float>(), endN.as<float>(),
                            countN.as<size_t>(), mutliplierN.as<float>()
                        )
                    );
                } else if(!typeStr.compare("single")) {
                    cfg.barConfig.emplace_back(
                        startN.as<float>(), endN.as<float>(),
                        mutliplierN.as<float>()
                    );
                }
            }
        }
    }
    catch(const YAML::ParserException& ex) {
        return GenError(ERROR_CODE_YAML_PARSE_FAIL,
            std::format("Failed to parse YAML file with error: {}", ex.msg)
        );
    }
    catch(const YAML::BadFile& ex) {
        return GenError(ERROR_CODE_FILE_OPEN_FAIL,
            std::format("Failed to open YAML file \"{}\"!", yamlFile)
        );
    }
    catch(const std::exception& ex) {
        return GenError(ERROR_CODE_UNKNOWN,
            ex.what()
        );
    }

    return STATUS_OK;
}

const VisualizerConfig_t VisualizerConfig_t::GetDefaultConfig() {
    // VisualizerConfig_t defaultConfig = {
    //     .fftSize = 2048,
    //     .decayConstant = 0.9f,
    //     .decayThreshold = 0.0f,
    //     .decayVelocity = 0.0f,
    //     .heightCalcFunc = DecayHeightCalc
    // };
    VisualizerConfig_t defaultConfig = {
        .fftSize = 2048,
        .decayConstant = 0.0f,
        .decayThreshold = 0.0f,
        .decayVelocity = 5.0f,
        .heightCalcFunc = ConstantFallVelocity
    };

    const auto subBass = GenNConfigs(20.0f, 60.0f, 2, 3.0f);
    const auto bass = GenNConfigs(61.0f, 250.0f, 5, 3.0f);
    const auto lowMidRange = GenNConfigs(251.0f, 500.0f, 10, 3.0f);
    const auto midRange = GenNConfigs(501.0f, 2000.0f, 10, 3.0f);
    const auto upperMidRange = GenNConfigs(2001.0f, 4000.0f, 4, 3.0f);
    const auto presence = GenNConfigs(4001.0f, 6000.0f, 6, 2.0f);
    const auto brilliance = GenNConfigs(6001.0f, 19000.0f, 8, 2.0f);

    defaultConfig.AppendConfigs(subBass);
    defaultConfig.AppendConfigs(bass);
    defaultConfig.AppendConfigs(lowMidRange);
    defaultConfig.AppendConfigs(midRange);
    defaultConfig.AppendConfigs(upperMidRange);
    defaultConfig.AppendConfigs(presence);
    defaultConfig.AppendConfigs(brilliance);
    return defaultConfig;
}

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
            // const auto heightDecay = m_prevHeights[i] * m_config.decayConstant;
            // const float heightDelta = heightDecay - m_heights[i];
            // m_heights[i] = (heightDelta > m_config.decayThreshold) ? 
            //     heightDecay : m_heights[i];
            // m_heights[i] = (heightDelta > m_config.decayVelocity) ? 
            m_heights[i] = m_config.heightCalcFunc(&m_config, m_heights[i], m_prevHeights[i]);
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

float DecayHeightCalc(const VisualizerConfig_t* cfg, const float height, const float prevHeight) {
    const auto heightDecay = prevHeight * cfg->decayConstant;
    const float heightDelta = heightDecay - height;
    return (heightDelta > cfg->decayThreshold) ? heightDecay : height;
}

float ConstantFallVelocity(const VisualizerConfig_t* cfg, const float height, const float prevHeight) {
    const auto heightDelta = prevHeight - height;
    return (heightDelta > cfg->decayVelocity) ? (prevHeight-cfg->decayVelocity) : height; 
}