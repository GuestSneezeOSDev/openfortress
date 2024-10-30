//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_PISTOL_H
#define TF_WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPistol C_TFPistol
#define CTFPistol_Scout C_TFPistol_Scout
#define CTFPistol_Mercenary C_TFPistol_Mercenary
#define CTFPistol_Akimbo C_TFPistol_Akimbo
#define CTFCRailPistol C_TFCRailPistol
#endif

// We allow the pistol to fire as fast as the player can click.
// This is the minimum time between shots.
#define	PISTOL_FASTEST_REFIRE_TIME		0.1f

// The faster the player fires, the more inaccurate he becomes
#define	PISTOL_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum time penalty we'll allow

//=============================================================================
//
// TF Weapon Pistol.
//
class CTFPistol : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFPistol, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFPistol();
//	~CTFPistol() {}

	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL; }
	CNetworkVar( float,	m_flSoonestPrimaryAttack );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttablePistol[];
	static acttable_t m_acttableSecondary2[];
private:
	CTFPistol( const CTFPistol & ) {}
};

// Scout specific version
class CTFPistol_Scout : public CTFPistol
{
public:
	DECLARE_CLASS( CTFPistol_Scout, CTFPistol );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL_SCOUT; }
};
class CTFPistol_Mercenary : public CTFPistol
{
public:
	DECLARE_CLASS( CTFPistol_Mercenary, CTFPistol );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL_MERCENARY; }
	virtual void	Precache( void );
	
	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttablePistol[];
	static acttable_t m_acttableSecondary2[];

};
class CTFPistol_Akimbo : public CTFPistol
{
public:
	DECLARE_CLASS( CTFPistol_Akimbo, CTFPistol );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL_AKIMBO; }
	
	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttablePistolAkimbo[];
};
class CTFCRailPistol : public CTFPistol
{
public:
	DECLARE_CLASS( CTFCRailPistol, CTFPistol );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_RAILPISTOL; }
};


#endif // TF_WEAPON_PISTOL_H