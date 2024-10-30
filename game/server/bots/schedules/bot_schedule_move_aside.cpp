//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots/in_utils.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CMoveAsideSchedule )
{
    ADD_TASK( BTASK_SAVE_ASIDE_SPOT, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CMoveAsideSchedule )
{
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );

    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}

//================================================================================
//================================================================================
float CMoveAsideSchedule::GetDesire() const
{
    if ( GetProfile()->IsEasiest() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( GetLocomotion()->HasDestination() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->IsCombating() || GetBot()->IsAlerted() ) {
        if ( HasCondition( BCOND_LIGHT_DAMAGE ) || HasCondition( BCOND_REPEATED_DAMAGE ) )
			return BOT_DESIRE_STRAFE_COMBAT;
    }

    //if ( HasCondition( BCOND_ENEMY_OCCLUDED_BY_FRIEND ) )
        //return 0.91f;

    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && !GetBot()->IsCombating() ) {
        if ( HasCondition( BCOND_ENEMY_LOST ) || HasCondition( BCOND_HEAR_COMBAT ) )
			return BOT_DESiRE_STRAFE_ALERTED;
    }

    if ( m_nMoveAsideTimer.IsElapsed() ) {
        if ( !HasCondition( BCOND_WITHOUT_ENEMY ) || GetBot()->IsCombating() )
			return BOT_DESiRE_STRAFE_ALERTED;

        if ( GetBot()->IsIdle() )
			return BOT_DESIRE_STRAFE_IDLE;
    }

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CMoveAsideSchedule::Start()
{
	BaseClass::Start();
	m_nMoveAsideTimer.Start( RandomFloat(0.0, 0.6f) );
}