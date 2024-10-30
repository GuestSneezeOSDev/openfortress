#include "cbase.h"

#include "filesystem.h"
#include "of_items_game.h"
#include "weapon_parse.h"

#include "tf_shareddefs.h"

#include "tier0/memdbgon.h"

extern bool ReadWeaponDataFromKeyValuesForSlot( KeyValues *pKV, const char *szWeaponName, WEAPON_FILE_INFO_HANDLE *phandle, bool bReParse );
extern void RemoveWeaponFromDatabase( const char *szName );
extern ConVar of_weapon_testing;
#ifdef CLIENT_DLL
extern ConVar of_respawn_particle;
#endif

// Creates a new KeyValues object, first value is original settings, second is override values
KeyValues *MergeKeyValues( KeyValues *pBase, KeyValues *pAdded )
{
	KeyValues *pRet = new KeyValues( pAdded->GetName() );
	pAdded->CopySubkeys( pRet );

	pRet->RecursiveMergeKeyValues( pBase );

	return pRet;
}

void COFCosmeticInfo::Parse( KeyValues *pKV )
{
	m_szModel = ReadAndAllocStringValueOrNULL( pKV, "model" );
	m_szViewModel = ReadAndAllocStringValueOrNULL( pKV, "viewmodel" );

	m_szRegion = GetItemSchema()->AddOrReturnRegionName( pKV->GetString("region") );
#ifdef CLIENT_DLL
	m_szBackpackIcon = ReadAndAllocStringValueOrNULL( pKV, "backpack_icon" );
#endif
	m_bTeamSkins = pKV->GetBool( "team_skins", m_bTeamSkins );
	m_bUsesBrightskins = pKV->GetBool( "uses_brightskins", m_bUsesBrightskins );
	m_iSkinOffset = pKV->GetInt( "skin_offset", m_iSkinOffset );

	KeyValues *pBodygroups = pKV->FindKey("bodygroups");
	if( pBodygroups )
	{
		FOR_EACH_VALUE( pBodygroups, pBodygroup )
		{
			m_Bodygroups.Insert( pBodygroup->GetName(), pBodygroup->GetInt() );
		}
	}

	// Lastly, parse through all the styles
	KeyValues *pStyles = pKV->FindKey( "styles" );
	if( pStyles )
	{
		FOR_EACH_SUBKEY( pStyles, pStyle )
		{
			KeyValues *pInfo = MergeKeyValues( pKV, pStyle );
			// Since we're going through styles rn, that means the keyvalues 100% have a styles keyvalue
			// so uhh, to not get like infinite recursion, delete that stuff :3 - Kay
			pInfo->RemoveSubKey(pInfo->FindKey("Styles"));

			m_StyleInfo[m_StyleInfo.AddToTail()].Parse( pInfo );

			pInfo->deleteThis();
		}
	}
}

void COFSchemaWeaponInfo::Parse( KeyValues *pKV, KeyValues *pBase )
{
	KeyValues *pWeaponData = pKV->FindKey( "WeaponData" );
	if( !pWeaponData )
		pWeaponData = pBase;

	if( !pWeaponData )
	{
		DevWarning( "Unable to parse weapon %s as it lacks WeaponData!\n", pKV->GetName() );
		return;
	}

	m_szWeaponName = ReadAndAllocStringName(pKV);

	ReadWeaponDataFromKeyValuesForSlot( pWeaponData, m_szWeaponName, &m_nWeaponHandle, true );

	m_szWeaponClass = ReadAndAllocStringValueOrNULL( pKV, "weapon_class" );

	m_szBackpackIcon = ReadAndAllocStringValueOrNULL( pKV, "backpack_icon" );

	m_bShowInLoadout = pKV->GetBool( "ShowInArsenal" );
	m_iLoadoutAnim = pKV->GetInt( "loadout_anim" );

	KeyValues *pSlot = pKV->FindKey( "slot" );
	if( pSlot )
	{
		for( int i = TF_CLASS_SCOUT; i < TF_CLASS_COUNT; i++ )
		{
			m_iWeaponSlot[i] = pSlot->GetInt( g_aPlayerClassNames_NonLocalized[i], -1 );
		}
	}

	KeyValues *pAttributes = pKV->FindKey( "attributes" );
	if( pAttributes )
	{
		FOR_EACH_SUBKEY( pAttributes, pAttribute )
		{
			COFAttributeInfo *hAttribute = &m_Attributes[m_Attributes.AddToTail()];

			hAttribute->m_szName = ReadAndAllocStringName( pAttribute );
			hAttribute->m_szValue = ReadAndAllocStringValueOrNULL( pAttribute, "value" );
		}
	}
}

