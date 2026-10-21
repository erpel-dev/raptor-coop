#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SDL.h"
#include "demo.h"
#include "anims.h"
#include "bonus.h"
#include "eshot.h"
#include "entypes.h"
#include "input.h"
#include "joyapi.h"
#include "kbdapi.h"
#include "loadsave.h"
#include "objects.h"
#include "player.h"
#include "ptrapi.h"
#include "shots.h"

extern int control_pause, k_Up, k_Down, k_Left, k_Right;
extern int k_Fire, k_FireSp, k_ChangeSp, k_Mega, m_lookup[3];
extern int cur_play, max_play, think_cnt, objuse_flag, fx_volume;
extern RECORD playback[MAX_DEMO + 1];
extern SDL_GameController* ControllerHandles[MAX_CONTROLLERS];

// Assertions must remain enabled in Release/RelWithDebInfo builds.
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
    if (std::getenv("GITHUB_ACTIONS")) \
        std::fprintf(stderr, "::error file=%s,line=%d::Failed: %s\n", __FILE__, __LINE__, #condition); \
    std::exit(1); } } while (0)

static void Reset(int count = 2)
{
    std::memset(keyboard, 0, sizeof(int) * 256);
    std::memset(buttons, 0, sizeof(buttons));
    std::memset(&plr, 0, sizeof(plr));
    plr.sweapon = -1;
    reg_flag = 1;
    OBJS_Init();
    OBJS_Add(S_ENERGY);
    RAP_InitPlayers(count);
    demo_mode = DEMO_OFF;
    control = I_KEYBOARD;
    control_pause = 0;
    startendwave = -1;
    godmode = 0;
    fx_volume = 0;
    curplr_diff = DIFF_1;
    ANIMS_Clear();
    joy_num_pads = 0;
    objuse_flag = think_cnt = 0;
    for (int i = 0; i < count; i++)
    {
        players[i].x = players[i].oldx = 120 + 40 * i;
        players[i].y = 120;
        players[i].cx = players[i].x + 16;
        players[i].cy = 136;
        players[i].energy = 100;
        players[i].basepic = players[i].pic = 3;
    }
    k_Up = SC_UP; k_Down = SC_DOWN; k_Left = SC_LEFT; k_Right = SC_RIGHT;
    k_Fire = SC_CTRL; k_FireSp = SC_ALT;
    k_ChangeSp = SC_SPACE; k_Mega = SC_RIGHT_SHIFT;
    for (int i = 0; i < 3; i++) m_lookup[i] = i;
    for (int i = 0; i < 4; i++) j_lookup[i] = i;
    mouseb1 = mouseb2 = mouseb3 = touchmode = 0;
}

static void KeyboardAndMouse()
{
    Reset();
    keyboard[SC_RIGHT] = keyboard[SC_A] = 1;
    keyboard[SC_RIGHT_CTRL] = keyboard[SC_CTRL] = 1;
    IPT_MovePlayerPlr(0); IPT_MovePlayerPlr(1);
    CHECK(players[0].x > 120 && players[1].x < 160);
    CHECK(players[0].buttons[0] && players[1].buttons[0]);
    for (int count = 1; count <= 2; count++)
    {
        Reset(count);
        control = I_MOUSE;
        cur_mx = players[0].cx + 80; cur_my = players[0].cy;
        mouseb1 = mouseb2 = mouseb3 = 1;
        IPT_MovePlayerPlr(0);
        CHECK(players[0].x > 120);
        CHECK(players[0].buttons[0] && players[0].buttons[1] && players[0].buttons[2]);
        mouseb1 = mouseb2 = mouseb3 = 0;
        keyboard[count == 1 ? SC_CTRL : SC_RIGHT_CTRL] = 1;
        IPT_MovePlayerPlr(0);
        CHECK(players[0].buttons[0] && !players[0].buttons[1] && !players[0].buttons[2]);
    }
    std::puts("PASS keyboard independence and mouse/keyboard button merging (1P and 2P)");
}

static int DeviceIndex(SDL_JoystickID instance)
{
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_JoystickGetDeviceInstanceID(i) == instance)
            return i;
    return -1;
}

