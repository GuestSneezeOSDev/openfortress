//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_crowbar.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

extern ConVar of_haste_fire_multiplier;
//=============================================================================
//
// Weapon Crowbar tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCrowbar, DT_TFWeaponCrowbar )

BEGIN_NETWORK_TABLE( CTFCrowbar, DT_TFWeaponCrowbar )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCrowbar )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_crowbar, CTFCrowbar );
//PRECACHE_WEAPON_REGISTER( tf_weapon_crowbar );

IMPLEMENT_NETWORKCLASS_ALIASED( TFUmbrella, DT_TFWeaponUmbrella )

BEGIN_NETWORK_TABLE( CTFUmbrella, DT_TFWeaponUmbrella )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFUmbrella )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_umbrella, CTFUmbrella );
//PRECACHE_WEAPON_REGISTER( tf_weapon_umbrella );

IMPLEMENT_NETWORKCLASS_ALIASED( TFCCrowbar, DT_TFCWeaponCrowbar )

BEGIN_NETWORK_TABLE( CTFCCrowbar, DT_TFCWeaponCrowbar )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCCrowbar )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_crowbar, CTFCCrowbar );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_crowbar );

IMPLEMENT_NETWORKCLASS_ALIASED( TFCUmbrella, DT_TFCWeaponUmbrella )

BEGIN_NETWORK_TABLE( CTFCUmbrella, DT_TFCWeaponUmbrella )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCUmbrella )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_umbrella, CTFCUmbrella );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_umbrella );

IMPLEMENT_NETWORKCLASS_ALIASED(TFLeadPipe, DT_TFWeaponLeadPipe)

BEGIN_NETWORK_TABLE(CTFLeadPipe, DT_TFWeaponLeadPipe)
#ifdef GAME_DLL
	SendPropFloat(SENDINFO(m_flMeleeCharge), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN),
#else
	RecvPropFloat(RECVINFO(m_flMeleeCharge)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFLeadPipe)
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD(m_flMeleeCharge, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_lead_pipe, CTFLeadPipe);
//PRECACHE_WEAPON_REGISTER( tf_weapon_lead_pipe );

//=============================================================================
//
// Weapon Crowbar functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFCrowbar::CTFCrowbar()
{
}
CTFUmbrella::CTFUmbrella()
{
}
CTFCCrowbar::CTFCCrowbar()
{
}
CTFCUmbrella::CTFCUmbrella()
{
}
CTFLeadPipe::CTFLeadPipe()
{
}
acttable_t m_acttableMeleeAllClass[] =
{
	{ ACT_MP_STAND_IDLE,						ACT_MP_STAND_MELEE_ALLCLASS,			        false },		
	{ ACT_MP_CROUCH_IDLE,	                    ACT_MP_CROUCH_MELEE_ALLCLASS,	                false },
	{ ACT_MP_WALK,								ACT_MP_WALK_MELEE,								false },
	{ ACT_MP_RUN,		                    ACT_MP_RUN_MELEE_ALLCLASS,		                false },
	{ ACT_MP_AIRWALK,	                    ACT_MP_AIRWALK_MELEE_ALLCLASS,	                false },
	{ ACT_MP_CROUCHWALK,                  ACT_MP_CROUCHWALK_MELEE_ALLCLASS,               false },
	{ ACT_MP_JUMP,	                    ACT_MP_JUMP_MELEE_ALLCLASS,	                    false },
	{ ACT_MP_JUMP_START,                  ACT_MP_JUMP_START_MELEE_ALLCLASS,               false },
	{ ACT_MP_JUMP_FLOAT,                  ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS,               false },
	{ ACT_MP_JUMP_LAND,                   ACT_MP_JUMP_LAND_MELEE_ALLCLASS,				false },
	{ ACT_MP_SWIM,	                    ACT_MP_SWIM_MELEE_ALLCLASS,					    false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,                ACT_MP_ATTACK_STAND_MELEE_ALLCLASS,			    false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,               ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS,		    false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,	                ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,	            false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,              ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,           false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },
};

//Act tables for Merc
acttable_t m_acttableCrowbar[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_CROWBAR, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_CROWBAR, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_CROWBAR, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_CROWBAR, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_CROWBAR, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_CROWBAR, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_CROWBAR, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_CROWBAR, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_CROWBAR, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_CROWBAR, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_CROWBAR, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_CROWBAR, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_CROWBAR, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_CROWBAR, false },
};

//Act table remapping for Merc
acttable_t *CTFCrowbar::ActivityList( int &iActivityCount )
{
	if( GetTFPlayerOwner() )
	{
		switch( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() )
		{
			case TF_CLASS_MERCENARY:
				iActivityCount = ARRAYSIZE(m_acttableCrowbar);
				return m_acttableCrowbar;
				break;
			case TF_CLASS_CIVILIAN:
				return BaseClass::ActivityList(iActivityCount);
				break;
			default:
				iActivityCount = ARRAYSIZE(m_acttableMeleeAllClass);
				return m_acttableMeleeAllClass;
				break;
		}
	}

	return BaseClass::ActivityList(iActivityCount);
}

