//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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

// P_enemy.c

#include <stdlib.h>
#include "doomdef.h"
#include "i_system.h"
#include "i_timer.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "v_video.h"

/*
================
=
= P_Move
=
= Move in the current direction
= returns false if the move is blocked
================
*/

fixed_t xspeed[8] =
    { FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000 };
fixed_t yspeed[8] =
    { 0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000 };

#define	MAXSPECIALCROSS		8
extern line_t **spechit; // [crispy] remove SPECHIT limit
extern int numspechit;

boolean P_Move(mobj_t * actor)
{
    fixed_t tryx, tryy;
    line_t *ld;
    boolean good;

    if (actor->movedir == DI_NODIR)
    {
        return (false);
    }
    tryx = actor->x + actor->info->speed * xspeed[actor->movedir];
    tryy = actor->y + actor->info->speed * yspeed[actor->movedir];
    if (!P_TryMove(actor, tryx, tryy))
    {                           // open any specials
        if (actor->flags & MF_FLOAT && floatok)
        {                       // must adjust height
            if (actor->z < tmfloorz)
            {
                actor->z += FLOATSPEED;
            }
            else
            {
                actor->z -= FLOATSPEED;
            }
            actor->flags |= MF_INFLOAT;
            return (true);
        }
        if (!numspechit)
        {
            return false;
        }
        actor->movedir = DI_NODIR;
        good = false;
        while (numspechit--)
        {
            ld = spechit[numspechit];
            // if the special isn't a door that can be opened, return false
            if (P_UseSpecialLine(actor, ld))
            {
                good = true;
            }
        }
        return (good);
    }
    else
    {
        actor->flags &= ~MF_INFLOAT;
    }
    if (!(actor->flags & MF_FLOAT))
    {
        if (actor->z > actor->floorz)
        {
            P_HitFloor(actor);
        }
        actor->z = actor->floorz;
    }
    return (true);
}

//----------------------------------------------------------------------------
//
// FUNC P_TryWalk
//
// Attempts to move actor in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor returns FALSE.
// If move is either clear of block only by a door, returns TRUE and sets.
// If a door is in the way, an OpenDoor call is made to start it opening.
//
//----------------------------------------------------------------------------

boolean P_TryWalk(mobj_t * actor)
{
    if (!P_Move(actor))
    {
        return (false);
    }
    actor->movecount = P_Random() & 15;
    return (true);
}

/*
================
=
= P_NewChaseDir
=
================
*/

dirtype_t opposite[] =
    { DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST, DI_EAST, DI_NORTHEAST,
    DI_NORTH, DI_NORTHWEST, DI_NODIR
};

dirtype_t diags[] =
    { DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST };

