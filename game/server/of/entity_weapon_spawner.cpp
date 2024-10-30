//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF Weapon Spawner.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_weaponbase.h"
#include "tf_shareddefs.h"
#include "tf_weapon_builder.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "entity_weapon_spawner.h"
#include "tf_weapon_parse.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "game.h"
#include "of_map_data.h"

#include "tier0/memdbgon.h"

extern ConVar of_multiweapons;
extern ConVar of_weaponspawners;
extern ConVar of_allow_allclass_spawners;
extern ConVar of_allow_allclass_pickups;
extern ConVar of_spawners_ammo_ratio;
extern ConVar of_spawners_ammo_ratio_resupply;
extern ConVar of_pickups_ammo_ratio;
extern ConVar of_spawners_resupply;

extern ConVar weaponstay;
extern ConVar of_randomizer;

ConVar of_spawners_dynamic_player_start_count("of_spawners_dynamic_player_start_count", "8", FCVAR_REPLICATED, "Sets the player count at which we start dynamically changing the weapon spawn times.\n");
ConVar of_spawners_dynamic_player_max_count("of_spawners_dynamic_player_max_count", "16", FCVAR_REPLICATED, "Sets the player count at which the dynamic weapon spawner time reaches its maximum multiplier.\n");
ConVar of_spawners_dynamic_max_mult("of_spawners_dynamic_max_mult", "-1", FCVAR_REPLICATED, "When not negative, enables dynamically changing the weapon spawn times.\n The value determines what the spawn times are multiplied by when of_spawners_dynamic_player_max_count is reached.\n");
 
