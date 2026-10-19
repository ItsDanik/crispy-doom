// MiSTer hybrid core support, see mister_port.h

#include "mister_port.h"

#include <mister_hybrid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "../m_misc.h"

#define EXIT_TO_MENU 0
#define EXIT_RESTART 42 // danik_hybrid_launch.sh starts the game again
#define EXIT_RELOADED 43 // the same, after the core has settled

static const char *const TITLE = "CrispyDoom";

// A field is 262 lines of 400 pixels at 6.25MHz (hybrid_host.sv): 59.64Hz
#define FIELD_US 16768

// The games with a menu entry of their own (package/games.txt), and the
// shareware the entries of Doom, Heretic and Strife start when the full game
// is not there. `files` of the expansions are filled in by FindFiles(): the version
// that is there
static const char *const DoomFiles[] = { "DOOM.WAD", NULL };
static const char *const Doom1Files[] = { "DOOM1.WAD", NULL };
static const char *const Doom2Files[] = { "DOOM2.WAD", NULL };
static const char *const TNTFiles[] = { "TNT.WAD", NULL };
static const char *const PlutoniaFiles[] = { "PLUTONIA.WAD", NULL };
static const char *const HereticFiles[] = { "HERETIC.WAD", NULL };
static const char *const Heretic1Files[] = { "HERETIC1.WAD", NULL };
static const char *const HexenFiles[] = { "HEXEN.WAD", NULL };
static const char *const HexDDFiles[] = { "HEXEN.WAD", "HEXDD.WAD", NULL };
static const char *const StrifeFiles[] = { "STRIFE1.WAD", NULL };
static const char *const Strife0Files[] = { "STRIFE0.WAD", NULL };
static const char *SigilFiles[] = { "DOOM.WAD", NULL, NULL };
static const char *Sigil2Files[] = { "DOOM.WAD", NULL, NULL };
static const char *const NerveFiles[] = { "DOOM2.WAD", "NERVE.WAD", NULL };
static const char *MasterFiles[] = { "DOOM2.WAD", NULL, NULL };

static mh_game Games[] = {
    { "doom", "Doom", DoomFiles, NULL,
      "Doom was not found. Copy DOOM.WAD (or DOOM1.WAD of the shareware) to %s/." },
    { "doom1", "Doom (Shareware)", Doom1Files, "doom", NULL },
    { "sigil", "Doom - SIGIL", NULL, NULL,
      "SIGIL was not found. Copy SIGIL_v1_21.wad (or another version of SIGIL) "
      "and DOOM.WAD of The Ultimate Doom (Doom with four episodes) to %s/." },
    { "sigil2", "Doom - SIGIL II", NULL, NULL,
      "SIGIL II was not found. Copy SIGIL_II_V1_0.WAD and DOOM.WAD of The "
      "Ultimate Doom (Doom with four episodes) to %s/." },
    { "doom2", "Doom II", Doom2Files, NULL,
      "Doom II was not found. Copy DOOM2.WAD to %s/." },
    { "nerve", "Doom II - No Rest for the Living", NerveFiles, NULL,
      "No Rest for the Living was not found. Copy NERVE.WAD and DOOM2.WAD to "
      "%s/." },
    { "master", "Doom II - Master Levels", NULL, NULL,
      "The Master Levels were not found. Copy MASTERLEVELS.WAD (or the 20 WADs "
      "of the Master Levels CD: ATTACK.WAD, CANYON.WAD and so on) and DOOM2.WAD to "
      "%s/." },
    { "tnt", "Final Doom - TNT Evilution", TNTFiles, NULL,
      "TNT: Evilution (Final Doom) was not found. Copy TNT.WAD to %s/." },
    { "plutonia", "Final Doom - Plutonia Experiment", PlutoniaFiles, NULL,
      "The Plutonia Experiment (Final Doom) was not found. Copy PLUTONIA.WAD "
      "to %s/." },
    { "heretic", "Heretic", HereticFiles, NULL,
      "Heretic was not found. Copy HERETIC.WAD (or HERETIC1.WAD of the "
      "shareware) to %s/." },
    { "heretic1", "Heretic (Shareware)", Heretic1Files, "heretic", NULL },
    { "hexen", "Hexen", HexenFiles, NULL,
      "Hexen was not found. Copy HEXEN.WAD to %s/." },
    { "hexdd", "Hexen - Deathkings", HexDDFiles, NULL,
      "Hexen: Deathkings of the Dark Citadel was not found. Copy HEXDD.WAD "
      "and HEXEN.WAD (Hexen 1.1) to %s/." },
    { "strife", "Strife", StrifeFiles, NULL,
      "Strife was not found. Copy STRIFE1.WAD and VOICES.WAD (or STRIFE0.WAD "
      "of the shareware) to %s/." },
    { "strife0", "Strife (Shareware)", Strife0Files, "strife", NULL },
};
#define NUM_GAMES ((int)(sizeof(Games) / sizeof(Games[0])))

