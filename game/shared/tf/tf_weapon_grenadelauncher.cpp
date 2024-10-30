//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_WeaponGrenadeLauncher )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_WeaponGrenadeLauncher )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
//PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );


IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher_Mercenary, DT_WeaponGrenadeLauncher_Mercenary )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher_Mercenary, DT_WeaponGrenadeLauncher_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeLauncher_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher_mercenary, CTFGrenadeLauncher_Mercenary );
//PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher_mercenary );

IMPLEMENT_NETWORKCLASS_ALIASED( TFCGrenadeLauncher, DT_TFCGrenadeLauncher)

BEGIN_NETWORK_TABLE( CTFCGrenadeLauncher, DT_TFCGrenadeLauncher )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCGrenadeLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_grenadelauncher, CTFCGrenadeLauncher );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_grenadelauncher );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFGrenadeLauncher )
END_DATADESC()
#endif

acttable_t CTFGrenadeLauncher_Mercenary::m_acttableChinaLake[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_CHINA_LAKE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_CHINA_LAKE, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_CHINA_LAKE, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_CHINA_LAKE, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_CHINA_LAKE, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_CHINA_LAKE, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_CHINA_LAKE, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_CHINA_LAKE, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_CHINA_LAKE, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_CHINA_LAKE, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_CHINA_LAKE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_CHINA_LAKE, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_CHINA_LAKE, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_CHINA_LAKE, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_CHINA_LAKE, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_CHINA_LAKE, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_CHINA_LAKE, false },

	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MERC_RELOAD_STAND_CHINA_LAKE_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MERC_RELOAD_CROUCH_CHINA_LAKE_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MERC_RELOAD_SWIM_CHINA_LAKE_LOOP, false },

	{ ACT_MP_RELOAD_STAND_END, ACT_MERC_RELOAD_STAND_CHINA_LAKE_END, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MERC_RELOAD_CROUCH_CHINA_LAKE_END, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MERC_RELOAD_SWIM_CHINA_LAKE_END, false },
};

acttable_t* CTFGrenadeLauncher_Mercenary::ActivityList(int& iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableChinaLake);
		return m_acttableChinaLake;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Deploy( void )
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetMaxClip1( void ) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetDefaultClip1( void ) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();
	return;
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	
	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponIdle( void )
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	FireProjectile( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}

float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	return BaseClass::GetProjectileSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Add pipebombs to our list as they're fired
//-----------------------------------------------------------------------------
CBaseEntity *CTFGrenadeLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
	if ( pProjectile )
	{
#ifdef GAME_DLL
		// If we've gone over the max pipebomb count, detonate the oldest

//		CTFGrenadePipebombProjectile *pPipebomb = (CTFGrenadePipebombProjectile*)pProjectile;
//		pPipebomb->SetLauncher( this );
 #endif
	}

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SecondaryAttack( void )
{
#ifdef GAME_DLL

	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeLauncher::Reload( void )
{
	return BaseClass::Reload();
}
