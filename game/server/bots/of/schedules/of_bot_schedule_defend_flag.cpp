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
#include "entity_capture_flag.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS(CDefendFlagSchedule)
{
	CCaptureFlag *pTargetFlag = NULL;

	for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i)
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag * >(ICaptureFlagAutoList::AutoList()[i]);

		if (pFlag->GetTeamNumber() != GetHost()->GetTeamNumber())
			continue;

		pTargetFlag = pFlag;
		break;
	}

	if( !pTargetFlag )
		return;

	ADD_TASK(BTASK_SAVE_POSITION, NULL);
	ADD_TASK(BTASK_RUN, NULL);
	ADD_TASK(BTASK_MOVE_DESTINATION, pTargetFlag);
	ADD_TASK(BTASK_RESTORE_POSITION, NULL);
}

SET_SCHEDULE_INTERRUPTS(CDefendFlagSchedule)
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
float CDefendFlagSchedule::GetDesire() const
{
	CCaptureFlag *pTargetFlag = NULL;

	for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i)
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag * >(ICaptureFlagAutoList::AutoList()[i]);

		if (pFlag->GetTeamNumber() != GetHost()->GetTeamNumber())
		{
			continue;
		}

		pTargetFlag = pFlag;
		break;
	}

	if( !pTargetFlag )
		return BOT_DESIRE_NONE;

	if (pTargetFlag->IsStolen() || pTargetFlag->IsDropped())
	{
		return BOT_DESIRE_DEFEND_TAKEN_FLAG;
	}
	else
	{
		return BOT_DESIRE_DEFEND_FLAG;
	}
}

//================================================================================
//================================================================================
void CDefendFlagSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

	switch (pTask->task)
	{
	case BTASK_MOVE_DESTINATION:
		{
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
/*
//================================================================================
//================================================================================
void CDefendFlagSchedule::TaskComplete()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch (pTask->task)
	{
	case BTASK_RESTORE_POSITION:
		{

		DevMsg("Okay this is meant to fucking do something.");

		CNavArea* pArea = GetHost()->GetLastKnownArea();

		if (pArea == NULL) {
			pArea = TheNavMesh->GetNearestNavArea(GetHost());
		}

		if (pArea == NULL) {
			Fail("Without last known area.");
			return;
		}

		SavePosition(pArea->GetRandomPoint(), 5.0f);

			BaseClass::TaskComplete();
			break;
		}
	default:
		{
			BaseClass::TaskComplete();
			break;
		}
	}
}
*/