// How each starts, the same order: the program (Crispy Doom, Heretic, Hexen
// or Strife), the episode New Game starts (0: the episode menu), the
// expansion Crispy loads itself next to the IWAD, and the folder of the saved
// games of an expansion (with the IWAD's they would be mixed up)
enum { PROGRAM_DOOM, PROGRAM_HERETIC, PROGRAM_HEXEN, PROGRAM_STRIFE };
static const struct {
    int program, episode, expansion;
    const char *savedir;
} Setups[NUM_GAMES] = {
    { PROGRAM_DOOM, 0, MISTER_EXP_NONE, NULL },                   // doom
    { PROGRAM_DOOM, 0, MISTER_EXP_NONE, NULL },                   // doom1
    { PROGRAM_DOOM, 5, MISTER_EXP_NONE, "savegames/sigil" },      // sigil
    { PROGRAM_DOOM, 6, MISTER_EXP_NONE, "savegames/sigil2" },     // sigil2
    { PROGRAM_DOOM, 0, MISTER_EXP_NONE, NULL },                   // doom2
    { PROGRAM_DOOM, 2, MISTER_EXP_NERVE, "savegames/nerve" },     // nerve
    { PROGRAM_DOOM, 3, MISTER_EXP_MASTER, "savegames/master" },   // master
    { PROGRAM_DOOM, 0, MISTER_EXP_NONE, NULL },                   // tnt
    { PROGRAM_DOOM, 0, MISTER_EXP_NONE, NULL },                   // plutonia
    { PROGRAM_HERETIC, 0, MISTER_EXP_NONE, NULL },                // heretic
    { PROGRAM_HERETIC, 0, MISTER_EXP_NONE, NULL },                // heretic1
    { PROGRAM_HEXEN, 0, MISTER_EXP_NONE, NULL },                  // hexen
    { PROGRAM_HEXEN, 2, MISTER_EXP_NONE, "savegames/hexdd" },     // hexdd
    { PROGRAM_STRIFE, 0, MISTER_EXP_NONE, NULL },                 // strife
    { PROGRAM_STRIFE, 0, MISTER_EXP_NONE, NULL },                 // strife0
};

// The programs as packaged (package.sh) and as built (build/host/src/)
static const char *const Programs[][2] = {
    { "CrispyDoom", "crispy-doom" },
    { "CrispyHeretic", "crispy-heretic" },
    { "CrispyHexen", "crispy-hexen" },
    { "CrispyStrife", "crispy-strife" },
};

#define NO_GAME \
    "No game found. Copy DOOM.WAD, DOOM2.WAD, TNT.WAD, PLUTONIA.WAD, " \
    "HERETIC.WAD, HEXEN.WAD or STRIFE1.WAD (or the shareware's DOOM1.WAD, " \
    "HERETIC1.WAD, STRIFE0.WAD) to %s/."

// The player came through the list: quitting a game goes back to it
static int PickedFromList = 0;
static int Picked = -1;

// The first of `names` in the current folder (any case), or NULL
static char *FindFirst(const char *const *names)
{
    char *found;

    for (; *names != NULL; names++) {
        if ((found = M_FileCaseExists(*names)) != NULL)
            return found;
    }
    return NULL;
}

