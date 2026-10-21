#include "SDL.h"
#include "stdio.h"
#include "common.h"
#include "prefapi.h"
#include "windows.h"
#include "kbdapi.h"
#include "ptrapi.h"
#include "rap.h"
#include "demo.h"
#include "input.h"
#include "i_video.h"
#include "joyapi.h"
#include "player.h"

#define MAX_ADDX 10
#define MAX_ADDY 8

int control = 1;
int haptic;
int joy_ipt_MenuNew;

int k_Up;
int k_Down;
int k_Left;
int k_Right;
int k_Fire;
int k_FireSp;
int k_ChangeSp;
int k_Mega;

int m_lookup[3];
int j_lookup[4];

int buttons[4];

int xm, ym;
int g_addx, g_addy;

int ipt_start;
int control_pause;

/*------------------------------------------------------------------------
   IPT_GetButtons () - Reads in Joystick and Keyboard game buttons
  ------------------------------------------------------------------------*/
void 
IPT_GetButtons(
    void
)
{
    static int lasttick;
    int now = SDL_GetTicks();
    
    if (now - lasttick < 1000 / 26)
        return;
    
    lasttick += 1000 / 26;
    
    if (!ipt_start)
        return;

#if 0
    int num;
    
    if (control == I_JOYSTICK)
    {
        num = inp(0x2);
        
        num >>= 4;
        
        if ((num & 1) == 0)
            buttons[j_lookup[0]] = 1;
        if ((num & 2) == 0)
            buttons[j_lookup[1]] = 1;
        if ((num & 4) == 0)
            buttons[j_lookup[2]] = 1;
        if ((num & 8) == 0)
            buttons[j_lookup[3]] = 1;
    }
#endif
    
    if (KBD_Key(k_Fire))
        buttons[0] = 1;
    
    if (KBD_Key(k_FireSp))
        buttons[1] = 1;
    
    if (KBD_Key(k_ChangeSp))
        buttons[2] = 1;
    
    if (KBD_Key(k_Mega))
        buttons[3] = 1;
}

/*------------------------------------------------------------------------
IPT_GetJoyStick()
  ------------------------------------------------------------------------*/
void 
IPT_GetJoyStick(
    void
)
{
    int getstickx, getsticky;

    //Get Button

    if (JOY_GetMappedButton(FIRE))
        buttons[0] = 1;

    if (JOY_GetMappedButton(CHWEAPON))
        buttons[1] = 1;

    if (JOY_GetMappedButton(MEGABOMB))
        buttons[2] = 1;

    if (JOY_GetMappedButton(MEGAFIRE))
        buttons[3] = 1;
    
    if (JOY_GetAxis(JOYTRIGGERRIGHT) > 0)                         //Fire
        buttons[0] = 1;
    if (JOY_GetAxis(JOYTRIGGERLEFT) > 0)                          //Fire Special
        buttons[1] = 1;
    if (JOY_GetButton(JOYLEFTSHOULDER))                           //Change Special
        buttons[2] = 1;
    if (JOY_GetButton(JOYRIGHTSHOULDER))                          //Mega
        buttons[3] = 1;
       
    //Move Player DPad
           
    if (JOY_GetButton(JOYLEFT))
    {
        if (g_addx >= 0)
            g_addx = -1;
        g_addx--;
        if (-g_addx > MAX_ADDX)
            g_addx = -MAX_ADDX;
    }
    else if (JOY_GetButton(JOYRIGHT))
    {
             if (g_addx <= 0)
                 g_addx = 1;
             g_addx++;
             if (g_addx > MAX_ADDX)
                 g_addx = MAX_ADDX;
    }
    else
    {
        if (g_addx)
            g_addx /= 2;
    }
    
    if (JOY_GetButton(JOYUP))
    {
        if (g_addy >= 0)
            g_addy = -1;
        g_addy--;
        if (-g_addy > MAX_ADDY)
            g_addy = -MAX_ADDY;
    }
    else if (JOY_GetButton(JOYDOWN))
    {
             if (g_addy <= 0)
                 g_addy = 1;
             g_addy++;
             if (g_addy > MAX_ADDY)
                 g_addy = MAX_ADDY;
    }
    else
    {
        if (g_addy)
            g_addy /= 2;
    }

    //Move Player Analog Stick

    getstickx = JOY_GetAxis(JOYSTICKX);
    getsticky = JOY_GetAxis(JOYSTICKY);

    if (getstickx != 0)
    {
        if (getstickx > MAX_ADDX)
            getstickx = MAX_ADDX;
        if (getstickx < -MAX_ADDX)
            getstickx = -MAX_ADDX;
        g_addx = getstickx;
    }
    
    if (getsticky != 0)
    {
        if (getsticky > MAX_ADDY)
            getsticky = MAX_ADDY;
        if (getsticky < -MAX_ADDY)
            getsticky = -MAX_ADDY;
        g_addy = getsticky;
    }
}

