#include "player.h"
#include "objects.h"
#include "input.h"
#include "shots.h"
#include "fx.h"
#include <string.h>

Player players[MAX_PLAYERS];
int num_players = 1;
int coop_enabled = 0;

/* Player 2: VGA red ramp 32-47 → yellow ramp 48-63. */
static char p2_ship_remap[256];
static int p2_ship_remap_ready;

static void
RAP_InitShipRemap(
    void
)
{
    int i;

    if (p2_ship_remap_ready)
        return;

    for (i = 0; i < 256; i++)
        p2_ship_remap[i] = (char)i;

    for (i = 0; i < 16; i++)
        p2_ship_remap[32 + i] = (char)(48 + i);

    p2_ship_remap_ready = 1;
}

void
RAP_InitPlayers(
    int count
)
{
    int i;
    int energy;

    if (count < 1)
        count = 1;
    if (count > MAX_PLAYERS)
        count = MAX_PLAYERS;

    num_players = count;
    energy = OBJS_GetAmt(S_ENERGY);
    if (energy < 1)
        energy = 1;

    memset(players, 0, sizeof(players));

    for (i = 0; i < num_players; i++)
    {
        players[i].alive = 1;
        players[i].draw = 1;
        players[i].energy = energy;
        players[i].basepic = playerbasepic;
        players[i].pic = playerbasepic;
        players[i].y = PLAYERINITY;
        players[i].oldx = PLAYERINITX;
        if (num_players == 1)
            players[i].x = PLAYERINITX;
        else if (i == 0)
            players[i].x = PLAYERINITX - 40;
        else
            players[i].x = PLAYERINITX + 40;

        if (players[i].x < PLAYERMINX)
            players[i].x = PLAYERMINX;
        if (players[i].x + PLAYERWIDTH > PLAYERMAXX)
            players[i].x = PLAYERMAXX - PLAYERWIDTH;

        players[i].cx = players[i].x + (PLAYERWIDTH / 2);
        players[i].cy = players[i].y + (PLAYERHEIGHT / 2);
    }

    RAP_InitShipRemap();
    RAP_SyncPlayerGlobalsFrom(0);
}

void
RAP_SyncPlayerGlobalsFrom(
    int idx
)
{
    if (idx < 0 || idx >= MAX_PLAYERS)
        idx = 0;

    playerx = players[idx].x;
    playery = players[idx].y;
    player_cx = players[idx].cx;
    player_cy = players[idx].cy;
    playerpic = players[idx].pic;
    playerbasepic = players[idx].basepic;
    g_flash = players[idx].flash;
    g_addx = players[idx].addx;
    g_addy = players[idx].addy;
}

void
RAP_StorePlayerGlobalsTo(
    int idx
)
{
    if (idx < 0 || idx >= MAX_PLAYERS)
        return;

    players[idx].x = playerx;
    players[idx].y = playery;
    players[idx].cx = player_cx;
    players[idx].cy = player_cy;
    players[idx].pic = playerpic;
    players[idx].basepic = playerbasepic;
    players[idx].flash = g_flash;
    players[idx].addx = g_addx;
    players[idx].addy = g_addy;
}

int
RAP_LivingPlayerCount(
    void
)
{
    int i, n = 0;
    for (i = 0; i < num_players; i++)
    {
        if (players[i].alive && players[i].energy > 0)
            n++;
    }
    return n;
}

int
RAP_FirstLivingPlayer(
    void
)
{
    int i;
    for (i = 0; i < num_players; i++)
    {
        if (players[i].alive && players[i].energy > 0)
            return i;
    }
    return 0;
}

int
RAP_NearestPlayerTo(
    int x,
    int y
)
{
    int i, best = RAP_FirstLivingPlayer();
    int bestd = 0x7fffffff;

    for (i = 0; i < num_players; i++)
    {
        int dx, dy, d;
        if (!players[i].alive || players[i].energy <= 0)
            continue;
        dx = players[i].cx - x;
        dy = players[i].cy - y;
        d = dx * dx + dy * dy;
        if (d < bestd)
        {
            bestd = d;
            best = i;
        }
    }
    return best;
}

