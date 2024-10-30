//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Melee 
//
//=============================================================================

#ifndef TF_WEAPONBASE_MELEE_H
#define TF_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseMelee C_TFWeaponBaseMelee
#endif

//=============================================================================
//
// Weapon Base Melee Class
//
class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseMelee, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseMelee();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }
	virtual void	Precache();
	virtual void	ItemPostFrame();
	virtual void	BurstFire();
	virtual void	Spawn();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	Deploy();
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_NONE; }
	virtual bool	ShouldDrawCrosshair( void )						{ return true; }
	virtual void	WeaponReset( void );

	virtual bool	CalcIsAttackCriticalHelper( void );

	virtual void	DoViewModelAnimation( void );

	virtual bool DoSwingTrace( trace_t &tr );
	virtual void	Smack( void );
	virtual float GetSmackDelay( void );

	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );

	// Call when we hit an entity. Use for special weapon effects on hit.
	virtual void	OnEntityHit( CBaseEntity *pEntity );

	virtual void	SendPlayerAnimEvent( CTFPlayer *pPlayer );

	int				IsCurrentAttackACritical( void );
	bool			ConnectedHit( void ) { return m_bConnected; }
	virtual float	GetShieldChargeMeter( void ) { return m_flChargeMeter; }
	virtual void	SetShieldChargeMeter(float bValue){ m_flChargeMeter = bValue; };
	virtual bool	CanShieldCharge( void );
	virtual void	ShieldChargeThink( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual bool 	IsMeleeWeapon( void )const { return true; };

public:	

	CTFWeaponInfo	*m_pWeaponInfo;

private:
	CNetworkVar( float,		m_flChargeMeter );
	int m_iNumBeepsToBeep;
	int m_iChargeSound;
	
protected:

	void			Swing( CTFPlayer *pPlayer );
	void			SwingNoDelay( CTFPlayer *pPlayer );

protected:

	float	m_flSmackTime;
	bool	m_bConnected;

private:

	CTFWeaponBaseMelee( const CTFWeaponBaseMelee & ) {}
};

#endif // TF_WEAPONBASE_MELEE_H
