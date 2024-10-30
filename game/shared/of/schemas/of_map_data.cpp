//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "filesystem.h"
#include "of_map_data.h"
#include "tier0/memdbgon.h"

COFMapDataManager *gpMapData = NULL;

void InitMapDataManager()
{
	gpMapData = new COFMapDataManager();
}

COFMapDataManager *GetMapData()
{
	return gpMapData;
}

void COFMapDataManager::InitMapData()
{
	if( m_MapData )
	{
		m_MapData->deleteThis();
	}
	m_MapData = new KeyValues( "MapData" );
}

void COFMapDataManager::ParseMapDataSchema( void )
{	
	InitMapData();
	
	char szMapData[MAX_PATH];
	char szMapName[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	UTIL_VarArgs( "maps/%s",STRING( gpGlobals->mapname ) )
#endif
	, szMapName, sizeof( szMapName ) );
	Q_snprintf( szMapData, sizeof( szMapData ), "%s_mapdata.txt", szMapName );

	if( !filesystem->FileExists( szMapData , "GAME" ) )	
	{
		Msg( "%s not present, not parsing¸\n", szMapData );
		return;
	}
	
	m_MapData->LoadFromFile( filesystem, szMapData );
	
	FOR_EACH_SUBKEY( m_MapData, kvSubKey )
	{
		DevMsg( "%s\n", kvSubKey->GetName() );
	}
	
	ConColorMsg( Color( 144, 238, 144, 255 ), "Sucessfully parsed %s.\n", szMapData );
}

KeyValues *COFMapDataManager::GetEntity( const char *szClassname, int iIndex )
{
	KeyValues *pClass = m_MapData->FindKey( szClassname );
	if( !pClass )
		return NULL;

	return pClass->FindKey( UTIL_VarArgs( "%d", iIndex ) );
}

KeyValues *COFMapDataManager::GetBotTemlpate( const char *szBotName )
{
	KeyValues *pTemplates = m_MapData->FindKey( "BotTemplates" );
	if( !pTemplates )
	{
		Warning("Failed to find BotTemplates, aborting\n");
		return NULL;
	}

	return pTemplates->FindKey( szBotName );
}

KeyValues *COFMapDataManager::GetWavePreset( const char *szWaveName )
{
	KeyValues *pWaves = m_MapData->FindKey( "WavePresets" );
	if( !pWaves )
	{
		Warning("Attempted to load preset without mapdata entries\n");
		return NULL;
	}
	return pWaves->FindKey( szWaveName );
}