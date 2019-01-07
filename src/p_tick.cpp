//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//		Ticker.
//
//-----------------------------------------------------------------------------

#include "p_local.h"
#include "p_effect.h"
#include "c_console.h"
#include "b_bot.h"
#include "doomstat.h"
#include "sbar.h"
#include "r_data/r_interpolate.h"
#include "d_player.h"
#include "r_utility.h"
#include "p_spec.h"
#include "g_levellocals.h"
#include "events.h"
#include "actorinlines.h"

extern gamestate_t wipegamestate;

//==========================================================================
//
// P_CheckTickerPaused
//
// Returns true if the ticker should be paused. In that case, it also
// pauses sound effects and possibly music. If the ticker should not be
// paused, then it returns false but does not unpause anything.
//
//==========================================================================

bool P_CheckTickerPaused ()
{
	// pause if in menu or console and at least one tic has been run
	if ( !netgame
		 && gamestate != GS_TITLELEVEL
		 && ((menuactive != MENU_Off && menuactive != MENU_OnNoPause) ||
			 ConsoleState == c_down || ConsoleState == c_falling)
		 && !demoplayback
		 && !demorecording
		 && players[consoleplayer].viewz != NO_VALUE
		 && wipegamestate == gamestate)
	{
		S_PauseSound (!(level.flags2 & LEVEL2_PAUSE_MUSIC_IN_MENUS), false);
		return true;
	}
	return false;
}

//
// P_Ticker
//
// (split up to prepare for running multiple levels.)
//
void P_Ticker (void)
{
	int i;

	if (paused || P_CheckTickerPaused())
		return;
	
	level.Tick();
	
	// [BC] Do a quick check to see if anyone has the freeze time power. If they do,
	// then don't resume the sound, since one of the effects of that power is to shut
	// off the music.
	for (i = 0; i < MAXPLAYERS; i++ )
	{
		if (playeringame[i] && players[i].timefreezer != 0)
			break;
	}
	
	if ( i == MAXPLAYERS )
		S_ResumeSound (false);

}

void FLevelLocals::Tick()
{
	// run the tic


	interpolator.UpdateInterpolations ();
	r_NoInterpolate = true;

	DPSprite::NewTick();

	// [RH] Frozen mode is only changed every 4 tics, to make it work with A_Tracer().
	if ((maptime & 3) == 0)
	{
		if (bglobal.changefreeze)
		{
			bglobal.freeze ^= 1;
			bglobal.changefreeze = 0;
		}
	}


	P_ResetSightCounters (false);
	R_ClearInterpolationPath();

	// Reset all actor interpolations for all actors before the current thinking turn so that indirect actor movement gets properly interpolated.
	TThinkerIterator<AActor> it;
	AActor *ac;

	while ((ac = it.Next()))
	{
		ac->ClearInterpolation();
	}

	// Since things will be moving, it's okay to interpolate them in the renderer.
	r_NoInterpolate = false;

	P_ThinkParticles();	// [RH] make the particles think

	for (int i = 0; i<MAXPLAYERS; i++)
		if (playeringame[i] &&
			/*Added by MC: Freeze mode.*/!(bglobal.freeze && players[i].Bot != NULL))
			P_PlayerThink (&players[i]);

	// [ZZ] call the WorldTick hook
	E_WorldTick();
	StatusBar->SetLevel(this);
	StatusBar->CallTick ();		// [RH] moved this here
	// Reset carry sectors
	if (Scrolls.Size() > 0)
	{
		memset (&Scrolls[0], 0, sizeof(Scrolls[0])*Scrolls.Size());
	}
	DThinker::RunThinkers ();

	//if added by MC: Freeze mode.
	if (!bglobal.freeze && !(flags2 & LEVEL2_FROZEN))
	{
		P_UpdateSpecials (&level);
		P_RunEffects ();	// [RH] Run particle effects
	}

	// for par times
	time++;
	maptime++;
	totaltime++;
}