extern void UTIL_PrecacheSchemaWeapon( const char *szName );
//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CWeaponSpawner )
// Inputs.
DEFINE_KEYFIELD( m_iszWeaponModel, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszWeaponModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),
DEFINE_KEYFIELD( m_bDisableSpin, FIELD_BOOLEAN, "disable_spin" ),
DEFINE_KEYFIELD( m_bDisableShowOutline, FIELD_BOOLEAN, "disable_glow" ),
DEFINE_KEYFIELD( m_iIndex, FIELD_INTEGER, "Index" ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetWeaponModel", InputSetWeaponModel ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetWeaponName", InputSetWeaponName ),
DEFINE_THINKFUNC( AnnouncerThink ),
DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CWeaponSpawner, DT_WeaponSpawner )
	SendPropString( SENDINFO( m_szWeaponName ) ),
	SendPropBool( SENDINFO( m_bDisableSpin ) ),
	SendPropBool( SENDINFO( m_bDisableShowOutline ) ),
	SendPropBool( SENDINFO( m_bInitialDelay ) ),
	SendPropBool( SENDINFO( m_bRespawning ) ),
	SendPropBool( SENDINFO( m_bSuperWeapon ) ),
	SendPropBool( SENDINFO( m_bDropped ) ),
	SendPropTime( SENDINFO( m_flRespawnTick ) ),
	SendPropTime( SENDINFO( fl_RespawnTime ) ),
	SendPropTime( SENDINFO( fl_RespawnDelay ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( dm_weapon_spawner, CWeaponSpawner );

IMPLEMENT_AUTO_LIST( IWeaponSpawnerAutoList );

CWeaponSpawner::CWeaponSpawner()
{
	m_flRespawnTick = 0.0f;
	Q_strncpy( m_szWeaponName.GetForModify(), "tf_weapon_shotgun", 64 );
	m_iszWeaponModel = MAKE_STRING( (char*)'\0');
	m_iszPickupSound = MAKE_STRING( "Player.PickupWeapon" );

	m_bDropped = false;
	m_flNextPickupTime = 0;
	
	m_iIndex = -1;

	AddFlag( FL_OBJECT ); // So NPCs will notice it
}

bool CWeaponSpawner::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "weaponname" ) )
		Q_strncpy( m_szWeaponName.GetForModify(), szValue, 64 );
	else
		BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CWeaponSpawner::Spawn( void )
{
	m_nRenderFX = kRenderFxNone;

	if( !m_bDropped && GetMapData() )
	{
		KeyValues* pWeaponSpawner = GetMapData()->GetEntity( GetClassname(), m_iIndex.Get() );
		if ( pWeaponSpawner )
		{
			FOR_EACH_VALUE( pWeaponSpawner, pValue )
			{
				KeyValue( pValue->GetName(), pValue->GetString() );
			}
		}
	}

	// fixup tf_weapon_shotgun_<class> strings to tf_weapon_shotgun
	if ( !Q_strncmp( "tf_weapon_shotgun_", m_szWeaponName.Get(), 18 ) )
		Q_strncpy( m_szWeaponName.GetForModify(), "tf_weapon_shotgun", 64 );

	Update();

	// Use a custom model if there is one
	if( m_iszWeaponModel.ToCStr()[0] == '\0' && m_pWeaponInfo )
		m_iszWeaponModel = MAKE_STRING( m_pWeaponInfo->szWorldModel );

	Precache();
	SetModel( m_iszWeaponModel.ToCStr() );
	BaseClass::Spawn();
	// This isn't the actual spin, this just looks up a sequence that makes the weapon centered
	ResetSequence( LookupSequence( "spin" ) );

	SetSolid( SOLID_BBOX );
	// Give weapons a consistant hitbox
	if( !m_bDropped )
		UTIL_SetSize( this, -Vector(25,25,18), Vector(25,25,18) );
	else
		UTIL_SetSize( this, -Vector(35, 35, 25), Vector(35, 35, 65) );

	RegisterThinkContext( "AnnounceThink" );
	SetContextThink( &CWeaponSpawner::AnnouncerThink, gpGlobals->curtime, "AnnounceThink" );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the Weapon Spawner
//-----------------------------------------------------------------------------
void CWeaponSpawner::Precache( void )
{
	UTIL_PrecacheSchemaWeapon( m_szWeaponName.Get() );
	PrecacheScriptSound( m_iszPickupSound.ToCStr() );

	PrecacheModel( m_iszWeaponModel.ToCStr() );
}

ETFWeaponTier CWeaponSpawner::GetWeaponTier( CTFWeaponBase *pWeapon )
{
	if( pWeapon->GetTFWpnData().m_bAlwaysDrop )
		return OF_WEAPON_TIER_GODLIKE;

	// "rare" weapons, this is kinda terrible
	if( pWeapon->IsRocketWeapon()
		|| pWeapon->IsGrenadeWeapon()
		|| pWeapon->GetWeaponFileInfoHandle() == LookupWeaponInfoSlot( "tf_weapon_railgun" )
		|| pWeapon->GetWeaponFileInfoHandle() == LookupWeaponInfoSlot( "tf_weapon_lightning_gun" )
		|| pWeapon->GetWeaponFileInfoHandle() == LookupWeaponInfoSlot( "tf_weapon_gatling_gun" ) )
		return OF_WEAPON_TIER_AWESOME;

	// Normal Pills
	return OF_WEAPON_TIER_NORMAL;
}

const char *CWeaponSpawner::CheckHasAkimbo( CTFWeaponBase *pOwnedWeapon, CTFPlayer *pPlayer )
{
	if( !pOwnedWeapon )
		return NULL;

	// Don't need to check for akimbo if the weapons don't match
	if( pOwnedWeapon->GetWeaponFileInfoHandle() != m_hWpnInfo )
		return NULL;

	// Regular spawners don't give akimbo untill we have full ammo
	if( !m_bDropped && pOwnedWeapon->ReserveAmmo() < pOwnedWeapon->GetMaxReserveAmmo() )
		return NULL;

	const char *szWeaponName = NULL;
	while( pOwnedWeapon )
	{
		if( pOwnedWeapon->GetTFWpnData().m_szAltWeaponToGive[0] != '\0' )
		{
			CTFWeaponBase *pNew = (CTFWeaponBase *)pPlayer->Weapon_OwnsThisType(pOwnedWeapon->GetTFWpnData().m_szAltWeaponToGive);
			if( pNew )
				pOwnedWeapon = pNew;
			else
			{
				szWeaponName = pOwnedWeapon->GetTFWpnData().m_szAltWeaponToGive;
				break;
			}
		}
		else
			break;
	};

	return szWeaponName;
}

void CWeaponSpawner::GiveWeaponDefault( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer )
{
	WEAPON_FILE_INFO_HANDLE hHandle = m_hWpnInfo;
	int iSlot = -1;
	int iPos = -1;
	TFGameRules()->GetWeaponSlot( hHandle, iSlot, iPos, pPlayer );

	if( iSlot == -1 || iPos == -1 )
		return;

	CTFWeaponBase *pOwnedWeapon = pPlayer->GetWeaponInSlot( iSlot, iPos );

	const char *szAkimbo = CheckHasAkimbo( pOwnedWeapon, pPlayer );

	if( szAkimbo )
	{
		hHandle = LookupWeaponInfoSlot( szAkimbo );
		TFGameRules()->GetWeaponSlot( hHandle, iSlot, iPos, pPlayer );
		pOwnedWeapon = pPlayer->GetWeaponInSlot( iSlot, iPos );
	}

	if( pOwnedWeapon )
	{
		if( pOwnedWeapon->GetWeaponFileInfoHandle() == hHandle )
		{
			pRetWeapon = pOwnedWeapon;
			bRestockAmmo = true;
			return;
		}
		else
		{
			// If the slot already has something but we're not picking it up
			if ( !(pPlayer->m_nButtons & IN_USE) )
			{
				// show the swap hud
				IGameEvent *event = gameeventmanager->CreateEvent( "player_swap_weapons" );
				if ( event )
				{
					event->SetInt( "playerid", pPlayer->entindex() );
					event->SetString( "current_wep", pOwnedWeapon->GetSchemaName() );
					event->SetString( "swap_wep", szAkimbo ? szAkimbo : m_szWeaponName.Get() );
				}
				gameeventmanager->FireEvent( event );
				pPlayer->m_Shared.SetSwapWeaponSpawner( this );
				// and bail out
				return;
			}
			// Otherwise just give us the weapon ( the existing weapon is automatically dropped )
			pRetWeapon = (CTFWeaponBase *) pPlayer->GiveNamedItem( szAkimbo ? szAkimbo : m_szWeaponName.Get() );
			bRestockAmmo = false;
		}
	}
	else
	{
		// Create the specified weapon
		pRetWeapon = (CTFWeaponBase *) pPlayer->GiveNamedItem( szAkimbo ? szAkimbo : m_szWeaponName.Get() );
		bRestockAmmo = false;
	}

	if( pRetWeapon )
	{
		pRetWeapon->GiveTo( pPlayer );
		if( m_bDropped && !szAkimbo )
		{
			// Since some weapons are given attributes when spawned, and the weapon we already owned had these
			// They are already stored in m_hAttributes, so we need to purge the pre-given ones
			// as to not create duplicates - Kay
			pRetWeapon->PurgeAttributes();

			FOR_EACH_VEC( m_hDroppedWeaponInfo.m_hAttributes, i )
				pRetWeapon->AddAttribute( m_hDroppedWeaponInfo.m_hAttributes[i] );
		}
	}
}

void CWeaponSpawner::GiveWeaponMulti( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer )
{
	int iSlot = -1;
	int iPos = -1;
	TFGameRules()->GetWeaponSlot( m_hWpnInfo, iSlot, iPos, pPlayer );

	if( iSlot == -1 || iPos == -1 )
		return;

	CTFWeaponBase *pOwnedWeapon = pPlayer->GetWeaponInSlot( iSlot, iPos );

	const char *szAkimbo = CheckHasAkimbo( pOwnedWeapon, pPlayer );
	if( szAkimbo )
		pOwnedWeapon = NULL;

	if( pOwnedWeapon && FStrEq( pOwnedWeapon->GetSchemaName(), m_szWeaponName.Get() ) )
	{
		pRetWeapon = pOwnedWeapon;
		bRestockAmmo = true;
		return;
	}
	else
	{
		// Create the specified weapon
		pRetWeapon = (CTFWeaponBase *) pPlayer->GiveNamedItem( szAkimbo ? szAkimbo : m_szWeaponName.Get() );
		if( pRetWeapon )
		{
			pRetWeapon->GiveTo( pPlayer );
			if( m_bDropped && !szAkimbo )
			{
				// Since some weapons are given attributes when spawned, and the weapon we already owned had these
				// They are already stored in m_hAttributes, so we need to purge the pre-given ones
				// as to not create duplicates - Kay
				pRetWeapon->PurgeAttributes();

				FOR_EACH_VEC( m_hDroppedWeaponInfo.m_hAttributes, i )
					pRetWeapon->AddAttribute( m_hDroppedWeaponInfo.m_hAttributes[i] );
			}
		}
		bRestockAmmo = false;
		return;
	}
}

void CWeaponSpawner::GiveWeaponSlots( CTFWeaponBase *&pRetWeapon, bool &bRestockAmmo, CTFPlayer *pPlayer )
{
	int iSlot = -1;
	int iPos = -1;
	TFGameRules()->GetWeaponSlot( m_hWpnInfo, iSlot, iPos, pPlayer );

	if( iSlot == -1 || iPos == -1 )
		return;

	// Check if we have the weapon already, if we do, give ammo
	CTFWeaponBase *pSameWeapon = (CTFWeaponBase*)pPlayer->Weapon_OwnsThisType(m_szWeaponName.Get(), 0);

	const char *szAkimbo = CheckHasAkimbo( pSameWeapon, pPlayer );

	if( pSameWeapon && ! szAkimbo )
	{
		pRetWeapon = pSameWeapon;
		bRestockAmmo = true;
		return;
	}

	// Check if we have a free slot
	CTFWeaponBase *pOwnedWeapon = pPlayer->GetWeaponInSlot( iSlot, iPos );
	if( pOwnedWeapon )
	{
		// Can't replace starting melee
		if( pOwnedWeapon->GetSlot() == 0 )
			return;

		// If the slot already has something but we're not picking it up
		if ( !(pPlayer->m_nButtons & IN_USE) )
		{
			// show the swap hud
			IGameEvent *event = gameeventmanager->CreateEvent( "player_swap_weapons" );
			if ( event )
			{
				event->SetInt( "playerid", pPlayer->entindex() );
				event->SetString( "current_wep", pOwnedWeapon->GetSchemaName() );
				event->SetString( "swap_wep", szAkimbo ? szAkimbo : m_szWeaponName.Get() );
			}
			gameeventmanager->FireEvent( event );
			pPlayer->m_Shared.SetSwapWeaponSpawner( this );
			// and bail out
			return;
		}

		// Otherwise just give us the weapon ( the existing weapon is automatically dropped )
		// Create the specified weapon
		pRetWeapon = CreateWeaponNoGive( szAkimbo ? szAkimbo : m_szWeaponName.Get() );

		if( pRetWeapon )
		{
			pRetWeapon->SetLocalOrigin(GetLocalOrigin());
			pRetWeapon->SetSlotOverride(iSlot);
			pRetWeapon->SetPositionOverride(0);

			if ( !pRetWeapon->IsMarkedForDeletion() ) 
			{
				pRetWeapon->Touch( this );
			}
			pRetWeapon->DefaultTouch( this );
		}
		bRestockAmmo = false;
	}
	else
	{
		// Create the specified weapon
		pRetWeapon = CreateWeaponNoGive( szAkimbo ? szAkimbo : m_szWeaponName.Get() );

		if( pRetWeapon )
		{
			pRetWeapon->SetLocalOrigin(GetLocalOrigin());
			pRetWeapon->SetSlotOverride(iSlot);
			pRetWeapon->SetPositionOverride(0);

			if ( pRetWeapon != NULL && !(pRetWeapon->IsMarkedForDeletion()) ) 
			{
				pRetWeapon->Touch( this );
			}
			pRetWeapon->DefaultTouch( this );
		}
		bRestockAmmo = false;
	}

	if( pRetWeapon )
	{
		pRetWeapon->GiveTo( pPlayer );
		if( m_bDropped && !szAkimbo )
		{
			// Since some weapons are given attributes when spawned, and the weapon we already owned had these
			// They are already stored in m_hAttributes, so we need to purge the pre-given ones
			// as to not create duplicates - Kay
			pRetWeapon->PurgeAttributes();

			FOR_EACH_VEC( m_hDroppedWeaponInfo.m_hAttributes, i )
				pRetWeapon->AddAttribute( m_hDroppedWeaponInfo.m_hAttributes[i] );
		}
	}
}

bool CWeaponSpawner::GiveAmmo( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon  )
{
	float flAmmoMod = of_spawners_ammo_ratio_resupply.GetFloat();

	if( pWeapon->GetTFWpnData().m_flPickupMultiplier > 0.0f )
		flAmmoMod *= m_pWeaponInfo->m_flPickupMultiplier;
	
	return pPlayer->RestockAmmo(pWeapon->GetTFWpnData().m_bIgnoreAmmoRatios ? 1.0f : flAmmoMod, pWeapon) > 0;
}

void CWeaponSpawner::GiveInitialAmmo( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon  )
{
	// We don't want to have full ammo off of spawners unless its a super weaopn
	float flAmmoMod = 1.0f;
	pWeapon->m_iReserveAmmo = 0;

	if( m_bDropped )
	{
		if( m_hDroppedWeaponInfo.m_iClip > -1 )
			pWeapon->m_iClip1 = m_hDroppedWeaponInfo.m_iClip;

		// If we're just setting the reserve ammo, stop us here
		if( m_hDroppedWeaponInfo.m_iReserveAmmo > -1 )
		{
			pWeapon->m_iReserveAmmo = m_hDroppedWeaponInfo.m_iReserveAmmo;
			return;
		}

		flAmmoMod *= of_pickups_ammo_ratio.GetFloat();
	}
	else
		flAmmoMod *= pWeapon->GetTFWpnData().m_flPickupMultiplier <= 0 ? 1.0f : of_spawners_ammo_ratio.GetFloat();

	pPlayer->RestockAmmo( pWeapon->GetTFWpnData().m_bIgnoreAmmoRatios ? 1.0f : flAmmoMod, pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the Weapon Spawner
//-----------------------------------------------------------------------------
bool CWeaponSpawner::MyTouch( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return false;
	}

	if( !ValidTouch( pPlayer ) )
		return false;
	
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	if( m_flNextPickupTime > gpGlobals->curtime )
		return false;

	// code to hurt enemy players when weapon is thrown
	// Above IsZombie check so we can damage them
	if( ThrownWeaponTouch( pTFPlayer ) )
		return false;

	if ( pTFPlayer->m_Shared.IsZombie() )
		return false;

	if ((!of_allow_allclass_pickups.GetBool() && !pTFPlayer->GetPlayerClass()->IsClass(TF_CLASS_MERCENARY)) && m_bDropped)
		return false;

	if ((!of_allow_allclass_spawners.GetBool() && !pTFPlayer->GetPlayerClass()->IsClass(TF_CLASS_MERCENARY)) && !(m_bDropped))
		return false;
		
	if( !m_pWeaponInfo )
		return false;

	ETFInventoryType nInvType = TFGameRules()->GetPlayerInventorySystem( pTFPlayer );

	CTFWeaponBase *pRetWeapon = NULL;
	bool bRestockAmmo = false;

	switch( nInvType )
	{
		case OF_INVENTORY_DEFAULT:
			GiveWeaponDefault( pRetWeapon, bRestockAmmo, pTFPlayer );
			break;
		case OF_INVENTORY_MULTI:
			GiveWeaponMulti( pRetWeapon, bRestockAmmo, pTFPlayer );
			break;
		case OF_INVENTORY_SLOTS:
			GiveWeaponSlots( pRetWeapon, bRestockAmmo, pTFPlayer );
			break;
	}

	if( !pRetWeapon )
		return false;

	// We have it already, dont take it Freeman, but get ammo
	if( bRestockAmmo )
	{
		// Dropped weapons never restock ammo, we have dropped ammo packs for that
		if( m_bDropped )
			return false;

		if( !of_spawners_resupply.GetBool() )
			return false;

		if( !GiveAmmo( pTFPlayer, pRetWeapon ) )
			return false;
	}
	else
		GiveInitialAmmo( pTFPlayer, pRetWeapon );

	// did we give them anything?
	// Filter the sound to the player who picked this up
	CSingleUserRecipientFilter filter( pTFPlayer );
	// Play the sound
	EmitSound( filter, entindex(), m_iszPickupSound.ToCStr() );

	if( !bRestockAmmo )
	{
		switch( GetWeaponTier( pRetWeapon ) )
		{
			case OF_WEAPON_TIER_GODLIKE:
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_ULTRARARE );
				if ( strcmp( GetSuperWeaponPickupLine(), "None" ) || strcmp( GetSuperWeaponPickupLineSelf(), "None" ) )
					TeamplayRoundBasedRules()->BroadcastSoundFFA( pTFPlayer->entindex(), GetSuperWeaponPickupLineSelf(), GetSuperWeaponPickupLine() );
			break;

			case OF_WEAPON_TIER_AWESOME:
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_RARE );
			break;

			// common weapons
			default:
			case OF_WEAPON_TIER_NORMAL:
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_COMMON ); 
		}
		/*
		if ( pTFPlayer->IsFakeClient() )
		{
			CTFBot *actor = ToTFBot( pTFPlayer );
			if ( actor )
			{
				actor->Weapon_Switch( pGivenWeapon );
				actor->m_bPickedUpWeapon = true;
			}
		}
		*/
	}

	if( pTFPlayer && pTFPlayer->m_Shared.GetSwapWeaponSpawner() == this )
		pTFPlayer->m_Shared.SetSwapWeaponSpawner( NULL );

	// Leave the weapon spawner active if weaponstay is on
	if ( (weaponstay.GetBool() || GetRespawnTime() <= 0) && !m_pWeaponInfo->m_bAlwaysDrop && !m_bDropped )
		return false;

	// Otherwise add the distort effect
	m_nRenderFX = kRenderFxDistort;		

	// And despawn the spawner
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CWeaponSpawner::EndTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer && pPlayer->m_Shared.GetSwapWeaponSpawner() == this )
	{
		pPlayer->m_Shared.SetSwapWeaponSpawner( NULL );
	}
}

