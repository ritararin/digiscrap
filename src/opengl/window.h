#pragma once
// -----------------------------------------------------------------------
// OpenGL window wrapper around GLFW.
// Creates an 800×600 window with an OpenGL 3.3 Core context.
// -----------------------------------------------------------------------

#include <functional>
#include <string>

struct GLFWwindow;   // forward-declare so users don't need GLFW headers

namespace gl {

// Callbacks the host sets before entering the loop.
struct WindowCallbacks {
    std::function<void(int width, int height)> onResize;   // window resize
    std::function<void(double dt)>             onUpdate;   // per-frame logic
    std::function<void()>                      onRender;   // draw call
    std::function<void(int key, int action)>   onKey;      // key event
};

// Create the GLFW window + GL context.  Returns nullptr on failure.
GLFWwindow* createWindow(int width, int height, const std::string& title);

// Run the render loop until the window is closed.
void runLoop(GLFWwindow* window, const WindowCallbacks& cb);

// Clean up GLFW.
void destroyWindow(GLFWwindow* window);

} // namespace gl

