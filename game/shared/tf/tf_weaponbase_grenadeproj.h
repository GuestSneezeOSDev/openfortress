//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADEPROJ_H
#define TF_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#else
struct bomblet_t;
#include "tf_fx.h"
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFWeaponBaseGrenadeProj : public CBaseGrenade
{
public:

	DECLARE_CLASS( CTFWeaponBaseGrenadeProj, CBaseGrenade );
	DECLARE_NETWORKCLASS();

							CTFWeaponBaseGrenadeProj();
	virtual					~CTFWeaponBaseGrenadeProj();
	virtual void			Spawn();
	virtual void			Precache();

	void					InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, CTFWeaponBase *pWeapon );

	// Unique identifier.
	virtual int GetWeaponID( void ) const { return TF_WEAPON_NONE; }

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );

	virtual float		GetShakeAmplitude( void ) { return 10.0; }
	virtual float		GetShakeRadius( void ) { return 300.0; }
	void				SetCritical( int bCritical ) { m_bCritical = bCritical; }
	
	virtual int			GetDamageType();
	virtual int			GetCustomDamageType();

	virtual bool	IsDeflectable( void ) { return true; }
	virtual bool	WasDefelected() { return m_bWasDeflected; };
	bool	m_bWasDeflected;
#ifdef GAME_DLL
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
#ifdef OF_DLL
	virtual void SetExplosionVisualInfo( TFExplosion_Visuals_t *pInfo );
	virtual TFExplosion_Visuals_t GetExplosionVisualInfo() { return m_ExplosionVisualInfo; };
	virtual void SetBombletExplosionVisualInfo(TFExplosion_Visuals_t* pInfo);
	virtual TFExplosion_Visuals_t GetBombletExplosionVisualInfo() { return m_BombletExplosionVisualInfo; };
#endif
#endif
		CNetworkVar( float,		m_flSpawnTime );
private:

	CTFWeaponBaseGrenadeProj( const CTFWeaponBaseGrenadeProj & );

protected:
	CNetworkVar(float, m_flDetonateTime);
	// Client specific.
#ifdef CLIENT_DLL

public:

	virtual void			OnDataChanged( DataUpdateType_t type );

	int						m_bCritical;
	
	virtual C_BaseEntity	*GetItemTintColorOwner( void ) { return GetThrower(); }

	HPARTICLEFFECT			m_hTimerParticle;
	// Server specific.
#else

public:

	DECLARE_DATADESC();

	static CTFWeaponBaseGrenadeProj *Create( const char *szName, const Vector &position, const QAngle &angles, 
				const Vector &velocity, const AngularImpulse &angVelocity, 
				CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags, CTFWeaponBase *pWeapon );

	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	void					Detonate( void );

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	void					RemoveGrenade( bool bBlinkOut = true );

	void					SetTimer( float time ){ m_flDetonateTime = time; }
	float					GetDetonateTime( void ){ return m_flDetonateTime; }

	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual void			Explode( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }
	
public:
	CTFWeaponBase		 	*pFuckThisShit; // Massive ass hack, this gets set in tf_weapon_grenade_pipebomb

	bomblet_t				*m_pBombletInfo;
protected:

	void					DrawRadius( float flRadius );

	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

private:

	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	bool					m_bInSolid;

	CNetworkVar( int,		m_bCritical );

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;

#ifdef OF_DLL
	TFExplosion_Visuals_t	m_ExplosionVisualInfo;
	TFExplosion_Visuals_t	m_BombletExplosionVisualInfo;
#endif
#endif
};


//=============================================================================
//
// TF Mirv Grenade Projectile and Bombs (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeMirvProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeMirvProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRV; }

	// Creation.
	static CTFGrenadeMirvProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                     const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	void			DetonateThink( void );

	DECLARE_DATADESC();

private:

	bool			m_bPlayedLeadIn;
};

#endif

#endif // TF_WEAPONBASE_GRENADEPROJ_H