void COFSchemaWeaponInfo::ParseFromFile( IFileSystem* filesystem, const char *szWeaponName, const unsigned char *pICEKey )
{
	if( !ReadWeaponDataFromFileForSlot( filesystem, szWeaponName, &m_nWeaponHandle, pICEKey, true ) )
	{
		DevWarning( "Unable to parse file weapon %s.\nProbably unused weapon.\n", szWeaponName );
		return;
	}

	int len = Q_strlen(szWeaponName) + 1;
	char *szAlloc = new char [ len ];
	Q_strncpy( szAlloc, szWeaponName, len );
	m_szWeaponName = szAlloc;

#ifdef CLIENT_DLL
	m_szBackpackIcon = GetFileWeaponInfoFromHandle( m_nWeaponHandle )->iconRed->szTextureFile;
#endif
	m_bShowInLoadout = false;
	m_iLoadoutAnim = -1;

	m_bParsedThroughFile = true;
}

static COFParticleInfo *CreateParticleInfoFn()
{
	return new COFParticleInfo();
}

static COFCosmeticInfo *CreateCosmeticInfoFn()
{
	return new COFCosmeticInfo();
}

static COFSchemaWeaponInfo *CreateWeaponInfoFn()
{
	return new COFSchemaWeaponInfo();
}

