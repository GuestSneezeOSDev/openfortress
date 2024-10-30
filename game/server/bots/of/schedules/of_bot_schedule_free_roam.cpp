//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "weapon_base.h"
#else
#include "bots/in_utils.h"
#endif

#include "in_buttons.h"
#include "of_bot_schedules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CFreeRoamSchedule )
{
	if ( !GetMemory() )
		return;

	CDataMemory *memory = GetMemory()->GetDataMemory("FreeRoamLocation");
    Assert( memory );

	ADD_TASK( BTASK_DECIDE_NEW_ROAM_LOCATION, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 0.5f, 1.0f ) ); // TODO
}

SET_SCHEDULE_INTERRUPTS( CFreeRoamSchedule )
{

    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );

	ADD_INTERRUPT( BCOND_WEAPON_AVAILABLE );
	ADD_INTERRUPT( BCOND_POWERUP_AVAILABLE );
	ADD_INTERRUPT( BCOND_HEALTH_PICKUP_AVAIBLE) ;
	ADD_INTERRUPT( BCOND_AMMO_PICKUP_AVAIBLE );
}

//================================================================================
// Always do this if you have nothing else to do, but don't prioritise it over other schedules
//================================================================================
float CFreeRoamSchedule::GetDesire() const
{
	return BOT_DESIRE_FREE_ROAM;
}

//================================================================================
//================================================================================
void CFreeRoamSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) 
	{
        case BTASK_MOVE_DESTINATION:
        {
			// Override our destination as the freeroam position
            CDataMemory *memory = GetMemory()->GetDataMemory( "FreeRoamLocation" );
			pTask->vecValue = memory->vecValue;

            BaseClass::TaskRun();
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}