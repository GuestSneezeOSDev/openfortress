#include "cbase.h"

#include "of_loadout.h"
#include "filesystem.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "cdll_int.h"
#else
#include "of_usercmd_keyvalues_handler.h"
#endif

#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
void SendLoadoutToServer()
{
	if( GetLoadout() )
	{
		KeyValues *pKV = new KeyValues( "LoadoutInfo" );
		GetLoadout()->CopySubkeys( pKV );

		// call frees pKV for us
		SendKeyValuesToServer( pKV );

		//pKV->deleteThis();
	}
}

static ConCommand send_loadout_to_server( "send_loadout_to_server", SendLoadoutToServer, "", FCVAR_SERVER_CAN_EXECUTE );

KeyValues* gLoadout;
KeyValues* GetLoadout()
{
	return gLoadout;
}

KeyValues* GetCosmeticLoadoutForClass( int iClass )
{
	if( iClass > TF_CLASS_JUGGERNAUT )
		return NULL;

	if( !GetLoadout() )
		return NULL;
	
	KeyValues *kvCosmetics = GetLoadout()->FindKey("Cosmetics");
	if( !kvCosmetics )
		return NULL;
	
	KeyValues *kvClass = kvCosmetics->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	
	return kvClass;
	
}

KeyValues* GetWeaponLoadoutForClass( int iClass )
{
	if( iClass > TF_CLASS_JUGGERNAUT )
		return NULL;

	if( !GetLoadout() )
		return NULL;
	
	KeyValues *kvWeapons = GetLoadout()->FindKey("Weapons");
	if( !kvWeapons )
		return NULL;
	
	KeyValues *kvClass = kvWeapons->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	
	return kvClass;
	
}

void ResetLoadout( const char *szCatName )
{
	if( !gLoadout )
		gLoadout = new KeyValues( "Loadout" );
	
	char szCatNameFull[64];
	Q_strncpy(szCatNameFull, szCatName, sizeof(szCatNameFull) );
	strlwr(szCatNameFull);
	
	int iCategory = 0;
	
	for( int i = 0; i < 2 ; i++ )
	{
		if( !Q_strcmp( szCatNameFull, g_aLoadoutCategories[i] ) )
		{
			iCategory = i;
			break;
		}
	}
	
	KeyValues *pCategory = new KeyValues( szCatNameFull );
	gLoadout->AddSubKey( pCategory );

	for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		KeyValues *pClass = new KeyValues( g_aPlayerClassNames_NonLocalized[i] );
		
		switch( iCategory )
		{
			case 0:
			pClass->SetString( "hat", "0" );
			break;
			case 1:
			if( i == TF_CLASS_MERCENARY )
			{
				pClass->SetString( "1", "tf_weapon_assaultrifle" );
				pClass->SetString( "2", "tf_weapon_pistol_mercenary" );
				pClass->SetString( "3", "tf_weapon_crowbar" );
			}
			break;
		}
		pCategory->AddSubKey( pClass );
	}
	gLoadout->SaveToFile( filesystem, "cfg/loadout.cfg" );
}

void ParseLoadout( void )
{	
	if ( !filesystem->FileExists( "cfg/loadout.cfg" , "MOD" ) )
	{
		ResetLoadout( "Cosmetics" );
		ResetLoadout( "Weapons" );
	}
	else
	{
		gLoadout = new KeyValues( "Loadout" );
		GetLoadout()->LoadFromFile( filesystem, "cfg/loadout.cfg" );
	}
}

#else
COFLoadoutManager *gpLoadoutManager = NULL;

void InitLoadoutManager()
{
	gpLoadoutManager = new COFLoadoutManager();
}

COFLoadoutManager *LoadoutManager()
{
	return gpLoadoutManager;
}

bool COFLoadoutManager::RegisterPlayer( CTFPlayer *pPlayer )
{
	uint64 nID = GetPlayerID( pPlayer );
	if( nID == 0 )
		return false;

	m_PlayerLoadout.Insert( nID, new COFPlayerLoadout );

	return true;
}

bool COFLoadoutManager::RemovePlayer( CTFPlayer *pPlayer )
{
	uint64 nID = GetPlayerID( pPlayer );
	if( nID == 0 )
		return false;

	m_PlayerLoadout.Remove( nID );

	return true;
}

uint64 COFLoadoutManager::GetPlayerID( CTFPlayer *pPlayer )
{
	// duh?
	// also, i don't fucking know why IsBot/IsFakeClient don't return true for the stv bot?? that's insane? whatever
	//  -sappho
	if
	(
		pPlayer->IsHLTV()
		|| pPlayer->IsReplay()
	)
	{
		return 0;
	}

	if(
		pPlayer->IsBot()
		|| pPlayer->IsFakeClient()
	)
	{
		return pPlayer->entindex();
	}


	const CSteamID *psteamID = NULL;
	psteamID = engine->GetClientSteamID( pPlayer->edict() );
	if( !psteamID )
	{
		Warning( "Couldn't get client steam ID\n" );
		return 0;
	}

	return psteamID->ConvertToUint64();
}

COFPlayerLoadout *COFLoadoutManager::GetLoadout( CTFPlayer *pPlayer )
{
	unsigned short handle = m_PlayerLoadout.Find(GetPlayerID(pPlayer));
	if( handle == m_PlayerLoadout.InvalidIndex() )
	{
		return NULL;
	}
	return m_PlayerLoadout[handle];
}
#endif