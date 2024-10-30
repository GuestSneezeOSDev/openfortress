//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_TRANQ_H
#define TF_WEAPON_TRANQ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFCTranq C_TFCTranq
#endif

//=============================================================================
//
// TF Weapon Tranq.
//
class CTFCTranq : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFCTranq, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFCTranq() {}
	~CTFCTranq() {}

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_TRANQ; }

	virtual void	Precache();

private:

	CTFCTranq( const CTFCTranq & ) {}
};

#endif // TF_WEAPON_TRANQ_H