//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//       SDL Joystick code.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_event.h"
#include "doomtype.h"
#include "i_joystick.h"
#include "i_system.h"

#include "m_config.h"
#include "m_fixed.h"
#include "m_misc.h"

#if 0
static SDL_GameController *gamepad = NULL;
static SDL_Joystick *joystick = NULL;
#endif

// Configuration variables:

// Standard default.cfg Joystick enable/disable

static int usejoystick = 0;

// Use SDL_gamecontroller interface for the selected device
static int use_gamepad = 0;

// SDL_GameControllerType of gamepad
static int gamepad_type = 0;

// SDL GUID and index of the joystick to use.
static char *joystick_guid = "";
static int joystick_index = -1;

// Which joystick axis to use for horizontal movement, and whether to
// invert the direction:

static int joystick_x_axis = 0;
static int joystick_x_invert = 0;

// Which joystick axis to use for vertical movement, and whether to
// invert the direction:

static int joystick_y_axis = 1;
static int joystick_y_invert = 0;

// Which joystick axis to use for strafing?

static int joystick_strafe_axis = -1;
static int joystick_strafe_invert = 0;

// Which joystick axis to use for looking?

static int joystick_look_axis = -1;
static int joystick_look_invert = 0;

// Configurable dead zone for each axis, specified as a percentage of the axis
// max value.
static int joystick_x_dead_zone = 33;
static int joystick_y_dead_zone = 33;
static int joystick_strafe_dead_zone = 33;
static int joystick_look_dead_zone = 33;

int use_analog = 0;

int joystick_turn_sensitivity = 10;
int joystick_move_sensitivity = 10;
int joystick_look_sensitivity = 10;

