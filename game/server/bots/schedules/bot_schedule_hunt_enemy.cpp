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
SET_SCHEDULE_TASKS( CHuntEnemySchedule )
{
    bool carefulApproach = GetDecision()->ShouldMustBeCareful();

	CDataMemory *memory = GetMemory()->GetDataMemory( "IdealDistance" );
	Assert( memory );

    //ADD_TASK( BTASK_SET_FAIL_SCHEDULE, SCHEDULE_COVER );
    ADD_TASK( BTASK_RUN, NULL );

#ifndef HL2MP
    // We must be careful!
    if ( carefulApproach ) 
	{
        // We run towards the target until we reach a distance of between 700 and 900 units
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( memory->GetFloat()+200, memory->GetFloat()+400 ) );

        // We walked slowly until reaching a short distance and 
        // we waited a little in case the target leaves its coverage.
        //ADD_TASK( BTASK_SNEAK, NULL );
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( memory->GetFloat() - 100, memory->GetFloat()+100 ) );
        ADD_TASK( BTASK_WAIT, RandomFloat( 0.5f, 1.0f ) );
    }
	else
#endif
	// Remove the random distance if we are using melee if possible
    ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( memory->GetFloat() - 100, memory->GetFloat() + 100 ) );
}


SET_SCHEDULE_INTERRUPTS( CHuntEnemySchedule )
{
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO );
    ADD_INTERRUPT( BCOND_EMPTY_CLIP1_AMMO );
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_WITHOUT_ENEMY );
    ADD_INTERRUPT( BCOND_ENEMY_DEAD );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
	ADD_INTERRUPT( BCOND_LOW_HEALTH );

    ADD_INTERRUPT( BCOND_NEW_ENEMY );
    ADD_INTERRUPT( BCOND_WEAPON_AVAILABLE );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY );
}

//================================================================================
//================================================================================
float CHuntEnemySchedule::GetDesire() const
{
    if ( !GetMemory() || !GetLocomotion() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanHuntThreat() )
        return BOT_DESIRE_NONE;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    if ( memory == NULL )
        return BOT_DESIRE_NONE;

    // We have no vision of the enemy
    if ( HasCondition( BCOND_ENEMY_LOST ) ) 
	{
		if ( HasCondition( BCOND_ENEMY_LAST_POSITION_VISIBLE ) )
			return BOT_DESIRE_HUNT_LOST_ENEMY;
		else
			return BOT_DESIRE_NONE;
    }

    // We do not have direct vision to the enemy (a person, window, etc.)
    if ( HasCondition( BCOND_ENEMY_OCCLUDED ) )
		return BOT_DESIRE_HUNT_LOST_ENEMY;

    // We do not have range of attack
    if ( HasCondition( BCOND_ENEMY_FAR ) || HasCondition( BCOND_ENEMY_TOO_FAR ) || HasCondition( BCOND_TOO_FAR_TO_ATTACK ) )
		return BOT_DESIRE_HUNT_FAR_ENEMY;

    return BOT_DESIRE_NONE;
}