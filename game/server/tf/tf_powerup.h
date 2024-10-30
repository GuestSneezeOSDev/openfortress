//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef TF_POWERUP_H
#define TF_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#include "items.h"

class CTFDroppedPowerup;

typedef CHandle<CTFDroppedPowerup> PowerupHandle;

enum powerupsize_t
{
	POWERUP_TINY,
	POWERUP_SMALL,
	POWERUP_MEDIUM,
	POWERUP_FULL,
	POWERUP_MEGA,
	
	POWERUP_SIZES,
};

extern float PackRatios[POWERUP_SIZES];

//=============================================================================
//
// CTF Powerup class.
//

class CTFPowerup : public CItem
{
public:
	DECLARE_CLASS( CTFPowerup, CItem );
	
	CTFPowerup();

	void			Spawn( void );
	CBaseEntity*	Respawn( void );
	virtual void	Materialize( void );
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );

	bool IsDisabled( void );
	void SetDisabled( bool bDisabled );

	virtual float	GetRespawnDelay( void );
	virtual float	GetRespawnTime( void );

	CNetworkVar( bool, m_bInitialDelay );
	CNetworkVar( float, fl_RespawnTime );
	CNetworkVar( float, fl_RespawnDelay );

#ifdef OF_DLL
	CNetworkVar( float, m_flCooldown );
#endif
	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	virtual powerupsize_t GetPowerupSize( void ) { return POWERUP_FULL; }
	string_t m_iszSpawnSound;

	CNetworkVarForDerived( bool, m_bRespawning );

// CTFPowerup::DropSingleInstance(Vector&, CBaseCombatCharacter*, float, float)
#ifdef OF_DLL
	void			DropSingleInstance( const Vector &Velocity, CBaseCombatCharacter *pBCC, float flLifetime, float flTime, bool bUsePhysics = false );
#else
	void			DropSingleInstance( const Vector &Velocity, CBaseCombatCharacter *pBCC, float flLifetime, float flTime );
#endif

	COutputEvent 	m_OnRespawn;
	bool			m_bDisabled;
	bool			m_bHide;

private:
	DECLARE_DATADESC();
};

#endif // TF_POWERUP_H


