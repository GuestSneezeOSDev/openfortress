//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ROCKET_H
#define TF_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#include "tf_weaponbase.h"
#include "iscorer.h"


//=============================================================================
//
// Generic rocket.
//
//=============================================================================
class CTFProjectile_Rocket : public CTFBaseRocket, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_Rocket, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

	// Creation.
	static CTFProjectile_Rocket *Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flSpeed = 1100.0f );	
	virtual void Spawn();
	virtual void Precache();

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );

	int		GetCritical() { return m_bCritical; }
	void	SetCritical( int bCritical ) { m_bCritical = bCritical; }
	virtual int		GetDamageType();
	virtual int		GetCustomDamageType();
	//virtual bool	wasDefelected() { return m_bWasDeflected; };

private:
	CBaseHandle m_Scorer;
	CNetworkVar( int, m_bCritical );
};

//=============================================================================
//
// TFC Pyro's Rocket.
//
//=============================================================================
class CTFCProjectile_IncendRocket : public CTFProjectile_Rocket
{
public:

	DECLARE_CLASS(CTFCProjectile_IncendRocket, CTFProjectile_Rocket);
	
	static CTFCProjectile_IncendRocket *Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flSpeed = 1100.0f );	
};

//=============================================================================
//
// Bouncer Rocket.
//
//=============================================================================
class CTFProjectile_BouncyRocket : public CTFProjectile_Rocket
{
public:

	DECLARE_CLASS(CTFProjectile_BouncyRocket, CTFProjectile_Rocket);
	DECLARE_NETWORKCLASS();

	CTFProjectile_BouncyRocket();
	
	static CTFProjectile_BouncyRocket *Create(CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles,
											  CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flSpeed = 750.0f, float creationTime = 0.f, Vector velocity = Vector(0.f,0.f,0.f) );

	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void FlyThink(void);
	virtual void RocketTouch(CBaseEntity *pOther);
	void BounceSound(const short data);
	void SetFuseTime( float flTime ) { m_flDetTime = flTime; }

	virtual int		GetDamageType();
	virtual void 	SetModel( const char *szModelName );
	float creationTime;

private:
	QAngle	m_rotationVector;
	float	m_flLastBounce;
	int		m_iOldWaterLevel;
	
	char	szImpactHard[64];
	char	szImpactSoft[64];
	CNetworkVar( float, m_flDetTime );
};
#endif	//TF_PROJECTILE_ROCKET_H