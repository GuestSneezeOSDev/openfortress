//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#ifndef TF_WEAPONBASE_GUN_H
#define TF_WEAPONBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseGun C_TFWeaponBaseGun
#endif

#define ZOOM_CONTEXT		"ZoomContext"
#define ZOOM_REZOOM_TIME	0.1f

//=============================================================================
//
// Weapon Base Melee Gun
//

//=============================================================================
//
// Assault Rifle cvars
//
//=============================================================================
extern ConVar of_weapon_assaultrifle_burstshots;
extern ConVar of_weapon_assaultrifle_bursttime;
extern ConVar of_weapon_assaultrifle_time_between_bursts;

class CTFWeaponBaseGun : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseGun, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 

	DECLARE_PREDICTABLE();

	DECLARE_DATADESC();

	CTFWeaponBaseGun();

	virtual void PrimaryAttack();
	virtual void ItemPostFrame();
	virtual void HandleFireOnEmpty();
	virtual bool FiresInBursts();
	virtual void SecondaryAttack( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual void	DoViewModelAnimation( void );

	// Derived classes call this to fire a bullet.
	//bool TFBaseGunFire( void );

	virtual void DoFireEffects();

	void ToggleZoom( void );

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	void				GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true );

	void				OnShotFired(void);
	
	void FireBullet( CTFPlayer *pPlayer );
	CBaseEntity *FireRocket( CTFPlayer *pPlayer );
	CBaseEntity *FireCoom( CTFPlayer *pPlayer );
	CBaseEntity *FireNail( CTFPlayer *pPlayer, int iSpecificNail );
	CBaseEntity *FirePipeBomb( CTFPlayer *pPlayer, bool bRemoteDetonate );
	CBaseEntity *FirePipeBombDM( CTFPlayer *pPlayer, bool bRemoteDetonate );
	CBaseEntity *FireTripmine( CTFPlayer *pPlayer );
	CBaseEntity *FireIncendRocket( CTFPlayer *pPlayer );
	CBaseEntity *FireBouncer(CTFPlayer *pPlayer);
	CBaseEntity *FireSawblade( CTFPlayer *pPlayer );

	virtual float GetWeaponSpread( void );
	virtual float GetProjectileSpeed( void );

	void UpdatePunchAngles( CTFPlayer *pPlayer );
	virtual float GetProjectileDamage( void );
	int GetBulletsPerShot( void ) const;

	virtual void ZoomIn( void );
	virtual void ZoomOut( void );
	void ZoomOutIn( void );

	virtual void PlayWeaponShootSound( void );

	void			BurstFire( void );
	void			BeginBurstFire( void );
	virtual bool	Reload( void );

private:

	CTFWeaponBaseGun( const CTFWeaponBaseGun & );	
	float	m_flChargeBeginTime;
	CNetworkVar( float,	m_flAccurateAtTick );
};

#endif // TF_WEAPONBASE_GUN_H