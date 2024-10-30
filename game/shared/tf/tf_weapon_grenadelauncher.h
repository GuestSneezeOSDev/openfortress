//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_GRENADELAUNCHER_H
#define TF_WEAPON_GRENADELAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeLauncher C_TFGrenadeLauncher
#define CTFGrenadeLauncher_Mercenary C_TFGrenadeLauncher_Mercenary
#define CTFCGrenadeLauncher C_TFCGrenadeLauncher
#endif

#define TF_GRENADE_LAUNCER_MIN_VEL 1200

#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeLauncher : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFGrenadeLauncher();
	~CTFGrenadeLauncher();

	virtual void	Spawn( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADELAUNCHER; }
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	PrimaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual float	GetProjectileSpeed( void );

	virtual bool	Reload( void );
	
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );

	virtual int GetMaxClip1( void ) const;
	virtual int GetDefaultClip1( void ) const;

public:

	void LaunchGrenade( void );

private:

	CTFGrenadeLauncher( const CTFGrenadeLauncher & ) {}
};

// Mercenary specific version
class CTFGrenadeLauncher_Mercenary : public CTFGrenadeLauncher
{
public:
	DECLARE_CLASS( CTFGrenadeLauncher_Mercenary, CTFGrenadeLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADELAUNCHER_MERCENARY; }

	virtual acttable_t* ActivityList(int& iActivityCount);
	static acttable_t m_acttableChinaLake[];
};


class CTFCGrenadeLauncher : public CTFGrenadeLauncher
{
public:
	DECLARE_CLASS( CTFCGrenadeLauncher, CTFGrenadeLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_GRENADELAUNCHER; }
};

#endif // TF_WEAPON_GRENADELAUNCHER_H