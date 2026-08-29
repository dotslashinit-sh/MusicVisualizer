#include "VisualVisualizer.hpp"

#include <exception>
#include <print>
#include <vector>

int main() {
    VisualizerConfig_t config = {
        .fftSize = 2048,
        .decayConstant = 0.45f
    };
    const int BAR_COUNT = 50;
    float barFreqGap = (20000.0f - 20.0f)/config.fftSize;
    for(int i = 0; i < BAR_COUNT; i++) {
        config.barConfig.emplace_back(i*barFreqGap, (i+1)*barFreqGap, 5.0f);
    }

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