static void Controllers()
{
    Reset(1);
    CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    for (int i = 0; i < 2; i++)
        CHECK(SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 0) >= 0);
    IPT_CalJoy(); CHECK(joy_num_pads == 2);
    // SDL versions differ in device enumeration order. Drive the actual slots.
    SDL_Joystick *pad[2]; SDL_JoystickID instance[2];
    for (int i = 0; i < 2; i++)
    {
        pad[i] = SDL_GameControllerGetJoystick(ControllerHandles[i]); CHECK(pad[i]);
        instance[i] = SDL_JoystickInstanceID(pad[i]);
        // Older SDL mappings span the full signed range for trigger axes.
        CHECK(SDL_JoystickSetVirtualAxis(pad[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768) == 0);
        CHECK(SDL_JoystickSetVirtualAxis(pad[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768) == 0);
    }
    CHECK(SDL_JoystickSetVirtualAxis(pad[0], SDL_CONTROLLER_AXIS_LEFTX, 32767) == 0);
    CHECK(SDL_JoystickSetVirtualButton(pad[0], SDL_CONTROLLER_BUTTON_A, 1) == 0);
    SDL_JoystickUpdate();
    IPT_MovePlayerPlr(0);
    CHECK(players[0].x > 120 && players[0].buttons[0]);
    // A neutral connected pad must preserve keyboard acceleration.
    SDL_JoystickSetVirtualAxis(pad[0], SDL_CONTROLLER_AXIS_LEFTX, 0);
    SDL_JoystickUpdate();
    players[0].addx = 4; keyboard[SC_RIGHT] = 1;
    IPT_MovePlayerPlr(0); CHECK(players[0].addx == 5);
    // In controller mode, each ship reads its own controller.
    num_players = 2; players[1] = players[0];
    control = I_JOYSTICK;
    SDL_JoystickSetVirtualAxis(pad[0], SDL_CONTROLLER_AXIS_LEFTX, 32767);
    SDL_JoystickSetVirtualAxis(pad[1], SDL_CONTROLLER_AXIS_LEFTX, -32768);
    SDL_JoystickSetVirtualButton(pad[1], SDL_CONTROLLER_BUTTON_B, 1);
    SDL_JoystickUpdate();
    IPT_MovePlayerPlr(0); IPT_MovePlayerPlr(1);
    CHECK(players[0].addx > 0 && players[1].addx < 0);
    CHECK(players[0].buttons[0] && !players[1].buttons[0] && players[1].buttons[1]);
    IPT_CloJoy(1);
    for (int i = 0; i < 2; i++) CHECK(SDL_JoystickDetachVirtual(DeviceIndex(instance[i])) == 0);
    SDL_Quit();
    std::puts("PASS mixed keyboard/gamepad movement and two independent controllers");
}

static void Replays()
{
    Reset(1);
    demo_mode = DEMO_RECORD; cur_play = 1;
    players[0].x = 200; players[0].y = 80; players[0].pic = 4;
    players[0].buttons[0] = players[0].buttons[2] = 1;
    // Deliberately stale legacy values must not be recorded.
    playerx = playery = 0; std::memset(buttons, 0, sizeof(buttons));
    CHECK(DEMO_Think() == 0);
    CHECK(LE_SHORT(playback[1].px) == 200 && playback[1].b1 && playback[1].b3);
    std::memset(players[0].buttons, 0, sizeof(players[0].buttons));
    players[0].x = 120;
    demo_mode = DEMO_PLAYBACK; cur_play = 1; max_play = 2;
    CHECK(DEMO_Think() == 0);
    CHECK(players[0].x == 200 && players[0].y == 80 && players[0].pic == 4);
    CHECK(players[0].buttons[0] && players[0].buttons[2]);
    RAP_SyncPlayerGlobalsFrom(0);
    CHECK(playerx == 200 && player_cx == 216);
    std::puts("PASS canonical player-state replay record/playback round trip");
}

static int fired[MAX_PLAYERS];
static int Fire(int) { fired[g_shot_owner]++; return 1; }

static void Weapons()
{
    Reset();
    OBJS_Add(S_DUMB_MISSLE); OBJS_Add(S_MINI_GUN);
    plr.sweapon = S_DUMB_MISSLE;
    players[0].alive = 0; players[0].energy = 0;
    players[1].buttons[1] = 1;
    RAP_UsePlayerWeapons(); CHECK(plr.sweapon == S_MINI_GUN);
    players[1].buttons[1] = 1;
    RAP_UsePlayerWeapons(); CHECK(plr.sweapon == S_MINI_GUN); // held, no repeat
    RAP_UsePlayerWeapons(); // release
    players[1].buttons[1] = 1;
    RAP_UsePlayerWeapons(); CHECK(plr.sweapon == S_DUMB_MISSLE);
    players[0].alive = 1; players[0].energy = 100;
    players[1].b2_flag = 0;
    players[0].buttons[1] = players[1].buttons[1] = 1;
    RAP_UsePlayerWeapons(); CHECK(plr.sweapon == S_MINI_GUN); // one shared change
    // Exercise the actual action dispatch with an asset-free weapon action.
    OBJS_Add(S_FORWARD_GUNS);
    OBJS_GetLib(S_FORWARD_GUNS)->actf = Fire;
    plr.sweapon = -1;
    fired[0] = fired[1] = 0;
    players[0].buttons[0] = players[1].buttons[0] = 1;
    RAP_UsePlayerWeapons(); CHECK(fired[0] == 1 && fired[1] == 1);
    std::puts("PASS survivor weapon switching, hold/release, simultaneous change and shot ownership");
}

static void Shields()
{
    Reset();
    players[0].energy = 0; players[0].alive = 0;
    CHECK(RAP_LowestLivingEnergy() == 100);
    players[1].energy = 8; CHECK(RAP_LowestLivingEnergy() == 8);
    players[1].energy = 7; CHECK(RAP_LowestLivingEnergy() == 7);
    players[1].alive = 0; CHECK(RAP_LowestLivingEnergy() == 0);
    Reset(); curplr_diff = DIFF_1;
    players[0].energy = players[1].energy = 50;
    for (int i = 0; i < 97; i++) OBJS_Think();
    CHECK(players[0].energy == 51 && players[1].energy == 51);
    players[0].energy = 100; players[1].energy = 50;
    for (int i = 0; i < 97; i++) OBJS_Think();
    CHECK(players[0].energy == 100 && players[1].energy == 51);
    players[0].alive = 0; players[0].energy = 0;
    for (int i = 0; i < 97; i++) OBJS_Think();
    CHECK(players[0].energy == 0 && players[1].energy == 52);
    startendwave = 10;
    for (int i = 0; i < 97; i++) OBJS_Think();
    CHECK(players[1].energy == 52);
    Reset(); curplr_diff = DIFF_1;
    OBJS_GetLib(S_SUPER_SHIELD)->start_cnt = 50;
    OBJS_Add(S_SUPER_SHIELD);
    for (int i = 0; i < 97; i++) OBJS_Think();
    CHECK(OBJS_GetAmt(S_SUPER_SHIELD) == 51); // one shared recharge, not two
    std::puts("PASS living-player shield warnings and fair, capped, mission-only regeneration");
}

extern BONUS first_bonus, last_bonus;
extern ESHOT *ESHOT_Get(void);

static void ControllerRemoval()
{
    Reset(); CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    SDL_JoystickID instance[3];
    for (int i = 0; i < 2; i++)
    {
        int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 0);
        CHECK(device >= 0); instance[i] = SDL_JoystickGetDeviceInstanceID(device);
    }
    IPT_CalJoy(); IPT_CalJoy();
    CHECK(joy_num_pads == 2 && IPT_JoyPadAttached(0) && IPT_JoyPadAttached(1));
    for (int i = 0; i < 2; i++)
        instance[i] = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(ControllerHandles[i]));
    SDL_GameController *survivor = SDL_GameControllerFromInstanceID(instance[1]);
    CHECK(SDL_JoystickDetachVirtual(DeviceIndex(instance[0])) == 0);
    SDL_PumpEvents(); IPT_CloJoy(0);
    CHECK(joy_num_pads == 1 && !IPT_JoyPadAttached(0) && IPT_JoyPadAttached(1));
    // The remaining controller keeps its owner even though its device index changed.
    control = I_JOYSTICK; keyboard[SC_RIGHT] = keyboard[SC_RIGHT_CTRL] = 1;
    IPT_MovePlayerPlr(0);
    CHECK(players[0].x > 120 && players[0].buttons[0]);
    CHECK(SDL_GameControllerFromInstanceID(instance[1]) == survivor);
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 0);
    CHECK(device >= 0); instance[2] = SDL_JoystickGetDeviceInstanceID(device);
    IPT_CalJoy(); IPT_CalJoy();
    CHECK(joy_num_pads == 2 && IPT_JoyPadAttached(0) && IPT_JoyPadAttached(1));
    CHECK(SDL_GameControllerFromInstanceID(instance[1]) == survivor);
    IPT_CloJoy(1);
    // Repeated enumeration must not leak extra controller references.
    CHECK(!SDL_GameControllerFromInstanceID(instance[1]));
    CHECK(!SDL_GameControllerFromInstanceID(instance[2]));
    IPT_CalJoy();
    CHECK(SDL_JoystickDetachVirtual(DeviceIndex(instance[1])) == 0);
    CHECK(SDL_JoystickDetachVirtual(DeviceIndex(instance[2])) == 0);
    SDL_PumpEvents(); IPT_CloJoy(0);
    CHECK(joy_num_pads == 0 && !IPT_JoyPadAttached(0) && !IPT_JoyPadAttached(1));
    control = I_KEYBOARD; keyboard[SC_D] = keyboard[SC_CTRL] = 1;
    IPT_MovePlayerPlr(1);
    CHECK(players[1].x > 160 && players[1].buttons[0]);
    SDL_Quit();
    std::puts("PASS disconnect fallback, surviving controller ownership, reconnect and handle lifetime");
}

