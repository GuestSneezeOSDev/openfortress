//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include "tf_gamerules.h"
#include "tf_controls.h"
#include "tf_mapinfomenu.h"
#include "filesystem.h"

#ifdef _WIN32
	#include "winerror.h"
#endif

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );
const char *GetMapType(const char *mapName);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::CTFMapInfoMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_MAPINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pTitle = new CExLabel( this, "MapInfoTitle", " " );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pContinue = new CExButton( this, "MapInfoContinue", "#TF_Continue" );
	m_pBack = new CExButton( this, "MapInfoBack", "#TF_Back" );
	m_pIntro = new CExButton( this, "MapInfoWatchIntro", "#TF_WatchIntro" );
#endif

	// info window about this map
	m_pMapInfo = new CExRichText( this, "MapInfoText" );
	m_pMapImage = new ImagePanel( this, "MapImage" );

	m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::~CTFMapInfoMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/MapInfoMenu.res");

	CheckIntroState();
	CheckBackContinueButtons();

	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

	// Save off the map name so we can re-load the page in ApplySchemeSettings().
	Q_strncpy( m_szMapName, mapname, sizeof( m_szMapName ) );
	Q_strupr( m_szMapName );

#ifdef _X360
	char *pExt = Q_stristr( m_szMapName, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	LoadMapPage( m_szMapName );
	SetMapTitle();

#ifndef _X360
	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
#endif

	if ( IsX360() )
	{
		SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( GetMapType( m_szMapName ) ) );
	}
	else
	{
		if ( TFGameRules() )
		{
			SetDialogVariable( "gamemode", TFGameRules()->GetLocalizedGameTypeName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	m_KeyRepeat.Reset();

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		CheckIntroState();
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::CheckForIntroMovie()
{
	if ( g_pFullFileSystem->FileExists( TFGameRules()->GetVideoFileForMap() ) )
		return true;

	return false;
}

const char *COM_GetModDirectory();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::HasViewedMovieForMap()
{
	return ( UTIL_GetMapKeyCount( "viewed" ) > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckIntroState()
{
	if ( CheckForIntroMovie() && HasViewedMovieForMap() )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", true );
		}
#else
		if ( m_pIntro && !m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( true );
		}
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", false );
		}
#else
		if ( m_pIntro && m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( false );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckBackContinueButtons()
{
#ifndef _X360
	if ( m_pBack && m_pContinue )
	{
		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			m_pBack->SetVisible( true );
			m_pContinue->SetText( "#TF_Continue" );
		}
		else
		{
			m_pBack->SetVisible( false );
			m_pContinue->SetText( "#TF_Close" );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnCommand( const char *command )
{
	m_KeyRepeat.Reset();

	if ( !Q_strcmp( command, "back" ) )
	{
		 // only want to go back to the Welcome menu if we're not already on a team
		if ( !IsX360() && ( GetLocalPlayerTeam() == TEAM_UNASSIGNED ) )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_INFO, true );
		}
	}
	else if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() && !HasViewedMovieForMap() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );

			UTIL_IncrementMapKey( "viewed" );
		}
		else
		{
			// On console, we may already have a team due to the lobby assigning us one.
			// We tell the server we're done with the map info menu, and it decides what to do with us.
			if ( IsX360() )
			{
				engine->ClientCmd( "closedwelcomemenu" );
			}
			else if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
			{
				m_pViewPort->ShowPanel( PANEL_TEAM, true );
			}

			UTIL_IncrementMapKey( "viewed" );
		}
	}
	else if ( !Q_strcmp( command, "intro" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );
		}
		else
		{
			m_pViewPort->ShowPanel( PANEL_TEAM, true );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::Update()
{ 
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::LoadMapPage(const char* mapName)
{
	// load the map image (if it exists for the current map)
	char szMapImage[MAX_PATH]{};
	Q_snprintf(szMapImage, sizeof(szMapImage), "VGUI/maps/menu_photos_%s", mapName);
	Q_strlower(szMapImage);

	IMaterial* pMapMaterial = materials->FindMaterial(szMapImage, TEXTURE_GROUP_VGUI, false);
	if (pMapMaterial && !IsErrorMaterial(pMapMaterial))
	{
		if (m_pMapImage)
		{
			if (!m_pMapImage->IsVisible())
			{
				m_pMapImage->SetVisible(true);
			}

			// take off the vgui/ at the beginning when we set the image
			Q_snprintf(szMapImage, sizeof(szMapImage), "maps/menu_photos_%s", mapName);
			Q_strlower(szMapImage);

			m_pMapImage->SetImage(szMapImage);
		}
	}
	else
	{
		if (m_pMapImage && m_pMapImage->IsVisible())
		{
			m_pMapImage->SetVisible(false);
		}
	}

	// load the map description files
	char mapRES[MAX_PATH]{};

	char uilanguage[64]{};
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_%s.txt", mapName, uilanguage );

	// try English if the file doesn't exist for our language
	if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
	{
		Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_english.txt", mapName );

		// if the file doesn't exist for English either, try the filename without any language extension
		if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName );
		}
	}

	// if no map specific description exists, load default text
	if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
	{
		const char *pszDefault = "maps/default.txt";

		if ( TFGameRules() )
		{
			if ( TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
			{
				pszDefault = "maps/default_ctf.txt";
			}
			else if ( TFGameRules()->InGametype( TF_GAMETYPE_CP ) )
			{
				pszDefault = "maps/default_cp.txt";
			}
			else if (TFGameRules()->InGametype(TF_GAMETYPE_ARENA))
			{
				pszDefault = "maps/default_arena.txt";
			}
		}

		if ( g_pFullFileSystem->FileExists( pszDefault ) )
		{
			Q_strncpy( mapRES, pszDefault, sizeof( mapRES ) );
		}
		else
		{
			m_pMapInfo->SetText( "" );

			// we haven't loaded a valid map image for the current map
			if ( m_pMapImage && !m_pMapImage->IsVisible() )
			{
				if ( m_pMapInfo )
				{
					m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
				}
			}

			return; 
		}
	}
	// read into a memory block
	int len = 0;
	byte* memBlock = UTIL_LoadFileForMe(mapRES, &len);

	// check the first character, make sure this a little - endian unicode file
	if ( LittleShort( ((ucs2*)memBlock)[0] ) != 0xFEFF )
	{
		// its a ascii char file
		m_pMapInfo->SetText( (char*)memBlock );
	}
	else
	{
		// convert UCS-2 LE buffer to wide string
		int bufSize = len + 2;
		wchar_t* wBuf = new wchar_t[bufSize] {};
		ucs2* pUCS2 = (ucs2*)memBlock;
		V_UCS2ToUnicode(pUCS2, wBuf, len * sizeof(wchar_t));

		// ensure little-endian unicode reads correctly on all platforms
		CByteswap byteSwap;
		byteSwap.SetTargetBigEndian(false);
		byteSwap.SwapBufferToTargetEndian(wBuf, wBuf, bufSize);

		m_pMapInfo->SetText(wBuf + 1);
		delete[] wBuf;
	}
	// go back to the top of the text buffer
	m_pMapInfo->GotoTextStart();

	UTIL_FreeFile(memBlock);

	// we haven't loaded a valid map image for the current map
	if ( m_pMapImage && !m_pMapImage->IsVisible() )
	{
		if ( m_pMapInfo )
		{
			m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::SetMapTitle()
{
	SetDialogVariable( "mapname", GetMapDisplayName( m_szMapName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "continue" );
	}
	else if ( code == KEY_XBUTTON_Y )
	{
		OnCommand( "intro" );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", 1) );
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", -1) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	BaseClass::OnThink();
}

struct s_MapInfo
{
	const char	*pDiskName;
	const char	*pDisplayName;
	const char	*pGameType;
};

static s_MapInfo s_Maps[] = {
	{	"ctf_2fort",	"2Fort",		"#Gametype_CTF",		},
	{	"cp_dustbowl",	"Dustbowl",		"#TF_AttackDefend",		},
	{	"cp_granary",	"Granary",		"#Gametype_CP",			},
	{	"cp_well",		"Well (CP)",	"#Gametype_CP",			},
	{	"cp_gravelpit", "Gravel Pit",	"#TF_AttackDefend",		},
	{	"tc_hydro",		"Hydro",		"#TF_TerritoryControl",	},
	{	"ctf_well",		"Well (CTF)",	"#Gametype_CTF",		},
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapDisplayName( const char *mapName )
{
	static char szDisplayName[256];
	char szTempName[256];
	const char *pszSrc = NULL;

	szDisplayName[0] = '\0';

	if ( !mapName )
		return szDisplayName;
/*
	// check the worldspawn entity to see if the map author has specified a name
	if ( GetClientWorldEntity() )
	{
		const char *pszMapDescription = GetClientWorldEntity()->m_iszMapDescription;
		if ( Q_strlen( pszMapDescription ) > 0 )
		{
			Q_strncpy( szDisplayName, pszMapDescription, sizeof( szDisplayName ) );
			Q_strupr( szDisplayName );
			
			return szDisplayName;
		}
	}
*/
	// check our lookup table
	Q_strncpy( szTempName, mapName, sizeof( szTempName ) );
	Q_strlower( szTempName );

	for ( int i = 0; i < ARRAYSIZE( s_Maps ); ++i )
	{
		if ( !Q_stricmp( s_Maps[i].pDiskName, szTempName ) )
		{
			return s_Maps[i].pDisplayName;
		}
	}

	// we haven't found a "friendly" map name, so let's just clean up what we have
	if ( !Q_strncmp( szTempName, "cp_", 3 ) ||
		 !Q_strncmp( szTempName, "tc_", 3 ) ||
		 !Q_strncmp( szTempName, "ad_", 3 ) ||
		 !Q_strncmp( szTempName, "gg_", 3 ) ||
		 !Q_strncmp( szTempName, "pl_", 3 ) ||
		 !Q_strncmp( szTempName, "de_", 3 ) ||
		 !Q_strncmp( szTempName, "cs_", 3 ) ||
		 !Q_strncmp( szTempName, "dm_", 3 ))
	{
		pszSrc = szTempName + 3;
	}
	else if ( !Q_strncmp( szTempName, "ctf_", 4 ) || !Q_strncmp( szTempName, "esc_", 4 ) || !Q_strncmp( szTempName, "dom_", 4 ) )
	{
		pszSrc = szTempName + 4;
	}
	else if ( !Q_strncmp( szTempName, "mctf_", 5 ) || !Q_strncmp( szTempName, "duel_", 5 ) || !Q_strncmp( szTempName, "koth_", 5 ) )
	{
		pszSrc = szTempName + 5;
	}
	else
	{
		pszSrc = szTempName;
	}

	Q_strncpy( szDisplayName, pszSrc, sizeof( szDisplayName ) );
	Q_strupr( szDisplayName );

	return szDisplayName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapType(const char *mapName)
{
	if (!IsX360())
	{
		// we haven't found a "friendly" map name, so let's just clean up what we have
		if (!Q_strnicmp(mapName, "cp_", 3))
		{
			return "#Gametype_CP";
		}
		else if (!Q_strnicmp(mapName, "tc_", 3))
		{
			return "#TF_TerritoryControl";
		}
		else if (!Q_strnicmp(mapName, "pl_", 3))
		{
			return "#Gametype_Escort";
		}
		else if (!Q_strnicmp(mapName, "plr_", 4))
		{
			return "#Gametype_EscortRace";
		}
		else if (!Q_strnicmp(mapName, "ad_", 3))
		{
			return "#TF_AttackDefend";
		}
		else if (!Q_strnicmp(mapName, "ctf_", 4))
		{
			return "#Gametype_CTF";
		}
		else if (!Q_strnicmp(mapName, "koth_", 5))
		{
			return "#Gametype_Koth";
		}
		else if (!Q_strnicmp(mapName, "arena_", 6))
		{
			return "#Gametype_Arena";
		}
		else if (!Q_strnicmp(mapName, "sd_", 3))
		{
			return "#Gametype_SD";
		}
		else if (!Q_strnicmp(mapName, "cs_", 3) || !Q_strnicmp(mapName, "de_", 3))
		{
			return "#Gametype_DE";
		}
		else if (!Q_strnicmp(mapName, "dm_", 3) || !Q_strnicmp(mapName, "duel_", 5))
		{
			return "#Gametype_DM";
		}
		if(!Q_strnicmp(mapName, "mctf_", 5))
		{
			return "#Gametype_MCTF";
		}
		else
		{
			if (TFGameRules())
			{
				return TFGameRules()->GetGameTypeName();
			}
		}
	}

	return "";
}