//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_DROPPED_POWERUP_H
#define TF_DROPPED_POWERUP_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"
#include "tf_powerup.h"

class CTFDroppedPowerup : public CTFPowerup
{
public:
	DECLARE_CLASS( CTFDroppedPowerup, CTFPowerup );
	DECLARE_SERVERCLASS();

	CTFDroppedPowerup();
	~CTFDroppedPowerup();

	virtual void Spawn();		

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	int m_iPowerupID;

	float GetCreationTime( void ) { return m_flCreationTime; }
	CNetworkVar( float, m_flCreationTime );
	CNetworkVar( float, m_flDespawnTime );
	char  szTimerIcon[128];
	void  SetInitialVelocity( Vector &vecVelocity );
	
	const char* GetPowerupDroppedLine( void );

private:

	bool m_bAllowOwnerPickup;
	CNetworkVector( m_vecInitialVelocity );

private:
	CTFDroppedPowerup( const CTFDroppedPowerup & );
	
	DECLARE_DATADESC();
};

#endif //TF_DROPPED_POWERUP_H