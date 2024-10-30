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
	if (actor->GetSkill() == EASY)                              return;
	if (actor->m_Shared.IsInvulnerable())                               return;
	if (actor->m_Shared.InCond(TF_COND_ZOOMED))                         return;
	if (actor->IsTaunting())                                            return;
	if (!actor->IsCombatWeapon())                                       return;
	if (actor->m_Shared.InCond(TF_COND_DISGUISED))                      return;
	if (actor->m_Shared.InCond(TF_COND_DISGUISING))                     return;
	if (actor->m_Shared.IsStealthed())                                  return;

	// don't dodge on areas we marked as PRECISE e.g. thin walkways
	CNavArea *area = actor->GetLastKnownArea();
	if ( area != nullptr && area->HasAttributes( NAV_MESH_PRECISE ) ) {
		return;
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat == nullptr)
		return;
	if (!threat->IsVisibleRecently())
		return;
	
	if (!actor->IsLineOfFireClear(threat->GetLastKnownPosition())) 
	{
		return;
	}
	
	Vector eye_vec  = EyeVectorsFwd(actor);
	Vector side_dir = Vector(-eye_vec.y, eye_vec.x, 0.0f).Normalized();
	
	switch (RandomInt(0, 2)) {
	case 1: // 33% chance to go left
		if (actor->GetLocomotionInterface()->HasPotentialGap(actor->GetAbsOrigin(), actor->GetAbsOrigin() + (25.0f * side_dir))) {
			actor->PressLeftButton();
		}
		break;
	case 2: // 33% chance to go right
		if (actor->GetLocomotionInterface()->HasPotentialGap(actor->GetAbsOrigin(), actor->GetAbsOrigin() - (25.0f * side_dir))) {
			actor->PressRightButton();
		}
		break;
	}


	int seed = 0;
	CTFNavArea *area = this->GetLastKnownTFArea();
	if ( area == nullptr ) return 0.0f;
	
	int time_seed = 1 + ( int )( gpGlobals->curtime / 1.0f );
	seed += ( area->GetID() * time_seed * this->entindex() );
	
	float flRandValue = abs( FastCos( seed ) );

	// jump occasionally (20% chance)
	if ( flRandValue < 0.2f )
	{
		actor->GetLocomotionInterface()->Jump();
	}

	ADD_TASK( BTASK_DECIDE_NEW_ROAM_LOCATION, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 3.0f, 6.0f ) ); // TODO
}

SET_SCHEDULE_INTERRUPTS( CFreeRoamSchedule )
{
    ADD_INTERRUPT( BCOND_SEE_HATE );
    ADD_INTERRUPT( BCOND_SEE_FEAR );
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
	ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE );
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