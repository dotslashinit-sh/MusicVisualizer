#include "Window.hpp"

#include <format>
#include <exception>
#include <string>

bool Window::m_gladLoaded = false;

void WindowResizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

Window::Window(const WindowConfig& cfg) : m_window(nullptr) {
    m_window = glfwCreateWindow(cfg.width, cfg.height, cfg.title, NULL, NULL);
    if(m_window == NULL) {
        const std::string message = std::format("Failed to create window with dimenions: {}:{} and title \"{}\"",
            cfg.width, cfg.height, cfg.title);
        throw std::exception(message.c_str());
    }
    glfwMakeContextCurrent(m_window);

    if(!m_gladLoaded) {
        if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::exception("Failed to initialize GLAD!");
        }
        m_gladLoaded = true;
    }

    glViewport(0, 0, cfg.width, cfg.height);
    glfwSetFramebufferSizeCallback(m_window, &WindowResizeCallback);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
}