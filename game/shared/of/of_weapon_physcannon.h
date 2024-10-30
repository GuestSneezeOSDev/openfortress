//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OFD_WEAPON_PHYSGAUNTLET_H
#define OFD_WEAPON_PHYSGAUNTLET_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFGravityGauntlets C_TFGravityGauntlets
#endif


//-----------------------------------------------------------------------------
// Do we have the super-phys gun?
//-----------------------------------------------------------------------------
bool PlayerHasMegaPhysCannon();

// force the physcannon to drop an object (if carried)
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis );
void PhysCannonBeginUpgrade( CBaseAnimating *pAnim );

bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupController, CBaseEntity *pHeldEntity );
float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject );
float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject );

CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon );

#include "tf_weaponbase_melee.h"
#ifdef CLIENT_DLL
#include "particles_simple.h"
#endif
//=============================================================================
//
// Gravity Gauntlet class.
//
class CTFGravityGauntlets : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFGravityGauntlets, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFGravityGauntlets();
	~CTFGravityGauntlets();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRAVITYGAUNTLETS; }
	virtual void		ItemPostFrame( void );
	void				OnPull( CBaseEntity *pPullTarget );
	virtual bool		IsMeleeWeapon()const { return false; };
	virtual void		PrimaryAttack();
	virtual void		Smack( void );
	// Skip the melee functions because they cause us to always be considered has having ammo
	virtual bool		HasPrimaryAmmo()								{ return CTFWeaponBase::HasPrimaryAmmo(); }
	virtual bool		CanBeSelected()									{ return CTFWeaponBase::CanBeSelected(); }	
	virtual void		OnAirblast( CBaseEntity *pEntity );
#ifdef CLIENT_DLL
	void				StopPullEffect( void );
	void				UpdatePullEffect( void );
#endif
private:

	CTFGravityGauntlets( const CTFGravityGauntlets & ) {}
	CNetworkVector( m_vecPullPos );
#ifdef CLIENT_DLL
	HPARTICLEFFECT m_hPullEffect;
	HPARTICLEFFECT m_hHoverEffect;
	float	flNextError;
	bool	bIsHovering;
#else
	float	m_flAmmoConsumption;
#endif
};

#endif // OFD_WEAPON_PHYSGAUNTLET_H