void P_NewChaseDir(mobj_t * actor)
{
    fixed_t deltax, deltay;
    dirtype_t d[3];
    dirtype_t tdir, olddir, turnaround;

    if (!actor->target)
        I_Error("P_NewChaseDir: called with no target");

    olddir = actor->movedir;
    turnaround = opposite[olddir];

    deltax = actor->target->x - actor->x;
    deltay = actor->target->y - actor->y;
    if (deltax > 10 * FRACUNIT)
        d[1] = DI_EAST;
    else if (deltax < -10 * FRACUNIT)
        d[1] = DI_WEST;
    else
        d[1] = DI_NODIR;
    if (deltay < -10 * FRACUNIT)
        d[2] = DI_SOUTH;
    else if (deltay > 10 * FRACUNIT)
        d[2] = DI_NORTH;
    else
        d[2] = DI_NODIR;

// try direct route
    if (d[1] != DI_NODIR && d[2] != DI_NODIR)
    {
        actor->movedir = diags[((deltay < 0) << 1) + (deltax > 0)];
        if (actor->movedir != turnaround && P_TryWalk(actor))
            return;
    }

// try other directions
    if (P_Random() > 200 || abs(deltay) > abs(deltax))
    {
        tdir = d[1];
        d[1] = d[2];
        d[2] = tdir;
    }

    if (d[1] == turnaround)
        d[1] = DI_NODIR;
    if (d[2] == turnaround)
        d[2] = DI_NODIR;

    if (d[1] != DI_NODIR)
    {
        actor->movedir = d[1];
        if (P_TryWalk(actor))
            return;             /*either moved forward or attacked */
    }

    if (d[2] != DI_NODIR)
    {
        actor->movedir = d[2];
        if (P_TryWalk(actor))
            return;
    }

/* there is no direct path to the player, so pick another direction */

    if (olddir != DI_NODIR)
    {
        actor->movedir = olddir;
        if (P_TryWalk(actor))
            return;
    }

    if (P_Random() & 1)         /*randomly determine direction of search */
    {
        for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
        {
            if (tdir != turnaround)
            {
                actor->movedir = tdir;
                if (P_TryWalk(actor))
                    return;
            }
        }
    }
    else
    {
        // Iterate over all movedirs.
        tdir = DI_SOUTHEAST;

        for (;;)
        {
            if (tdir != turnaround)
            {
                actor->movedir = tdir;
                if (P_TryWalk(actor))
                    return;
            }

            if (tdir == DI_EAST)
            {
                break;
            }

            --tdir;
        }
    }

    if (turnaround != DI_NODIR)
    {
        actor->movedir = turnaround;
        if (P_TryWalk(actor))
            return;
    }

    actor->movedir = DI_NODIR;  // can't move
}

//---------------------------------------------------------------------------
//
// FUNC P_LookForMonsters
//
//---------------------------------------------------------------------------

#define MONS_LOOK_RANGE (20*64*FRACUNIT)
#define MONS_LOOK_LIMIT 64

boolean P_LookForMonsters(mobj_t * actor)
{
    int count;
    mobj_t *mo;
    thinker_t *think;

    if (!P_CheckSight(players[0].mo, actor))
    {                           // Player can't see monster
        return (false);
    }
    count = 0;
    for (think = thinkercap.next; think != &thinkercap; think = think->next)
    {
        if (think->function != P_MobjThinker)
        {                       // Not a mobj thinker
            continue;
        }
        mo = (mobj_t *) think;
        if (!(mo->flags & MF_COUNTKILL) || (mo == actor) || (mo->health <= 0))
        {                       // Not a valid monster
            continue;
        }
        if (P_AproxDistance(actor->x - mo->x, actor->y - mo->y)
            > MONS_LOOK_RANGE)
        {                       // Out of range
            continue;
        }
        if (P_Random() < 16)
        {                       // Skip
            continue;
        }
        if (count++ > MONS_LOOK_LIMIT)
        {                       // Stop searching
            return (false);
        }
        if (!P_CheckSight(actor, mo))
        {                       // Out of sight
            continue;
        }
        // Found a target monster
        actor->target = mo;
        return (true);
    }
    return (false);
}

/*
================
=
= P_LookForPlayers
=
= If allaround is false, only look 180 degrees in front
= returns true if a player is targeted
================
*/

boolean P_LookForPlayers(mobj_t * actor, boolean allaround)
{
    int c;
    int stop;
    player_t *player;
    angle_t an;
    fixed_t dist;

    if (!netgame && players[0].health <= 0)
    {                           // Single player game and player is dead, look for monsters
        return (P_LookForMonsters(actor));
    }
    c = 0;
    stop = (actor->lastlook - 1) & 3;
    for (;; actor->lastlook = (actor->lastlook + 1) & 3)
    {
        if (!playeringame[actor->lastlook])
            continue;

        if (c++ == 2 || actor->lastlook == stop)
            return false;       // done looking

        player = &players[actor->lastlook];
        if (player->health <= 0)
            continue;           // dead
        if (!P_CheckSight(actor, player->mo))
            continue;           // out of sight

        if (!allaround)
        {
            an = R_PointToAngle2(actor->x, actor->y,
                                 player->mo->x, player->mo->y) - actor->angle;
            if (an > ANG90 && an < ANG270)
            {
                dist = P_AproxDistance(player->mo->x - actor->x,
                                       player->mo->y - actor->y);
                // if real close, react anyway
                if (dist > MELEERANGE)
                    continue;   // behind back
            }
        }
        if (player->mo->flags & MF_SHADOW)
        {                       // Player is invisible
            if ((P_AproxDistance(player->mo->x - actor->x,
                                 player->mo->y - actor->y) > 2 * MELEERANGE)
                && P_AproxDistance(player->mo->momx, player->mo->momy)
                < 5 * FRACUNIT)
            {                   // Player is sneaking - can't detect
                return (false);
            }
            if (P_Random() < 225)
            {                   // Player isn't sneaking, but still didn't detect
                return (false);
            }
        }
        actor->target = player->mo;
        return (true);
    }
    return (false);
}