void CTFItemSchema::ParseItemsGame( void )
{
	KeyValues *pItemsGame = new KeyValues("ItemsGame");
	
	pItemsGame->LoadFromFile( filesystem, "scripts/items/items_game.txt" );

	KeyValues *pParticles = pItemsGame->FindKey( "RespawnParticles" );
	if( pParticles )
	{
		int iLastRespawnEffect = 1;
		FOR_EACH_SUBKEY( pParticles, kvSubKey )
		{
			COFParticleInfo *pNew = CreateParticleInfoFn();
			pNew->Parse( kvSubKey );
			iLastRespawnEffect = atoi(kvSubKey->GetName());
			m_RespawnParticles.Insert( iLastRespawnEffect, pNew );
		}
#ifdef CLIENT_DLL
		of_respawn_particle.SetMax( iLastRespawnEffect );
#endif
	}

	KeyValues *pCosmetics = pItemsGame->FindKey("Cosmetics");
	if( pCosmetics )
	{
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			COFCosmeticInfo *pNew = CreateCosmeticInfoFn();
			pNew->Parse( kvSubKey );
			int iIndex = atoi(kvSubKey->GetName());
			m_Cosmetics.Insert( iIndex, pNew );
			m_CosmeticOrder.AddToTail( iIndex );
		}
	}

	KeyValues *pWeapons = pItemsGame->FindKey( "Weapons" );
	if( pWeapons )
	{
		KeyValues *pBuilder = NULL;

		for( int i = 0; i < TF_WEAPON_COUNT; i++ )
		{
			COFSchemaWeaponInfo *pNew = CreateWeaponInfoFn();
			m_Weapons.Insert( g_aWeaponNames[i], pNew );

			KeyValues *pData = pWeapons->FindKey( g_aWeaponNames[i] );
			if( of_weapon_testing.GetBool() )
			{
				KeyValues *pBeta = pWeapons->FindKey( UTIL_VarArgs("%s_beta", g_aWeaponNames[i])  );
				if( pBeta )
					pData = pBeta;
			}

			if( pData )
			{
				pData->SetName( g_aWeaponNames[i] );
				pNew->Parse(pData);
			}
			else
				pNew->ParseFromFile( filesystem, g_aWeaponNames[i], NULL );
			
		}

		pBuilder = pWeapons->FindKey( "tf_weapon_builder" );

		bool bFile = false;
		if( !pBuilder )
		{
			pBuilder = ReadEncryptedKVFile( filesystem, "scripts/weapons/tf_weapon_builder", NULL, false );
			bFile = true;
		}
		else
			pBuilder = pBuilder->FindKey( "WeaponData" );

		if( pBuilder )
		{
			for( int i = 0; i < OBJ_LAST; i++ )
			{
				if( GetObjectInfo(i)->m_bVisibleInWeaponSelection )
				{
					// Builder weapons need defines to work, however just the basic tf_weapon_builder is fine
					// So unless we manually specified an override, copy that
					if ( GetWeapon( GetObjectInfo(i)->m_pObjectName ) )
						continue;
		
					COFSchemaWeaponInfo *pNew = CreateWeaponInfoFn();
					m_Weapons.Insert( GetObjectInfo(i)->m_pObjectName, pNew );
		
					KeyValues *pNewBuilder = new KeyValues( GetObjectInfo(i)->m_pObjectName );		
					pNew->Parse( pNewBuilder, pBuilder );
					pNew->m_bParsedThroughFile = true;
					pNew->m_szBackpackIcon = GetObjectInfo(i)->m_pIconActive;

					pNewBuilder->deleteThis();
				}
			}
			
			if( bFile )
				pBuilder->deleteThis();
		}
		else
		{
			Warning( "Builder base weapon not found!\n" );
		}

		FOR_EACH_SUBKEY( pWeapons, kvSubKey )
		{
			// No duplicates in case they're already defined in shareddefs
			if( GetItemSchema()->GetWeaponID( kvSubKey->GetName() ) == m_Weapons.InvalidIndex() )
			{
				// Dont load beta weapons, they're only loaded through existing non beta weapons
				char szEnd[5];
				Q_StrRight( kvSubKey->GetName(), 5, szEnd, 6 );
				if( !Q_strncasecmp(szEnd, "_beta", 5) )
					continue;

				KeyValues *pData = kvSubKey;
				if( of_weapon_testing.GetBool() )
				{
					KeyValues *pBeta = pWeapons->FindKey( UTIL_VarArgs( "%s_beta", kvSubKey->GetName() )  );
					if( pBeta )
						pData = pBeta;
				}
				pData->SetName( kvSubKey->GetName() );

				COFSchemaWeaponInfo *pNew = new COFSchemaWeaponInfo();
				m_Weapons.Insert( kvSubKey->GetName(), pNew );

				pNew->Parse( pData );
			}
		}
	}

	KeyValues *pAttributes = pItemsGame->FindKey("attributes");
	if( pAttributes )
	{
		FOR_EACH_SUBKEY( pAttributes, kvSubKey )
		{
			GetItemSchema()->AddAttribute( 
				atoi(kvSubKey->GetName()),
				kvSubKey->GetString("name", "error_no_name_defined") );
		}
	}

	pItemsGame->deleteThis();
}

char *CTFItemSchema::AddOrReturnRegionName( const char *szName )
{
	char *szRegion = const_cast<char*>(szName);

	int iRegion = GetCategoryID(szName);

	if( iRegion == m_Categories.InvalidIndex() )
	{
		char *pNewStr = new char[1 + strlen( szRegion )];
		V_strcpy( pNewStr, szRegion );
		iRegion = m_Categories.AddToTail( pNewStr );
	}

	return m_Categories[iRegion];
}

