#include "cbase.h"
#include "gamemounter.h"
#include "filesystem.h"
#include "steam/steam_api.h"

bool EvaluateExtraConditionals( const char* str )
{
	bool bNot = false; // should we negate this command?
	if ( *str == '!' )
		bNot = true;

	if ( Q_stristr( str, "$DEDICATED" ) )
	{
#ifdef CLIENT_DLL
		return false ^ bNot;
#else
		return engine->IsDedicatedServer() ^ bNot;
#endif
	}

	return false;
}

// brute forces our search paths, reads the users steam configs
// to determine any additional steam library directories people have
// as there's no other way to currently mount a different game (css) 
// if it's located in a different library without an absolute path
void MountPathLocal( KeyValues* pGame )
{
	const char* szGameName = pGame->GetName();
	const bool bRequired = pGame->GetBool( "required", false );

	if ( !steamapicontext || !steamapicontext->SteamApps() )
	{
		if ( bRequired )
			Error( "Failed to mount required game: %s, unable to determine app install path.\nPlease make sure Steam is running, and the game is installed properly.\n", szGameName );
		else
			Msg( "Skipping %s, unable to get app install path.\n", szGameName );

		return;
	}

	char szPath[ MAX_PATH * 2 ];
	int ccFolder = steamapicontext->SteamApps()->GetAppInstallDir( pGame->GetUint64( "appid" ), szPath, sizeof( szPath ) );

	if ( ccFolder > 0 )
	{
		ConColorMsg( Color( 90, 240, 90, 255 ), "Mounting %s (local)\n", szGameName );

		KeyValues *pPaths = pGame->FindKey( "paths" );

		if ( !pPaths )
			return;

		for ( KeyValues *pPath = pPaths->GetFirstSubKey(); pPath; pPath = pPath->GetNextKey() )
		{
			if ( !FStrEq( pPath->GetName(), "local" ) )
				continue;

			char szTempPath[ MAX_PATH * 2 ];
			Q_strncpy( szTempPath, szPath, ARRAYSIZE( szTempPath ) );

			V_AppendSlash( szTempPath, ARRAYSIZE( szTempPath ) );
			V_strncat( szTempPath, pPath->GetString(), ARRAYSIZE( szTempPath ) );

			g_pFullFileSystem->AddSearchPath( szTempPath, "GAME" );
			ConColorMsg( Color( 144, 238, 144, 255 ), "\tAdding path: %s\n", pPath->GetString() );
		}
	}
	else if ( bRequired )
	{
		Error( "Failed to mount required game: %s\n", szGameName );
	}
	else
	{
		Warning( "%s not found on system. Skipping.\n", szGameName );
	}
}

#ifdef GAME_DLL
void MountPathDedicated( KeyValues* pGame )
{
	const char* szGameName = pGame->GetName();
	//const bool bRequired = pGame->GetBool( "required", false );

	Msg( "Mounting %s (dedicated)\n", szGameName );

	KeyValues *pPaths = pGame->FindKey( "paths" );

	if ( !pPaths )
		return;

	for ( KeyValues *pPath = pPaths->GetFirstSubKey(); pPath; pPath = pPath->GetNextKey() )
	{
		if ( !FStrEq( pPath->GetName(), "dedicated" ) )
			continue;

		const char* szRelativePath = pPath->GetString();
		char gamedir[FILENAME_MAX];
		filesystem->GetCurrentDirectory( gamedir, sizeof( gamedir ) );
		V_StripLastDir( gamedir, sizeof( gamedir ) );
		V_strcat( gamedir, szGameName, sizeof( gamedir ) );
		V_AppendSlash( gamedir, sizeof( gamedir ) );
		V_strcat( gamedir, szRelativePath, sizeof( gamedir ) );
		V_FixSlashes( gamedir );

		g_pFullFileSystem->AddSearchPath( gamedir, "GAME" );
		Msg( "\tAdding path: %s\n", gamedir );
	}
}
#endif