/*
===============================================================================

						ACTION ROUTINES

===============================================================================
*/

/*
==============
=
= A_Look
=
= Stay in state until a player is sighted
=
==============
*/

void A_Look(mobj_t * actor)
{
    mobj_t *targ;

    actor->threshold = 0;       // any shot will wake up
    targ = actor->subsector->sector->soundtarget;
    if (targ && (targ->flags & MF_SHOOTABLE))
    {
        actor->target = targ;
        if (actor->flags & MF_AMBUSH)
        {
            if (P_CheckSight(actor, actor->target))
                goto seeyou;
        }
        else
            goto seeyou;
    }


    if (!P_LookForPlayers(actor, false))
        return;

// go into chase state
  seeyou:
    if (actor->info->seesound)
    {
        int sound;

/*
		switch (actor->info->seesound)
		{
		case sfx_posit1:
		case sfx_posit2:
		case sfx_posit3:
			sound = sfx_posit1+P_Random()%3;
			break;
		case sfx_bgsit1:
		case sfx_bgsit2:
			sound = sfx_bgsit1+P_Random()%2;
			break;
		default:
			sound = actor->info->seesound;
			break;
		}
*/
        sound = actor->info->seesound;
        if (actor->flags2 & MF2_BOSS)
        {                       // Full volume
            S_StartSound(NULL, sound);
        }
        else
        {
            S_StartSound(actor, sound);
        }
    }
    P_SetMobjState(actor, actor->info->seestate);
}


/*
==============
=
= A_Chase
=
= Actor has a melee attack, so it tries to close as fast as possible
=
==============
*/