float CWeaponSpawner::GetRespawnTime()
{
	float flRespawnTime = BaseClass::GetRespawnTime();
	
	if( of_spawners_dynamic_max_mult.GetFloat() < 0 )
		return flRespawnTime;
	
	if( !TFGameRules() )
		return flRespawnTime;
	
	int iActivePlayers = 0;
	for( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_MERCENARY; teamIndex++ )
	{
		CTeam *team = GetGlobalTeam(teamIndex);
		if( team )
			iActivePlayers += team->GetNumPlayers();
	}

	if( iActivePlayers < of_spawners_dynamic_player_start_count.GetInt() )
		return flRespawnTime;
	
	int iRange = of_spawners_dynamic_player_max_count.GetInt() - of_spawners_dynamic_player_start_count.GetInt();
	int iCurPos = (iActivePlayers - of_spawners_dynamic_player_start_count.GetInt());
	float flMult = RemapVal( iCurPos, 0, iRange, 1.0f, of_spawners_dynamic_max_mult.GetFloat() );
	
	return flRespawnTime * flMult;
}

CBaseEntity* CWeaponSpawner::Respawn( void )
{
	CBaseEntity *ret = BaseClass::Respawn();
	m_nRenderFX = kRenderFxDistort;
	m_flRespawnTick = GetNextThink();
	ResetSequence( LookupSequence("spin") );
	return ret;
}