// Virtual to physical button joystick button mapping. By default this
// is a straight mapping.
static int joystick_physical_buttons[NUM_VIRTUAL_BUTTONS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

void I_ShutdownGamepad(void)
{
#if 0
    if (gamepad != NULL)
    {
        SDL_GameControllerClose(gamepad);
        gamepad = NULL;
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
#endif
}

#if 0
static int FindFirstGamepad(void)
{
    int i;
    int gamepadindex = -1;

    for (i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            gamepadindex = i;
            break;
        }
    }

    return gamepadindex;
}
#endif

#if 0
static int FindSpecificGamepad(SDL_JoystickGUID guid)
{
    SDL_JoystickGUID dev_guid;
    int i;
    int gamepadindex = -1;

    for (i = 0; i < SDL_NumJoysticks(); ++i)
    {
        dev_guid = SDL_JoystickGetDeviceGUID(i);
        if (!memcmp(&guid, &dev_guid, sizeof(SDL_JoystickGUID)))
        {
            gamepadindex = i;
            break;
        }
    }

    return gamepadindex;
}
#endif

#if 0
static int DeviceIndexGamepad(void)
{
    SDL_JoystickGUID guid, dev_guid;
    int index = -1;

    if (strcmp(joystick_guid, ""))
    {
        guid = SDL_JoystickGetGUIDFromString(joystick_guid);

        // First, look for the gamepad at the previously-used index.
        if (joystick_index >= 0 && joystick_index < SDL_NumJoysticks())
        {
            dev_guid = SDL_JoystickGetDeviceGUID(joystick_index);
            if (!memcmp(&guid, &dev_guid, sizeof(SDL_JoystickGUID)))
            {
                return joystick_index;
            }
        }

        // Maybe the index has moved?
        index = FindSpecificGamepad(guid);
    }

    // If the previous gamepad isn't present, see if a different one is
    // available.
    if (index < 0)
    {
        index = FindFirstGamepad();
    }

    return index;
}
#endif

void I_InitGamepad(void)
{
#if 0
    SDL_JoystickGUID guid;
    int index;

    if (!use_gamepad)
    {
        return;
    }

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0)
    {
        return;
    }

    index = DeviceIndexGamepad();

    if (index < 0)
    {
        printf("I_InitGamepad: No gamepad found.\n");
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        return;
    }

    gamepad = SDL_GameControllerOpen(index);

    if (gamepad == NULL)
    {
        printf("I_InitGamepad: Failed to open gamepad: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        return;
    }

    joystick_index = index;
    gamepad_type = SDL_GameControllerTypeForIndex(index);

    if (strcmp(joystick_guid, ""))
    {
        joystick_guid = malloc(GUID_STRING_BUF_SIZE);
    }

    guid = SDL_JoystickGetDeviceGUID(joystick_index);
    SDL_JoystickGetGUIDString(guid, joystick_guid, GUID_STRING_BUF_SIZE);

    // GameController events do not fire if Joystick events are disabled.
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    printf("I_InitGamepad: %s\n", SDL_GameControllerName(gamepad));
    i_at_exit(I_ShutdownGamepad, true);
#endif
}

#if 0
static int GetTriggerStateGamepad(SDL_GameControllerAxis trigger)
{
    return (SDL_GameControllerGetAxis(gamepad, trigger) > TRIGGER_THRESHOLD);
}
#endif

// Get the state of the given virtual button.

#if 0
static int ReadButtonStateGamepad(int vbutton)
{
    int physbutton, state;

    // Map from virtual button to physical (SDL) button.
    if (vbutton < NUM_VIRTUAL_BUTTONS)
    {
        physbutton = joystick_physical_buttons[vbutton];
    }
    else
    {
        physbutton = vbutton;
    }

    switch (physbutton)
    {
        case GAMEPAD_BUTTON_TRIGGERLEFT:
            state = GetTriggerStateGamepad(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            break;

        case GAMEPAD_BUTTON_TRIGGERRIGHT:
            state = GetTriggerStateGamepad(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
            break;

        default:
            state = SDL_GameControllerGetButton(gamepad, physbutton);
            break;
    }

    return state;
}
#endif

// Get a bitmask of all currently-pressed buttons

#if 0
static int GetButtonsStateGamepad(void)
{
    int i;
    int result = 0;

    for (i = 0; i < MAX_VIRTUAL_BUTTONS; ++i)
    {
        if (ReadButtonStateGamepad(i))
        {
            result |= 1u << i;
        }
    }

    return result;
}
#endif

// Read the state of an axis, inverting if necessary.

#if 0
static int GetAxisStateGamepad(const int *axis_values, int axis, int invert,
                               int dead_zone)
{
    int result;

    // Axis -1 means disabled.

    if (axis < 0)
    {
        return 0;
    }

    // Dead zone is expressed as percentage of axis max value
    dead_zone = 32768 * dead_zone / 100;

    result = axis_values[axis];

    if (result < dead_zone && result > -dead_zone)
    {
        return 0;
    }

    if (invert)
    {
        result = -result;
    }

    result *= FRACUNIT / 32768; // Want FXP number between -1 and 1

    return result;
}
#endif

#define DIRECTION_DEADZONE (50 * 32768 / 100)

#if 0
static int GetDirectionalInputGamepad(const int *axis_values)
{
    int value;
    int dpad = JOY_DIR_NONE;
    int leftstick = JOY_DIR_NONE;
    int rightstick = JOY_DIR_NONE;

    // Dpad.
    if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
    {
        dpad |=  JOY_DIR_UP;
    }

    if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
    {
        dpad |=  JOY_DIR_DOWN;
    }

    if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
    {
        dpad |=  JOY_DIR_LEFT;
    }

    if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
    {
        dpad |= JOY_DIR_RIGHT;
    }

    // Left stick.
    value = axis_values[SDL_CONTROLLER_AXIS_LEFTY];

    if (value > DIRECTION_DEADZONE || value < -DIRECTION_DEADZONE)
    {
        if (value > 0)
        {
            leftstick |= JOY_DIR_DOWN;
        }
        else
        {
            leftstick |= JOY_DIR_UP;
        }
    }

    value = axis_values[SDL_CONTROLLER_AXIS_LEFTX];

    if (value > DIRECTION_DEADZONE || value < -DIRECTION_DEADZONE)
    {
        if (value > 0)
        {
            leftstick |= JOY_DIR_RIGHT;
        }
        else
        {
            leftstick |= JOY_DIR_LEFT;
        }
    }

    // Right stick.
    value = axis_values[SDL_CONTROLLER_AXIS_RIGHTX];

    if (value > DIRECTION_DEADZONE || value < -DIRECTION_DEADZONE)
    {
        if (value > 0)
        {
            rightstick |= JOY_DIR_RIGHT;
        }
        else
        {
            rightstick |= JOY_DIR_LEFT;
        }
    }

    value = axis_values[SDL_CONTROLLER_AXIS_RIGHTY];

    if (value > DIRECTION_DEADZONE || value < -DIRECTION_DEADZONE)
    {
        if (value > 0)
        {
            rightstick |= JOY_DIR_DOWN;
        }
        else
        {
            rightstick |= JOY_DIR_UP;
        }
    }

    return ((dpad << DPAD_SHIFT) | (leftstick << LSTICK_SHIFT) |
            (rightstick << RSTICK_SHIFT));
}
#endif

void I_UpdateGamepad(void)
{
#if 0
    if (gamepad != NULL)
    {
        event_t ev;
        int axis_values[SDL_CONTROLLER_AXIS_MAX];

        axis_values[SDL_CONTROLLER_AXIS_LEFTX] =
            SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);

        axis_values[SDL_CONTROLLER_AXIS_LEFTY] =
            SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);

        axis_values[SDL_CONTROLLER_AXIS_RIGHTX] =
            SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);

        axis_values[SDL_CONTROLLER_AXIS_RIGHTY] =
            SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

        ev.type = ev_joystick;
        ev.data1 = GetButtonsStateGamepad();
        ev.data2 = GetAxisStateGamepad(axis_values, joystick_x_axis,
                                       joystick_x_invert, joystick_x_dead_zone);
        ev.data3 = GetAxisStateGamepad(axis_values, joystick_y_axis,
                                       joystick_y_invert, joystick_y_dead_zone);
        ev.data4 = GetAxisStateGamepad(axis_values, joystick_strafe_axis,
                                       joystick_strafe_invert,
                                       joystick_strafe_dead_zone);
        ev.data5 = GetAxisStateGamepad(axis_values, joystick_look_axis,
                                       joystick_look_invert,
                                       joystick_look_dead_zone);
        ev.data6 = GetDirectionalInputGamepad(axis_values);

        D_PostEvent(&ev);
    }
#endif
}

