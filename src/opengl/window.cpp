#include "opengl/window.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace gl {

// ---- Error callback ---------------------------------------------------

static void glfwErrorCB(int code, const char* msg) {
    std::cerr << "[GLFW error " << code << "] " << msg << "\n";
}

// ---- Resize callback (stored globally for the GLFW C callback) --------

static WindowCallbacks* g_callbacks = nullptr;

static void framebufferSizeCB(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    if (g_callbacks && g_callbacks->onResize)
        g_callbacks->onResize(w, h);
}

static void keyCB(GLFWwindow*, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if (g_callbacks && g_callbacks->onKey)
        g_callbacks->onKey(key, action);
}

static void mouseButtonCB(GLFWwindow* win, int button, int action, int mods) {
    (void)mods;
    if (g_callbacks && g_callbacks->onMouseButton) {
        double mx = 0.0, my = 0.0;
        glfwGetCursorPos(win, &mx, &my);
        int winW = 800, winH = 600;
        glfwGetWindowSize(win, &winW, &winH);
        if (winW > 0 && winH > 0) {
            mx = mx * 800.0 / (double)winW;
            my = my * 600.0 / (double)winH;
        }
        g_callbacks->onMouseButton(button, action, mx, my);
    }
}

static void cursorPosCB(GLFWwindow* win, double mx, double my) {
    if (g_callbacks && g_callbacks->onCursorPos) {
        int winW = 800, winH = 600;
        glfwGetWindowSize(win, &winW, &winH);
        if (winW > 0 && winH > 0) {
            mx = mx * 800.0 / (double)winW;
            my = my * 600.0 / (double)winH;
        }
        g_callbacks->onCursorPos(mx, my);
    }
}

static void charCB(GLFWwindow*, unsigned int codepoint) {
    if (g_callbacks && g_callbacks->onChar)
        g_callbacks->onChar(codepoint);
}

static void dropCB(GLFWwindow*, int count, const char** paths) {
    if (g_callbacks && g_callbacks->onDrop && count > 0 && paths && paths[0])
        g_callbacks->onDrop(std::string(paths[0]));
}

static void scrollCB(GLFWwindow*, double xoffset, double yoffset) {
    if (g_callbacks && g_callbacks->onScroll)
        g_callbacks->onScroll(xoffset, yoffset);
}

// ---- Public API -------------------------------------------------------

GLFWwindow* createWindow(int width, int height, const std::string& title) {
    glfwSetErrorCallback(glfwErrorCB);

    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW\n";
        return nullptr;
    }

    // Request OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!win) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(win);

    // Load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialise GLAD (OpenGL loader)\n";
        glfwDestroyWindow(win);
        glfwTerminate();
        return nullptr;
    }

    // V-Sync on by default
    glfwSwapInterval(1);

    // Initial viewport
    int fbW, fbH;
    glfwGetFramebufferSize(win, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << "\n";

    return win;
}

void runLoop(GLFWwindow* window, const WindowCallbacks& cb) {
    g_callbacks = const_cast<WindowCallbacks*>(&cb);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCB);
    glfwSetKeyCallback(window, keyCB);
    glfwSetMouseButtonCallback(window, mouseButtonCB);
    glfwSetCursorPosCallback(window, cursorPosCB);
    glfwSetCharCallback(window, charCB);
    glfwSetDropCallback(window, dropCB);
    glfwSetScrollCallback(window, scrollCB);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Escape key closes the window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        double now = glfwGetTime();
        double dt  = now - lastTime;
        lastTime   = now;

        if (cb.onUpdate) cb.onUpdate(dt);

        glClearColor(0.96f, 0.94f, 0.88f, 1.0f);   // warm cream background
        glClear(GL_COLOR_BUFFER_BIT);

        if (cb.onRender) cb.onRender();

        glfwSwapBuffers(window);
    }

    g_callbacks = nullptr;
}

void destroyWindow(GLFWwindow* window) {
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

} // namespace gl

