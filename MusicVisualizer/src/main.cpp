#include "VisualVisualizer.hpp"

#include <exception>
#include <print>
#include <vector>
#include <filesystem>

int main(int argc, char** argv) {
    std::string configFile = (argc < 2) ? "config.yaml" : argv[argc-1];

    try {
        VisualizerConfig_t config{};
        Status_t status = VisualizerConfig_t::ParseYamlFile(configFile, config);
        if(status != STATUS_OK) {
            std::println("{}", status.value().Message);
            return 1;
        }
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
    } catch(const std::runtime_error& ex) {
        std::print("{}", ex.what());
        return 1;
    }

    return 0;
}