void I_ShutdownJoystick(void)
{
#if 0
    if (joystick != NULL)
    {
        SDL_JoystickClose(joystick);
        joystick = NULL;
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }
#endif
}

#if 0
static boolean IsValidAxis(int axis)
{
    int num_axes;

    if (axis < 0)
    {
        return true;
    }

    if (IS_BUTTON_AXIS(axis))
    {
        return true;
    }

    if (IS_HAT_AXIS(axis))
    {
        return HAT_AXIS_HAT(axis) < SDL_JoystickNumHats(joystick);
    }

    num_axes = SDL_JoystickNumAxes(joystick);

    return axis < num_axes;
}
#endif

#if 0
static int DeviceIndex(void)
{
    SDL_JoystickGUID guid, dev_guid;
    int i;

    guid = SDL_JoystickGetGUIDFromString(joystick_guid);

    // GUID identifies a class of device rather than a specific device.
    // Check if joystick_index has the expected GUID, as this can act
    // as a tie-breaker in case there are multiple identical devices.
    if (joystick_index >= 0 && joystick_index < SDL_NumJoysticks())
    {
        dev_guid = SDL_JoystickGetDeviceGUID(joystick_index);
        if (!memcmp(&guid, &dev_guid, sizeof(SDL_JoystickGUID)))
        {
            return joystick_index;
        }
    }

    // Check all devices to look for one with the expected GUID.
    for (i = 0; i < SDL_NumJoysticks(); ++i)
    {
        dev_guid = SDL_JoystickGetDeviceGUID(i);
        if (!memcmp(&guid, &dev_guid, sizeof(SDL_JoystickGUID)))
        {
            printf("I_InitJoystick: Joystick moved to index %d.\n", i);
            return i;
        }
    }

    // No joystick found with the expected GUID.
    return -1;
}
#endif

