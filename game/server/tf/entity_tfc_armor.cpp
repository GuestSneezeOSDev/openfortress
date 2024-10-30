//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF TFCArmor.
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_tfc_armor.h"


//=============================================================================
//
// CTF TFCArmor defines.
//

#define TF_TFC_ARMOR_PICKUP_SOUND	"TFCArmor.Touch"

LINK_ENTITY_TO_CLASS( item_tfc_armor_full, CTFCArmor );
LINK_ENTITY_TO_CLASS( item_tfc_armor_medium, CTFCArmorMedium );
LINK_ENTITY_TO_CLASS( item_tfc_armor_small, CTFCArmorSmall );
LINK_ENTITY_TO_CLASS( item_tfc_armor_custom, CTFCArmorCustom);

BEGIN_DATADESC( CTFCArmor )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CTFCArmorSmall )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CTFCArmorMedium )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC(CTFCArmorCustom)

DEFINE_KEYFIELD( i_ArmorGive, FIELD_INTEGER, "armor_amount"),

// Inputs.
DEFINE_KEYFIELD(m_iszModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iszModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(m_iszPickupSound, FIELD_STRING, "pickup_sound"),

END_DATADESC()

IMPLEMENT_AUTO_LIST( ITFCArmorAutoList );

//=============================================================================
//
// CTF TFCArmor functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the tfc_armor
//-----------------------------------------------------------------------------
void CTFCArmor::Spawn( void )
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
// Purpose: Precache function for the tfc_armor
//-----------------------------------------------------------------------------
void CTFCArmor::Precache( void )
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

	if (m_iszPickupSound == MAKE_STRING(""))
	{
		m_iszPickupSound = MAKE_STRING(TF_TFC_ARMOR_PICKUP_SOUND);
	}
	else if (
		(!V_strcmp(STRING(m_iszPickupSound), "AmmoPack.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "Healthkit.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "HealthKitTiny.Touch"))
		)
	{
		m_iszPickupSound = MAKE_STRING("TFCArmor.Touch");
	}

	PrecacheScriptSound(STRING(m_iszPickupSound));
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the tfc_armor
//-----------------------------------------------------------------------------
bool CTFCArmor::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if (!ValidTouch(pPlayer))
		return bSuccess;

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);

	if (!pTFPlayer)
		return bSuccess;

	if (pTFPlayer->TakeArmorTFC(ceil(pTFPlayer->GetPlayerClass()->GetMaxArmor() * PackRatios[GetPowerupSize()])))
	{
		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();

		UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( GetClassname() );
		MessageEnd();

		EmitSound( user, entindex(), STRING(m_iszPickupSound) );

		bSuccess = true;

		Assert( pTFPlayer );

		AddEffects(EF_NODRAW);

		//I ate your doorframe.
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the tfc_armor
//-----------------------------------------------------------------------------
bool CTFCArmorCustom::MyTouch(CBasePlayer* pPlayer)
{
	bool bSuccess = false;

	if (!ValidTouch(pPlayer))
		return bSuccess;

	CTFPlayer* pTFPlayer = ToTFPlayer(pPlayer);

	if (!pTFPlayer)
		return bSuccess;

	if (pTFPlayer->TakeArmorTFC(i_ArmorGive))
	{
		CSingleUserRecipientFilter user(pPlayer);
		user.MakeReliable();

		UserMessageBegin(user, "ItemPickup");
		WRITE_STRING(GetClassname());
		MessageEnd();

		EmitSound(user, entindex(), STRING(m_iszPickupSound) );

		bSuccess = true;

		Assert(pTFPlayer);

		AddEffects(EF_NODRAW);

		//I ate your doorframe.
	}

	return bSuccess;
}