#include "input.h"

#include <string.h>
#include <math.h>

char input_keys[INPUT_KEYS];
char input_keys_prev[INPUT_KEYS];

char input_mouse_buttons[INPUT_MOUSE_BUTTONS];
char input_mouse_buttons_prev[INPUT_MOUSE_BUTTONS];

Vec2 input_mouse_coord, input_prev_mouse_coord;

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) scancode;
    (void) mods;

    input_keys_prev[key] = input_keys[key];
    input_keys[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
}

void input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void) window;
    (void) mods;

    input_mouse_buttons_prev[button] = input_mouse_buttons[button];
    input_mouse_buttons[button] = action == GLFW_PRESS || action == GLFW_REPEAT;
}

void input_mouse_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void) window;

    input_prev_mouse_coord = input_mouse_coord;
    input_mouse_coord = (Vec2) { (float)xpos, (float)ypos };
}

void input_init(GLFWwindow* window)
{
    glfwSetKeyCallback(window, input_key_callback);
    glfwSetMouseButtonCallback(window, input_mouse_button_callback);
    glfwSetCursorPosCallback(window, input_mouse_position_callback);

    memset(input_keys, 0, INPUT_KEYS * sizeof(char));
    memset(input_keys_prev, 0, INPUT_KEYS * sizeof(char));
    memset(input_mouse_buttons, 0, INPUT_MOUSE_BUTTONS * sizeof(char));
    memset(input_mouse_buttons_prev, 0, INPUT_MOUSE_BUTTONS * sizeof(char));

    input_mouse_coord = input_prev_mouse_coord = (Vec2) { 0.0f, 0.0f };
}

void input_update()
{
    memcpy(input_keys_prev, input_keys, INPUT_KEYS * sizeof(char));
    memcpy(input_mouse_buttons_prev, input_mouse_buttons, INPUT_MOUSE_BUTTONS * sizeof(char));

    input_prev_mouse_coord = input_mouse_coord;
}

bool input_key_down(int key)
{
    return input_keys[key] && !input_keys_prev[key];
}

bool input_key(int key)
{
    return input_keys[key];
}

bool input_key_up(int key)
{
    return !input_keys[key] && input_keys_prev[key];
}

bool input_mouse_button_down(int button)
{
    return input_mouse_buttons[button] && !input_mouse_buttons_prev[button];
}

bool input_mouse_button(int button)
{
    return input_mouse_buttons[button];
}

bool input_mouse_button_up(int button)
{
    return !input_mouse_buttons[button] && input_mouse_buttons_prev[button];
}

Vec2 input_mouse_position()
{
    return input_mouse_coord;
}

Vec2 input_mouse_movement()
{
    Vec2 dist = {
        input_mouse_coord.x - input_prev_mouse_coord.x,
        input_mouse_coord.y - input_prev_mouse_coord.y,
    };
    
    if(fabsf(dist.x) > 5.0f || fabsf(dist.y) > 5.0f)
        return (Vec2) {0};
    else
        return dist;
}
