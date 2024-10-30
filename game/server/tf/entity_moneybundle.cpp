//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF moneybundle.
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_moneybundle.h"

//=============================================================================
//
// CTF moneybundle defines.
//

#define TF_MONEYBUNDLE_PICKUP_SOUND	"MVM.MoneyPickup"

LINK_ENTITY_TO_CLASS( item_moneybundle_large, CMoneyBundle);
LINK_ENTITY_TO_CLASS( item_moneybundle_medium, CMoneyBundleMedium);
LINK_ENTITY_TO_CLASS( item_moneybundle_small, CMoneyBundleSmall);
LINK_ENTITY_TO_CLASS( item_moneybundle_custom, CMoneyBundleCustom);

BEGIN_DATADESC( CMoneyBundle )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CMoneyBundleSmall )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CMoneyBundleMedium )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC(CMoneyBundleCustom)

DEFINE_KEYFIELD(fl_MoneyGive, FIELD_FLOAT, "money_amount"),

// Inputs.
DEFINE_KEYFIELD(m_iszModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iszModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(m_iszPickupSound, FIELD_STRING, "pickup_sound"),

END_DATADESC()

IMPLEMENT_AUTO_LIST( IMoneyBundleAutoList );

//=============================================================================
//
// CTF moneybundle functions.
//

CMoneyBundle::CMoneyBundle()
{
	m_iszModel = MAKE_STRING( "" );
	m_iszModelOLD = MAKE_STRING( "" );
	m_iszPickupSound = MAKE_STRING( "MVM.MoneyPickup" );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the moneybundle
//-----------------------------------------------------------------------------
void CMoneyBundle::Spawn( void )
{
	Precache();

	if ( m_iszModel == MAKE_STRING( "" ) )
	{
		if ( m_iszModelOLD != MAKE_STRING( "" ) )
			SetModel( STRING(m_iszModelOLD) );
		else
			SetModel( GetPowerupModel() );
	}
	else
	{
		SetModel(STRING(m_iszModel));
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the moneybundle
//-----------------------------------------------------------------------------
void CMoneyBundle::Precache( void )
{
	if ( m_iszModel == MAKE_STRING( "" ) )
	{
		if ( m_iszModelOLD != MAKE_STRING( "" ) )
			PrecacheModel( STRING(m_iszModelOLD) );
		else
			PrecacheModel( GetPowerupModel() );
	}
	else
	{
		PrecacheModel(STRING(m_iszModel));
	}

	PrecacheScriptSound( TF_MONEYBUNDLE_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch, for the money bundle
//-----------------------------------------------------------------------------
bool CMoneyBundle::MyTouch( CBasePlayer *pPlayer )
{
	bool m_bAddMoney = false;

	if (!ValidTouch(pPlayer))
		return m_bAddMoney;

	if (ITEM_AddOFMoney(pPlayer))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		AddEffects(EF_NODRAW);
		m_bAddMoney = true;
	}

	return m_bAddMoney;
}

bool CMoneyBundle::ITEM_AddOFMoney(CBasePlayer *pPlayer) //Is this even needed?
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	pTFPlayer->AddAccount(500 * PackRatios[GetPowerupSize()]);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch, for the money bundle
//-----------------------------------------------------------------------------
bool CMoneyBundleCustom::MyTouch(CBasePlayer *pPlayer)
{
	bool m_bAddMoney = false;

	if (!ValidTouch(pPlayer))
		return m_bAddMoney;

	if (ITEM_AddOFMoney(pPlayer))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		AddEffects(EF_NODRAW);
		m_bAddMoney = true;
	}

	return m_bAddMoney;
}

bool CMoneyBundleCustom::ITEM_AddOFMoney(CBasePlayer *pPlayer) //Is this even needed?
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	pTFPlayer->AddAccount(fl_MoneyGive);
	return true;
}