static void LaserTargets()
{
    for (int near = 0; near < 2; near++)
    {
        Reset(); ESHOT_Clear();
        int far = 1 - near;
        players[near].cx = players[far].cx = 150;
        players[near].cy = 90; players[far].cy = 160;
        SPRITE source = {}; SPRITE_SHIP enemy = {};
        enemy.lib = &source; enemy.x = 154; enemy.y = 30;
        ESHOT_LIB laser = {}; laser.num_frames = 10; laser.hits = 2;
        ESHOT *shot = ESHOT_Get(); CHECK(shot);
        shot->lib = &laser; shot->en = &enemy; shot->type = ES_LASER;
        ESHOT_Think();
        CHECK(players[near].energy == 98 && players[far].energy == 100);
        CHECK(shot->move.y2 >= 88 && shot->move.y2 <= 91);
        players[near].alive = 0;
        ESHOT_Think(); CHECK(players[far].energy == 98);
        // A ship above the emitter and ships outside the beam cannot intercept it.
        players[near].alive = 1; players[near].cy = 10; players[far].cx = 200;
        ESHOT_Think();
        CHECK(players[near].energy == 98 && players[far].energy == 98);
        CHECK(shot->move.y2 == 200);
        ESHOT_Clear();
    }
    std::puts("PASS nearest laser interception in both player orders, dead targets and misses");
}

