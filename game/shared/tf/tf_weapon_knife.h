//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_KNIFE_H
#define TF_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFKnife C_TFKnife
#define CTFCKnife C_TFCKnife
#define CTFCombatKnife C_TFCombatKnife
#endif

//=============================================================================
//
// Knife class.
//
class CTFKnife : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFKnife, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKnife();
	virtual void		PrimaryAttack( void );
	virtual bool		Deploy( void );
	virtual void		ItemPostFrame( void );
	virtual bool		SendWeaponAnim( int iActivity );
	virtual void		KnifeThink( void );

	virtual void		Smack( void );
	void				SwitchBodyGroups( void );
	virtual bool		DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual void		WeaponReset( void );

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_KNIFE; }
	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableKnife[];

	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );

	virtual void		SendPlayerAnimEvent( CTFPlayer *pPlayer );

	bool				IsBehindAndFacingTarget( CBaseEntity *pTarget );

	virtual bool		CalcIsAttackCriticalHelper( void );

private:
	EHANDLE				m_hBackstabVictim;

	CTFKnife( const CTFKnife & ) {}
	CNetworkVar( bool, m_bReady );
	CNetworkVar( bool, m_bBlood );
};

class CTFCKnife : public CTFKnife
{
public:

	DECLARE_CLASS( CTFCKnife, CTFKnife);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	virtual int			GetWeaponID( void ) const			{ return TFC_WEAPON_KNIFE; }
};

class CTFCombatKnife : public CTFKnife
{
public:

	DECLARE_CLASS(CTFCombatKnife, CTFKnife);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	virtual int			GetWeaponID(void) const			{ return TF_WEAPON_COMBATKNIFE; }
};
#endif // TF_WEAPON_KNIFE_H