// 1 if the WAD `file` has the lump `name`
static int HasLump(const char *file, const char *name)
{
    char *path = M_FileCaseExists(file);
    FILE *f = path ? fopen(path, "rb") : NULL;
    unsigned char header[12], entry[16];
    long count, offset, i;
    int found = 0;

    free(path);
    if (f == NULL)
        return 0;
    if (fread(header, 1, 12, f) == 12) {
        count = header[4] | header[5] << 8 | header[6] << 16 | (long)header[7] << 24;
        offset = header[8] | header[9] << 8 | header[10] << 16 | (long)header[11] << 24;
        if (fseek(f, offset, SEEK_SET) == 0) {
            for (i = 0; i < count && !found && fread(entry, 1, 16, f) == 16; i++)
                found = !strncasecmp((char *)entry + 8, name, 8);
        }
    }
    fclose(f);
    return found;
}

// The 20 WADs of the Master Levels CD (d_pwad.c)
static const char *const MasterWads[] = {
    "ATTACK.WAD", "CANYON.WAD", "CATWALK.WAD", "COMBINE.WAD", "FISTULA.WAD",
    "GARRISON.WAD", "MANOR.WAD", "PARADOX.WAD", "SUBSPACE.WAD", "SUBTERRA.WAD",
    "TTRAP.WAD", "VIRGIL.WAD", "MINOS.WAD", "BLOODSEA.WAD", "MEPHISTO.WAD",
    "NESSUS.WAD", "GERYON.WAD", "VESPERAS.WAD", "BLACKTWR.WAD", "TEETH.WAD",
};

static void FindFiles(void)
{
    // the names Crispy knows (d_pwad.c); SIGIL II's with MIDI music first,
    // the game plays no MP3
    static const char *const sigil[] = {
        "SIGIL_V1_23_REG.wad", "SIGIL_V1_23.wad", "SIGIL_v1_21.wad",
        "SIGIL_v1_2.wad", "SIGIL.wad", NULL };
    static const char *const sigil2[] = {
        "SIGIL_II_V1_0.WAD", "SIGIL_II_MP3_V1_0.WAD", NULL };
    static const char *const master[] = { "MASTERLEVELS.WAD", NULL };
    // SIGIL is for The Ultimate Doom, the DOOM.WAD with episode 4
    int ultimate = HasLump("DOOM.WAD", "E4M1");
    // SIGIL there, but on a Doom of three episodes: say so, not "not found"
    static const char doom3[] =
        "%s needs The Ultimate Doom, Doom with four episodes. The DOOM.WAD in "
        "%s/ has three: copy the DOOM.WAD of The Ultimate Doom there.";
    static char sigil_msg[256], sigil2_msg[256];
    const int doom = M_FileCaseExists("DOOM.WAD") != NULL;
    unsigned i;
    int g;

    for (g = 0; g < NUM_GAMES; g++) {
        if (!strcmp(Games[g].id, "sigil")) {
            SigilFiles[1] = FindFirst(sigil);
            Games[g].files = SigilFiles[1] && ultimate ? SigilFiles : MH_NOT_THERE;
            if (SigilFiles[1] && doom && !ultimate) {
                M_snprintf(sigil_msg, sizeof(sigil_msg), doom3, "SIGIL", "%s");
                Games[g].missing = sigil_msg;
            }
        } else if (!strcmp(Games[g].id, "sigil2")) {
            Sigil2Files[1] = FindFirst(sigil2);
            Games[g].files = Sigil2Files[1] && ultimate ? Sigil2Files : MH_NOT_THERE;
            if (Sigil2Files[1] && doom && !ultimate) {
                M_snprintf(sigil2_msg, sizeof(sigil2_msg), doom3, "SIGIL II", "%s");
                Games[g].missing = sigil2_msg;
            }
        } else if (!strcmp(Games[g].id, "master")) {
            MasterFiles[1] = FindFirst(master);
            for (i = 0; !MasterFiles[1] && i < sizeof(MasterWads) / sizeof(*MasterWads); i++) {
                if (!M_FileCaseExists(MasterWads[i]))
                    break;
            }
            if (!MasterFiles[1] && i == sizeof(MasterWads) / sizeof(*MasterWads))
                MasterFiles[1] = MasterWads[0];
            Games[g].files = MasterFiles[1] ? MasterFiles : MH_NOT_THERE;
        }
    }
}