/*------------------------------------------------------------------------
IPT_GetKeyBoard (
  ------------------------------------------------------------------------*/
void 
IPT_GetKeyBoard(
    void
)
{
    if (KBD_Key(k_Left) || KBD_Key(k_Right))
    {
        if (KBD_Key(k_Left))
        {
            if (g_addx >= 0)
                g_addx = -1;
            g_addx--;
            if (-g_addx > MAX_ADDX)
                g_addx = -MAX_ADDX;
        }
        else if (KBD_Key(k_Right))
        {
            if (g_addx <= 0)
                g_addx = 1;
            g_addx++;
            if (g_addx > MAX_ADDX)
                g_addx = MAX_ADDX;
        }
    }
    else
    {
        if (g_addx)
            g_addx /= 2;
    }
    
    if (KBD_Key(k_Up) || KBD_Key(k_Down))
    {
        if (KBD_Key(k_Up))
        {
            if (g_addy >= 0)
                g_addy = -1;
            g_addy--;
            if (-g_addy > MAX_ADDY)
                g_addy = -MAX_ADDY;
        }
        else if (KBD_Key(k_Down))
        {
            if (g_addy <= 0)
                g_addy = 1;
            g_addy++;
            if (g_addy > MAX_ADDY)
                g_addy = MAX_ADDY;
        }
    }
    else
    {
        if (g_addy)
            g_addy /= 2;
    }
}

/*------------------------------------------------------------------------
IPT_GetMouse (
  ------------------------------------------------------------------------*/
void 
IPT_GetMouse(
    void
)
{
    int plx, ply, ptrx, ptry;
    
    plx = playerx + (PLAYERWIDTH / 2);
    ply = playery + (PLAYERHEIGHT / 2);
    
    ptrx = cur_mx;
    ptry = cur_my;
    
    xm = ptrx - plx;
    ym = ptry - ply;
    
    if (xm)
    {
        xm >>= 3;
        
        if (!xm)
            xm = 1;
        else if (xm > 10)
            xm = 10;
        else if (xm < -10)
            xm = -10;
    }
    
    if (ym)
    {
        ym >>= 3;
        
        if (!ym)
            ym = 1;
        else if (ym > 10)
            ym = 10;
        else if (ym < -10)
            ym = -10;
    }
    
    g_addx = xm;
    g_addy = ym;

    if (mouseb1)
    {
        if (touchmode)
            buttons[0] = 1;
        else
            buttons[m_lookup[0]] = 1;
    }

    if (mouseb2)
    {
        if (touchmode)
            buttons[1] = 1;
        else
            buttons[m_lookup[1]] = 1;
    }

    if (mouseb3)
    {
        if (touchmode)
            buttons[2] = 1;
        else
            buttons[m_lookup[2]] = 1;
    }
}

/*------------------------------------------------------------------------
IPT_MouseGrab (
  ------------------------------------------------------------------------*/
