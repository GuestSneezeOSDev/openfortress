//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_PROJECTILE_BFG_H
#define TF_PROJECTILE_BFG_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_baseanimating.h"
// Server specific.
#else
#include "baseanimating.h"
#include "smoke_trail.h"
#include "tf_weaponbase.h"
#endif

#include "tf_weaponbase_rocket.h"

#ifdef CLIENT_DLL
#define CTFBFGProjectile C_TFBFGProjectile
#endif

//#define TF_ROCKET_RADIUS	(110.0f * 1.1f)	//radius * TF scale up factor


#ifdef GAME_DLL

class CTFBFGArea : public CBaseEntity
{
	DECLARE_CLASS( CTFBFGArea, CBaseEntity );
public:
	virtual void Spawn( void );

public:
	static CTFBFGArea *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float m_flDmgAmount, int m_iCustomDmgType );

	void FlameThink( void );
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );
public:
	bool					m_bRemove;				// time at which the flame should be removed
private:
	void OnCollide( CBaseEntity *pOther );

	Vector					m_vecInitialPos;		// position the flame was fired from
	Vector					m_vecPrevPos;			// position from previous frame
	int						m_iDmgType;				// damage type
	int						m_iCustomDmgType;		// custom damage type
	float					m_flDmgAmount;			// amount of base damage
	CUtlVector<EHANDLE>		m_hEntitiesBurnt;		// list of entities this flame has burnt
	EHANDLE					m_hAttacker;			// attacking player
	int						m_iAttackerTeam;		// team of attacking player

	virtual bool	IsDeflectable( void ) { return false; }
};

#endif // GAME_DLL

//=============================================================================
//
// TF Base Rocket.
//
class CTFBFGProjectile : public CTFBaseRocket
{

//=============================================================================
//
// Shared (client/server).
//
public:

	DECLARE_CLASS( CTFBFGProjectile, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

		CTFBFGProjectile();
		~CTFBFGProjectile();
	
	virtual void Spawn();

#ifdef GAME_DLL
	// Creation.
	static CTFBFGProjectile *Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flSpeed = 1100.0f );		
	virtual void Precache();

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );
	virtual int		GetDamageType();
	virtual int		GetCustomDamageType();		
	
	virtual void	RocketTouch( CBaseEntity *pOther );
	unsigned int	PhysicsSolidMaskForEntity( void ) const;
	virtual void 	FlyThink( void );
#else
	virtual void	OnDataChanged(DataUpdateType_t updateType);

	virtual void	CreateRocketTrails( void );
	virtual void	CreateLightEffects(void);
	virtual const char *GetTrailParticleName( void );
#endif
	CNetworkVar( bool,	m_bSpawnTrails );
	void	SetCritical( int bCritical ) { m_bCritical = bCritical; }	

#ifdef CLIENT_DLL
private:

	float	 m_flSpawnTime;
#else

public:
	DECLARE_DATADESC();
private:
	CBaseHandle m_Scorer;
	CTFBFGArea *pOrb;
#endif	
	CNetworkVar( int,	m_bCritical );

};

#endif // TF_PROJECTILE_BFG_H