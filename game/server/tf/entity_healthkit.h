//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#ifndef ENTITY_HEALTHKIT_H
#define ENTITY_HEALTHKIT_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF HealthKit class.
//

DECLARE_AUTO_LIST(IHealthKitAutoList)

class CHealthKit : public CTFPowerup, public IHealthKitAutoList
{
public:
	DECLARE_CLASS( CHealthKit, CTFPowerup );

	CHealthKit();

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );
	virtual const char *GetPowerupModel(void) { return "models/items/medkit_large.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_FULL; }

	string_t m_iszModel;
	string_t m_iszModelOLD;
	string_t m_iszPickupSound;
	
	DECLARE_DATADESC();

	bool   IsTiny( void ) { return false; }
	bool   IsMega( void ) { return false; }
};

class CHealthKitSmall : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitSmall, CHealthKit );
	powerupsize_t GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetPowerupModel( void ) { return "models/items/medkit_small.mdl"; }
	DECLARE_DATADESC();
};

class CHealthKitMedium : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitMedium, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetPowerupModel( void ) { return "models/items/medkit_medium.mdl"; }
	DECLARE_DATADESC();
};

//Pills

class CHealthKitTiny : public CHealthKit
{
public:

	DECLARE_CLASS(CHealthKitTiny, CHealthKit);

	CHealthKitTiny();

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_overheal.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_TINY; }
	string_t m_iszPickupSound;

	bool MyTouch(CBasePlayer *pPlayer);
	bool ITEM_GiveTFTinyHealth(CBasePlayer *pPlayer);
	void Precache(void);

	virtual bool IsTiny(void) { return true; }

	DECLARE_DATADESC();
};

#endif // ENTITY_HEALTHKIT_H
