//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_fists.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Fists tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFists, DT_TFWeaponFists )

BEGIN_NETWORK_TABLE( CTFFists, DT_TFWeaponFists )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFists )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_fists, CTFFists );
//PRECACHE_WEAPON_REGISTER( tf_weapon_fists );



IMPLEMENT_NETWORKCLASS_ALIASED( TFBerserk, DT_TFWeaponBerserk )

BEGIN_NETWORK_TABLE( CTFBerserk, DT_TFWeaponBerserk )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBerserk )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_berserk, CTFBerserk );
//PRECACHE_WEAPON_REGISTER( tf_weapon_berserk );



IMPLEMENT_NETWORKCLASS_ALIASED( TFClaws, DT_TFWeaponClaws )

BEGIN_NETWORK_TABLE( CTFClaws, DT_TFWeaponClaws )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFClaws )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_claws, CTFClaws );
//PRECACHE_WEAPON_REGISTER( tf_weapon_claws );



IMPLEMENT_NETWORKCLASS_ALIASED( TFJuggernaught, DT_TFWeaponJuggernaught )

BEGIN_NETWORK_TABLE( CTFJuggernaught, DT_TFWeaponJuggernaught )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFJuggernaught )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_juggernaught, CTFJuggernaught);
//PRECACHE_WEAPON_REGISTER( tf_weapon_juggernaught );


//=============================================================================
//
// Weapon Fists functions.
//

acttable_t CTFClaws::m_acttableClaws[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_ZOMBIECLAWS, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_ZOMBIECLAWS, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_ZOMBIECLAWS, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_ZOMBIECLAWS, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_ZOMBIECLAWS, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_ZOMBIECLAWS, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_ZOMBIECLAWS, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_ZOMBIECLAWS, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_ZOMBIECLAWS, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_ZOMBIECLAWS, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_ZOMBIECLAWS, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_ZOMBIECLAWS, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_ZOMBIECLAWS, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_ZOMBIECLAWS, false },
};

acttable_t *CTFClaws::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableClaws);
		return m_acttableClaws;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t CTFBerserk::m_acttableBerserk[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_BERSERK, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_BERSERK, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_BERSERK, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_BERSERK, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_BERSERK, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_BERSERK, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_BERSERK, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_BERSERK, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_BERSERK, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_BERSERK, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_BERSERK, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_BERSERK, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_BERSERK, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_BERSERK, false },
};

acttable_t CTFFists::m_acttableBerserk[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_BERSERK, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_BERSERK, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_BERSERK, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_BERSERK, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_BERSERK, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_BERSERK, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_BERSERK, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_BERSERK, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_BERSERK, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_BERSERK, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_BERSERK, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_BERSERK, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_BERSERK, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_BERSERK, false },
};

acttable_t *CTFBerserk::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableBerserk);
		return m_acttableBerserk;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t *CTFFists::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableBerserk);
		return m_acttableBerserk;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	// reversed for 360 because the primary attack is on the right side of the controller
	if ( IsX360() )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}
	else
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Set the weapon usage mode - primary, secondary.
	if ( IsX360() )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	pPlayer->DoClassSpecialSkill();

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::Punch( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Swing the weapon.
	Swing( pPlayer );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFFists::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( IsCurrentAttackACrit() )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFists::DoViewModelAnimation( void )
{
	Activity act;

	if ( IsCurrentAttackACritical() )
	{
		act = ACT_VM_SWINGHARD;
	}
	else
	{
		act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITLEFT : ACT_VM_HITRIGHT;
	}

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClaws::ItemPostFrame(void)
{
	CTFPlayer *pOwner = ToTFPlayer(GetPlayerOwner());
	if (!pOwner)
		return;

	if (pOwner->IsAlive())
	{
		if( pOwner->m_Shared.IsLunging() )
		{
			trace_t trace;
			if (DoSwingTrace(trace) && trace.m_pEnt->IsPlayer())
			{
				SendPlayerAnimEvent(pOwner);
				DoViewModelAnimation();
				WeaponSound(MELEE_MISS);
				Smack();
				pOwner->m_Shared.SetNoAirControl(false);
				m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
			}
		}
	}

	BaseClass::ItemPostFrame();
}

void CTFClaws::PrimaryAttack()
{
	CTFPlayer *pOwner = ToTFPlayer(GetPlayerOwner());
	if( !pOwner )
		return;

	if( pOwner->m_Shared.IsLunging() )
		return;
	
	BaseClass::PrimaryAttack();
}