#include "VisualVisualizer.hpp"

#include <exception>
#include <string>
#include <fstream>
#include <filesystem>
#include <format>

VisualVisualizer* VisualVisualizer::m_instance = nullptr;

VisualVisualizer::VisualVisualizer(const VisualizerConfig_t& config, const float barGap) :
    m_window(nullptr), m_visualizer(config), m_heights(config.barConfig.size()), m_barCount(config.barConfig.size()),
    m_barGap(barGap)
{
    if(m_instance != nullptr) {
        throw std::exception("An instance of visual engine already exists! Only one instance can exist at a time!");
    }
    if(glfwInit() != GLFW_TRUE) {
        throw std::exception("Failed to initialize GLFW!");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_instance = this;
}

void VisualVisualizer::CreateWindow(const WindowConfig& cfg) {
    if(m_window != nullptr) {
        throw std::exception("Attempted to create a new window without destroying existing one!");
    }
    m_window = new Window(cfg);
}

void VisualVisualizer::DestroyWindow() {
    if(m_window != nullptr) {
        delete m_window;
    }
    m_window = nullptr;
}

void VisualVisualizer::Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    while(!m_window->ShouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(m_vao);
        m_visualizer.Update();
        this->_UpdateHeightsBuffer();
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_barCount);
        m_window->SwapBuffers();
        glfwPollEvents();
    }
}

void VisualVisualizer::_UpdateHeightsBuffer() {
    m_visualizer.FetchBarHeights(m_heights.data());
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceData);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_barCount, m_heights.data(), GL_DYNAMIC_DRAW);
}

void VisualVisualizer::_InitializeBuffers() {
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_instanceData);

    // Instance data
    this->_UpdateHeightsBuffer();
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);
}

std::string ReadFile(const std::string& file) {
    std::ifstream filestream(file);
    if(!filestream.is_open()) {
        throw std::runtime_error(std::format("Failed to open file \"{}\"!", file));
    }
    std::string ret;
    ret.resize(std::filesystem::file_size(file));
    filestream.read(&ret[0], ret.size());
    return ret;
}

void CompileShader(unsigned int shader, const std::string& src) {
    const GLchar* const csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, NULL);
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog;
        infoLog.resize(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, &infoLog[0]);
        throw std::runtime_error(
            std::format("Failed to compile GLSL shader! Error log: {}", infoLog)
        );
    }
}

void VisualVisualizer::_SetupShaders() {
    const std::string vertexSrc = ReadFile("shaders/vertex.glsl");
    const std::string fragmentSrc = ReadFile("shaders/fragment.glsl");
    
    m_shaderProgram = glCreateProgram();
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(vertexShader, vertexSrc);
    CompileShader(fragmentShader, fragmentSrc);
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    int ls = 0;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &ls);
    if(ls != GL_TRUE) {
        GLint infoLogLength = 0;
        glGetProgramiv(m_shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::string infoLog;
        infoLog.resize(infoLogLength);
        glGetProgramInfoLog(m_shaderProgram, infoLogLength, &infoLogLength, &infoLog[0]);
        throw std::runtime_error(
            std::format("Failed to link shader program! Error log: {}", infoLog)
        );
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(m_shaderProgram);

    // Set the uniform values
    const auto windowSize = m_window->GetSize();
    const float windowWidth = static_cast<float>(std::get<0>(windowSize));
    const float windowHeight = static_cast<float>(std::get<1>(windowSize));
    float barGap = 2.0f * m_barGap/windowWidth;
    float barWidth = 2.0/m_barCount - barGap;

    unsigned int unifLoc = glGetUniformLocation(m_shaderProgram, "barGap");
    glUniform1f(unifLoc, barGap);
    unifLoc = glGetUniformLocation(m_shaderProgram, "barWidth");
    glUniform1f(unifLoc, barWidth);
    unifLoc = glGetUniformLocation(m_shaderProgram, "heightMax");
    glUniform1f(unifLoc, windowHeight);
}

void VisualVisualizer::SetupGraphics() {
    this->_InitializeBuffers();
    this->_SetupShaders();
}

void VisualVisualizer::_DestroyShaderProgram() {
    glDeleteProgram(m_shaderProgram);
}

VisualVisualizer::~VisualVisualizer() {
    this->_DestroyShaderProgram();
    this->DestroyWindow();
    glfwTerminate();
}