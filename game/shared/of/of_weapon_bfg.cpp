//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// OF E.Y.E.D.O.L. 9000 / BFG
//
//=============================================================================
#include "cbase.h"
#include "of_weapon_bfg.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBFG, DT_WeaponBFG )

BEGIN_NETWORK_TABLE( CTFBFG, DT_WeaponBFG )
#ifndef CLIENT_DLL
	//SendPropBool( SENDINFO(m_bLockedOn) ),
#else
	//RecvPropInt( RECVINFO(m_bLockedOn) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBFG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_gib, CTFBFG );
//PRECACHE_WEAPON_REGISTER( tf_weapon_gib );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFBFG )
END_DATADESC()
#endif

acttable_t CTFBFG::m_acttableBFG_Mercenary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_GIB, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_GIB, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_GIB, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_GIB, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_GIB, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_GIB, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_GIB, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_GIB, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_GIB, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_GIB, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_GIB, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_GIB, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_GIB, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_GIB, false },

	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_MERC_GIB_ATTACK_STAND_PREFIRE, false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_MERC_GIB_ATTACK_CROUCH_PREFIRE, false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_MERC_GIB_ATTACK_SWIM_PREFIRE, false },
};

acttable_t *CTFBFG::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableBFG_Mercenary);
		return m_acttableBFG_Mercenary;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFBFG::CTFBFG()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFBFG::~CTFBFG()
{
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBFG::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "rocketbackblast" );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFBFG::FireProjectile( CTFPlayer *pPlayer )
{
	m_flShowReloadHintAt = gpGlobals->curtime + 30;
	CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
	if ( pProjectile )
	{
#ifdef GAME_DLL
		CTFBaseRocket *pRocket = (CTFBaseRocket*)pProjectile;
		pRocket->SetLauncher( this );

		RocketHandle hHandle;
		hHandle = pRocket;
		m_Rockets.AddToTail( hHandle );

		m_iRocketCount = m_Rockets.Count();
 #endif
	}

	return pProjectile;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBFG::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();

#ifdef GAME_DLL

	if ( m_flShowReloadHintAt && m_flShowReloadHintAt < gpGlobals->curtime )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			pOwner->HintMessage( HINT_SOLDIER_RPG_RELOAD );
		}
		m_flShowReloadHintAt = 0;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBFG::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBFG::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	m_flShowReloadHintAt = 0;
	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBFG::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	BaseClass::CreateMuzzleFlashEffects( pAttachEnt, nIndex );

	// Don't do backblast effects in first person
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner->IsLocalPlayer() )
		return;

	ParticleProp()->Create( "rocketbackblast", PATTACH_POINT_FOLLOW, "backblast" );
}

#endif