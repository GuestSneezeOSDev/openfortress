//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// OF E.Y.E.D.O.L. 9000 / BFG
//
//=============================================================================
#ifndef OF_WEAPON_BFG_H
#define OF_WEAPON_BFG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"


// Client specific.
#ifdef CLIENT_DLL

#define CTFBFG C_TFBFG
#else
#include "tf_projectile_rocket.h"
#endif

//=============================================================================
//
// TF Weapon Rocket Launcher.
//
class CTFBFG : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFBFG, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableBFG_Mercenary[];

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFBFG();
	~CTFBFG();

#ifndef CLIENT_DLL
	virtual void	Precache();
#endif
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GIB; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
	//virtual void DrawCrosshair( void );
#endif

	// List of active pipebombs
	typedef CHandle<CTFBaseRocket>	RocketHandle;
	CUtlVector<RocketHandle>		m_Rockets;	
	
	// This is here so we can network the pipebomb count for prediction purposes
	CNetworkVar( int,				m_iRocketCount );

private:
	float	m_flShowReloadHintAt;

	//CNetworkVar( bool, m_bLockedOn );

	CTFBFG( const CTFBFG & ) {}
};

#endif // OF_WEAPON_BFG_H