void CTFItemSchema::ParseLevelItems( void )
{
	KeyValues *pItemsGame = new KeyValues("ItemsGame");

	char szMapItems[MAX_PATH];
	char szMapname[ 256 ];

	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	UTIL_VarArgs("maps/%s",STRING(gpGlobals->mapname))
#endif
	, szMapname, sizeof( szMapname ) );
	Q_snprintf( szMapItems, sizeof( szMapItems ), "%s_items_game.txt", szMapname );

	if( !filesystem->FileExists( szMapItems , "GAME" ) )
	{
		Msg( "%s not present, not parsing\n", szMapItems );
		// We set this to null so certain functions bail out faster
		pItemsGame->deleteThis();
		return;
	}

	pItemsGame->LoadFromFile( filesystem, szMapItems, "GAME" );

	KeyValues *pWeapons = pItemsGame->FindKey( "Weapons" );

	if( pWeapons )
	{
		FOR_EACH_SUBKEY( pWeapons, kvSubKey )
		{
			// Dont load beta weapons, they're only loaded through existing non beta weapons
			char szEnd[5];
			Q_StrRight( kvSubKey->GetName(), 6, szEnd, 6 );
			if( !Q_strncasecmp(szEnd, "_beta", 5) )
				continue;

			KeyValues *pData = kvSubKey;
			if( of_weapon_testing.GetBool() )
			{
				KeyValues *pBeta = pWeapons->FindKey( UTIL_VarArgs( "%s_beta", kvSubKey->GetName() ) );
				if( pBeta )
					pData = pBeta;
			}

			COFSchemaWeaponInfo *pInfo = GetItemSchema()->GetWeapon( kvSubKey->GetName() );
			if( pInfo )
				pInfo->Parse( kvSubKey );
			else
			{
				COFSchemaWeaponInfo *pNew = CreateWeaponInfoFn();
				ItemsGameHandle handle = m_Weapons.Insert( kvSubKey->GetName(), pNew );

				pNew->Parse( kvSubKey );

				m_LevelWeapons.AddToTail( handle );
			}
		}
	}

	KeyValues *pCosmetics = pItemsGame->FindKey( "Cosmetics" );
	if( pCosmetics )
	{
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			int iIndex = atoi( kvSubKey->GetName() );
			COFCosmeticInfo *pNew = GetCosmetic( iIndex );
			if( pNew )
				pNew->Parse( kvSubKey );
			else
			{
				pNew = CreateCosmeticInfoFn();
				m_Cosmetics.Insert( iIndex, pNew );

				pNew->Parse( kvSubKey );

				m_LevelCosmetics.AddToTail( iIndex );
				m_CosmeticOrder.AddToTail( iIndex );
			}
		}
	}

	pItemsGame->deleteThis();
}

void CTFItemSchema::OnMapChange()
{
	if( of_weapon_testing.GetBool() != m_bWeaponTesting || m_LevelWeapons.Count() || m_LevelCosmetics.Count() )
	{
		// Rerparse schema in case we changed up existing items
		PurgeSchema();
		ParseItemsGame();
	}
	m_bWeaponTesting = of_weapon_testing.GetBool();

	ParseLevelItems();
}

void ReloadItemsSchema()
{
	GetItemSchema()->m_bWeaponTesting = of_weapon_testing.GetBool();

	GetItemSchema()->PurgeSchema();
	GetItemSchema()->ParseItemsGame();
	GetItemSchema()->ParseLevelItems();
#ifdef CLIENT_DLL
	engine->ExecuteClientCmd( "schema_reload_items_game_server" );
#endif
}

static ConCommand schema_reload_items_game( 
#ifdef CLIENT_DLL
"schema_reload_items_game",
#else
"schema_reload_items_game_server",
#endif
 ReloadItemsSchema, "Reloads the items game.", FCVAR_NONE );

CTFItemSchema *gpItemSchema = NULL;

void InitItemSchema()
{
	gpItemSchema = new CTFItemSchema();
}

CTFItemSchema *GetItemSchema()
{
	return gpItemSchema;
}