void CWeaponSpawner::Materialize( void )
{
	BaseClass::Materialize();
	
	if (!m_pWeaponInfo)
		return;
	
	if ( m_pWeaponInfo->m_bAlwaysDrop && TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_PREROUND && strcmp(GetSuperWeaponRespawnLine(), "None" ) )
		TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, GetSuperWeaponRespawnLine() );
	
	if( m_pWeaponInfo->m_bAlwaysDrop )
		SetTransmitState( FL_EDICT_ALWAYS );
}

bool CWeaponSpawner::ThrownWeaponTouch( CTFPlayer *pTFPlayer )
{
	if( !pTFPlayer )
		return false;

	if( !m_hDroppedWeaponInfo.m_bThrown )
		return false;

	CBaseEntity* ownerent = GetOwnerEntity();

	if ( !ownerent )
	{
		return false;
	}

	if ( pTFPlayer->GetTeamNumber() != GetEnemyTeam( ownerent ) )
	{
		return false;
	}

	Vector vel;
	AngularImpulse angImp;

    auto vphysObj = VPhysicsGetObject();
    if (!vphysObj)
    {
        return false;
    }
    vphysObj->GetVelocity(&vel, &angImp);

	//If the weapon still has enough speed
	if( vel.Length() > 900.f )
	{
		//Hurt the player
		CTakeDamageInfo info;
		info.SetAttacker( GetOwnerEntity() );
		info.SetInflictor( this );
		float fldamage = 40.f * ( ToTFPlayer(GetOwnerEntity())->m_Shared.InCondCrit() ? 4.f : 1.f );
		info.SetDamage( fldamage );
		Vector vForce = vel;
		VectorNormalize( vForce );
		info.SetDamageForce( vForce );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_CLUB | DMG_USEDISTANCEMOD );
		info.SetDamageCustom( TF_DMG_CUSTOM_NONE );
		//SetKillIcon(vForce);

		pTFPlayer->TakeDamage(info);

		//stop the weapon
		vel *= Vector(0.1f, 0.1f, 1.f);
		VPhysicsGetObject()->SetVelocityInstantaneous(&vel, &angImp);

		//prevent the weapon to be picked up right away after impact
		m_flNextPickupTime = gpGlobals->curtime + 1.5f;
	}

	//Stop the hurt
	m_hDroppedWeaponInfo.m_bThrown = false;

	return true;
}

