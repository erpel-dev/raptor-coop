#pragma once

#include "rap.h"

#define MAX_PLAYERS 2

typedef struct
{
    int alive;              /* 1 = active in mission, 0 = spectator/dead */
    int x, y;
    int cx, cy;
    int pic;
    int basepic;
    int energy;             /* per-player shield energy */
    int addx, addy;
    int buttons[4];
    int draw;
    int flash;
    int oldx;               /* for bank-angle animation */
    int b2_flag;
    int b3_flag;
} Player;

extern Player players[MAX_PLAYERS];
extern int num_players;     /* 1 or 2 during a mission */
extern int coop_enabled;    /* SETUP.INI / session flag */

void RAP_InitPlayers(int count);
void RAP_SyncPlayerGlobalsFrom(int idx);
void RAP_StorePlayerGlobalsTo(int idx);
int RAP_LivingPlayerCount(void);
int RAP_NearestPlayerTo(int x, int y);
int RAP_FirstLivingPlayer(void);
Player *RAP_PlayerAt(int idx);
char *RAP_ShipColorRemap(int idx);