bool 
IPT_MouseGrab(
    void
)
{
    return ipt_start;
}

/***************************************************************************
IPT_Init () - Initializes INPUT system
 ***************************************************************************/
void 
IPT_Init(
    void
)
{
    
    I_SetGrabMouseCallback(IPT_MouseGrab);
    // ipt_tsm = TSM_NewService(IPT_GetButtons, 26, 254, 1);
    IPT_CalJoy();
}

/***************************************************************************
IPT_DeInit() - Freeze up resources used by INPUT system
 ***************************************************************************/
void 
IPT_DeInit(
    void
)
{
    // TSM_DelService(ipt_tsm);
}

/***************************************************************************
IPT_Start() - Tranfers control from PTRAPI stuff to IPT stuff
 ***************************************************************************/
void 
IPT_Start(
    void
)
{
    PTR_DrawCursor(0);
    PTR_Pause(1);
    ipt_start = 1;
    // TSM_ResumeService(ipt_tsm);
}

/***************************************************************************
IPT_End() - Tranfers control from IPT stuff to PTR stuff
 ***************************************************************************/
void 
IPT_End(
    void
)
{
    ipt_start = 0;
    // TSM_PauseService(ipt_tsm);
    PTR_Pause(0);
    PTR_DrawCursor(0);
}

/***************************************************************************
IPT_MovePlayer() - Perform player movement for current input device
 ***************************************************************************/
void 
IPT_MovePlayer(
    void
)
{
    IPT_MovePlayerPlr(0);
}

/* Apply stick/dpad style acceleration into g_addx/g_addy from a JoyPadState. */
static void
IPT_ApplyJoyPadMove(
    JoyPadState *jp
)
{
    int sx = jp->StickX;
    int sy = jp->StickY;

    for (int i = 0; i < 4; ++i)
        if (jp->buttons[i])
            buttons[i] = 1;

    if (jp->TriggerRight > 0)
        buttons[0] = 1;
    if (jp->TriggerLeft > 0)
        buttons[1] = 1;
    if (jp->LeftShoulder)
        buttons[2] = 1;
    if (jp->RightShoulder)
        buttons[3] = 1;

    if (jp->Left)
    {
        if (g_addx >= 0)
            g_addx = -1;
        g_addx--;
        if (-g_addx > MAX_ADDX)
            g_addx = -MAX_ADDX;
    }
    else if (jp->Right)
    {
        if (g_addx <= 0)
            g_addx = 1;
        g_addx++;
        if (g_addx > MAX_ADDX)
            g_addx = MAX_ADDX;
    }
    else
    {
        if (g_addx)
            g_addx /= 2;
    }

    if (jp->Up)
    {
        if (g_addy >= 0)
            g_addy = -1;
        g_addy--;
        if (-g_addy > MAX_ADDY)
            g_addy = -MAX_ADDY;
    }
    else if (jp->Down)
    {
        if (g_addy <= 0)
            g_addy = 1;
        g_addy++;
        if (g_addy > MAX_ADDY)
            g_addy = MAX_ADDY;
    }
    else
    {
        if (g_addy)
            g_addy /= 2;
    }

    if (sx != 0)
    {
        if (sx > 0)
            sx *= 2;
        if (sx > MAX_ADDX)
            sx = MAX_ADDX;
        if (sx < 0)
            sx *= 2;
        if (sx < -MAX_ADDX)
            sx = -MAX_ADDX;
        g_addx = sx;
    }

    if (sy != 0)
    {
        if (sy > 0)
            sy *= 2;
        if (sy > MAX_ADDY)
            sy = MAX_ADDY;
        if (sy < 0)
            sy *= 2;
        if (sy < -MAX_ADDY)
            sy = -MAX_ADDY;
        g_addy = sy;
    }
}

/* P2 fallback keys when no dedicated gamepad is available.
 * Left-side cluster: WASD + Left Ctrl/Alt (pairs with P1 arrows + Right Ctrl). */