void AddRequiredSearchPaths()
{
	SetExtraConditionalFunc( &EvaluateExtraConditionals ); //To-Do: Move this

	KeyValues *pMountFile = new KeyValues( "gamemounting.txt" );
	pMountFile->LoadFromFile( g_pFullFileSystem, "gamemounting.txt", "MOD" );

	for( KeyValues *pGame = pMountFile->GetFirstTrueSubKey(); pGame; pGame = pGame->GetNextTrueSubKey() )
	{

#ifndef OF_CLIENT_DLL
		if (engine->IsDedicatedServer())
		{
			MountPathDedicated(pGame);
		}
		// this still gets called by clients, but fails if sdk2k13 and of are on different drives.
		// kludge!
		// -sappho
		else
		{
			MountPathLocal(pGame);
		}

#else
		MountPathLocal( pGame ); // Client only mounts locally...
#endif

	}

	pMountFile->deleteThis();
}

void SetupHammerPaths( void )
{
	KeyValues *pMountFile = new KeyValues( "gamemounting.txt" );
	pMountFile->LoadFromFile( g_pFullFileSystem, "gamemounting.txt", "MOD" );
	
	KeyValues *pGameInfo = new KeyValues( "GameInfo" );
	pGameInfo->LoadFromFile( g_pFullFileSystem, "gameinfo_template.txt", "MOD" );
	KeyValues *pSearchPaths = pGameInfo->FindKey("FileSystem")->FindKey("SearchPaths");
	KeyValues *pFirst = pSearchPaths->GetFirstValue();
	KeyValues *pLast = pFirst;
	FOR_EACH_VALUE( pSearchPaths, kvValue )
	{
		if( FStrEq( kvValue->GetString(), "first" ) )
			pFirst = kvValue;
		else if( FStrEq( kvValue->GetString(), "last" ) )
			pLast = kvValue;
	}

	for( KeyValues *pGame = pMountFile->GetFirstTrueSubKey(); pGame; pGame = pGame->GetNextTrueSubKey() )
	{
		const char* szGameName = pGame->GetName();
		const bool bRequired = pGame->GetBool( "required", false );

		if ( !steamapicontext || !steamapicontext->SteamApps() )
		{
			Msg( "Skipping %s, unable to get app install path.\n", szGameName );
			continue;
		}

		char szPath[ MAX_PATH * 2 ];
		int ccFolder = steamapicontext->SteamApps()->GetAppInstallDir( pGame->GetUint64( "appid" ), szPath, sizeof( szPath ) );

		if ( ccFolder > 0 )
		{
			ConColorMsg( Color( 90, 240, 90, 255 ), "Adding %s paths into the gameinfo\n", szGameName );

			KeyValues *pPaths = pGame->FindKey( "paths" );

			if ( !pPaths )
				return;

			for ( KeyValues *pPath = pPaths->GetFirstSubKey(); pPath; pPath = pPath->GetNextKey() )
			{
				if ( !FStrEq( pPath->GetName(), "local" ) )
					continue;

				char szTempPath[ MAX_PATH * 2 ];
				Q_strncpy( szTempPath, szPath, ARRAYSIZE( szTempPath ) );

				V_AppendSlash( szTempPath, ARRAYSIZE( szTempPath ) );
				V_strncat( szTempPath, pPath->GetString(), ARRAYSIZE( szTempPath ) );

//				if( pFirst )
//					pFirst->SetStringValue( szTempPath );

				if( !pFirst )
					continue;

				pFirst->SetStringValue( szTempPath );
				
				ConColorMsg( Color( 144, 238, 144, 255 ), "\tAdding path: %s\n", szTempPath );
				
				if( pFirst == pLast )
					break;
				
				pFirst = pFirst->GetNextValue();
			}
		}
		else if ( bRequired )
		{
			Error( "Failed to mount required game: %s\n", szGameName );
		}
		else
		{
			Warning( "%s not found on system. Skipping.\n", szGameName );
		}
	}
	
	pGameInfo->SaveToFile( filesystem, "gameinfo.txt" );

	pMountFile->deleteThis();
}

static ConCommand setup_hammer_paths( "setup_hammer_paths", SetupHammerPaths, "Setsup your gameinfo to be usable in hammer.", FCVAR_NONE );