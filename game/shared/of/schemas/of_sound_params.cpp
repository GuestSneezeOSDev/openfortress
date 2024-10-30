//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "of_sound_params.h"

#include "KeyValues.h"
#include "filesystem.h"
#include "interval.h"

#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/IEngineSound.h"

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

const char *gszAttenuation[] =
{
	"ATTN_NORM",
	"ATTN_NONE",
	"ATTN_NORM",
	"ATTN_IDLE",
	"ATTN_STATIC",
	"ATTN_RICOCHET",
	"ATTN_GUNFIRE"
};

float gflAttenuation[] =
{
	ATTN_NORM,
	ATTN_NONE,
	ATTN_NORM,
	ATTN_IDLE,
	ATTN_STATIC,
	ATTN_RICOCHET,
	ATTN_GUNFIRE
};

float TranslateAttenuation( const char *key )
{
	if ( !key )
	{
		Assert( 0 );
		return ATTN_NORM;
	}

	char szAtt[64];
	Q_strncpy( szAtt, key, sizeof(szAtt) );
	Q_strnlwr( szAtt, sizeof(szAtt) );
	int i = UTIL_StringFieldToInt( szAtt, gszAttenuation, sizeof(gszAttenuation) );

	if( i == -1 )
		return ATTN_NORM;

	return gflAttenuation[i];
}

const char *COFSoundParameters::GetWav( int i )
{
	return  i < NumSoundNames() ? soundemitterbase->GetWaveName(GetSoundNames()[i].symbol) : NULL;
}

const char *COFSoundParameters::GetRandomWav()
{ 
	if( NumSoundNames() < 0 )
		return NULL;

	int iRand = RandomInt(0, NumSoundNames() - 1);
		
	return soundemitterbase->GetWaveName(GetSoundNames()[iRand].symbol);
};

CTFSoundManifest *gpSoundManifest;

void InitSoundManifest()
{
	gpSoundManifest = new CTFSoundManifest();
}

CTFSoundManifest *OFSoundManifest()
{
	return gpSoundManifest;
};

void CTFSoundManifest::ParseFile( KeyValues *pKV, bool bLevelSounds )
{
	if( bLevelSounds )
		m_LevelSoundscripts.PurgeAndDeleteElements();

	for( KeyValues *pSound = pKV; pSound; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
	{
		if( bLevelSounds ? m_LevelSoundscripts.HasElement( pSound->GetName() ) : m_Soundscripts.HasElement( pSound->GetName() ) )
			continue;

		COFSoundParameters *pSoundScript = CreateSoundEntry();
		if( bLevelSounds )
			m_LevelSoundscripts.Insert( pSound->GetName(), pSoundScript );
		else
			m_Soundscripts.Insert( pSound->GetName(), pSoundScript );

		ParseSoundEntry( pSound, pSoundScript );
	}
}

void CTFSoundManifest::ParseSoundEntry( KeyValues *pKV, COFSoundParameters *pSound )
{
	KeyValues *pKey = pKV->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "channel" ) )
		{
			pSound->ChannelFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "volume" ) )
		{
			pSound->VolumeFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "pitch" ) )
		{
			pSound->PitchFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "wave" ) )
		{
			soundemitterbase->ExpandSoundNameMacros( *pSound, pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "rndwave" ) )
		{
			KeyValues *pWaves = pKey->GetFirstSubKey();
			while ( pWaves )
			{
				soundemitterbase->ExpandSoundNameMacros( *pSound, pWaves->GetString() );

				pWaves = pWaves->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "attenuation" ) || !Q_strcasecmp( pKey->GetName(), "CompatibilityAttenuation" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "SNDLVL_", strlen( "SNDLVL_" ) ) )
			{
				DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound %s has \"attenuation\" with %s value!\n",
					pKV->GetName(), pKey->GetString());
			}

			if ( !Q_strncasecmp( pKey->GetString(), "ATTN_", strlen( "ATTN_" ) ) )
			{
				pSound->SetSoundLevel( ATTN_TO_SNDLVL( TranslateAttenuation( pKey->GetString() ) ) );
			}
			else
			{
				interval_t interval;
				interval = ReadInterval( pKey->GetString() );

				// Translate from attenuation to soundlevel
				float start = interval.start;
				float end	= interval.start + interval.range;

				pSound->SetSoundLevel( ATTN_TO_SNDLVL( start ), ATTN_TO_SNDLVL( end ) - ATTN_TO_SNDLVL( start ) );
			}

			// Goldsrc compatibility mode.. feed the sndlevel value through the sound engine interface in such a way
			// that it can reconstruct the original sndlevel value and flag the sound as using Goldsrc attenuation.
			bool bCompatibilityAttenuation = !Q_strcasecmp( pKey->GetName(), "CompatibilityAttenuation" );
			if ( bCompatibilityAttenuation )
			{
				if ( pSound->GetSoundLevel().range != 0 )
				{
					Warning( "CompatibilityAttenuation for sound %s must have same start and end values.\n", pKV->GetName() );
				}

				pSound->SetSoundLevel( SNDLEVEL_TO_COMPATIBILITY_MODE( pSound->GetSoundLevel().start ) );
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "soundlevel" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "ATTN_", strlen( "ATTN_" ) ) )
			{
				DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound %s has \"soundlevel\" with %s value!\n",
					pKV->GetName(), pKey->GetString() );
			}

			pSound->SoundLevelFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "play_to_owner_only" ) )
		{
			pSound->SetOnlyPlayToOwner( pKey->GetInt() ? true : false );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "delay_msec" ) )
		{
			// Don't allow negative delay
			pSound->SetDelayMsec( max( 0, pKey->GetInt() ) );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "intro" ) )
		{
			pSound->SetIntro( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "outro" ) )
		{
			pSound->SetOutro( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "win" ) )
		{
			pSound->SetWinMusic( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "name" ) )
		{
			pSound->SetMusicName( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "artist" ) )
		{
			pSound->SetArtist( pKey->GetString() );
		}

		pKey = pKey->GetNextKey();
	}
}

