//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: MVM MoneyBundle Ripoff.
//
//=============================================================================//
#ifndef ENTITY_MONEYBUNDLE_H
#define ENTITY_MONEYBUNDLE_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF HealthKit class.
//

DECLARE_AUTO_LIST(IMoneyBundleAutoList)

class CMoneyBundle : public CTFPowerup, public IMoneyBundleAutoList
{
public:
	DECLARE_CLASS( CMoneyBundle, CTFPowerup );

	CMoneyBundle();

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );
	bool ITEM_AddOFMoney(CBasePlayer *pPlayer);
	virtual const char *GetPowerupModel(void) { return "models/items/currencypack_large.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_FULL; }

	string_t m_iszModel;
	string_t m_iszModelOLD;
	string_t m_iszPickupSound;
	
	DECLARE_DATADESC();

	bool   IsCustom( void ) { return false; }
};

class CMoneyBundleSmall : public CMoneyBundle
{
public:
	DECLARE_CLASS( CMoneyBundleSmall, CMoneyBundle );
	powerupsize_t GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetPowerupModel( void ) { return "models/items/currencypack_small.mdl"; }
	DECLARE_DATADESC();
};

class CMoneyBundleMedium : public CMoneyBundle
{
public:
	DECLARE_CLASS( CMoneyBundleMedium, CMoneyBundle );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetPowerupModel( void ) { return "models/items/currencypack_medium.mdl"; }
	DECLARE_DATADESC();
};

class CMoneyBundleCustom : public CMoneyBundle
{
public:
	DECLARE_CLASS(CMoneyBundleCustom, CMoneyBundle);
	//powerupsize_t	GetPowerupSize(void) { return POWERUP_MEDIUM; }
	bool	MyTouch(CBasePlayer *pPlayer);
	bool ITEM_AddOFMoney(CBasePlayer *pPlayer);
	virtual const char *GetPowerupModel(void) { return "models/items/currencypack_medium.mdl"; }

	CNetworkVar(float, fl_MoneyGive);

	DECLARE_DATADESC();
};

#endif