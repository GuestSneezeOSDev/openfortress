//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Base Projectile
//
//=============================================================================
#ifndef TF_BASE_PROJECTILE_H
#define TF_BASE_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "tempent.h"

	#define CTFBaseProjectile C_TFBaseProjectile
	C_LocalTempEntity *ClientsideProjectileCallback( const CEffectData &data, float flGravityBase, const char *pszParticleName = NULL );
#else
	#include "iscorer.h"
#endif

/* 
CTFBaseProjectile
	|
	|-	CTFProjectile_Nail
	|-	CTFProjectile_Dart
	|-  CTFBaseRocket
			|
			|- Soldier rocket
			|- Pyro rocket
*/

//=============================================================================
//
// Generic projectile
//
class CTFBaseProjectile : public CBaseAnimating
#if !defined( CLIENT_DLL )
	, public IScorer
#endif
{
public:

	DECLARE_CLASS( CTFBaseProjectile, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTFBaseProjectile();
	~CTFBaseProjectile();

	void	Precache( void );
	void	Spawn( void );

	virtual int   GetWeaponID( void ) const { return m_iWeaponID; }
	void		  SetWeaponID( int iID ) { m_iWeaponID = iID; }

	bool		  IsCritical( void )				{ return m_bCritical > 0; }
	virtual void  SetCritical( int bCritical )		{ m_bCritical = bCritical; }

private:

	int				m_iWeaponID;
	int				m_bCritical;

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );

	static CTFBaseProjectile *Create( const char *pszClassname, const Vector &vecOrigin, 
		const QAngle &vecAngles, CBaseEntity *pOwner, float flVelocity, short iProjModelIndex, const char *pszDispatchEffect = NULL, CBaseEntity *pScorer = NULL, int bCritical = false );

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void ) { return 0.001f; }

#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );

private:

	float	 m_flSpawnTime;

#else

public:

	DECLARE_DATADESC();

#if !defined( CLIENT_DLL )
	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }
#endif

	void	SetScorer( CBaseEntity *pScorer );

	virtual void	ProjectileTouch( CBaseEntity *pOther );

	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	virtual Vector	GetDamageForce( void );
	virtual int		GetDamageType( void );
	virtual int		GetCustomDamageType( void );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

protected:
	virtual bool	WasDefelected() { return m_bWasDeflected; };
	bool			m_bWasDeflected;
	void			FlyThink( void );

protected:
	float			m_flDamage;

	CBaseHandle		m_Scorer;

#endif // ndef CLIENT_DLL
};

#endif	//TF_BASE_PROJECTILE_H