//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WEAPON_BUILDER_H
#define TF_WEAPON_BUILDER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

class CBaseObject;

//=========================================================
// Builder Weapon
//=========================================================
class CTFWeaponBuilder : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponBuilder, CTFWeaponBase );
public:
	CTFWeaponBuilder();
	~CTFWeaponBuilder();

	DECLARE_SERVERCLASS();

	virtual void	Equip(CBaseCombatCharacter *pOwner);
	virtual void	Detach();
	virtual void	SetSubType( int iSubType );
	virtual void	SetSubType( int iSubType, int iAltMode );
	virtual void	Precache( void );
	virtual bool	CanDeploy( void );
	virtual bool    CanHolster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	void			HaulingAttack( void );
	virtual void	WeaponIdle( void );
	virtual bool	Deploy( void );	
	virtual Activity GetDrawActivity( void );
	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual bool	AllowsAutoSwitchTo( void ) const;

	virtual int		GetType( void ) { return m_iObjectType; }
	virtual int		GetAltMode( void ) { return m_iAltMode; }

	void	SetCurrentState( int iState );
	void	SwitchOwnersWeaponToLast();

	// Placement
	void	StartPlacement( void );
	void	StopPlacement( void );
	void	UpdatePlacementState( void );		// do a check for valid placement
	bool	IsValidPlacement( void );			// is this a valid placement pos?

	// Building
	void	StartBuilding( void );


	// Selection
	bool	HasAmmo( void );
	int		GetSlot( void ) const;
	int		GetPosition( void ) const;
	const char *GetPrintName( void ) const;

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_BUILDER; }

	virtual void	WeaponReset( void );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttableBuildingDeployed[];

public:
	CNetworkVar( int, m_iBuildState );
	CNetworkVar( unsigned int, m_iObjectType );
	CNetworkVar( unsigned int, m_iAltMode );
	CNetworkVar( float, m_flSecondaryTimeout );

	CNetworkHandle( CBaseObject, m_hObjectBeingBuilt );

#ifdef GAME_DLL
	CUtlVector<int> m_iBuildableObjects;
#endif

	int m_iValidBuildPoseParam;

	float m_flNextDenySound;
};


#endif // TF_WEAPON_BUILDER_H
