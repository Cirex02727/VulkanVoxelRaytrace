#ifndef INPUT_H_
#define INPUT_H_

#include "vec.h"

#include <GLFW/glfw3.h>

#include <stdbool.h>

#define INPUT_KEYS GLFW_KEY_LAST + 1
#define INPUT_MOUSE_BUTTONS GLFW_MOUSE_BUTTON_LAST + 1

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void input_mouse_position_callback(GLFWwindow* window, double xpos, double ypos);

void input_init(GLFWwindow* window);

void input_update();

bool input_key_down(int key);

bool input_key(int key);

bool input_key_up(int key);

bool input_mouse_button_down(int button);

bool input_mouse_button(int button);

bool input_mouse_button_up(int button);

Vec2 input_mouse_position();

Vec2 input_mouse_movement();

#endif // INPUT_H_
