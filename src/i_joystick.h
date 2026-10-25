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
//      System-specific joystick interface.
//


#ifndef __I_JOYSTICK__
#define __I_JOYSTICK__

#include "SDL_gamecontroller.h"

// Number of "virtual" joystick buttons defined in configuration files.
// This needs to be at least as large as the number of different key
// bindings supported by the higher-level game code (joyb* variables).
#define NUM_VIRTUAL_BUTTONS 17

// Max allowed number of virtual mappings. Chosen to be less than joybspeed
// autorun value.
#define MAX_VIRTUAL_BUTTONS 20

// If this bit is set in a configuration file axis value, the axis is
// not actually a joystick axis, but instead is a "button axis". This
// means that instead of reading an SDL joystick axis, we read the
// state of two buttons to get the axis value. This is needed for eg.
// the PS3 SIXAXIS controller, where the D-pad buttons register as
// buttons, not as two axes.
#define BUTTON_AXIS 0x10000

// Query whether a given axis value describes a button axis.
#define IS_BUTTON_AXIS(axis) ((axis) >= 0 && ((axis) & BUTTON_AXIS) != 0)

// Get the individual buttons from a button axis value.
#define BUTTON_AXIS_NEG(axis)  ((axis) & 0xff)
#define BUTTON_AXIS_POS(axis)  (((axis) >> 8) & 0xff)

// Create a button axis value from two button values.
#define CREATE_BUTTON_AXIS(neg, pos) (BUTTON_AXIS | (neg) | ((pos) << 8))

// If this bit is set in an axis value, the axis is not actually a
// joystick axis, but is a "hat" axis. This means that we read (one of)
// the hats on the joystick.
#define HAT_AXIS    0x20000

#define IS_HAT_AXIS(axis) ((axis) >= 0 && ((axis) & HAT_AXIS) != 0)

// Get the hat number from a hat axis value.
#define HAT_AXIS_HAT(axis)         ((axis) & 0xff)
// Which axis of the hat? (horizonal or vertical)
#define HAT_AXIS_DIRECTION(axis)   (((axis) >> 8) & 0xff)

#define CREATE_HAT_AXIS(hat, direction) \
    (HAT_AXIS | (hat) | ((direction) << 8))

#define HAT_AXIS_HORIZONTAL 1
#define HAT_AXIS_VERTICAL   2

// When a trigger reads greater than this, consider it to be pressed.  30 comes
// from XINPUT_GAMEPAD_TRIGGER_THRESHOLD in xinput.h, and is scaled here for
// the SDL_GameController trigger max value.
#define TRIGGER_THRESHOLD (30 * 32767 / 255)

// To be used with SDL_JoystickGetGUIDString; see SDL_joystick.h
#define GUID_STRING_BUF_SIZE 33

// Helper macros for bitpacked directional data from gamepad inputs.
#define DPAD_SHIFT 0
#define LSTICK_SHIFT 4
#define RSTICK_SHIFT 8
#define JOY_GET_DPAD(x) (((x) >> DPAD_SHIFT) & 0xf)
#define JOY_GET_LSTICK(x) (((x) >> LSTICK_SHIFT) & 0xf)
#define JOY_GET_RSTICK(x) (((x) >> RSTICK_SHIFT) & 0xf)

#ifdef MISTER_HYBRID
// buttons pressed at the last poll of the joystick (i_joystick.c)
extern unsigned int mister_joy_pressed;
// a menu took a press of the pad (Menu OK, Menu Back, Menu): the buttons held
// now do nothing in the game until they are let go, or one that also uses,
// fires or jumps would do it as the menu or the dialogue closes (Strife's
// talk began again at once)
void MiSTer_MenuTookPad(void);
// the d-pad, L2 and Prev Weapon are actions of the game, set before
// I_InitJoystick(): MISTER_DPAD_* of the game's (0: the d-pad moves and
// turns, as in Doom)
extern int mister_dpad_actions;
enum
{
    MISTER_DPAD_INVENTORY = 1, // left and right in it, down uses the item
    MISTER_DPAD_FLY = 2,       // up flies up, L2 down (Heretic, Hexen)
    MISTER_DPAD_JUMP = 4,      // Prev Weapon jumps, Next Weapon goes round
                               // the weapons alone (Hexen, Strife)
    MISTER_DPAD_MISSION = 8,   // up shows the mission objectives while held,
                               // as key_mission does, outside the menus and
                               // the dialogues (Strife's G_Responder())
};
// the look of the right stick (joylook, -FRACUNIT up..FRACUNIT down, after
// the sensitivity) for a ticcmd: 2 a tic at full tilt, the fraction carried
// over to the next tic
int MiSTer_LookStep(int joylook);
#endif

// the joystick sensitivities are tenths; on the MiSTer the OSD's percentages
// (MiSTer_Sensitivity()), 150 the speed of 10
#ifdef MISTER_HYBRID
#define JOYSTICK_SENSITIVITY_UNIT 150
#else
#define JOYSTICK_SENSITIVITY_UNIT 10
#endif

// 4-way direction data for gamepad directional inputs.
enum
{
    JOY_DIR_NONE = 0x0,
    JOY_DIR_UP = 0x1,
    JOY_DIR_DOWN = 0x2,
    JOY_DIR_LEFT = 0x4,
    JOY_DIR_RIGHT = 0x8
};

// Extend the SDL_GameControllerButton enum to include the triggers.
enum
{
    GAMEPAD_BUTTON_TRIGGERLEFT = SDL_CONTROLLER_BUTTON_MAX,
    GAMEPAD_BUTTON_TRIGGERRIGHT,
    GAMEPAD_BUTTON_MAX
};

extern int use_analog;
extern int joystick_turn_sensitivity;
extern int joystick_move_sensitivity;
extern int joystick_look_sensitivity;

void I_InitJoystick(void);
void I_ShutdownJoystick(void);
void I_UpdateJoystick(void);

void I_BindJoystickVariables(void);

#endif /* #ifndef __I_JOYSTICK__ */