// The program this is, of Programs, and its path (exec of the other)
static int ThisProgram(char *dir, size_t size)
{
    char self[1024], *slash;
    ssize_t len;
    unsigned p;

    len = readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (len <= 0)
        return PROGRAM_DOOM;
    self[len] = '\0';
    slash = strrchr(self, '/');
    *slash = '\0';
    snprintf(dir, size, "%s", self);
    for (p = 0; p < sizeof(Programs) / sizeof(*Programs); p++) {
        if (!strcmp(slash + 1, Programs[p][0]) || !strcmp(slash + 1, Programs[p][1]))
            return p;
    }
    return PROGRAM_DOOM;
}

// The game the player picked runs in the other program: start it in the
// place of this one, with its command line, told which game it is
static void StartProgram(int program, const char *dir, char **argv)
{
    char path[1100], listed[2] = { '0' + PickedFromList, '\0' };
    int packaged;

    // packaged or as built, as this one
    snprintf(path, sizeof(path), "%s/%s", dir, Programs[program][0]);
    packaged = access(path, X_OK) == 0;
    if (!packaged)
        snprintf(path, sizeof(path), "%s/%s", dir, Programs[program][1]);
    setenv("MISTER_CRISPY_GAME", Games[Picked].id, 1);
    setenv("MISTER_CRISPY_LISTED", listed, 1);
    printf("mister: %s is in %s\n", Games[Picked].name, path);
    fflush(stdout);
    MH_Close();
    argv[0] = path;
    execv(path, argv);
    fprintf(stderr, "mister: cannot start %s\n", path);
}

int MiSTer_GameArgs(int *argc, char ***argv)
{
    char text[MH_MSG_COLS + 1], dir[1024];
    char **args, *extra, *arg;
    const char *handed;
    int pick = -1, n = 0, extras = 0, program, g;

    // the core first: the list and the messages are on its screen
    MH_Open();
    FindFiles();
    program = ThisProgram(dir, sizeof(dir));
    // picked by the other program, which started this one
    handed = getenv("MISTER_CRISPY_GAME");
    if (handed != NULL) {
        for (g = 0; g < NUM_GAMES; g++) {
            if (!strcmp(Games[g].id, handed))
                pick = g;
        }
        PickedFromList = getenv("MISTER_CRISPY_LISTED") && atoi(getenv("MISTER_CRISPY_LISTED"));
        unsetenv("MISTER_CRISPY_GAME");
        unsetenv("MISTER_CRISPY_LISTED");
    }
    if (pick < 0)
        pick = MH_PickGame(Games, NUM_GAMES, NULL, NO_GAME, &PickedFromList);
    if (pick < 0)
        return 0;
    Picked = pick;
    if (Setups[pick].program != program) {
        StartProgram(Setups[pick].program, dir, *argv);
        return 0;
    }
    if (MH_IsOpen()) {
        snprintf(text, sizeof(text), "Loading %s...", Games[pick].name);
        MH_UI_Message(TITLE, text, 0);
    }

    // MISTER_CRISPY_ARGS: more of the game's options, for tests ("-warp 29", or "-warp,29")
    extra = getenv("MISTER_CRISPY_ARGS");
    extra = M_StringDuplicate(extra ? extra : "");
    for (arg = strtok(extra, " ,"); arg != NULL; arg = strtok(NULL, " ,"))
        extras++;
    args = malloc((*argc + 8 + extras) * sizeof(*args));
    memcpy(args, *argv, *argc * sizeof(*args));
    n = *argc;
    extra = getenv("MISTER_CRISPY_ARGS");
    extra = M_StringDuplicate(extra ? extra : "");
    for (arg = strtok(extra, " ,"); arg != NULL; arg = strtok(NULL, " ,"))
        args[n++] = arg;
    args[n++] = "-iwad";
    args[n++] = (char *)Games[pick].files[0];
    // id's fix of TNT's MAP31 (the yellow key), where the player has it
    if (!strcmp(Games[pick].id, "tnt") && M_FileCaseExists("TNT31.WAD")) {
        args[n++] = "-file";
        args[n++] = "TNT31.WAD";
    }
    // Hexen without Deathkings, which Crispy loads itself when it is there
    // (d_pwad.c): the episode menu would ask for one of the two
    if (!strcmp(Games[pick].id, "hexen")) {
        args[n++] = "-nosideload";
    }
    // SIGIL as a PWAD, as made to be played: its title screen, music, demos
    if (Setups[pick].episode >= 5) {
        args[n++] = "-file";
        args[n++] = (char *)Games[pick].files[1];
    }
    if (Setups[pick].savedir != NULL) {
        M_MakeDirectory("savegames");
        args[n++] = "-savedir";
        args[n++] = (char *)Setups[pick].savedir;
    }
    args[n] = NULL;
    *argc = n;
    *argv = args;
    return 1;
}

