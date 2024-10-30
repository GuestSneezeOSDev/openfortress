//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "Ammo_base.h"
#else
#include "bots/in_utils.h"
#endif

#include "in_buttons.h"

#include "of_bot_schedules.h"
#include "entity_ammopack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS(CPickupAmmoSpawnerSchedule)
{
	CDataMemory *memory = GetMemory()->GetDataMemory("AmmoPackPickup");
	Assert(memory);

	ADD_TASK(BTASK_SAVE_POSITION, NULL);
	ADD_TASK(BTASK_RUN, NULL);
	ADD_TASK(BTASK_MOVE_DESTINATION, memory->GetEntity());
	ADD_TASK(BTASK_AIM, memory->GetEntity());
	ADD_TASK(BTASK_USE, NULL);
	ADD_TASK(BTASK_RESTORE_POSITION, NULL);
}

SET_SCHEDULE_INTERRUPTS(CPickupAmmoSpawnerSchedule)
{
	ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
	ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
	ADD_INTERRUPT(BCOND_REPEATED_DAMAGE);

	//ADD_INTERRUPT(BCOND_MOBBED_BY_ENEMIES);

	ADD_INTERRUPT(BCOND_WEAPON_AVAILABLE);
	ADD_INTERRUPT(BCOND_HEALTH_PICKUP_AVAIBLE);
	ADD_INTERRUPT(BCOND_POWERUP_AVAILABLE);

	ADD_INTERRUPT(BCOND_GOAL_UNREACHABLE); 
}

//================================================================================
//================================================================================
float CPickupAmmoSpawnerSchedule::GetDesire() const
{
	if (!GetMemory())
		return BOT_DESIRE_NONE;

	CDataMemory *memory = GetMemory()->GetDataMemory("AmmoPackPickup");

	if (!memory)
		return BOT_DESIRE_NONE;

	if (!GetDecision()->CanMove())
		return BOT_DESIRE_NONE;

	if (HasCondition(BCOND_AMMO_PICKUP_AVAIBLE))
	{
		int iMidAmmo = (GetHost()->GetActiveBaseWeapon()->GetMaxReserveAmmo() * 0.50);

		if (GetHost()->GetActiveBaseWeapon()->ReserveAmmo() < iMidAmmo)
		{
			return BOT_DESIRE_PICKUP_AMMO;
		}
		else
		{
			return BOT_DESIRE_NONE;
		}
	}
	else
	{
		return BOT_DESIRE_NONE;
	}
}

//================================================================================
//================================================================================
void CPickupAmmoSpawnerSchedule::TaskStart()
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
void CPickupAmmoSpawnerSchedule::TaskRun()
{
	CDataMemory *memory = GetMemory()->GetDataMemory("AmmoPackPickup");

	if (!memory)
	{
		Fail("Ammo pack no longer exist in memory.");
		return;
	}

	CAmmoPack *pAmmokit = dynamic_cast<CAmmoPack *>(memory->GetEntity());

	// Ammo doesnt exist anymore
	if (!pAmmokit)
	{
		Fail("Ammo pack no longer exist.");
		return;
	}

	BotTaskInfo_t *pTask = GetActiveTask();

	switch (pTask->task) {
	case BTASK_USE:
	{
		TaskComplete();
		break;
	}

	default:
	{
		// Can't take respawning items
		if (pAmmokit->m_bRespawning)
		{
			Fail("Ammo pack is respawning.");
			return;
		}
		else if ( pAmmokit->m_bDisabled )
		{
			Fail("Ammo pack is disabled.");
			return;
		}
		BaseClass::TaskRun();
		break;
	}
	}
}