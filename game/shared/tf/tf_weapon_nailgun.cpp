//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_nailgun.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon SMG tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFNailgun, DT_WeaponNailgun )

BEGIN_NETWORK_TABLE( CTFNailgun, DT_WeaponNailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFNailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_nailgun, CTFNailgun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_nailgun);

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFNailgun )
END_DATADESC()
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFCNailgun, DT_TFCNailgun )

BEGIN_NETWORK_TABLE( CTFCNailgun, DT_TFCNailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCNailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_nailgun, CTFCNailgun );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_nailgun);

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFCNailgun )
END_DATADESC()
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFCNailgunSuper, DT_TFCNailgunSuper )

BEGIN_NETWORK_TABLE( CTFCNailgunSuper, DT_TFCNailgunSuper )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCNailgunSuper )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_nailgun_super, CTFCNailgunSuper );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_nailgun_super );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFCNailgunSuper )
END_DATADESC()
#endif

acttable_t CTFNailgun::m_acttableNailgun_Mercenary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_NAILGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_NAILGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_NAILGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_NAILGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_NAILGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_NAILGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_NAILGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_NAILGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_NAILGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_NAILGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_NAILGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_NAILGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_NAILGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_NAILGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_NAILGUN, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_NAILGUN, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_NAILGUN, false },
};

//Act table remapping for Merc
acttable_t *CTFNailgun::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableNailgun_Mercenary);
		return m_acttableNailgun_Mercenary;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
//=============================================================================
//
// Weapon Nailgun functions.
//
//=============================================================================