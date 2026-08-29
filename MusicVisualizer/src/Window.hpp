#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <tuple>

typedef std::tuple<int, int> WindowSize_t;

struct WindowConfig{
    uint16_t width;
    uint16_t height;
    const char* title;
    bool borderless; // Doesn't do anything for now
};

class Window {
public:
    Window() = delete;
    Window(const WindowConfig& cfg);

    virtual ~Window();

    inline bool ShouldClose() {
        return (bool)glfwWindowShouldClose(m_window);
    }
    
    inline void CloseWindow() {
        glfwSetWindowShouldClose(m_window, true);
    }
    
    inline void SwapBuffers() {
        glfwSwapBuffers(m_window);
    }

    inline const WindowSize_t GetSize() const {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        return WindowSize_t(width, height);
    }

private:
    GLFWwindow* m_window;
    static bool m_gladLoaded;
};