void CWeaponSpawner::FlyThink( void )
{
	CBaseEntity *pList[ 16 ];

	Vector maxs( 35, 35, 65 );
	Vector mins( 35, 35, 25 );
	int count = UTIL_EntitiesInBox( pList, 16, GetAbsOrigin() - mins, GetAbsOrigin() + maxs, FL_CLIENT|FL_NPC );
	for ( int i = 0; i < count; i++ )
	{	
		CBaseEntity *pEntity = pList[ i ];

		if ( !pEntity )
			continue;

		if( pEntity == this )
			continue;
		
		Touch( pEntity );
	}

	SetNextThink( gpGlobals->curtime + 0.1f, "FlyThink" );
}

void CWeaponSpawner::SetupDroppedWeapon( DroppedWeaponInfo_t *pNewInfo )
{
	m_bDropped = true;
	Update();

	RegisterThinkContext( "FlyThink" );
	SetContextThink( &CWeaponSpawner::FlyThink, gpGlobals->curtime, "FlyThink" );

	m_hDroppedWeaponInfo.m_bThrown = pNewInfo->m_bThrown;
	m_hDroppedWeaponInfo.m_iClip = pNewInfo->m_iClip;
	m_hDroppedWeaponInfo.m_iReserveAmmo = pNewInfo->m_iReserveAmmo;

	m_hDroppedWeaponInfo.m_hAttributes.SetSize( pNewInfo->m_hAttributes.Count() );
	FOR_EACH_VEC( pNewInfo->m_hAttributes, i )
		m_hDroppedWeaponInfo.m_hAttributes[i] = pNewInfo->m_hAttributes.Element(i);

	pNewInfo->m_hAttributes.Purge();

    //DevMsg("%i %i %i\n", (int)m_bSuperWeapon, m_hDroppedWeaponInfo.m_iClip, m_hDroppedWeaponInfo.m_iReserveAmmo);

	delete pNewInfo;
}

