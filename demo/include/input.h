#pragma once

#include "common.h"

namespace spe
{

class Window;

class Input final
{
    friend class Window;

public:
    Input() = delete;

    static void Init();
    static void Update();
    static bool IsKeyDown(int key);
    static bool IsKeyPressed(int key);
    static bool IsKeyReleased(int key);
    static bool IsMouseDown(int button);
    static bool IsMousePressed(int button);
    static bool IsMouseReleased(int button);
    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseAcceleration();
    static glm::vec2 GetMouseScroll();

private:
    static std::array<bool, GLFW_KEY_LAST + 1> lastKeys;
    static std::array<bool, GLFW_KEY_LAST + 1> currKeys;

    static std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> lastBtns;
    static std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> currBtns;

    static glm::vec2 currMousePos;
    static glm::vec2 lastMousePos;
    static glm::vec2 mouseAcceleration;

    static glm::vec2 mouseScroll;
};

inline void Input::Init()
{
    lastKeys.fill(false);
    currKeys.fill(false);

    lastBtns.fill(false);
    currBtns.fill(false);
}

inline void Input::Update()
{
    lastKeys = currKeys;
    lastBtns = currBtns;

    mouseAcceleration = currMousePos - lastMousePos;
    lastMousePos = currMousePos;

    glm::clear(mouseScroll);
}

inline bool Input::IsKeyDown(int key)
{
    return currKeys[key];
}

inline bool Input::IsKeyPressed(int key)
{
    return currKeys[key] && !lastKeys[key];
}

inline bool Input::IsKeyReleased(int key)
{
    return !currKeys[key] && lastKeys[key];
}

inline bool Input::IsMouseDown(int button)
{
    return currBtns[button];
}

inline bool Input::IsMousePressed(int button)
{
    return currBtns[button] && !lastBtns[button];
}

inline bool Input::IsMouseReleased(int button)
{
    return !currBtns[button] && lastBtns[button];
}

inline glm::vec2 Input::GetMousePosition()
{
    return currMousePos;
}

inline glm::vec2 Input::GetMouseAcceleration()
{
    return mouseAcceleration;
}

inline glm::vec2 Input::GetMouseScroll()
{
    return mouseScroll;
}

} // namespace spe
