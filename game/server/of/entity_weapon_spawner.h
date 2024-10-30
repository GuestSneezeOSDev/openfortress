//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef ENTITY_WEAPON_SPAWNER_H
#define ENTITY_WEAPON_SPAWNER_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF WeaponSpawner class.
//

enum ETFWeaponTier
{
	OF_WEAPON_TIER_NORMAL,
	OF_WEAPON_TIER_AWESOME,
	OF_WEAPON_TIER_GODLIKE,
};

struct DroppedWeaponInfo_t
{
	DroppedWeaponInfo_t()
	{
		m_iReserveAmmo = -1;
		m_iClip = -1;
		m_bThrown = false;
	};
	int m_iReserveAmmo;
	int m_iClip;
	CUtlVector<CTFAttribute> m_hAttributes;
	bool m_bThrown;
};

DECLARE_AUTO_LIST(IWeaponSpawnerAutoList)
class CWeaponSpawner : public CTFPowerup, public IWeaponSpawnerAutoList
{
public:
	DECLARE_CLASS( CWeaponSpawner, CTFPowerup );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	
	CWeaponSpawner();

	void	Spawn( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual CBaseEntity* Respawn( void );
	void	Precache( void );

	ETFWeaponTier GetWeaponTier( CTFWeaponBase *pWeapon );
    bool	IsSuperWeapon(){ return m_bSuperWeapon; }

	const char *CheckHasAkimbo( CTFWeaponBase *pOwnedWeapon, CTFPlayer *pPlayer );
	void GiveWeaponDefault( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer );
	void GiveWeaponMulti( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer );
	void GiveWeaponSlots( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer );

	bool GiveAmmo( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );
	void GiveInitialAmmo( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );

	bool MyTouch( CBasePlayer *pPlayer );
	void EndTouch( CBaseEntity *pOther );
	
	float GetRespawnTime( void ) override;

	void Update ( void );
	void AnnouncerThink( void );

	powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }
	const char* GetSuperWeaponRespawnLine( void );
	const char* GetSuperWeaponPickupLine( void );
	const char* GetSuperWeaponPickupLineSelf( void );
	const char* GetSuperWeaponPickupLineIncoming( void );

	virtual void Materialize( void );

	bool	ThrownWeaponTouch( CTFPlayer *pTFPlayer );
	void	EXPORT FlyThink( void );
	void	SetupDroppedWeapon( DroppedWeaponInfo_t *pNewInfo );

	void	SetWeaponModel( char *szWeaponModel );
	void	SetWeaponName( char *szWeaponName );
	const char *GetWeaponName( void ){ return m_szWeaponName.GetForModify(); }

	void	SetWeaponModel( const char *szWeaponModel );
	void	SetWeaponName( const char *szWeaponName );
	
	void	InputSetWeaponModel( inputdata_t &inputdata );
	void	InputSetWeaponName( inputdata_t &inputdata );

	DroppedWeaponInfo_t* GetInfo() { return &m_hDroppedWeaponInfo; };

private:
	CNetworkVar( int, m_iIndex );
	CNetworkString( m_szWeaponName, 64 );
	string_t m_iszWeaponModel;
	string_t m_iszPickupSound;

	CNetworkVar( bool, m_bDisableSpin );
	CNetworkVar( bool, m_bDisableShowOutline );

	CNetworkVar( float, m_flRespawnTick );
	CNetworkVar( bool, m_bSuperWeapon );
	bool m_bWarningTriggered;

	CNetworkVar( bool, m_bDropped );
	float m_flNextPickupTime;

	DroppedWeaponInfo_t m_hDroppedWeaponInfo;

	CTFWeaponInfo *m_pWeaponInfo;
	WEAPON_FILE_INFO_HANDLE	m_hWpnInfo;
};

#endif // ENTITY_WEAPON_SPAWNER_H


