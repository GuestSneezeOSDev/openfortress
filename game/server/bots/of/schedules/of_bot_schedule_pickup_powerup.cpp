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
#include "entity_condpowerup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CPickupPowerupSpawnerSchedule )
{
	if (!GetMemory())
		return;

    CDataMemory *memory = GetMemory()->GetDataMemory( "PowerupSpawner" );
    Assert( memory );

    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, memory->GetEntity() );
    ADD_TASK( BTASK_AIM, memory->GetEntity() );
    ADD_TASK( BTASK_USE, NULL );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CPickupPowerupSpawnerSchedule )
{
	ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
	ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
	ADD_INTERRUPT(BCOND_REPEATED_DAMAGE);

	ADD_INTERRUPT(BCOND_GOAL_UNREACHABLE);
}

//================================================================================
//================================================================================
float CPickupPowerupSpawnerSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "PowerupSpawner" );

    if ( memory == NULL )
        return BOT_DESIRE_NONE;

	if ( !GetDecision()->CanMove() )
		return BOT_DESIRE_NONE;

	if (HasCondition(BCOND_HAS_POWERUP))
		return BOT_DESIRE_NONE;

	if ( HasCondition(BCOND_POWERUP_AVAILABLE) )
		return BOT_DESIRE_PICKUP_POWERUP;

	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CPickupPowerupSpawnerSchedule::TaskStart()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch( pTask->task )
	{
        case BTASK_USE:
        {
            InjectButton( IN_USE );
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}

//================================================================================
//================================================================================
void CPickupPowerupSpawnerSchedule::TaskRun()
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "PowerupSpawner" );

    if ( !memory ) 
	{
        Fail( "Powerup doesn't exist in memory." );
        return;
    }

	CCondPowerup *pPowerupSpawners = dynamic_cast<CCondPowerup *>(memory->GetEntity());

    // Weapon doesnt exist anymore
	if (!pPowerupSpawners)
	{
        Fail( "Powerup no longer exists" );
        return;
    }

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_USE:
        {
            TaskComplete();
            break;
        }

        default:
        {
			// Can't take respawning items
			if( pPowerupSpawners->m_bRespawning ) 
			{
				Fail( "Powerup is respawning." );
				return;
			}
			else if ( pPowerupSpawners->IsDisabled() )
			{
				Fail("Powerup is disabled.");
				return;
			}

            BaseClass::TaskRun();
            break;
        }
    }
}