void I_InitJoystick(void)
{
#if 0
    int index;

    if (!usejoystick || !strcmp(joystick_guid, ""))
    {
        return;
    }

    if (use_gamepad)
    {
        I_InitGamepad();
        return;
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
    {
        return;
    }

    index = DeviceIndex();

    if (index < 0)
    {
        printf("I_InitJoystick: Couldn't find joystick with GUID \"%s\": "
               "device not found or not connected?\n",
               joystick_guid);
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        return;
    }

    // Open the joystick

    joystick = SDL_JoystickOpen(index);

    if (joystick == NULL)
    {
        printf("I_InitJoystick: Failed to open joystick #%i: %s\n", index,
               SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        return;
    }

    if (!IsValidAxis(joystick_x_axis)
     || !IsValidAxis(joystick_y_axis)
     || !IsValidAxis(joystick_strafe_axis)
     || !IsValidAxis(joystick_look_axis))
    {
        printf("I_InitJoystick: Invalid joystick axis for configured joystick "
               "(run joystick setup again)\n");

        SDL_JoystickClose(joystick);
        joystick = NULL;
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }

    SDL_JoystickEventState(SDL_ENABLE);

    // Initialized okay!

    printf("I_InitJoystick: %s\n", SDL_JoystickName(joystick));

    i_at_exit(I_ShutdownJoystick, true);
#endif
}

#if 0
static boolean IsAxisButton(int physbutton)
{
    if (IS_BUTTON_AXIS(joystick_x_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_x_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_x_axis))
        {
            return true;
        }
    }
    if (IS_BUTTON_AXIS(joystick_y_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_y_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_y_axis))
        {
            return true;
        }
    }
    if (IS_BUTTON_AXIS(joystick_strafe_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_strafe_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_strafe_axis))
        {
            return true;
        }
    }
    if (IS_BUTTON_AXIS(joystick_look_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_look_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_look_axis))
        {
            return true;
        }
    }

    return false;
}
#endif

// Get the state of the given virtual button.

#if 0
static int ReadButtonState(int vbutton)
{
    int physbutton;

    // Map from virtual button to physical (SDL) button.
    if (vbutton < NUM_VIRTUAL_BUTTONS)
    {
        physbutton = joystick_physical_buttons[vbutton];
    }
    else
    {
        physbutton = vbutton;
    }

    // Never read axis buttons as buttons.
    if (IsAxisButton(physbutton))
    {
        return 0;
    }

    return SDL_JoystickGetButton(joystick, physbutton);
}
#endif

// Get a bitmask of all currently-pressed buttons
#if 0
static int GetButtonsState(void)
{
    int i;
    int result = 0;

    for (i = 0; i < MAX_VIRTUAL_BUTTONS; ++i)
    {
        if (ReadButtonState(i))
        {
            result |= 1 << i;
        }
    }

    return result;
}
#endif

// Read the state of an axis, inverting if necessary.

#if 0
static int GetAxisState(int axis, int invert, int dead_zone)
{
    int result;

    // Axis -1 means disabled.

    if (axis < 0)
    {
        return 0;
    }

    // Is this a button axis, or a hat axis?
    // If so, we need to handle it specially.

    result = 0;

    if (IS_BUTTON_AXIS(axis))
    {
        if (SDL_JoystickGetButton(joystick, BUTTON_AXIS_NEG(axis)))
        {
            result -= 32767;
        }
        if (SDL_JoystickGetButton(joystick, BUTTON_AXIS_POS(axis)))
        {
            result += 32767;
        }
    }
    else if (IS_HAT_AXIS(axis))
    {
        int direction = HAT_AXIS_DIRECTION(axis);
        int hatval = SDL_JoystickGetHat(joystick, HAT_AXIS_HAT(axis));

        if (direction == HAT_AXIS_HORIZONTAL)
        {
            if ((hatval & SDL_HAT_LEFT) != 0)
            {
                result -= 32767;
            }
            else if ((hatval & SDL_HAT_RIGHT) != 0)
            {
                result += 32767;
            }
        }
        else if (direction == HAT_AXIS_VERTICAL)
        {
            if ((hatval & SDL_HAT_UP) != 0)
            {
                result -= 32767;
            }
            else if ((hatval & SDL_HAT_DOWN) != 0)
            {
                result += 32767;
            }
        }
    }
    else
    {
        result = SDL_JoystickGetAxis(joystick, axis);

        // Dead zone is expressed as percentage of axis max value
        dead_zone = 32768 * dead_zone / 100;

        if (result < dead_zone && result > -dead_zone)
        {
            result = 0;
        }
    }

    if (invert)
    {
        result = -result;
    }

    result *= FRACUNIT / 32768; // Want FXP number between -1 and 1

    return result;
}
#endif

