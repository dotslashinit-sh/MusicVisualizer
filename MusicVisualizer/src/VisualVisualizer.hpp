#pragma once

#include "Window.hpp"
#include "Status.hpp"

#include <Visualizer.hpp>

class VisualVisualizer {
public:
    VisualVisualizer(const VisualizerConfig_t& config, const float barGap);
    VisualVisualizer() = delete;
    virtual ~VisualVisualizer();
    void CreateWindow(const WindowConfig&);
    void DestroyWindow();

    /// @brief Keeps executing the main loop.
    void Display();

    /// @brief Sets up all the opengl buffers and stuff required to render the graphics.
    void SetupGraphics();

private:
    // Pointer to an already existing instance of `VisualEngine` object. If no instance exists, this
    // will be `nullptr`.
    static VisualVisualizer* m_instance;

    Window* m_window;
    unsigned int m_vao;
    // unsigned int m_vbo;
    // unsigned int m_ebo;
    unsigned int m_instanceData;
    unsigned int m_shaderProgram;

    Visualizer m_visualizer;

    const unsigned int m_barCount;
    float* m_heights;
    const float m_barGap;

    void _InitializeBuffers();
    void _SetupShaders();
    void _UpdateHeightsBuffer();
    void _DestroyShaderProgram();
};