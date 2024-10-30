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
#include "func_capture_zone.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CCaptureFlagSchedule )
{
	CCaptureZone *pTargetZone = NULL;

	for (int i = 0; i < ICaptureZoneAutoList::AutoList().Count(); ++i)
	{
		CCaptureZone *pZone = static_cast< CCaptureZone * >(ICaptureZoneAutoList::AutoList()[i]);

		if (pZone->GetTeamNumber() != GetHost()->GetTeamNumber())
			continue;

		pTargetZone = pZone;
		break;
	}

	ADD_TASK(BTASK_SAVE_POSITION, NULL);
	ADD_TASK(BTASK_RUN, NULL);
	ADD_TASK(BTASK_MOVE_DESTINATION, pTargetZone);
	ADD_TASK(BTASK_AIM, pTargetZone);
	ADD_TASK(BTASK_RESTORE_POSITION, NULL);
}

SET_SCHEDULE_INTERRUPTS( CCaptureFlagSchedule )
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
float CCaptureFlagSchedule::GetDesire() const
{
	if ( GetHost()->HasItem() && GetHost()->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG)
	{
		return BOT_DESIRE_GOTO_CAPTURE_FLAG;
	}
	else
	{
		return 0.00f;
	}
}

//================================================================================
//================================================================================
void CCaptureFlagSchedule::TaskRun()
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