acttable_t *CTFCCrowbar::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableCrowbar);
		return m_acttableCrowbar;
	}
	else if ((GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() != TF_CLASS_CIVILIAN) || (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY))
	{
		iActivityCount = ARRAYSIZE(m_acttableMeleeAllClass);
		return m_acttableMeleeAllClass;
	}
	return BaseClass::ActivityList(iActivityCount);
}
//Act tables for Merc
acttable_t m_acttableLeadPipe[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_CROWBAR, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_CROWBAR, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_CROWBAR, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_CROWBAR, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_CROWBAR, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_CROWBAR, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_CROWBAR, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_CROWBAR, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_CROWBAR, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_CROWBAR, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_CROWBAR, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_CROWBAR, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_CROWBAR, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_CROWBAR, false },

	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_MERC_ATTACK_STAND_PREFIRE_CROWBAR, false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_MERC_ATTACK_CROUCH_PREFIRE_CROWBAR, false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_MERC_ATTACK_SWIM_PREFIRE_CROWBAR, false },
};

//Act table remapping for Merc
acttable_t *CTFLeadPipe::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableLeadPipe);
		return m_acttableLeadPipe;
	}
	else if ((GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() != TF_CLASS_CIVILIAN) || (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY))
	{
		iActivityCount = ARRAYSIZE(m_acttableMeleeAllClass);
		return m_acttableMeleeAllClass;
	}
	return BaseClass::ActivityList(iActivityCount);
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFLeadPipe::GetMeleeDamage(CBaseEntity *pTarget, int &iCustomDamage)
{
	float flBaseDamage = BaseClass::GetMeleeDamage(pTarget, iCustomDamage);

	if (m_flMeleeCharge > 0.0f)
	{
		flBaseDamage = flBaseDamage + ((flBaseDamage * m_flMeleeCharge)*2);
		m_flMeleeCharge = 0.0f;
	}

	return flBaseDamage;
}
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFLeadPipe::PrimaryAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	if (!CanAttack())
	{
		WeaponIdle();
		return;
	}

	switch (m_iWeaponState)
	{
	default:
	case AC_STATE_IDLE:
		{
			m_flMeleeCharge = 0.0f;

			m_iNumBeepsToBeep = 1;
			if (m_flNextPrimaryAttack <= gpGlobals->curtime)
			{
				//DevMsg("You start to charge up the Pipe Wrench\n");
				WindUp();
			}

			break;
		}
	case AC_STATE_CHARGE:
		{
			if (!m_bReady)
			{
				m_bReady = true;
				SendWeaponAnim(ACT_BACKSTAB_VM_UP);
			}

			if (m_bReady)
			{
				float m_flMeleeChargeRate;

				if (pPlayer->m_Shared.InCond(TF_COND_HASTE))
				{
					m_flMeleeChargeRate = 0.75f + of_haste_fire_multiplier.GetFloat();
				}
				else
				{
					m_flMeleeChargeRate = 0.75f;
				}

				m_flMeleeCharge = min(1.f, m_flMeleeCharge + m_flMeleeChargeRate * 0.5f * gpGlobals->frametime);

				//float m_flMeleeChargeMath = m_flMeleeCharge;
				//DevMsg("The Melee Charge is %f\n", m_flMeleeChargeMath);
			}
			break;
		}
	}
	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLeadPipe::SendWeaponAnim(int iActivity)
{
	if (iActivity == ACT_VM_IDLE && m_bReady)
	{
		return BaseClass::SendWeaponAnim(ACT_BACKSTAB_VM_IDLE);
	}

	return BaseClass::SendWeaponAnim(iActivity);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLeadPipe::WindUp(void)
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

		// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRE);

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_CHARGE;

#ifdef GAME_DLL
	pPlayer->StopRandomExpressions();
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLeadPipe::ItemPostFrame()
{
	CTFPlayer *pOwner = ToTFPlayer(GetOwner());
	if (!pOwner)
		return;

	if (pOwner->m_afButtonReleased & IN_ATTACK)
	{
		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{ 
			Swing(pOwner);

			m_bReady	 = false;

			float m_flMeleeChargeRate;

			if (pOwner->m_Shared.InCond(TF_COND_HASTE))
			{
				m_flMeleeChargeRate = 0.75f;
			}
			else
			{
				m_flMeleeChargeRate = 1.0f;
			}

			m_flNextPrimaryAttack = (gpGlobals->curtime + (GetFireRate()) + (m_flMeleeCharge * m_flMeleeChargeRate));

			//float m_flNextPrimaryAttackMath = ((GetFireRate()) + (m_flMeleeCharge * m_flMeleeChargeRate));
			//DevMsg("The Attack Delay is %f\n", m_flNextPrimaryAttackMath);

			m_iWeaponState = AC_STATE_IDLE;
		}
	}

#ifdef CLIENT_DLL
	if ((m_flMeleeCharge == 1.f) && m_iNumBeepsToBeep > 0)
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();


		if (!pLocalPlayer)
			return;

		pLocalPlayer->EmitSound("Leadpipe.FullCharge");

		m_iNumBeepsToBeep = 0;
		//DevMsg("EmitSound\n");
	}
#endif

	BaseClass::ItemPostFrame();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLeadPipe::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_flMeleeCharge = 0.f;

	m_iNumBeepsToBeep = 1;

	m_bReady = false;

	return BaseClass::Holster(pSwitchingTo);
}