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
#include "entity_weapon_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CPickupWeaponSpawnerSchedule )
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "WeaponSpawner" );
    Assert( memory );

    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, memory->GetEntity() );
    ADD_TASK( BTASK_AIM, memory->GetEntity() );
    ADD_TASK( BTASK_USE, NULL );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CPickupWeaponSpawnerSchedule )
{
	ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
	ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
	ADD_INTERRUPT(BCOND_REPEATED_DAMAGE);

	//ADD_INTERRUPT(BCOND_MOBBED_BY_ENEMIES);

	ADD_INTERRUPT(BCOND_POWERUP_AVAILABLE);

	ADD_INTERRUPT(BCOND_GOAL_UNREACHABLE);
}

//================================================================================
//================================================================================
float CPickupWeaponSpawnerSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "WeaponSpawner" );
	//CWeaponSpawner *pSpawner = dynamic_cast<CWeaponSpawner *>(memory->GetEntity());

	/*
	if (!pSpawner)
	{
		return BOT_DESIRE_NONE;
	}
	*/

	if (!memory)
	{
		return BOT_DESIRE_NONE;
	}

	if (!GetDecision()->CanMove())
	{
		return BOT_DESIRE_NONE;
	}

	if (HasCondition(BCOND_WEAPON_AVAILABLE))
	{
		/*
		if (pSpawner)
		{
			
			if ( GetHost()->Weapon_OwnsThisType( pSpawner->GetWeaponName() ) )
			{
				if ((GetHost()->Weapon_OwnsThisType(pSpawner->GetWeaponName()))->ReserveAmmo() >= (GetHost()->Weapon_OwnsThisType(pSpawner->GetWeaponName()))->GetMaxReserveAmmo())
					return BOT_DESIRE_NONE;
				else
					return BOT_DESIRE_PICKUP_SPAWNER_REFILL;
			}
			else
				return BOT_DESIRE_PICKUP_SPAWNER_NEW;
		}
		*/
		return BOT_DESIRE_PICKUP_SPAWNER_NEW;
	}

	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CPickupWeaponSpawnerSchedule::TaskStart()
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
void CPickupWeaponSpawnerSchedule::TaskRun()
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "WeaponSpawner" );

    if ( !memory ) 
	{
        Fail( "Memory doesn't exist." );
        return;
    }

    CWeaponSpawner *pSpawner = dynamic_cast<CWeaponSpawner *>( memory->GetEntity() );

    // Weapon doesnt exist anymore
    if ( !pSpawner ) 
	{
        Fail( "Weapon spawner doesn't exist." );
        return;
    }

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) 
	{
        case BTASK_USE:
        {
			if (pSpawner->m_bRespawning)
			{
				Fail("The weapon spawner is respawning.");
				return;
			}
			else if (pSpawner->IsDisabled())
			{
				Fail("The weapon spawner is disabled.");
				return;
			}
			else
				TaskComplete();
            break;
        }

        default:
        {
			// Can't take respawning items
			if (pSpawner->m_bRespawning)
			{
				Fail("The weapon spawner is respawning.");
				return;
			}
			else if (pSpawner->IsDisabled())
			{
				Fail("The weapon spawner is disabled.");
				return;
			}
			else
				BaseClass::TaskRun();

            break;
        }
    }
}