//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#include "bots/bot_manager.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots/in_utils.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Called on each frame to update component operation.
//================================================================================
void CBotAttack::Update()
{
    VPROF_BUDGET( "CBotAttack::Update", VPROF_BUDGETGROUP_BOTS );

    if (
        HasCondition( BCOND_CAN_RANGE_ATTACK1 ) || HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ||
        HasCondition( BCOND_CAN_RANGE_ATTACK2 ) || HasCondition( BCOND_CAN_MELEE_ATTACK2 )
        ) 
	{
        if ( GetDecision()->IsUsingFiregun() ) 
		{
            FiregunAttack();
        }
        else 
		{
            MeleeWeaponAttack();
        }
    }

	if( HasCondition( BCOND_SHOULD_STRAFE ) )
	{
		Strafe();
	}

	if( m_AttackHoldDownTimer.HasStarted() && !m_AttackHoldDownTimer.IsElapsed() )
		GetBot()->InjectButton( IN_ATTACK );

	if( m_Attack2HoldDownTimer.HasStarted() && !m_Attack2HoldDownTimer.IsElapsed() )
		GetBot()->InjectButton( IN_ATTACK2 );
}

//================================================================================
// Activates the attack of a firearm
//================================================================================
void CBotAttack::FiregunAttack()
{
    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();
    Assert( pWeapon );

    // Check to see if we can crouch for accuracy
    if ( GetDecision()->CanCrouchAttack() ) {
        if ( GetDecision()->ShouldCrouchAttack() ) {
            GetLocomotion()->Crouch();
        }
        else {
            GetLocomotion()->StandUp();
        }
    }

	if( !GetHost()->GetActiveBaseWeapon() )
		return;

    if ( HasCondition( BCOND_CAN_RANGE_ATTACK1 ) ) 
	{
        GetBot()->Combat();
		m_AttackHoldDownTimer.Start( GetHost()->GetActiveBaseWeapon()->GetFireRate() + 0.5f );
		static_cast<CBotDecision*>(GetDecision())->m_flLastAttackTime = gpGlobals->curtime;
    }

    // TODO
    if ( HasCondition( BCOND_CAN_RANGE_ATTACK2 ) ) 
	{
        GetBot()->Combat();
        m_Attack2HoldDownTimer.Start( GetHost()->GetActiveBaseWeapon()->GetFireRate() + 0.5f );
		static_cast<CBotDecision*>(GetDecision())->m_flLastAttackTime = gpGlobals->curtime;
    }
}

//================================================================================
//================================================================================
void CBotAttack::MeleeWeaponAttack()
{
    if ( HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ) 
	{
        GetBot()->Combat();
        InjectButton( IN_ATTACK );
    }

    if ( HasCondition( BCOND_CAN_MELEE_ATTACK2 ) ) 
	{
        GetBot()->Combat();
        InjectButton( IN_ATTACK2 );
    }
}

void CBotAttack::Strafe()
{
	// Apparently bots don't strafe in "Precise" areas
	// ie. Walkways thin bridges etc
	CNavArea *pArea = GetLocomotion()->GetLastKnownArea();
	if ( pArea && pArea->HasAttributes( NAV_MESH_PRECISE ) ) 
		return;
	
	const CEntityMemory *memory = GetMemory()->GetPrimaryThreat();
	if( !memory )
		return;

	if( !memory->IsVisibleRecently(3.0f) )
		return;
	
	CBaseEntity *pBlockedBy = NULL;

	if( !GetDecision()->IsLineOfSightClear( memory->GetLastKnownPosition(), memory->GetEntity(), &pBlockedBy ) ) 
		return;

	// Shake Shake!
	// TODO: Make a version of this specific for Strafing instead of reusing the wiggle function
	GetLocomotion()->Strafe();

}