int MiSTer_Episode(void)
{
    return Picked >= 0 ? Setups[Picked].episode : 0;
}

int MiSTer_Expansion(void)
{
    return Picked >= 0 ? Setups[Picked].expansion : MISTER_EXP_NONE;
}

static uint64_t NowUS(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int MiSTer_TimeUS(uint64_t *us)
{
    static uint32_t base;
    static int based = 0;
    static uint64_t last, last_at;

    // Once the core is gone (another core loaded) the field counter stands
    // still: the time goes on by the system clock, so that no loop of the
    // game waits for it for ever, and the next frame quits
    if (!MH_CheckAlive() || !MH_IsOpen()) {
        if (!based)
            return 0;
        *us = last + (NowUS() - last_at);
        return 1;
    }
    if (!based) {
        base = MH_FieldCounter();
        based = 1;
    }
    last = (uint64_t)(uint32_t)(MH_FieldCounter() - base) * FIELD_US;
    last_at = NowUS();
    *us = last;
    return 1;
}

int MiSTer_Option(int bit)
{
    // Resolution: 320x240 is its first value, the default, so the bit of 240
    // lines is set for 200 (core/CrispyDoom.sv)
    if (bit == MISTER_OSD_240)
        return MH_IsOpen() && !MH_OSD_GAME_BITS(MH_OSDStatus(), bit, 1);
    return MH_IsOpen() && MH_OSD_GAME_BITS(MH_OSDStatus(), bit, 1);
}

int MiSTer_Triggers(void)
{
    return MH_IsOpen() ? MH_OSDTriggers() : 0;
}

// GAME: 1 if the game's frames take most of a field: each is drawn a field
// ahead and the library shows it at its own field (MH_PresentAtField()), so
// that one a little slower than a field is still in time, for one field more
// latency. Decide by benchmark, hybrid/CLAUDE.md ("Frame pacing").
#define DRAW_AHEAD 1

static uint32_t LastField; // with DRAW_AHEAD the field the last frame is shown at
static int FieldValid = 0;

void MiSTer_ResetFrames(void)
{
    FieldValid = 0;
}

unsigned MiSTer_WaitFrame(void)
{
    uint32_t field;
    unsigned fields;

    if (!MH_IsOpen())
        return 0;
    if (!FieldValid) {
        LastField = MH_FieldCounter();
        FieldValid = 1;
    }
    if (DRAW_AHEAD && !MH_Interlaced()) {
        uint32_t target = LastField + 1;

        MH_WaitField(target - 3); // not before the field two before its own
        field = MH_FieldCounter();
        if ((int32_t)(field + 1 - target) > 0)
            target = field + 1; // too late for it: the next vblank
        if (!MH_IsOpen())
            return 0;
        MH_PresentAtField(target);
        fields = target - LastField;
        LastField = target;
        return fields;
    }
    MH_WaitFrame(LastField);
    field = MH_FieldCounter();
    fields = field - LastField; // 0: the core is gone
    LastField = field;
    return fields;
}

void MiSTer_HudRows(int height, int *top, int *bottom)
{
    int scale = height >= 480 ? 2 : 1;
    int t, b;

    MH_HudMargins(&t, &b);
    *top = t * scale;
    *bottom = height - b * scale;
}

void MiSTer_ShowHudOptions(int shown)
{
    if (MH_IsOpen())
        MH_SetMenuMask(shown ? 0 : 1); // "D0" of the HUD lines in CONF_STR
}

void MiSTer_ShowError(const char *message)
{
    if (MH_Open())
        MH_UI_Message(TITLE, message, MH_UI_WAIT | MH_UI_ERROR);
}

int MiSTer_ExitCode(void)
{
    // The player loaded the core again while the game ran
    if (MH_CoreReloaded())
        return EXIT_RELOADED;
    // Not if we are quitting because another core was loaded
    if (PickedFromList && MH_Open())
        return EXIT_RESTART;
    return EXIT_TO_MENU;
}
