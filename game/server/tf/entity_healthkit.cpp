//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_healthkit.h"

ConVar  of_item_debug("of_item_debug", "0", FCVAR_CHEAT, "Visualize Item bounding boxes." );
ConVar	sk_item_healthkit_tiny("sk_item_healthkit_tiny", "8");

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKit.Touch"
#define OF_HEALTHKIT_TINY_PICKUP_SOUND	"HealthKitTiny.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

BEGIN_DATADESC( CHealthKit )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CHealthKitSmall )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CHealthKitMedium )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

IMPLEMENT_AUTO_LIST( IHealthKitAutoList );

//=============================================================================
//
// CTF HealthKit functions.
//

CHealthKit::CHealthKit()
{
	m_iszModel = MAKE_STRING( "" );
	m_iszModelOLD = MAKE_STRING( "" );
	m_iszPickupSound = MAKE_STRING( "HealthKit.Touch" );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
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

	AddFlag(FL_OBJECT); // So NPCs will notice it

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
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
		m_iszPickupSound = MAKE_STRING(TF_HEALTHKIT_PICKUP_SOUND);
	}
	else if (
		(!V_strcmp(STRING(m_iszPickupSound), "AmmoPack.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "TFCArmor.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "HealthKitTiny.Touch"))
		)
	{
		m_iszPickupSound = MAKE_STRING("HealthKit.Touch");
	}

	PrecacheScriptSound(STRING(m_iszPickupSound));
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;
	
	// Render debug visualization
	if ( of_item_debug.GetBool() )
		NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;

	if (!ValidTouch(pPlayer))
		return bSuccess;

	if ( pPlayer->TakeHealth( ceil(pPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()]), DMG_GENERIC ) )
	{
		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();

		UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( GetClassname() );
		MessageEnd();

		EmitSound( user, entindex(), STRING(m_iszPickupSound) );

		bSuccess = true;

		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		Assert( pTFPlayer );

		// Healthkits cures burning...
		if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING))
			pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
		//...and poison
		if (pTFPlayer->m_Shared.InCond(TF_COND_POISON))
			pTFPlayer->m_Shared.RemoveCond(TF_COND_POISON);
		//...and Tranqed
		if (pTFPlayer->m_Shared.InCond(TF_COND_TRANQ))
			pTFPlayer->m_Shared.RemoveCond(TF_COND_TRANQ);
		//...and Fucked Up Legs
		if (pTFPlayer->m_Shared.InCond(TF_COND_PIERCED_LEGS))
			pTFPlayer->m_Shared.RemoveCond(TF_COND_PIERCED_LEGS);
		AddEffects( EF_NODRAW );
	}

	return bSuccess;
}

//=============================================================================//
//
// Purpose: Quake-like health shards in the form of pills. Louis would be very happy.
//
//=============================================================================//
LINK_ENTITY_TO_CLASS(item_healthkit_tiny, CHealthKitTiny);
PRECACHE_REGISTER(item_healthkit_tiny);

BEGIN_DATADESC(CHealthKitTiny)

// Inputs.
DEFINE_KEYFIELD(m_iszModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iszModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(m_iszPickupSound, FIELD_STRING, "pickup_sound"),

END_DATADESC()

CHealthKitTiny::CHealthKitTiny()
{
	m_iszPickupSound = MAKE_STRING("HealthKitTiny.Touch");
}

void CHealthKitTiny::Precache(void)
{
	if (m_iszModel == MAKE_STRING(""))
	{
		if (m_iszModelOLD != MAKE_STRING(""))
			PrecacheModel(STRING(m_iszModelOLD));
		else
			PrecacheModel(GetPowerupModel());
	}
	else
	{
		PrecacheModel(STRING(m_iszModel));
	}

	if (m_iszPickupSound == MAKE_STRING(""))
	{
		PrecacheScriptSound(OF_HEALTHKIT_TINY_PICKUP_SOUND);
		m_iszPickupSound = MAKE_STRING("HealthKitTiny.Touch");
	}
	else if (
		(!V_strcmp(STRING(m_iszPickupSound), "AmmoPack.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "TFCArmor.Touch"))
		||
		(!V_strcmp(STRING(m_iszPickupSound), "Healthkit.Touch"))
		)
	{
		m_iszPickupSound = MAKE_STRING("HealthKitTiny.Touch");
	}
	PrecacheScriptSound(STRING(m_iszPickupSound));
}

bool CHealthKitTiny::MyTouch(CBasePlayer *pPlayer)
{
	bool m_bDoHeal = false;

	if (!ValidTouch(pPlayer))
		return m_bDoHeal;

	if (ITEM_GiveTFTinyHealth(pPlayer))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		AddEffects(EF_NODRAW);
		m_bDoHeal = true;
	}

	return m_bDoHeal;
}

bool CHealthKitTiny::ITEM_GiveTFTinyHealth(CBasePlayer *pPlayer)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	int iHealthBefore = pTFPlayer->GetHealth();
	int iHealthToAdd = clamp(sk_item_healthkit_tiny.GetInt(), 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - iHealthBefore);
	pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);

	if (iHealthBefore < pTFPlayer->m_Shared.GetDefaultHealth())
		iHealthToAdd = max(0, pTFPlayer->GetHealth() - pTFPlayer->m_Shared.GetDefaultHealth());

	pTFPlayer->m_iMegaOverheal += iHealthToAdd;

	return true;
}