//========= Open Fortress Micro License 2020, Open Fortress Team , All rights reserved. ============
//
// Purpose: Now Playing Hud element, displays which round music is playing on round start
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "of_hud_nowplaying.h"
#include "c_of_music_player.h"
#include "of_sound_params.h"

// Hud stuff
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudNowPlaying );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudNowPlaying::CTFHudNowPlaying( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudNowPlaying" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );
	
	m_pNameContainer = new EditablePanel( this, "MusicNameContainer" );
	m_pNameBG = new CTFImagePanel( m_pNameContainer, "MusicNameBG" );
	m_pNameLabel = new CExLabel( m_pNameContainer, "MusicNameLabel", "" );
	
	m_pArtistContainer = new EditablePanel( this, "ArtistNameContainer" );
	m_pArtistBG = new CTFImagePanel( m_pArtistContainer, "ArtistNameBG" );
	m_pArtistLabel = new CExLabel( m_pArtistContainer, "ArtistNameLabel", "" );

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudNowPlaying::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudNowPlaying.res" );
}

void CTFHudNowPlaying::OnThink()
{
}

bool CTFHudNowPlaying::ShouldDraw()
{
	if( !IsVisible() )
		return false;

	return CHudElement::ShouldDraw();
}

void CTFHudNowPlaying::OnHudAnimationEnd( KeyValues *pKV )
{
	const char *szName = pKV->GetString("name");
	if( FStrEq( szName, "NowPlayingFadeOut" ) )
		SetVisible( false );
}

static const ConVar *snd_musicvolume = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudNowPlaying::AnnounceSong( char *szName )
{
	snd_musicvolume = g_pCVar->FindVar("snd_musicvolume");
	
	if( snd_musicvolume->GetFloat() <= 0.0f )
		return;

	if( !DMMusicManager() )
		return;

	COFSoundParameters *pSoundscript = OFSoundManifest()->GetSoundscript( DMMusicManager()->szRoundMusic );
	if( !pSoundscript )
		return;

	const char *szSongName = pSoundscript->GetMusicName();

	bool bLongName = false;
	
	if( !szSongName )
	{
		szSongName = DMMusicManager()->szRoundMusic;
		bLongName = true;
	}

	static char temp[128];
	V_strncpy( temp, szSongName, sizeof( temp ) );	
	
	if( bLongName )
	{
		bool bFound = false;
		for( const char *szCurr = &szSongName[0]; *szCurr != '\0' && szCurr; szCurr++ )
		{
			if( *szCurr == '.' )
			{
				if( !Q_strncmp( szSongName, "DeathmatchMusic", (int)(szCurr - szSongName) ) )
				{
					szSongName = szCurr + 1;
				}
			}
		}

		if( !bFound || Q_strlen( szSongName ) > 19 )
			return;
	}

	m_pNameContainer->SetDialogVariable( "SongName", szSongName );

	int textLen = 0;
	
	wchar_t szUnicode[512];

	wchar_t wszSongName[128];
	g_pVGuiLocalize->ConvertANSIToUnicode( szSongName, wszSongName, sizeof( wszSongName ) );
	
	g_pVGuiLocalize->ConstructString( szUnicode, sizeof( szUnicode ), g_pVGuiLocalize->Find( "#OF_SongName" ), 1, wszSongName );

	int len = wcslen( szUnicode );
	for ( int i=0;i<len;i++ )
		textLen += surface()->GetCharacterWidth( m_pNameLabel->GetFont(), szUnicode[i] );

	int w,h;
	m_pNameLabel->GetSize( w,h );
	m_pNameLabel->SetSize( textLen, h );
	
	m_pNameLabel->SetText(szUnicode);

	textLen += XRES(24); // Padding

	m_pNameBG->GetSize( w,h );
	m_pNameBG->SetSize( textLen, h );

	m_pNameContainer->GetSize( w,h );
	m_pNameContainer->SetSize( textLen, h );
	
	const char *szArtistName = pSoundscript->GetArtist();

	if( szArtistName[0] == '\0' )
	{
		m_pArtistContainer->SetVisible( false );
	}
	else
	{
		m_pArtistContainer->SetVisible( true );
		m_pArtistContainer->SetDialogVariable( "ArtistName", szArtistName );
		
		textLen = 0;
		
		wchar_t wszArtistName[128];
		g_pVGuiLocalize->ConvertANSIToUnicode( szArtistName, wszArtistName, sizeof( wszArtistName ) );
		g_pVGuiLocalize->ConstructString( szUnicode, sizeof( szUnicode ), g_pVGuiLocalize->Find( "#OF_ArtistName" ), 1, wszArtistName );

		len = wcslen( szUnicode );
		for ( int i=0;i<len;i++ )
			textLen += surface()->GetCharacterWidth( m_pArtistLabel->GetFont(), szUnicode[i] );
		
		textLen += XRES(14); // Padding

		m_pArtistLabel->GetSize( w,h );
		m_pArtistLabel->SetText(szUnicode);
		m_pArtistLabel->SetSize( textLen, h );

		m_pArtistBG->GetSize( w,h );
		m_pArtistBG->SetSize( textLen, h );

		m_pArtistContainer->GetSize( w,h );
		m_pArtistContainer->SetSize( textLen, h );
	}
	
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "NowPlaying" );
	
	SetVisible( true );
}