void CWeaponSpawner::Update( void )
{
	m_hWpnInfo = LookupWeaponInfoSlot( m_szWeaponName.Get() );
	m_pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( m_hWpnInfo ) );
	if ( m_pWeaponInfo )
	{
		m_bSuperWeapon = m_pWeaponInfo->m_bAlwaysDrop;
		SetNextThink( gpGlobals->curtime, "AnnounceThink" );
	}
	else
	{
		m_bSuperWeapon = false;
	}
}

void CWeaponSpawner::AnnouncerThink()
{
	// don't bother thinking if we aren't a superweapon
	if ( m_bSuperWeapon )
	{
		if ( m_bRespawning && ( m_flRespawnTick - gpGlobals->curtime < 10.0f && !m_bWarningTriggered ) && TeamplayRoundBasedRules() )
		{
			TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, GetSuperWeaponPickupLineIncoming() );
			m_bWarningTriggered = true;
		}
		else if ( m_bRespawning && ( m_flRespawnTick - gpGlobals->curtime > 10.0f && m_bWarningTriggered ) ) // This fixes the case where you pick up the powerup as soon as it respawns
		{
			m_bWarningTriggered = false;
		}
		if ( m_bWarningTriggered && !m_bRespawning )
			m_bWarningTriggered = false;	

		SetNextThink( gpGlobals->curtime + 0.5f, "AnnounceThink" );
	}
	else
	{
		SetNextThink( -1, "AnnounceThink" );
	}
}


