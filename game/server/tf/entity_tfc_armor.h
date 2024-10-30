//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF TFCArmor.
//
//=============================================================================//
#ifndef ENTITY_TFC_ARMOR_H
#define ENTITY_TFC_ARMOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF TFCArmor class.
//

DECLARE_AUTO_LIST(ITFCArmorAutoList)

class CTFCArmor : public CTFPowerup, public ITFCArmorAutoList
{
public:
	DECLARE_CLASS( CTFCArmor, CTFPowerup );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );
	virtual const char *GetPowerupModel(void) { return "models/items/armor_heavy.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_FULL; }

	string_t m_iszModel = MAKE_STRING( "" );
	string_t m_iszModelOLD = MAKE_STRING( "" );
	string_t m_iszPickupSound = MAKE_STRING( "TFCArmor.Touch" );
	
	DECLARE_DATADESC();
};

class CTFCArmorSmall : public CTFCArmor
{
public:
	DECLARE_CLASS( CTFCArmorSmall, CTFCArmor );
	powerupsize_t GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetPowerupModel( void ) { return "models/items/armor_light.mdl"; }
	DECLARE_DATADESC();
};

class CTFCArmorMedium : public CTFCArmor
{
public:
	DECLARE_CLASS( CTFCArmorMedium, CTFCArmor );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetPowerupModel( void ) { return "models/items/armor_medium.mdl"; }
	DECLARE_DATADESC();
};

class CTFCArmorCustom : public CTFCArmor
{
public:
	DECLARE_CLASS(CTFCArmorCustom, CTFCArmor);
	//powerupsize_t	GetPowerupSize(void) { return POWERUP_MEDIUM; }
	bool	MyTouch(CBasePlayer* pPlayer);
	//bool ITEM_AddArmor(CBasePlayer* pPlayer);
	virtual const char* GetPowerupModel(void) { return "models/items/armor_medium.mdl"; }

	CNetworkVar(int, i_ArmorGive);

	DECLARE_DATADESC();
};


#endif // ENTITY_TFC_ARMOR_H
