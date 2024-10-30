//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_ROCKET_H
#define TF_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "of_baseschemaitem.h"
#ifdef CLIENT_DLL
	#include "c_baseanimating.h"
	#define CTFBaseRocket C_TFBaseRocket
#else
	struct bomblet_t;
#include "tf_fx.h"
#endif

//#define TF_ROCKET_RADIUS	(110.0f * 1.1f)	//radius * TF scale up factor

//=============================================================================
//
// TF Base Rocket.
//
class CTFBaseRocket : public CBaseAnimating
{

//=============================================================================
//
// Shared (client/server).
//
public:

	DECLARE_CLASS( CTFBaseRocket, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTFBaseRocket();
	~CTFBaseRocket();

	void	Precache( void );
	void	Spawn( void );

	virtual void	SetLauncher( CBaseEntity *pLauncher ) { m_hOriginalLauncher = pLauncher; }
	CBaseSchemaEntity		*GetOriginalLauncher(void) { return (CBaseSchemaEntity*)m_hOriginalLauncher.Get(); }
	
	virtual void	UpdateOnRemove( void );

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );
	
public:

	CNetworkHandle( CBaseEntity, m_hOriginalLauncher );
	
	float	m_flCreationTime;

	virtual bool	IsDeflectable( void ) { return true; }
	
//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );

private:

	float	 m_flSpawnTime;

//=============================================================================
//
// Server specific.
//
#else

public:

	DECLARE_DATADESC();

	static CTFBaseRocket *Create( CTFWeaponBase *pWeapon, const char *szClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, float flSpeed = 1100.0f );	

	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual void 	Detonate( void );
	virtual void 	ExplodeManualy( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType );

	virtual float	GetDamage() { return m_flDamage; }
	virtual int		GetDamageType() { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int		GetCustomDamageType();
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	virtual void	SetDamageRadius(float flDamageRadius) { m_flDamageRadius = flDamageRadius; }

#ifdef OF_DLL
	virtual void SetExplosionVisualInfo( TFExplosion_Visuals_t *pInfo );
	virtual TFExplosion_Visuals_t GetExplosionVisualInfo() { return m_ExplosionVisualInfo; };
	virtual void SetBombletExplosionVisualInfo( TFExplosion_Visuals_t* pInfo );
	virtual TFExplosion_Visuals_t GetBombletExplosionVisualInfo() { return m_BombletExplosionVisualInfo; };
#endif

	virtual float	GetRadius() { return m_flDamageRadius; }	
	void			DrawRadius( float flRadius );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual int		GetWeaponID( void ) const			{ return m_nWeaponID; }


	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

	void			SetHomingTarget( CBaseEntity *pHomingTarget );
	void			SetHoming( bool bHoming );

	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual bool	WasDefelected() { return m_bWasDeflected; };
	bool	m_bWasDeflected;
	
public:	
	CNetworkVar( int,	m_bCritical );

	bomblet_t *m_pBombletInfo;

protected:

	void			FlyThink( void );

protected:

	// Not networked.
	float					m_flDamage;
	float					m_flDamageRadius;
	int 					m_nWeaponID;

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;


	CHandle<CBaseEntity>	m_hEnemy;
	
	CHandle<CBaseEntity>	m_hHomingTarget;
	
	bool	m_bHoming;

#ifdef OF_DLL
	TFExplosion_Visuals_t	m_ExplosionVisualInfo;
	TFExplosion_Visuals_t	m_BombletExplosionVisualInfo;
#endif

#endif
};

#endif // TF_WEAPONBASE_ROCKET_H