#ifndef MISTER_PORT_H
#define MISTER_PORT_H

// MiSTer hybrid core: the game runs on the ARM, the FPGA core shows the
// picture at 15kHz and provides audio and input. SDL's "mister" drivers
// (hybrid/sdl2 of the core) do most of the work; this is what is specific to
// Crispy Doom. Everything is a no-op without the core, so the PC build still
// runs.

#include <stdint.h>

// Joystick buttons as SDL sees them: the buttons of the core's "J1," list
// (core/CrispyDoom.sv) in order, then the two the SDL driver adds
enum {
    MISTER_JOY_FIRE,
    MISTER_JOY_USE,
    MISTER_JOY_RUN,
    MISTER_JOY_NEXT_WEAPON,
    MISTER_JOY_PREV_WEAPON,
    MISTER_JOY_STRAFE,
    MISTER_JOY_MAP,
    MISTER_JOY_MENU,
    MISTER_JOY_CORE_OK,
    MISTER_JOY_CORE_BACK,
    // with mister_dpad_actions (Heretic, Hexen, Strife): the d-pad and L2
    // as buttons of their own, not moving and running
    MISTER_JOY_DPAD_UP,
    MISTER_JOY_DPAD_DOWN,
    MISTER_JOY_DPAD_LEFT,
    MISTER_JOY_DPAD_RIGHT,
    MISTER_JOY_L2,

    MISTER_JOY_MENU_OK = 28,
    MISTER_JOY_MENU_BACK = 29
};
// In the game's menus: confirm and go back
#define MISTER_JOY_OK_MASK ((1u << MISTER_JOY_CORE_OK) | (1u << MISTER_JOY_MENU_OK))
#define MISTER_JOY_BACK_MASK ((1u << MISTER_JOY_CORE_BACK) | (1u << MISTER_JOY_MENU_BACK))

// The game's OSD options (CONF_STR in core/CrispyDoom.sv): status bits
enum {
    // Resolution: 320x200, 640x200, 320x240, 640x240
    MISTER_OSD_640 = 50,
    MISTER_OSD_240 = 51,
    MISTER_OSD_SOFT_FM = 26,   // FM Music: FPGA, Software
    // the Cheats page
    MISTER_OSD_GOD = 32,
    MISTER_OSD_NOCLIP = 33,
    MISTER_OSD_INFINITE_AMMO = 34,
    MISTER_OSD_BUDDHA = 35,
    MISTER_OSD_NOTARGET = 36
};
// 1 while the option is on, 0 without the core
int MiSTer_Option(int bit);
// The Cheats page's triggers selected since the last call (MH_OSDTriggers()):
// call it from one place, every tic
enum {
    MISTER_TRIGGER_GIVE_ALL = 1,  // IDKFA
    MISTER_TRIGGER_KILL_ALL = 2,  // TNTEM
    MISTER_TRIGGER_NEXT_LEVEL = 4,
    MISTER_TRIGGER_FULL_MAP = 8   // IDDT
};
int MiSTer_Triggers(void);

// The command line of the game the player picked (MH_PickGame(): the menu
// entry's, or one of the list of the games that are there) in place of
// argc / argv. Returns 0 to quit (no game, or the player backed out)
int MiSTer_GameArgs(int *argc, char ***argv);
// The episode the game's menu entry starts, New Game without the episode menu
// (SIGIL 5, SIGIL II 6, No Rest for the Living 2 and the Master Levels 3 of
// Crispy's Doom II), or 0
int MiSTer_Episode(void);
// The expansion Crispy loads itself next to the IWAD (d_pwad.c), as the game
// asks for it; none for the others, so that each game is the one of its menu
// entry
enum { MISTER_EXP_NONE, MISTER_EXP_NERVE, MISTER_EXP_MASTER };
int MiSTer_Expansion(void);

// The game's clock while the core is there: microseconds from the core's
// field counter, so that a frame's time is the field it is shown in. Returns
// 0 without the core (the game takes the system clock then)
int MiSTer_TimeUS(uint64_t *us);

// Frame pacing: call once per frame after presenting it. Sleeps until the
// core's next field and returns how many fields (1/59.64s) have passed since
// the last call. 0 without the core: use the original timing then.
unsigned MiSTer_WaitFrame(void);
// The next call starts counting anew, after the game stood still (loading)
void MiSTer_ResetFrames(void);

// HUD margins (hybrid/README.md, "HUD margins"): where the HUD goes on a
// screen of `height` lines (240, or 480: the margins are doubled), as the
// player set it in the OSD (CRT Options). `top` is the first row of the top
// HUD, `bottom` the row under the last row of the bottom HUD (a HUD of h rows
// is drawn from bottom - h). Call it every frame: the player changes the
// margins while the game runs.
void MiSTer_HudRows(int height, int *top, int *bottom);
// The OSD's HUD margins are greyed out in 320x200, which has none
void MiSTer_ShowHudOptions(int shown);

// Problems the player can fix (missing data): shown under the core's logo
// until the player leaves the core, in plain words that say which files go
// where, in 4 lines of 71 characters (MH_MSG_ROWS, MH_MSG_COLS) at most
void MiSTer_ShowError(const char *message);
// Exit code of the process after the player quit: the launcher starts the
// game again (back to the list of the games) or returns to the menu
int MiSTer_ExitCode(void);

#endif
