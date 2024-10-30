//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "of_announcer_schema.h"

#include "tf_shareddefs.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "tf_gamerules.h"

#include "of_sound_params.h"

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase* soundemitterbase;

#if 0
#ifdef CLIENT_DLL
extern ConVar of_announcer_override;

void GamemodeSupportTest()
{
	COFAnnouncerInfo *pAnnouncer = GetAnnouncers()->GetAnnouncer( of_announcer_override.GetString() );
	if( !pAnnouncer )
		return;

	char szMessage[2048] = "";

	Q_strcat( szMessage,
		VarArgs( "Announcer %s:\n", of_announcer_override.GetString() ),
		sizeof(szMessage) );

	for( int i = TF_GAMETYPE_FIRST; i < TF_GAMETYPE_LAST; i++ )
	{
		Q_strcat( szMessage,
			VarArgs( 
			"\t'%s'\t\t'%d'\n",
			g_aGameTypePrefixes[i], 
			( ( pAnnouncer->m_iSupportedGamemodes & ( 1<<i ) ) != 0 ) 
			), 
			sizeof(szMessage) );
	}

	if( TFGameRules() )
	{
		Q_strcat( szMessage,
			VarArgs( "\n\nOn Map %s:\n", engine->GetLevelName() ),
			sizeof(szMessage) );

		for( int i = TF_GAMETYPE_FIRST; i < TF_GAMETYPE_LAST; i++ )
		{
			Q_strcat(szMessage,
			VarArgs( 
				"\t'%s'\t\t'%d'\n",
				g_aGameTypePrefixes[i], 
				( ( TFGameRules()->GetGameType() & ( 1<<i ) ) != 0 ) 
				),
				sizeof(szMessage) );
		}
	}
	Msg( szMessage );
}
#endif

static ConCommand announcer_test_support
(
	"announcer_test_support",
	GamemodeSupportTest,
	"",
	FCVAR_NONE 
);
#endif

bool COFAnnouncerInfo::SupportsCurrentGamemodes()
{
	if( m_bUniversal )
		return true;

	Assert( TFGameRules() );

	return ( m_iSupportedGamemodes & TFGameRules()->GetGameType() ) == TFGameRules()->GetGameType();
}

bool COFAnnouncerInfo::SupportsGamemode( int nGametype )
{
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	return m_bUniversal || ( ( m_iSupportedGamemodes & ( 1<<nGametype ) ) != 0 );
}

void COFAnnouncerInfo::Parse( KeyValues *pKV )
{
	m_szSoundScriptFile = ReadAndAllocStringValueOrNULL( pKV, "game_sounds" );

	if( m_szSoundScriptFile )
		OFSoundManifest()->AddSoundsFromFile(m_szSoundScriptFile);

	KeyValues *pGamemodes = pKV->FindKey( "gamemodes" );
	if( !pGamemodes )
		return;

	m_bUniversal = pGamemodes->GetBool( "any", false );

	if( m_bUniversal )
		return;

	for( int i = TF_GAMETYPE_FIRST; i < TF_GAMETYPE_LAST; i++ )
	{
		if( pGamemodes->GetBool( g_aGameTypePrefixes[i] ) )
			m_iSupportedGamemodes |= ( 1 << i );
	}
}

COFAnnouncerSchema *gpAnnouncerSchema = NULL;

void InitAnnouncerSchema()
{
	gpAnnouncerSchema = new COFAnnouncerSchema();
}

COFAnnouncerSchema* GetAnnouncers()
{
	return gpAnnouncerSchema;
}

COFAnnouncerInfo *COFAnnouncerSchema::GetAnnouncer( const char *szName )
{
	unsigned short handle = m_Announcers.Find( szName );
	if( handle == m_Announcers.InvalidIndex() )
		return NULL;

	return m_Announcers.Element( handle );
}

void COFAnnouncerSchema::ParseAnnouncers()
{
	m_HandleOrder.Purge();
	m_Announcers.PurgeAndDeleteElements();

	KeyValues *pAnnouncerManifest = new KeyValues( "Announcers" );
	pAnnouncerManifest->LoadFromFile( filesystem, "scripts/announcer_support.txt" );
	
	FOR_EACH_SUBKEY( pAnnouncerManifest, pAnnouncer )
	{
		COFAnnouncerInfo *pNew = new COFAnnouncerInfo;
		unsigned short handle = m_Announcers.Insert( pAnnouncer->GetName(), pNew );
		m_HandleOrder.AddToTail( handle );

		pNew->Parse( pAnnouncer );
	}

	pAnnouncerManifest->deleteThis();

	FileFindHandle_t fh;

	CUtlVector<char *> fileNames;

	char path[512];
	Q_snprintf( path, sizeof( path ), "scripts/announcers/*.txt" );
	Q_FixSlashes( path );

	char const *fn = g_pFullFileSystem->FindFirstEx( path, "MOD", &fh );
	if( fn )
	{
		do
		{
			char ext[10];
			Q_ExtractFileExtension( fn, ext, sizeof(ext) );

			if( !Q_stricmp( ext, "txt" ) )
			{
				char temp[512];
				Q_snprintf( temp, sizeof(temp), "scripts/announcers/%s", fn );

				char *found = new char[strlen(temp) + 1];
				Q_strncpy( found, temp, strlen(temp) + 1 );

				Q_FixSlashes( found );
				fileNames.AddToTail( found );
			}

			fn = g_pFullFileSystem->FindNext( fh );

		} while ( fn );

		g_pFullFileSystem->FindClose( fh );
	}

	// did we find any?
	if( fileNames.Count() > 0 )
	{
		FOR_EACH_VEC( fileNames, index )
		{
			if( fileNames.IsValidIndex(index) && fileNames[index] )
			{
				KeyValues *kvScript = new KeyValues( "Announcer" );
				
				if( !kvScript->LoadFromFile( filesystem, fileNames[index] ) )
				{
					kvScript->deleteThis();
					continue;
				}

				Assert( kvScript->GetString( "name", NULL ) != NULL );

				COFAnnouncerInfo *pNew = new COFAnnouncerInfo;
				unsigned short handle = m_Announcers.Insert( kvScript->GetString( "name" ), pNew );
				m_HandleOrder.AddToTail( handle );

				pNew->Parse( kvScript );

				kvScript->deleteThis();
			}
		}
		fileNames.PurgeAndDeleteElements();
	}
}

void ReloadAnnouncers()
{
	GetAnnouncers()->ParseAnnouncers();
#ifdef CLIENT_DLL
	engine->ExecuteClientCmd( "schema_reload_announcers_server" );
#endif
}

static ConCommand schema_reload_announcers( 
#ifdef CLIENT_DLL
"schema_reload_announcers",
#else
"schema_reload_announcers_server",
#endif
 ReloadAnnouncers, "Reloads the custom announcers.", FCVAR_NONE );