void A_Chase(mobj_t * actor)
{
    int delta;

    if (actor->reactiontime)
    {
        actor->reactiontime--;
    }

    // Modify target threshold
    if (actor->threshold)
    {
        actor->threshold--;
    }

    if (gameskill == sk_nightmare)
    {                           // Monsters move faster in nightmare mode
        actor->tics -= actor->tics / 2;
        if (actor->tics < 3)
        {
            actor->tics = 3;
        }
    }

//
// turn towards movement direction if not there yet
//
    if (actor->movedir < 8)
    {
        actor->angle &= (7 << 29);
        delta = actor->angle - (actor->movedir << 29);
        if (delta > 0)
        {
            actor->angle -= ANG90 / 2;
        }
        else if (delta < 0)
        {
            actor->angle += ANG90 / 2;
        }
    }

    if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {                           // look for a new target
        if (P_LookForPlayers(actor, true))
        {                       // got a new target
            return;
        }
        P_SetMobjState(actor, actor->info->spawnstate);
        return;
    }

//
// don't attack twice in a row
//
    if (actor->flags & MF_JUSTATTACKED)
    {
        actor->flags &= ~MF_JUSTATTACKED;
        if (gameskill != sk_nightmare)
            P_NewChaseDir(actor);
        return;
    }

//
// check for melee attack
//      
    if (actor->info->meleestate && P_CheckMeleeRange(actor))
    {
        if (actor->info->attacksound)
            S_StartSound(actor, actor->info->attacksound);
        P_SetMobjState(actor, actor->info->meleestate);
        return;
    }

//
// check for missile attack
//
    if (actor->info->missilestate)
    {
        if (gameskill < sk_nightmare && actor->movecount)
            goto nomissile;
        if (!P_CheckMissileRange(actor))
            goto nomissile;
        P_SetMobjState(actor, actor->info->missilestate);
        actor->flags |= MF_JUSTATTACKED;
        return;
    }
  nomissile:

//
// possibly choose another target
//
    if (netgame && !actor->threshold && !P_CheckSight(actor, actor->target))
    {
        if (P_LookForPlayers(actor, true))
            return;             // got a new target
    }

//
// chase towards player
//
    if (--actor->movecount < 0 || !P_Move(actor))
    {
        P_NewChaseDir(actor);
    }

//
// make active sound
//
    if (actor->info->activesound && P_Random() < 3)
    {
        if (actor->type == MT_WIZARD && P_Random() < 128)
        {
            S_StartSound(actor, actor->info->seesound);
        }
        else if (actor->type == MT_SORCERER2)
        {
            S_StartSound(NULL, actor->info->activesound);
        }
        else
        {
            S_StartSound(actor, actor->info->activesound);
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FaceTarget
//
//----------------------------------------------------------------------------

void A_FaceTarget(mobj_t * actor)
{
    if (!actor->target)
    {
        return;
    }
    actor->flags &= ~MF_AMBUSH;
    actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x,
                                   actor->target->y);
    if (actor->target->flags & MF_SHADOW)
    {                           // Target is a ghost
        actor->angle += P_SubRandom() << 21;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Pain
//
//----------------------------------------------------------------------------

void A_Pain(mobj_t * actor)
{
    if (actor->info->painsound)
    {
        S_StartSound(actor, actor->info->painsound);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HeadAttack
//
//----------------------------------------------------------------------------

void A_HeadAttack(mobj_t * actor)
{
    int i;
    mobj_t *fire;
    mobj_t *baseFire;
    mobj_t *mo;
    mobj_t *target;
    int randAttack;
    static int atkResolve1[] = { 50, 150 };
    static int atkResolve2[] = { 150, 200 };
    int dist;

    // Ice ball             (close 20% : far 60%)
    // Fire column  (close 40% : far 20%)
    // Whirlwind    (close 40% : far 20%)
    // Distance threshold = 8 cells

    target = actor->target;
    if (target == NULL)
    {
        return;
    }
    A_FaceTarget(actor);
    if (P_CheckMeleeRange(actor))
    {
        P_DamageMobj(target, actor, actor, HITDICE(6));
        return;
    }
    dist = P_AproxDistance(actor->x - target->x, actor->y - target->y)
        > 8 * 64 * FRACUNIT;
    randAttack = P_Random();
    if (randAttack < atkResolve1[dist])
    {                           // Ice ball
        P_SpawnMissile(actor, target, MT_HEADFX1);
        S_StartSound(actor, sfx_hedat2);
    }
    else if (randAttack < atkResolve2[dist])
    {                           // Fire column
        baseFire = P_SpawnMissile(actor, target, MT_HEADFX3);
        if (baseFire != NULL)
        {
            P_SetMobjState(baseFire, S_HEADFX3_4);      // Don't grow
            for (i = 0; i < 5; i++)
            {
                fire = P_SpawnMobj(baseFire->x, baseFire->y,
                                   baseFire->z, MT_HEADFX3);
                if (i == 0)
                {
                    S_StartSound(actor, sfx_hedat1);
                }
                fire->target = baseFire->target;
                fire->angle = baseFire->angle;
                fire->momx = baseFire->momx;
                fire->momy = baseFire->momy;
                fire->momz = baseFire->momz;
                fire->damage = 0;
                fire->health = (i + 1) * 2;
                P_CheckMissileSpawn(fire);
            }
        }
    }
    else
    {                           // Whirlwind
        mo = P_SpawnMissile(actor, target, MT_WHIRLWIND);
        if (mo != NULL)
        {
            mo->z -= 32 * FRACUNIT;
            mo->special1.m = target;
            mo->special2.i = 50;  // Timer for active sound
            mo->health = 20 * TICRATE;       // Duration
            S_StartSound(actor, sfx_hedat3);
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Scream
//
//----------------------------------------------------------------------------

void A_Scream(mobj_t * actor)
{
    switch (actor->type)
    {
        case MT_CHICPLAYER:
        case MT_SORCERER1:
        case MT_MINOTAUR:
            // Make boss death sounds full volume
            S_StartSound(NULL, actor->info->deathsound);
            break;
        case MT_PLAYER:
            // Handle the different player death screams
            if (actor->special1.i < 10)
            {                   // Wimpy death sound
                S_StartSound(actor, sfx_plrwdth);
            }
            else if (actor->health > -50)
            {                   // Normal death sound
                S_StartSound(actor, actor->info->deathsound);
            }
            else if (actor->health > -100)
            {                   // Crazy death sound
                S_StartSound(actor, sfx_plrcdth);
            }
            else
            {                   // Extreme death sound
                S_StartSound(actor, sfx_gibdth);
            }
            break;
        default:
            S_StartSound(actor, actor->info->deathsound);
            break;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Explode
//
// Handles a bunch of exploding things.
//
//----------------------------------------------------------------------------

void A_Explode(mobj_t * actor)
{
    int damage;

    damage = 128;
    switch (actor->type)
    {
        case MT_FIREBOMB:      // Time Bombs
            actor->z += 32 * FRACUNIT;
            actor->flags &= ~MF_SHADOW;
            break;
        case MT_MNTRFX2:       // Minotaur floor fire
            damage = 24;
            break;
        case MT_SOR2FX1:       // D'Sparil missile
            damage = 80 + (P_Random() & 31);
            break;
        default:
            break;
    }
    P_RadiusAttack(actor, actor->target, damage);
    P_HitFloor(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_BossDeath
//
// Trigger special effects if all bosses are dead.
//
//----------------------------------------------------------------------------

void A_BossDeath(mobj_t * actor)
{
    mobj_t *mo;
    thinker_t *think;
    line_t dummyLine;
    static mobjtype_t bossType[6] = {
        MT_HEAD,
        MT_MINOTAUR,
        MT_SORCERER2,
        MT_HEAD,
        MT_MINOTAUR,
        -1
    };

    if (gamemap != 8)
    {                           // Not a boss level
        return;
    }
    if (actor->type != bossType[gameepisode - 1])
    {                           // Not considered a boss in this episode
        return;
    }
    // Make sure all bosses are dead
    for (think = thinkercap.next; think != &thinkercap; think = think->next)
    {
        if (think->function != P_MobjThinker)
        {                       // Not a mobj thinker
            continue;
        }
        mo = (mobj_t *) think;
        if ((mo != actor) && (mo->type == actor->type) && (mo->health > 0))
        {                       // Found a living boss
            return;
        }
    }
    if (gameepisode > 1)
    {                           // Kill any remaining monsters
        P_Massacre();
    }
    dummyLine.tag = 666;
    EV_DoFloor(&dummyLine, lowerFloor);
}

//----------------------------------------------------------------------------
//
// PROC A_SkullPop
//
//----------------------------------------------------------------------------

void A_SkullPop(mobj_t * actor)
{
    mobj_t *mo;
    player_t *player;

    actor->flags &= ~MF_SOLID;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z + 48 * FRACUNIT,
                     MT_BLOODYSKULL);
    //mo->target = actor;
    mo->momx = P_SubRandom() << 9;
    mo->momy = P_SubRandom() << 9;
    mo->momz = FRACUNIT * 2 + (P_Random() << 6);
    // Attach player mobj to bloody skull
    player = actor->player;
    actor->player = NULL;
    mo->player = player;
    mo->health = actor->health;
    mo->angle = actor->angle;

    // fraggle: This check wasn't originally here in the Vanilla Heretic
    // source, causing crashes if the player respawns before this
    // function is called.

    if (player != NULL)
    {
        player->mo = mo;
        player->lookdir = 0;
        player->damagecount = 32;
    }
}