const char* CWeaponSpawner::GetSuperWeaponPickupLineSelf( void )
{
	int m_iWeaponID = AliasToWeaponID( m_szWeaponName.Get() );
	
	switch ( m_iWeaponID )
	{
		case TF_WEAPON_GIB:
		return "GIBTakenSelf";
		break;
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		return "QuadTakenSelf";
		break;
		case TF_WEAPON_RIPPER:
		return "RipperTakenSelf";
		break;
	}
	return "None";
}

const char* CWeaponSpawner::GetSuperWeaponPickupLine( void )
{
	int m_iWeaponID = AliasToWeaponID( m_szWeaponName.Get() );
	
	switch ( m_iWeaponID )
	{
		case TF_WEAPON_GIB:
		return "GIBTaken";
		break;
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		return "QuadTaken";
		break;
		case TF_WEAPON_RIPPER:
		return "RipperTaken";
		break;
	}
	return "None";
}

const char* CWeaponSpawner::GetSuperWeaponRespawnLine( void )
{
	int m_iWeaponID = AliasToWeaponID( m_szWeaponName.Get() );
	
	switch ( m_iWeaponID )
	{
		case TF_WEAPON_GIB:
		return "GIBSpawn";
		break;
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		return "QuadSpawn";
		break;
		case TF_WEAPON_RIPPER:
		return "RipperSpawn";
		break;
	}
	return "None";
}

const char *CWeaponSpawner::GetSuperWeaponPickupLineIncoming( void )
{ 	
	int m_iWeaponID = AliasToWeaponID( m_szWeaponName.Get() );
	
	switch ( m_iWeaponID )
	{
		case TF_WEAPON_GIB:
		return "GIBIncoming";
		break;
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		return "QuadIncoming";
		break;	
		case TF_WEAPON_RIPPER:
		return "RipperIncoming";
		break;
	}
	return "SuperWeaponsIncoming";
}

// We don't allocate weapon models since they're already properly saved somewhere else
void CWeaponSpawner::SetWeaponModel( char *szWeaponModel )
{
	m_iszWeaponModel = MAKE_STRING( szWeaponModel );
}

void CWeaponSpawner::SetWeaponName( char *szWeaponName )
{
	Q_strncpy( m_szWeaponName.GetForModify(), szWeaponName, 64 );
}

void CWeaponSpawner::SetWeaponModel( const char *szWeaponModel )
{
	m_iszWeaponModel = MAKE_STRING( szWeaponModel );
}

void CWeaponSpawner::SetWeaponName( const char *szWeaponName )
{
	Q_strncpy( m_szWeaponName.GetForModify(), szWeaponName, 64 );
}

void CWeaponSpawner::InputSetWeaponModel( inputdata_t &inputdata )
{
	const char *name = inputdata.value.String();

	if ( name ) 
	{
		m_iszWeaponModel = MAKE_STRING( name );

		CBaseEntity::PrecacheModel( name );
		SetModel( name );

		ResetSequence( LookupSequence("spin") );
	}
}