static void AddHealthPickup()
{
    BONUS_Clear();
    BONUS_Add(S_ENERGY, players[0].cx - MAP_LEFT, players[0].cy - 1);
}

static void HealthPickups()
{
    for (int hurt = 0; hurt < 2; hurt++)
    {
        Reset(); players[1] = players[0];
        players[hurt].energy = 25;
        AddHealthPickup(); BONUS_Think();
        CHECK(players[hurt].energy == 50 && players[1-hurt].energy == 100);
        CHECK(first_bonus.next == &last_bonus);
    }
    Reset(); players[1] = players[0];
    players[0].energy = 60; players[1].energy = 20;
    AddHealthPickup(); BONUS_Think();
    CHECK(players[0].energy == 60 && players[1].energy == 45);
    Reset(); players[1] = players[0];
    AddHealthPickup(); BONUS_Think();
    CHECK(first_bonus.next != &last_bonus); // neither can use it
    players[1].energy = 50; BONUS_Think();
    CHECK(players[1].energy == 75 && first_bonus.next == &last_bonus);
    Reset(); players[1] = players[0];
    OBJS_GetLib(S_SUPER_SHIELD)->start_cnt = 50; OBJS_Add(S_SUPER_SHIELD);
    players[1].energy = 25;
    AddHealthPickup(); BONUS_Think();
    CHECK(players[1].energy == 50 && OBJS_GetAmt(S_SUPER_SHIELD) == 50);
    players[1].energy = 100;
    AddHealthPickup(); BONUS_Think();
    CHECK(OBJS_GetAmt(S_SUPER_SHIELD) == 75 && first_bonus.next == &last_bonus);
    Reset(); players[1] = players[0]; players[0].alive = 0; players[0].energy = 0;
    players[1].energy = 90;
    AddHealthPickup(); BONUS_Think();
    CHECK(players[0].energy == 0 && players[1].energy == 100);
    std::puts("PASS overlapping pickup fairness, full-health preservation, shield recharge and dead ships");
}

