#pragma once

#include "AudioStream.hpp"
#include "AudioBuffer.hpp"
#include "ErrorReporter.hpp"

#include <fftw3.h>

#include <tuple>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>

/// First - starting frequency of the bar (inclusive).
/// Second - ending frequency of the bar (inclusive).
/// Third - multiplier for the height.
/// Note: The exact accuracy of the frequency component depends on the fft size and is not always guaranteed!
///  This means that some amount of spectral leakage will occur. For example, if the start frequency is 20kHz, 
///  and we are working on stream with sample rate 44.1kHz, with FFT size 2048, then the bin width will be almost 21.533Hz,
///  which means the 20kHz frequency bin will contain component from 19.982Khz (approx).
typedef std::tuple<float, float, float> BarConfig_t;

struct VisualizerConfig_t;

// Type of callback function used to calculate height.
// Structure of function type:
// Parameters (in the order they appear):
//  `const float VisualizerConfig_t&`: Visualizer configuration
//  `const float height`: Current height value
//  'const float prevHeight`: Previous height value
typedef std::function<float(const VisualizerConfig_t*, const float, const float)> HeightCalcFunc_t;

struct VisualizerConfig_t {
    // Size of the FFT (N)
    size_t fftSize;

    // Configuration for each bar in the order from left to right.
    std::vector<BarConfig_t> barConfig;

    // Max rate of bar fall.
    float decayConstant;

    // When new height value falls below this threshold, the height will decay.
    // The new height will be previous_height * decayConstant
    float decayThreshold;

    // Maximum rate of fall of height
    float decayVelocity;
    
    HeightCalcFunc_t heightCalcFunc;

    // Appends the configs inside `cfgList` into the end of `barConfig`.
    inline void AppendConfigs(const std::vector<BarConfig_t>& cfgList) {
        barConfig.insert_range(barConfig.end(), cfgList);
    }
};

/// @brief A visualizer object that coontinously reads the audio samples from the default audio output of the device
/// and generates the height of the bars at the specified isntance.
/// 
/// Usage: Create a `Visualizer` object with the required configuration. Call `Update()` to fetch audio samples
/// and calculate the FFT and heights, then call `FetchBarHeights()` to get the height of the bars. This must be reapeated
/// for each program loop. 
class Visualizer {
public:
    Visualizer(const VisualizerConfig_t& config);
    virtual ~Visualizer();

    // /// @brief Starts capturing audio and calculate height.
    // void Start();

    // /// @brief Stops capturing audio and height calculation.
    // void Stop();

    /// @brief Called every loop to update the height of the bars.
    void Update();

    /// @brief Copies the height of the bars to the output array.
    /// @param heightsOut The size of this array must be atleast as much as `fftSize` specified in config when creating
    ///  the visualizer.
    void FetchBarHeights(float* heightsOut);

private:
    float m_binWidth;
    const unsigned int m_fftOutSize;
    unsigned int m_sampleRate;
    AudioStream_t* m_stream;
    AudioBuffer_t m_buffer;
    VisualizerConfig_t m_config;
    ErrorReporter m_rep;

    fftwf_plan m_fftPlan;

    // Used internally by `_CalculateSpectrum` to calculate the FFT and store it.
    fftwf_complex* m_fftOut;

    // Contains duplicate values of `m_fftOut` which are used in the main loop for calculating height.
    fftwf_complex* m_fftOutCp;

    std::unique_ptr<std::thread> m_captureThreadPtr;
    std::mutex m_fftLock;
    std::vector<float> m_heights;
    std::vector<float> m_prevHeights;
    std::vector<float> m_windowCoeffs;
    std::atomic_bool m_shouldShutdown;

    /// @brief Worker function for updating audio buffer and calculating spectrum.
    void _CaptureWorker();

    /// @brief Updates audio the buffer, applies the Hanning window,
    ///  calculates the FFT of the audio samples in the buffer
    /// and then copies it to `m_fftOut` and `m_fftOutCp`
    void _CalculateSpectrum();

    /// @brief Calculates the height of the bars based on the bar configuration and fft output.
    ///  Must be called after `_CalculateSpectrum()`.
    void _CalculateHeight();

    /// @brief Precalculates the window coefficients that needs to be applied.
    void _PrecalculateWindowCoeffs();
};

float DecayHeightCalc(const VisualizerConfig_t* cfg, const float height, const float prevHeight);
float FallVelocityHeightCalc(const VisualizerConfig_t* cfg, const float height, const float prevHeight);