void CWeaponSpawner::InputSetWeaponName( inputdata_t &inputdata )
{
	const char *name = inputdata.value.String();

	if ( name ) 
	{
		// precache the weapon...
		UTIL_PrecacheSchemaWeapon( name );

		Q_strncpy( m_szWeaponName.GetForModify(), name, 64 );

		Update();
	}
}

//=============================================================================//
//
// Purpose: C Weapon Giver entity
//
//=============================================================================//


class CWeaponGiver : public CBaseEntity
{
public:
	DECLARE_CLASS( CWeaponGiver, CBaseEntity );
	DECLARE_DATADESC();
	
	void	GiveWeaponToActivator( inputdata_t &inputdata );
	void	RemoveWeaponFromActivator( inputdata_t &inputdata );
};

BEGIN_DATADESC(CWeaponGiver)
DEFINE_INPUTFUNC( FIELD_STRING, "GiveWeapon", GiveWeaponToActivator ),
DEFINE_INPUTFUNC( FIELD_STRING, "RemoveWeapon", RemoveWeaponFromActivator ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( of_weapon_giver, CWeaponGiver );

void CWeaponGiver::RemoveWeaponFromActivator(inputdata_t &inputdata)
{
	const char *name = inputdata.value.String();

	if( name )
	{
		CTFPlayer *pPlayer = ToTFPlayer(inputdata.pActivator);
		if( pPlayer )
		{
				CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon(0);
				for (int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++)
				{
					pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon(iWeapon);
					if (pWeapon)
					{
						if (FStrEq(pWeapon->GetSchemaName(), name))
						{
							bool SameWeapon = false;

							if (pPlayer->GetActiveTFWeapon() == pWeapon)
							{
								SameWeapon = true;
							}

							pPlayer->SetWeaponInSlot( NULL, pWeapon->GetSlot(),pWeapon->GetPosition() );

							pPlayer->Weapon_Detach(pWeapon);
							UTIL_Remove(pWeapon);

							if (SameWeapon)
							{
								pPlayer->SwitchToNextBestWeapon(NULL);
							}

						}
					}
				}
		}
	}
}

void CWeaponGiver::GiveWeaponToActivator( inputdata_t &inputdata )
{
	const char *name = inputdata.value.String();

	if ( name ) 
	{
		// precache the weapon...
		UTIL_PrecacheSchemaWeapon( name );

		WEAPON_FILE_INFO_HANDLE m_hWpnInfo = LookupWeaponInfoSlot(name);

		CTFPlayer *pPlayer = ToTFPlayer( inputdata.pActivator );
		if( pPlayer )
		{
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>(GetFileWeaponInfoFromHandle(m_hWpnInfo));
			if( !pWeaponInfo )
				return;

			if( !TFGameRules() )
				return;


			int iSlot = -1, iPos = -1;

			if( m_hWpnInfo == GetInvalidWeaponInfoHandle() )
				return;

			for( int i = 0; i < pPlayer->WeaponCount(); i++ )
			{
				CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
				if( pWeapon && pWeapon->GetWeaponFileInfoHandle() == m_hWpnInfo )
				{
					iSlot = pWeapon->GetSlot();
					iPos = pWeapon->GetPosition();
				}
			}

			TFGameRules()->GetWeaponSlot(m_hWpnInfo, iSlot, iPos, pPlayer);

			if( iSlot == -1 || iPos == -1 )
				return;

			// We have it already, dont take it Freeman, but get ammo

			CTFWeaponBase* pExistingWeapon = pPlayer->GetWeaponInSlot(iSlot, iPos);

			if( pExistingWeapon && pExistingWeapon->GetWeaponFileInfoHandle() == m_hWpnInfo)
			{
				if( pExistingWeapon->ReserveAmmo() < pWeaponInfo->iMaxReserveAmmo )
				{
					if( !pExistingWeapon->GetTFWpnData().m_bIgnoreAmmoRatios )
					{
						pExistingWeapon->m_iReserveAmmo = (pExistingWeapon->m_iReserveAmmo + (pWeaponInfo->iMaxReserveAmmo * of_spawners_ammo_ratio_resupply.GetFloat()));
					}
					else
						pExistingWeapon->m_iReserveAmmo = (pExistingWeapon->m_iReserveAmmo + pWeaponInfo->iMaxReserveAmmo);
				}
				else
					return;
			}

			if( pExistingWeapon && !pExistingWeapon->CanHolster() )
			{
				return;
			}

			CTFWeaponBase *pGivenWeapon = (CTFWeaponBase *)pPlayer->GiveNamedItem(name);  // Create the specified weapon
			if( pGivenWeapon )
			{
				pGivenWeapon->GiveTo(pPlayer); 											// Give it to the player
				pPlayer->Weapon_Switch(pGivenWeapon);
			}
		}
	}
}