static void
IPT_GetP2KeyBoard(
    void
)
{
    if (KBD_Key(SC_A) || KBD_Key(SC_D))
    {
        if (KBD_Key(SC_A))
        {
            if (g_addx >= 0)
                g_addx = -1;
            g_addx--;
            if (-g_addx > MAX_ADDX)
                g_addx = -MAX_ADDX;
        }
        else if (KBD_Key(SC_D))
        {
            if (g_addx <= 0)
                g_addx = 1;
            g_addx++;
            if (g_addx > MAX_ADDX)
                g_addx = MAX_ADDX;
        }
    }
    else if (g_addx)
        g_addx /= 2;

    if (KBD_Key(SC_W) || KBD_Key(SC_S))
    {
        if (KBD_Key(SC_W))
        {
            if (g_addy >= 0)
                g_addy = -1;
            g_addy--;
            if (-g_addy > MAX_ADDY)
                g_addy = -MAX_ADDY;
        }
        else if (KBD_Key(SC_S))
        {
            if (g_addy <= 0)
                g_addy = 1;
            g_addy++;
            if (g_addy > MAX_ADDY)
                g_addy = MAX_ADDY;
        }
    }
    else if (g_addy)
        g_addy /= 2;

    if (KBD_Key(SC_CTRL))
        buttons[0] = 1;
    if (KBD_Key(SC_ALT))
        buttons[1] = 1;
    if (KBD_Key(SC_M))
        buttons[2] = 1;
    if (KBD_Key(SC_N))
        buttons[3] = 1;
}

void
IPT_SampleButtonsPlr(
    int pidx
)
{
    Player *pl;
    JoyPadState jp;
    int pad;
    int fire_key, firesp_key;

    if (pidx < 0 || pidx >= num_players)
        return;

    pl = &players[pidx];
    pl->buttons[0] = pl->buttons[1] = pl->buttons[2] = pl->buttons[3] = 0;

    if (pidx == 0)
    {
        /* In co-op, put P1 fire on Right Ctrl/Alt (arrow-key side). */
        if (num_players > 1)
        {
            fire_key = SC_RIGHT_CTRL;
            firesp_key = SC_RIGHT_ALT;
        }
        else
        {
            fire_key = k_Fire;
            firesp_key = k_FireSp;
        }

        if (KBD_Key(fire_key))
            pl->buttons[0] = 1;
        /* Enter as extra P1 fire in co-op — avoids arrow+RCtrl keyboard ghosting. */
        if (num_players > 1 && KBD_Key(SC_ENTER))
            pl->buttons[0] = 1;
        if (KBD_Key(firesp_key))
            pl->buttons[1] = 1;
        if (KBD_Key(k_ChangeSp))
            pl->buttons[2] = 1;
        if (KBD_Key(k_Mega))
            pl->buttons[3] = 1;

        if (control == I_JOYSTICK || (control == I_KEYBOARD && IPT_JoyPadAttached(0) && num_players == 1))
        {
            IPT_ReadJoyPad(0, &jp);
            buttons[0] = buttons[1] = buttons[2] = buttons[3] = 0;
            int key_addx = g_addx, key_addy = g_addy;
            IPT_ApplyJoyPadMove(&jp);
            // A neutral pad must not damp or replace keyboard movement.
            if (!jp.StickX && !jp.Left && !jp.Right)
                g_addx = key_addx;
            if (!jp.StickY && !jp.Up && !jp.Down)
                g_addy = key_addy;
            pl->buttons[0] |= buttons[0];
            pl->buttons[1] |= buttons[1];
            pl->buttons[2] |= buttons[2];
            pl->buttons[3] |= buttons[3];
        }
        return;
    }

    /* Player 2: prefer a dedicated gamepad. */
    pad = (control == I_JOYSTICK) ? 1 : 0;
    if (IPT_JoyPadAttached(pad))
    {
        IPT_ReadJoyPad(pad, &jp);
        buttons[0] = buttons[1] = buttons[2] = buttons[3] = 0;
        g_addx = pl->addx;
        g_addy = pl->addy;
        IPT_ApplyJoyPadMove(&jp);
        pl->buttons[0] = buttons[0];
        pl->buttons[1] = buttons[1];
        pl->buttons[2] = buttons[2];
        pl->buttons[3] = buttons[3];
        pl->addx = g_addx;
        pl->addy = g_addy;
    }
    else
    {
        /* No second pad: WASD + Left Ctrl/Alt. */
        buttons[0] = buttons[1] = buttons[2] = buttons[3] = 0;
        g_addx = pl->addx;
        g_addy = pl->addy;
        IPT_GetP2KeyBoard();
        pl->buttons[0] = buttons[0];
        pl->buttons[1] = buttons[1];
        pl->buttons[2] = buttons[2];
        pl->buttons[3] = buttons[3];
        pl->addx = g_addx;
        pl->addy = g_addy;
    }
}

