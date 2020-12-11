//
// Copyright(C) 2020 by Ryan Krafnick
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
//	DSDA Tools
//

#ifndef __DSDA__
#define __DSDA__

void dsda_ReadCommandLine(void);
void dsda_DisplayNotifications(void);
void dsda_WatchCard(card_t card);
void dsda_WatchCrush(mobj_t* thing, int damage);
void dsda_WatchDamage(mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage);
void dsda_WatchDeath(mobj_t* thing);
void dsda_WatchKill(player_t* player, mobj_t* target);
void dsda_WatchResurrection(mobj_t* target);
void dsda_WatchSpawn(mobj_t* spawned);
void dsda_WatchIconSpawn(mobj_t* spawned);
void dsda_WatchCommand(void);
void dsda_WatchBeforeLevelSetup(void);
void dsda_WatchAfterLevelSetup(void);
void dsda_WatchLevelCompletion(void);
void dsda_WatchWeaponFire(weapontype_t weapon);
void dsda_WatchSecret(void);
void dsda_WatchDeferredInitNew(skill_t skill, int episode, int map);
void dsda_WatchNewGame(void);
void dsda_WatchLevelReload(int* reloaded);
void dsda_WatchRecordDemo(const char* name);
void dsda_WriteAnalysis(void);

int dsda_MaxKillRequirement(void);
char* dsda_NewDemoName(void);

#define DBG_MAX_DDD_FRAMES 2048
typedef struct {
  int gametic;
  int evdata2, evdata3; // mouse/joystick x/y move from SDL << 4, see SDL/i_video.c I_GetEvent()
  int mousex, mousey; // relative mouse move after accel+sens, see g_game.c G_Responder()
  short angleturn; // ticcmd->angleturn, see g_game.c G_BuildTicCmd()
  short carry; // angleturn fractional carry
} dbg_ddd_frame_t;
extern dbg_ddd_frame_t dbg_ddd_frames[DBG_MAX_DDD_FRAMES];
extern dbg_ddd_frame_t* g_dbg_ddd_cur_frame;

#endif
