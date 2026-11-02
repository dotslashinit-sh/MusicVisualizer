#include "VisualVisualizer.hpp"

#include "yaml-cpp/yaml.h"

#include <exception>
#include <print>
#include <vector>

std::vector<BarConfig_t> GenNConfigs(float start, float end, size_t n, const float multiplier = 1.0f) {
    std::vector<BarConfig_t> ret;
    const float width = (end-start)/n;
    while(n > 0) {
        ret.emplace_back(start, start+width, multiplier);
        n--;
        start += width;
    }
    return ret;
}

int main() {
    // VisualizerConfig_t config = {
    //     .fftSize = 2048,
    //     .decayConstant = 0.9f,
    //     .decayThreshold = 0.0f,
    //     .decayVelocity = 0.0f,
    //     .heightCalcFunc = DecayHeightCalc
    // };
    VisualizerConfig_t config = {
        .fftSize = 2048,
        .decayConstant = 0.0f,
        .decayThreshold = 0.0f,
        .decayVelocity = 5.0f,
        .heightCalcFunc = FallVelocityHeightCalc
    };
    // const int BAR_COUNT = 200;
    // const float barFreqGap = (20000.0f - 20.0f)/config.fftSize;
    // for(int i = 1; i < BAR_COUNT+1; i++) {
    //     config.barConfig.emplace_back(i*barFreqGap, (i+1)*barFreqGap, 5.0f);
    // }

    // config.barConfig = {
    //     // Sub bass
    //     BarConfig_t(20.0f, 60.0f, 1.0f),

    //     // Bass
    //     GenNConfigs(61.0f, 250.0f, 10, 1.0)...

    //     // Low mid range
    //     BarConfig_t(251.0f, 124.0f, 1.0f),
    //     BarConfig_t(125.0f, 188.0f, 1.0f),
    //     BarConfig_t(189.0f, 250.0f, 1.0f),
    // };

    const auto subBass = GenNConfigs(20.0f, 60.0f, 2, 3.0f);
    const auto bass = GenNConfigs(61.0f, 250.0f, 5, 3.0f);
    const auto lowMidRange = GenNConfigs(251.0f, 500.0f, 10, 3.0f);
    const auto midRange = GenNConfigs(501.0f, 2000.0f, 10, 3.0f);
    const auto upperMidRange = GenNConfigs(2001.0f, 4000.0f, 4, 3.0f);
    const auto presence = GenNConfigs(4001.0f, 6000.0f, 6, 2.0f);
    const auto brilliance = GenNConfigs(6001.0f, 19000.0f, 8, 2.0f);

    config.AppendConfigs(subBass);
    config.AppendConfigs(bass);
    config.AppendConfigs(lowMidRange);
    config.AppendConfigs(midRange);
    config.AppendConfigs(upperMidRange);
    config.AppendConfigs(presence);
    // config.AppendConfigs(brilliance);

    try {
        VisualVisualizer* engine = new VisualVisualizer(config, 1.0f);
        WindowConfig windowCfg = {
            .width = 800,
            .height = 600,
            .title = "Music Visualizer"
        };
        engine->CreateWindow(windowCfg);
        engine->SetupGraphics();
        engine->Display();
        delete engine;
    } catch(const std::exception& ex) {
        std::print("{}", ex.what());
        return 1;
    }

    return 0;
}