/***************************************************************************
IPT_MovePlayerPlr() - Move a specific co-op player
 ***************************************************************************/
void
IPT_MovePlayerPlr(
    int pidx
)
{
    Player *pl;
    int delta;

    if (demo_mode == DEMO_PLAYBACK || control_pause)
        return;

    if (pidx < 0 || pidx >= num_players)
        return;

    pl = &players[pidx];
    if (!pl->alive || pl->energy <= 0)
        return;

    RAP_SyncPlayerGlobalsFrom(pidx);

    if (!control_pause)
    {
        buttons[0] = buttons[1] = buttons[2] = buttons[3] = 0;
        g_addx = pl->addx;
        g_addy = pl->addy;

        if (pidx == 0)
        {
            switch (control)
            {
            case I_KEYBOARD:
            default:
                IPT_GetKeyBoard();
                IPT_SampleButtonsPlr(0);
                break;

            case I_JOYSTICK:
            {
                JoyPadState jp;
                if (!IPT_JoyPadAttached(0))
                {
                    IPT_GetKeyBoard();
                    IPT_SampleButtonsPlr(0);
                    break;
                }
                IPT_ReadJoyPad(0, &jp);
                IPT_ApplyJoyPadMove(&jp);
                pl->buttons[0] = buttons[0];
                pl->buttons[1] = buttons[1];
                pl->buttons[2] = buttons[2];
                pl->buttons[3] = buttons[3];
                break;
            }

            case I_MOUSE:
                IPT_SampleButtonsPlr(0);
                buttons[0] = buttons[1] = buttons[2] = buttons[3] = 0;
                IPT_GetMouse();
                for (int i = 0; i < 4; i++)
                    pl->buttons[i] |= buttons[i];
                break;
            }
        }
        else
        {
            /* SampleButtonsPlr updates pl->addx/addy and fills g_addx/g_addy. */
            IPT_SampleButtonsPlr(pidx);
            g_addx = pl->addx;
            g_addy = pl->addy;
        }
    }

    playerx += g_addx;
    playery += g_addy;

    if (startendwave == -1)
    {
        if (playery < MINPLAYERY)
        {
            playery = MINPLAYERY;
            g_addy = 0;
        }
        else if (playery > MAXPLAYERY)
        {
            playery = MAXPLAYERY;
            g_addy = 0;
        }

        if (playerx < PLAYERMINX)
        {
            playerx = PLAYERMINX;
            g_addx = 0;
        }
        else if (playerx + PLAYERWIDTH > PLAYERMAXX)
        {
            playerx = PLAYERMAXX - PLAYERWIDTH;
            g_addx = 0;
        }
    }

    delta = abs(playerx - pl->oldx);
    delta >>= 2;

    if (delta > 3)
        delta = 3;

    if (playerx < pl->oldx)
    {
        if (playerbasepic + delta > playerpic)
            playerpic++;
    }
    else if (playerx > pl->oldx)
    {
        if (playerbasepic - delta < playerpic)
            playerpic--;
    }
    else
    {
        if (playerpic > playerbasepic)
            playerpic--;
        else if (playerpic < playerbasepic)
            playerpic++;
    }

    pl->oldx = playerx;
    player_cx = playerx + (PLAYERWIDTH / 2);
    player_cy = playery + (PLAYERHEIGHT / 2);

    pl->addx = g_addx;
    pl->addy = g_addy;
    RAP_StorePlayerGlobalsTo(pidx);
}