static void DamageEvents()
{
    Reset(); players[0].energy = 5; players[1].energy = 10;
    OBJS_Add(S_MINI_GUN);
    OBJS_SubEnergyPlr(1, 2);
    CHECK(players[1].energy == 8 && RAP_LowestLivingEnergy() == 5);
    CHECK(RAP_ApplyLowShieldDamage() == 1 && !OBJS_IsEquip(S_MINI_GUN));
    CHECK(RAP_ApplyLowShieldDamage() == 0); // consumed exactly once
    OBJS_Add(S_MINI_GUN); OBJS_Add(S_DUMB_MISSLE);
    int count = OBJS_GetNum();
    OBJS_SubEnergyPlr(0, 1); OBJS_SubEnergyPlr(1, 1);
    CHECK(RAP_ApplyLowShieldDamage() == 1 && OBJS_GetNum() == count - 1);
    CHECK(RAP_ApplyLowShieldDamage() == 0);
    Reset(); players[0].energy = 10; players[1].energy = 50;
    OBJS_Add(S_MINI_GUN); OBJS_SubEnergyPlr(0, 1);
    CHECK(RAP_ApplyLowShieldDamage() == 0 && OBJS_IsEquip(S_MINI_GUN));
    players[1].energy = 5; // no new damage; an old hit must not carry over
    CHECK(RAP_ApplyLowShieldDamage() == 0 && OBJS_IsEquip(S_MINI_GUN));
    OBJS_Add(S_SUPER_SHIELD); OBJS_SubEnergyPlr(0, 1);
    CHECK(RAP_ApplyLowShieldDamage() == 0 && OBJS_IsEquip(S_MINI_GUN));
    Reset(); players[0].energy = players[1].energy = 5;
    OBJS_SubEnergyPlr(1, 1);
    CHECK(RAP_ApplyLowShieldDamage() == 0); // no destructible weapon, no false damage message
    std::puts("PASS per-player low-shield hits, one shared loss per frame, event reset and shield absorption");
}

static void MissionExit()
{
    Reset(); IPT_PauseControl(1);
    players[0].addx = 7; players[1].addy = 3;
    IPT_MovePlayerPlr(0); IPT_MovePlayerPlr(1);
    CHECK(players[0].x == 120 && players[1].y == 120); // no uncontrolled drift
    for (int i = 0; i < END_FLYOFF; i++) RAP_FlyPlayersOffscreen();
    CHECK(players[0].y < -PLAYERHEIGHT && players[1].y < -PLAYERHEIGHT);
    CHECK(players[0].y == players[1].y && players[0].x < players[1].x);
    CHECK(players[0].energy == 100 && players[1].energy == 100);
    CHECK(players[1].cx == players[1].x + PLAYERWIDTH/2);
    for (int survivor = 0; survivor < 2; survivor++)
    {
        Reset(); players[1-survivor].alive = 0; players[1-survivor].energy = 0;
        for (int i = 0; i < END_FLYOFF; i++) RAP_FlyPlayersOffscreen();
        CHECK(players[survivor].y < -PLAYERHEIGHT && players[1-survivor].y == 120);
        CHECK(players[survivor].x >= 152 && players[survivor].x <= 168);
    }
    Reset(1);
    for (int i = 0; i < END_FLYOFF; i++) RAP_FlyPlayersOffscreen();
    CHECK(players[0].y < -PLAYERHEIGHT);
    std::puts("PASS two-ship formation exit, either single survivor, 1P exit and paused input");
}

int main()
{
    KeyboardAndMouse(); Controllers(); Replays(); Weapons(); Shields();
    ControllerRemoval(); LaserTargets(); HealthPickups(); DamageEvents(); MissionExit();
    std::puts("All 10 co-op regression groups passed");
    return 0;
}
