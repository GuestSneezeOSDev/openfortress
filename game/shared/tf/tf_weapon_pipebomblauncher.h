//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_PIPEBOMBLAUNCHER_H
#define TF_WEAPON_PIPEBOMBLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_pipebomb.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPipebombLauncher C_TFPipebombLauncher
#define CTFDynamite C_TFDynamite
#define CTFCPipebombLauncher C_TFCPipebombLauncher
#endif

//=============================================================================
//
// TF Weapon Pipebomb Launcher.
//
#ifdef GAME_DLL
	class CTFPipebombLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon, public IEntityListener
#else
	class CTFPipebombLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
#endif
{
public:

	DECLARE_CLASS( CTFPipebombLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFPipebombLauncher();
	~CTFPipebombLauncher();

	virtual void	Spawn( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PIPEBOMBLAUNCHER; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemBusyFrame( void );
	//virtual void	ItemPostFrame(void);
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	PrimaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual float	GetProjectileSpeed( void );
	virtual bool	Reload( void );
	virtual void	WeaponReset( void );
	virtual bool	IsStickyBombLauncher() const { return true; }

public:
	// ITFChargeUpWeapon
	virtual float GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float GetChargeMaxTime( void );
	int	GetPipeBombCount( void ) { return m_iPipebombCount; }


	void LaunchGrenade( void );
	void ExplodeInHand(void);
	bool DetonateRemotePipebombs( bool bFizzle );
	void AddPipeBomb( CTFGrenadePipebombProjectile *pBomb );
	void			DeathNotice( CBaseEntity *pVictim );


#ifdef GAME_DLL
	void			UpdateOnRemove( void );
	
	


	// This is here so we can network the pipebomb count for prediction purposes
	CNetworkVar( int,				m_iPipebombCount );	
#endif

#ifdef CLIENT_DLL
	int				m_iPipebombCount;
#endif

	// List of active pipebombs
	typedef CHandle<CTFGrenadePipebombProjectile>	PipebombHandle;
	CUtlVector<PipebombHandle>		m_Pipebombs;

private:
	float	m_flChargeBeginTime;
	float	m_flLastDenySoundTime;

	CTFPipebombLauncher( const CTFPipebombLauncher & ) {}
};

// Dynamite Pack
class CTFDynamite : public CTFPipebombLauncher
{
public:
	DECLARE_CLASS( CTFDynamite, CTFPipebombLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_DYNAMITE_BUNDLE; }

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableDynamiteBundle[];

	virtual bool	IsStickyBombLauncher() const { return false; }
	//bool			CanSoftZoom(void) { return false; }
};


class CTFCPipebombLauncher : public CTFPipebombLauncher
{
public:
	DECLARE_CLASS( CTFCPipebombLauncher, CTFPipebombLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_PIPEBOMBLAUNCHER; }
};


#endif // TF_WEAPON_PIPEBOMBLAUNCHER_H