/***************************************************************************
IPT_PauseControl() - Lets routines run without letting user control anyting
 ***************************************************************************/
void 
IPT_PauseControl(
    int flag
)
{
    control_pause = flag;
}

/***************************************************************************
IPT_FMovePlayer() - Forces player to move addx/addy
 ***************************************************************************/
void 
IPT_FMovePlayer(
    int addx,              // INPUT : add to x pos
    int addy               // INPUT : add to y pos
)
{
    int i = RAP_FirstLivingPlayer();
    RAP_SyncPlayerGlobalsFrom(i);
    g_addx = addx;
    g_addy = addy;
    players[i].addx = addx;
    players[i].addy = addy;

    playerx += g_addx;
    playery += g_addy;
    player_cx = playerx + (PLAYERWIDTH / 2);
    player_cy = playery + (PLAYERHEIGHT / 2);
    RAP_StorePlayerGlobalsTo(i);
}

/***************************************************************************
IPT_LoadPrefs() - Load Input Prefs from setup.ini
 ***************************************************************************/
void 
IPT_LoadPrefs(
    void
)
{
    opt_detail = INI_GetPreferenceLong("Setup", "Detail", 1);
    control = INI_GetPreferenceLong("Setup", "Control", 0);
    haptic = INI_GetPreferenceLong("Setup", "Haptic", 1);
    joy_ipt_MenuNew = INI_GetPreferenceLong("Setup", "joy_ipt_MenuNew", 0);
    coop_enabled = INI_GetPreferenceLong("Setup", "Coop", 0);
    {
        const char *env = getenv("RAPTOR_COOP");
        if (env && env[0] == '1')
            coop_enabled = 1;
    }
    
    k_Up = INI_GetPreferenceLong("Keyboard", "MoveUp", SC_UP);
    k_Down = INI_GetPreferenceLong("Keyboard", "MoveDn", SC_DOWN);
    k_Left = INI_GetPreferenceLong("Keyboard", "MoveLeft", SC_LEFT);
    k_Right = INI_GetPreferenceLong("Keyboard", "MoveRight", SC_RIGHT);
    k_Fire = INI_GetPreferenceLong("Keyboard", "Fire", SC_CTRL);
    k_FireSp = INI_GetPreferenceLong("Keyboard", "FireSp", SC_ALT);
    k_ChangeSp = INI_GetPreferenceLong("Keyboard", "ChangeSp", SC_SPACE);
    k_Mega = INI_GetPreferenceLong("Keyboard", "MegaFire", SC_RIGHT_SHIFT);
    
    m_lookup[0] = INI_GetPreferenceLong("Mouse", "Fire", 0);
    m_lookup[1] = INI_GetPreferenceLong("Mouse", "FireSp", 1);
    m_lookup[2] = INI_GetPreferenceLong("Mouse", "ChangeSp", 2);
    
    j_lookup[0] = INI_GetPreferenceLong("JoyStick", "Fire", 0);
    j_lookup[1] = INI_GetPreferenceLong("JoyStick", "FireSp", 1);
    j_lookup[2] = INI_GetPreferenceLong("JoyStick", "ChangeSp", 2);
    j_lookup[3] = INI_GetPreferenceLong("JoyStick", "MegaFire", 3);
}
