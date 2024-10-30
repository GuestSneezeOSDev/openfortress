//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Medkit Class
//
//=============================================================================
#ifndef TF_WEAPON_Medkit_H
#define TF_WEAPON_Medkit_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFMedkit C_TFMedkit
#endif

//=============================================================================
//
// Medkit class.
//
class CTFMedkit : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFMedkit, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFMedkit();
	virtual void		PrimaryAttack( void );
	virtual int			GetWeaponID( void ) const			{ return TFC_WEAPON_MEDKIT; }
	virtual void		Swing( CTFPlayer *pPlayer );
	void				SwingMiss( CTFPlayer *pPlayer );

	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );

private:
	EHANDLE				m_hVictim;

	CTFMedkit( const CTFMedkit & ) {}
};

#endif // TF_WEAPON_MEDKIT_H
