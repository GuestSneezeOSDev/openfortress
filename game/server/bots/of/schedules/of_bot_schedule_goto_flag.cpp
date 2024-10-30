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
SET_SCHEDULE_TASKS(CGotoFlagSchedule)
{
	CCaptureFlag *pTargetFlag = NULL;

	for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i)
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag * >(ICaptureFlagAutoList::AutoList()[i]);

		if (pFlag->GetTeamNumber() == GetHost()->GetTeamNumber())
			continue;

		pTargetFlag = pFlag;
		break;
	}

	if( !pTargetFlag )
		return;

	ADD_TASK(BTASK_SAVE_POSITION, NULL);
	ADD_TASK(BTASK_RUN, NULL);
	ADD_TASK(BTASK_MOVE_DESTINATION, pTargetFlag);
	ADD_TASK(BTASK_AIM, pTargetFlag);
	ADD_TASK(BTASK_RESTORE_POSITION, NULL);
}

SET_SCHEDULE_INTERRUPTS(CGotoFlagSchedule)
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
float CGotoFlagSchedule::GetDesire() const
{
	CCaptureFlag *pTargetFlag = NULL;

	for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i)
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag * >(ICaptureFlagAutoList::AutoList()[i]);

		if (pFlag->GetTeamNumber() == GetHost()->GetTeamNumber())
		{
			continue;
		}

		pTargetFlag = pFlag;
		break;
	}

	if( !pTargetFlag )
		return BOT_DESIRE_NONE;

	if ( pTargetFlag->IsStolen() || pTargetFlag->IsDropped() )
	{
		return BOT_DESIRE_GOTO_DROPPED_FLAG;
	}
	else
	{
		return BOT_DESIRE_GOTO_FLAG;
	}
}

//================================================================================
//================================================================================
void CGotoFlagSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) 
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