Player *
RAP_PlayerAt(
    int idx
)
{
    if (idx < 0 || idx >= MAX_PLAYERS)
        return &players[0];
    return &players[idx];
}

char *
RAP_ShipColorRemap(
    int idx
)
{
    if (idx != 1)
        return NULL;

    RAP_InitShipRemap();
    return p2_ship_remap;
}

/* Keep warning and damage decisions independent of dead players' zero energy. */
int RAP_LowestLivingEnergy(void)
{
    int lowest = MAX_SHIELD;
    int found = 0;
    for (int i = 0; i < num_players; i++)
    {
        if (!players[i].alive || players[i].energy <= 0)
            continue;
        if (players[i].energy < lowest)
            lowest = players[i].energy;
        found = 1;
    }
    return found ? lowest : 0;
}

void RAP_UsePlayerWeapons(void)
{
    int changed_weapon = 0;
    for (int pi = 0; pi < num_players; pi++)
    {
        Player *pl = &players[pi];
        if (!pl->alive || pl->energy <= 0 || !pl->draw)
            continue;

        RAP_SyncPlayerGlobalsFrom(pi);
        g_shot_owner = pi;
        g_flash = 0;

        if (pl->buttons[0])
        {
            OBJS_Use(S_FORWARD_GUNS);
            OBJS_Use(S_PLASMA_GUNS);
            OBJS_Use(S_MICRO_MISSLE);
            pl->buttons[0] = 0;
            if (plr.sweapon != -1)
                OBJS_Use(plr.sweapon);
        }

        if (pl->buttons[1])
        {
            pl->buttons[1] = 0;
            if (!pl->b2_flag)
            {
                SND_Patch(FX_SWEP, 127);
                pl->b2_flag = 1;
                if (!changed_weapon)
                {
                    OBJS_GetNext();
                    changed_weapon = 1;
                }
            }
        }
        else if (pl->b2_flag == 1)
            pl->b2_flag = 0;

        if (pl->buttons[2])
        {
            pl->buttons[2] = 0;
            if (!pl->b3_flag)
            {
                pl->b3_flag = 1;
                OBJS_Use(S_MEGA_BOMB);
            }
        }
        else if (pl->b3_flag == 1)
            pl->b3_flag = 0;

        pl->flash = g_flash;
        RAP_StorePlayerGlobalsTo(pi);
    }
    g_shot_owner = RAP_FirstLivingPlayer();
    RAP_SyncPlayerGlobalsFrom(g_shot_owner);
}

/* Consume damage once per frame, even when no weapon-loss rule applies. */
int RAP_ApplyLowShieldDamage(void)
{
    int living = 0, damaged = 0, all_low = 1;
    for (int i = 0; i < num_players; i++)
    {
        if (players[i].alive && players[i].energy > 0)
        {
            living++;
            if (players[i].damage_pending)
                damaged = 1;
            if (players[i].energy > SHIELD_LOW)
                all_low = 0;
        }
        players[i].damage_pending = 0;
    }
    if (living && damaged && all_low && !godmode && startendwave == -1
        && OBJS_GetAmt(S_SUPER_SHIELD) < 1)
        return OBJS_LoseObj();
    return 0;
}

void RAP_FlyPlayersOffscreen(void)
{
    int living = RAP_LivingPlayerCount();
    for (int i = 0; i < num_players; i++)
    {
        Player *pl = &players[i];
        if (!pl->alive || pl->energy <= 0 || !pl->draw)
            continue;
        int target_x = 160;
        if (living > 1)
            target_x += i == 0 ? -40 : 40;
        if (pl->x < target_x - 8)
            pl->x += 8;
        else if (pl->x > target_x + 8)
            pl->x -= 8;
        pl->y -= 8;
        pl->cx = pl->x + PLAYERWIDTH / 2;
        pl->cy = pl->y + PLAYERHEIGHT / 2;
        pl->oldx = pl->x;
        pl->addx = pl->addy = 0;
    }
    RAP_SyncPlayerGlobalsFrom(RAP_FirstLivingPlayer());
}
