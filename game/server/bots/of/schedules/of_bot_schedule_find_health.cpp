//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "Health_base.h"
#else
#include "bots/in_utils.h"
#endif

#include "in_buttons.h"

#include "of_bot_schedules.h"
#include "entity_healthkit.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS(CPickupHealthSpawnerSchedule)
{
	CDataMemory *memory = GetMemory()->GetDataMemory("HealthPackPickup");
	Assert(memory);

	ADD_TASK(BTASK_SAVE_POSITION, NULL);
	ADD_TASK(BTASK_RUN, NULL);
	ADD_TASK(BTASK_MOVE_DESTINATION, memory->GetEntity());
	ADD_TASK(BTASK_AIM, memory->GetEntity());
	ADD_TASK(BTASK_USE, NULL);
	ADD_TASK(BTASK_RESTORE_POSITION, NULL);
}

SET_SCHEDULE_INTERRUPTS(CPickupHealthSpawnerSchedule)
{
	ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
	ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
	ADD_INTERRUPT(BCOND_REPEATED_DAMAGE);

	//ADD_INTERRUPT(BCOND_MOBBED_BY_ENEMIES);

	ADD_INTERRUPT(BCOND_WEAPON_AVAILABLE);

	ADD_INTERRUPT(BCOND_GOAL_UNREACHABLE);
}

//================================================================================
//================================================================================
float CPickupHealthSpawnerSchedule::GetDesire() const
{
	if (!GetMemory())
		return BOT_DESIRE_NONE;

	CDataMemory *memory = GetMemory()->GetDataMemory("HealthPackPickup");

	if (!memory)
		return BOT_DESIRE_NONE;

	if (!GetDecision()->CanMove())
		return BOT_DESIRE_NONE;

	int iFullHealth = (GetHost()->GetPlayerClass()->GetMaxHealth());

	int iHighHealth = (iFullHealth * 0.9);

	int iMidHealth = (iFullHealth * 0.7);

	int iLowHealth = (iFullHealth * 0.4);

	int iCriticalHealth = (iFullHealth * 0.1);

	int iCurrentHealth = GetHost()->GetHealth();

	CHealthKit *pHealthKit = dynamic_cast<CHealthKit *>(memory->GetEntity());
	// REWRITE THIS USING SHORT CIRCUITING!!
	// -sappho
	if ( HasCondition(BCOND_HEALTH_PICKUP_AVAIBLE) )
	{
		if ( iCurrentHealth < iFullHealth)
		{
			if (iCurrentHealth < iFullHealth && iCurrentHealth >= iHighHealth)
			{
				//DevMsg("the MAX of our HIGH HEALTH range is: %i.\n", iHighHealth);
				return BOT_DESIRE_PICKUP_HEALTH_TINY;
			}
			else if (iCurrentHealth < iHighHealth && iCurrentHealth >= iMidHealth)
			{
				return BOT_DESIRE_PICKUP_HEALTH_SMALL;
			}
			else if (iCurrentHealth < iMidHealth && iCurrentHealth >= iLowHealth)
			{
				return BOT_DESIRE_PICKUP_HEALTH_MEDIUM;
			}
			else if (iCurrentHealth < iLowHealth && iCurrentHealth >= iCriticalHealth)
			{
				return BOT_DESIRE_PICKUP_HEALTH_LARGE;
			}
			else if (iCurrentHealth < iCriticalHealth)
			{
				return BOT_DESIRE_PICKUP_HEALTH_CRITICAL;
			}
			else
				return BOT_DESIRE_PICKUP_HEALTH_MEDIUM;
		}
	}

	//DevMsg("We reached the end of this and is returning 0 for some reason.\n");
	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CPickupHealthSpawnerSchedule::TaskStart()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch (pTask->task)
	{
	case BTASK_USE:
		{
			InjectButton(IN_USE);
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
void CPickupHealthSpawnerSchedule::TaskRun()
{
	CDataMemory *memory = GetMemory()->GetDataMemory("HealthPackPickup");

	if (!memory)
	{
		Fail("Healthkit is no longer in memory.");
		return;
	}

	CHealthKit *pHealthKit = dynamic_cast<CHealthKit *>(memory->GetEntity());

	// Health doesnt exist anymore
	if (!pHealthKit)
	{
		Fail("Healthkit no longer exists.");
		return;
	}

	BotTaskInfo_t *pTask = GetActiveTask();

	switch (pTask->task) 
	{
		case BTASK_USE:
		{
			TaskComplete();
			break;
		}

		default:
		{
			// Can't take respawning items
			if (pHealthKit->m_bRespawning)
			{
				Fail("Healthkit is respawning.");
				return;
			}
			else if (pHealthKit->m_bDisabled)
			{
				Fail("Healthkit is disabled.");
				return;
			}

			BaseClass::TaskRun();
			break;
		}
	}
}