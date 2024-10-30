//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_DROPPED_WEAPON_H
#define TF_DROPPED_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"

typedef unsigned short WEAPON_FILE_INFO_HANDLE;

class CTFAttribute;

//needed to remove dropped weapons at the end of every round
DECLARE_AUTO_LIST( ICondDroppedWeaponAutoList );

class CTFDroppedWeapon : public CItem, public ICondDroppedWeaponAutoList
{
public:
	DECLARE_CLASS( CTFDroppedWeapon, CItem );
	DECLARE_SERVERCLASS();

	CTFDroppedWeapon();

	virtual void Spawn();
	virtual void Precache();	

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	static CTFDroppedWeapon *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, WEAPON_FILE_INFO_HANDLE m_hWpnInfo, const char *pszClassname, CUtlVector<CTFAttribute> *hAttributes = NULL, bool bThrown = false );

	float GetCreationTime( void ) { return m_flCreationTime; }
	void  SetInitialVelocity( Vector &vecVelocity );
	
	int GetTeamNum(){ return m_iTeamNum; }
	void SetTeamNum( int iTeam ) { m_iTeamNum = iTeam; }

public:
	CNetworkVar( int, m_iReserveAmmo );
	CNetworkVar( int, m_iClip );
	CNetworkVar( bool, m_bFlamethrower );
	
	CTFWeaponInfo	*pWeaponInfo;
	char			pszWeaponName[64];
	WEAPON_FILE_INFO_HANDLE	m_hWpnInfo;

private:
	bool m_bAllowOwnerPickup;
	bool m_bThrown;
	float m_flCreationTime;
	float m_flNextPickupTime;
	int m_iTeamNum;

	CNetworkVector( m_vecInitialVelocity );
	CUtlVector<CTFAttribute> m_hAttributes;

	CTFDroppedWeapon( const CTFDroppedWeapon & );

	DECLARE_DATADESC();
};

#endif //TF_DROPPED_WEAPON_H