COFSoundParameters *CTFSoundManifest::GetSoundscript( const char *szName )
{
	// First check if a level Soundscript overrides it
	unsigned short i = m_LevelSoundscripts.Find( szName );
	if( i != m_LevelSoundscripts.InvalidIndex() )
		return m_LevelSoundscripts[i];

	i = m_Soundscripts.Find( szName );
	if( i == m_Soundscripts.InvalidIndex() )
		return NULL;

	return m_Soundscripts[i];
}

void CTFSoundManifest::AddSoundsFromFile( const char* szFile )
{
	// Check if the file is parsed already
	FOR_EACH_VEC( m_AddedSoundScripts, i )
	{
		if( !Q_strcasecmp(m_AddedSoundScripts[i], szFile) )
		{
			// Stop if it is
			return;
		}
	}

	KeyValues *pKv = new KeyValues( "SoundScript" );
	pKv->LoadFromFile( filesystem, szFile, "GAME" );

	if( !pKv )
		return;

	ParseFile(pKv);

	pKv->deleteThis();

	char *pNewStr = new char[1 + strlen( szFile )];
	V_strcpy( pNewStr, szFile );
	m_AddedSoundScripts.AddToTail( pNewStr );

	soundemitterbase->AddSoundOverrides( pNewStr, true );
}

void CTFSoundManifest::ReParsePermanentSoundFiles()
{
	FOR_EACH_VEC( m_AddedSoundScripts, i )
	{
		KeyValues *pKv = new KeyValues( "SoundScript" );
		pKv->LoadFromFile( filesystem, m_AddedSoundScripts[i], "GAME");

		if( !pKv )
			continue;

		ParseFile(pKv);

		pKv->deleteThis();
	}
}

void CTFSoundManifest::ReAddPermanentSoundFiles()
{
	FOR_EACH_VEC( m_AddedSoundScripts, i )
	{
		soundemitterbase->AddSoundOverrides( m_AddedSoundScripts[i], true );
	}
}

void ParseSoundManifest( void )
{
	OFSoundManifest()->Clear();

	KeyValues *pManifestFile = new KeyValues( "game_sounds_manifest" );
	pManifestFile->LoadFromFile( filesystem, "scripts/game_sounds_manifest.txt" );
	
	if ( pManifestFile )
	{
		KeyValues *pManifest = new KeyValues( "Manifest" );
		pManifest = pManifestFile->GetFirstValue();
		for( pManifest; pManifest != NULL; pManifest = pManifest->GetNextValue() ) // Loop through all the keyvalues
		{
			KeyValues *pSoundFile = new KeyValues( "SoundFile" );
			pSoundFile->LoadFromFile( filesystem, pManifest->GetString() );

			if( pSoundFile )
				OFSoundManifest()->ParseFile( pSoundFile );
		}

		OFSoundManifest()->ReParsePermanentSoundFiles();
	}

	pManifestFile->deleteThis();
}

void ParseLevelSoundManifest( void )
{	
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	UTIL_VarArgs("maps/%s",STRING(gpGlobals->mapname))
#endif
	, mapname, sizeof( mapname ) );
	Q_snprintf( mapsounds, sizeof( mapsounds ), "%s_level_sounds.txt", mapname );
	if( !filesystem->FileExists( mapsounds , "GAME" ) )
	{
		DevMsg( "%s not present, not parsing\n", mapsounds );
		return;
	}
	DevMsg("%s\n", mapsounds);
	KeyValues *pSoundFile = new KeyValues( "level_sounds" );
	pSoundFile->LoadFromFile( filesystem, mapsounds, "GAME" );

	if( pSoundFile )
		OFSoundManifest()->ParseFile( pSoundFile );

	pSoundFile->deleteThis();
}

#ifdef CLIENT_DLL
void PrecacheUISoundScript( char *szSoundScript )
{
	COFSoundParameters *pSoundScript = OFSoundManifest()->GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return;

	for( int i = 0; i < pSoundScript->NumSoundNames(); i++ )
	{
		if( pSoundScript->GetWav(i) )
			enginesound->PrecacheSound( pSoundScript->GetWav(i), true, false );
	}
}
#endif

#ifdef CLIENT_DLL
CON_COMMAND( announcer_test, "Checks every announcer soundscript if it exists for the specified announcer" )
{
	const char *szName = args[1];
	if( !szName )
		return;

	KeyValues *pKv = new KeyValues("Scripts");
	pKv->LoadFromFile( filesystem, "scripts/announcer_test.txt" );
	
	if( !pKv )
		return;

	FOR_EACH_VALUE( pKv, kvValue )
	{
		const char *szScript = VarArgs( "%s.%s", szName, kvValue->GetName() );
		bool bExists = !!OFSoundManifest()->GetSoundscript(szScript);
		Color clr = bExists ? Color( 0, 255, 0, 255 ) : Color( 255, 0, 0, 255 );
		
		ConColorMsg( clr, "%s\t%s\n", szScript, bExists ? "+" : "-" );
	}

	pKv->deleteThis();
}
#endif