#if 0
static int GetDirectionalInput(int x, int y)
{
    int retval = JOY_DIR_NONE;

    if (x > 0)
    {
        retval |= JOY_DIR_RIGHT;
    }
    else if (x < 0)
    {
        retval |= JOY_DIR_LEFT;
    }

    if (y > 0)
    {
        retval |= JOY_DIR_DOWN;
    }
    else if (y < 0)
    {
        retval |= JOY_DIR_UP;
    }

    return (retval << DPAD_SHIFT);
}
#endif

void I_UpdateJoystick(void)
{
#if 0
    if (use_gamepad)
    {
        I_UpdateGamepad();
        return;
    }

    if (joystick != NULL)
    {
        event_t ev;

        ev.type = ev_joystick;
        ev.data1 = GetButtonsState();
        ev.data2 = GetAxisState(joystick_x_axis, joystick_x_invert,
                                joystick_x_dead_zone);
        ev.data3 = GetAxisState(joystick_y_axis, joystick_y_invert,
                                joystick_y_dead_zone);
        ev.data4 = GetAxisState(joystick_strafe_axis, joystick_strafe_invert,
                                joystick_strafe_dead_zone);
        ev.data5 = GetAxisState(joystick_look_axis, joystick_look_invert,
                                joystick_look_dead_zone);
        ev.data6 = GetDirectionalInput(ev.data2, ev.data3);
        D_PostEvent(&ev);
    }
#endif
}

void I_bind_joystick_variables(void)
{
  int i;

  m_bind_int_variable("use_joystick", &usejoystick);
  m_bind_int_variable("use_gamepad", &use_gamepad);
  m_bind_int_variable("gamepad_type", &gamepad_type);
  m_bind_string_variable("joystick_guid", &joystick_guid);
  m_bind_int_variable("joystick_index", &joystick_index);
  m_bind_int_variable("joystick_x_axis", &joystick_x_axis);
  m_bind_int_variable("joystick_y_axis", &joystick_y_axis);
  m_bind_int_variable("joystick_strafe_axis", &joystick_strafe_axis);
  m_bind_int_variable("joystick_x_invert", &joystick_x_invert);
  m_bind_int_variable("joystick_y_invert", &joystick_y_invert);
  m_bind_int_variable("joystick_strafe_invert", &joystick_strafe_invert);
  m_bind_int_variable("joystick_look_axis", &joystick_look_axis);
  m_bind_int_variable("joystick_look_invert", &joystick_look_invert);
  m_bind_int_variable("joystick_x_dead_zone", &joystick_x_dead_zone);
  m_bind_int_variable("joystick_y_dead_zone", &joystick_y_dead_zone);
  m_bind_int_variable("joystick_strafe_dead_zone",
                      &joystick_strafe_dead_zone);
  m_bind_int_variable("joystick_look_dead_zone", &joystick_look_dead_zone);
  m_bind_int_variable("use_analog", &use_analog);
  m_bind_int_variable("joystick_turn_sensitivity",
                      &joystick_turn_sensitivity);
  m_bind_int_variable("joystick_move_sensitivity",
                      &joystick_move_sensitivity);
  m_bind_int_variable("joystick_look_sensitivity",
                      &joystick_look_sensitivity);

  for (i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
      char name[32];
      snprintf(name, sizeof(name), "joystick_physical_button%i", i);
      m_bind_int_variable(name, &joystick_physical_buttons[i]);
    }
}