CTFItemSchema::CTFItemSchema()
{
	SetDefLessFunc( m_Cosmetics );
	SetDefLessFunc( m_RespawnParticles );

	m_bWeaponTesting = of_weapon_testing.GetBool();
}

void CTFItemSchema::PurgeSchema()
{
	m_CosmeticOrder.Purge();

	m_Cosmetics.PurgeAndDeleteElements();
	m_LevelCosmetics.Purge();

	m_Categories.PurgeAndDeleteElements();

	m_AttributeList.Purge();

	m_RespawnParticles.PurgeAndDeleteElements();

	FOR_EACH_VEC( m_LevelWeapons, i )
	{
		COFSchemaWeaponInfo *pWpn = GetWeapon( m_LevelWeapons[i] );
		if ( pWpn )
		{
			RemoveWeaponFromDatabase( pWpn->m_szWeaponName );
		}
	}

	m_Weapons.PurgeAndDeleteElements();
	m_LevelWeapons.Purge();
}

void CTFItemSchema::PurgeLevelItems()
{
	FOR_EACH_VEC( m_LevelWeapons, i )
	{
		COFSchemaWeaponInfo *pWpn = GetWeapon( m_LevelWeapons[i] );
		if ( pWpn )
		{
			RemoveWeaponFromDatabase( pWpn->m_szWeaponName );
			m_Weapons.Remove( pWpn->m_szWeaponName );
			delete pWpn;
		}
	}

	FOR_EACH_VEC( m_LevelCosmetics, i )
	{
		m_Cosmetics.Remove( m_LevelCosmetics[i] );
		m_CosmeticOrder.FindAndRemove( m_LevelCosmetics[i] );
	}

	m_LevelWeapons.Purge();
	m_LevelCosmetics.Purge();
}

void CTFItemSchema::AddAttribute( int iID, const char *szName )
{
	m_AttributeList.Insert( szName, iID );
}

int CTFItemSchema::GetAttributeID( char *szName )
{
	int ret = m_AttributeList.Find(szName);
	return ret == m_AttributeList.InvalidIndex() ? -1 : m_AttributeList[ret];
}

int CTFItemSchema::GetAttributeID( const char *szName )
{
	char szFullName[128];
	Q_strncpy( szFullName, szName, 128 );
	return GetAttributeID( szFullName );
}

COFSchemaWeaponInfo *CTFItemSchema::GetWeapon( ItemsGameHandle iID )
{
	if( m_Weapons.IsValidIndex(iID) && iID != m_Weapons.InvalidIndex() )
		return m_Weapons[iID];

	return NULL;
}

COFSchemaWeaponInfo *CTFItemSchema::GetWeapon( const char *szWeaponName )
{
	unsigned short i = m_Weapons.Find( szWeaponName );
	if( m_Weapons.IsValidIndex(i) && m_Weapons.InvalidIndex() != i )
		return m_Weapons[i];

	return NULL;
}

ItemsGameHandle CTFItemSchema::GetWeaponID( const char *szWeaponName )
{
	return m_Weapons.Find(szWeaponName);
}

const char *CTFItemSchema::GetWeaponName( ItemsGameHandle iID )
{
	if( !m_Weapons.IsValidIndex( iID ) || m_Weapons.InvalidIndex() == iID )
		return NULL;

	return m_Weapons.GetElementName( iID );
}

COFCosmeticInfo *CTFItemSchema::GetCosmetic( int iIndex )
{
	int i = m_Cosmetics.Find(iIndex);
	if( !m_Cosmetics.IsValidIndex( i ) || m_Cosmetics.InvalidIndex() == i )
		return NULL;

	return m_Cosmetics[i];
}

COFParticleInfo *CTFItemSchema::GetRespawnParticle( int iIndex )
{
	int i = m_RespawnParticles.Find(iIndex);
	if( !m_RespawnParticles.IsValidIndex( i ) || m_RespawnParticles.InvalidIndex() == i )
		return NULL;

	return m_RespawnParticles[i];
}