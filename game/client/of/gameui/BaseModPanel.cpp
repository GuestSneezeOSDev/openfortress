#include "cbase.h"
#include "BaseModPanel.h"
#include "ienginevgui.h"
#include "GameUI/IGameConsole.h"
#include "inputsystem/iinputsystem.h"
#include "filesystem.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui/ILocalize.h"

#ifdef _X360
	#include "xbox/xbox_launch.h"
#endif

#include "VGenericConfirmation.h"
#include "VInGameMainMenu.h"
#include "VMainMenu.h"
#include "VFooterPanel.h"

#include "gameui/dm_loadout.h"
#include "createserverpanel.h"
#include "of_compendium.h"
#include "vgui/ISurface.h"
#include "vgui_controls/AnimationController.h"
#include "tier0/icommandline.h"
#include "vgui/videobackground.h"

#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace BaseModUI;
using namespace vgui;

//setup in GameUI_Interface.cpp

extern class IMatchSystem *matchsystem;
extern const char *COM_GetModDirectory( void );
static CBaseModPanel	*g_pBasePanel = NULL;

extern ISoundEmitterSystemBase *soundemitterbase;

bool g_bIsCreatingNewGameMenuForPreFetching = false;

//=============================================================================
CBaseModPanel* CBaseModPanel::m_CFactoryBasePanel = NULL;

#ifdef POSIX
static CDllDemandLoader g_GameUIDLL("GameUI");
#endif

IGameUI* GameUI()
{
	static IGameUI *g_pGameUI = NULL;
	if (!g_pGameUI)
	{
#ifdef POSIX
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
#else
		CreateInterfaceFn gameUIFactory = Sys_GetFactory("GameUI");
#endif
		if (!gameUIFactory)
		{
			Assert(0);
		}
		g_pGameUI = (IGameUI *)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
		if (!g_pGameUI)
		{
			Assert(0);
		}
	}
	return g_pGameUI;
}

IGameConsole* GameConsole()
{
	static IGameConsole *g_pGameConsole = NULL;
	if (!g_pGameConsole)
	{
#ifdef POSIX 
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
#else
		CreateInterfaceFn gameUIFactory = Sys_GetFactory("GameUI");
#endif
		if (!gameUIFactory)
		{
			Assert(0);
		}
		g_pGameConsole = (IGameConsole *)gameUIFactory(GAMECONSOLE_INTERFACE_VERSION, NULL);
		if (!g_pGameConsole)
		{
			Assert(0);
		}
	}
	return g_pGameConsole;
}

KeyValues* gBackgroundSettings;
KeyValues* BackgroundSettings()
{
	return gBackgroundSettings;
}

void InitBackgroundSettings()
{
	if (gBackgroundSettings)
	{
		gBackgroundSettings->deleteThis();
	}
	gBackgroundSettings = new KeyValues("MenuBackgrounds");
	gBackgroundSettings->LoadFromFile(g_pFullFileSystem, "scripts/menu_backgrounds.txt");
}

#ifndef _CERT
#ifdef _X360
ConVar ui_gameui_debug( "ui_gameui_debug", "1" );
#else
ConVar ui_gameui_debug( "ui_gameui_debug", "0" );
#endif
int UI_IsDebug()
{
	return (*(int *)(&ui_gameui_debug)) ? ui_gameui_debug.GetInt() : 0;
}
#endif

#if defined( _X360 )
static void InstallStatusChanged( IConVar *pConVar, const char *pOldValue, float flOldValue )
{
	// spew out status
	if ( ((ConVar *)pConVar)->GetBool() && g_pXboxInstaller )
	{
		g_pXboxInstaller->SpewStatus();
	}
}
ConVar xbox_install_status( "xbox_install_status", "0", 0, "Show install status", InstallStatusChanged );
#endif

// Use for show demos to force the correct campaign poster
ConVar demo_campaign_name( "demo_campaign_name", "L4D2C5", FCVAR_CHEAT, "Short name of campaign (i.e. L4D2C5), used to show correct poster in demo mode." );

ConVar ui_lobby_noresults_create_msg_time( "ui_lobby_noresults_create_msg_time", "2.5", FCVAR_CHEAT );

ConVar ui_scaling	( "ui_scaling", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Scales VGUI elements with different screen resolutions." );

CBaseModPanel::CBaseModPanel() : BaseClass(0, "CBaseModPanel"),
m_bClosingAllWindows(false),
m_lastActiveUserId(0)
{
	g_pBasePanel = this;
	GameUI()->SetMainMenuOverride(GetVPanel());

	MakePopup(false);

	Assert(m_CFactoryBasePanel == 0);
	m_CFactoryBasePanel = this;

	g_pVGuiLocalize->AddFile("Resource/ep2_%language%.txt");

	for (int k = 0; k < WPRI_COUNT; ++k)
	{
		m_ActiveWindow[k] = WT_NONE;
	}

	// delay 3 frames before doing activation on initialization
	// needed to allow engine to exec startup commands (background map signal is 1 frame behind) 
	m_DelayActivation = 3;

	m_UIScheme = vgui::scheme()->LoadSchemeFromFileEx(0, "resource/ClientScheme.res", "ClientScheme");

	SetScheme(m_UIScheme);

	// Only one user on the PC, so set it now
	SetLastActiveUserId(IsPC() ? 0 : -1);

	ListenForGameEvent( "gameui_activated" );
	ListenForGameEvent( "gameui_hidden" );

	// Precache critical font characters for the 360, dampens severity of these runtime i/o hitches
	IScheme *pScheme = vgui::scheme()->GetIScheme(m_UIScheme);
	m_hDefaultFont = pScheme->GetFont("Default", true);
	vgui::surface()->PrecacheFontCharacters(m_hDefaultFont, NULL);
	vgui::surface()->PrecacheFontCharacters(pScheme->GetFont("DefaultBold", true), NULL);
	vgui::surface()->PrecacheFontCharacters(pScheme->GetFont("DefaultLarge", true), NULL);
	vgui::surface()->PrecacheFontCharacters(pScheme->GetFont("FrameTitle", true), NULL);

#ifdef _X360
	x360_audio_english.SetValue(XboxLaunch()->GetForceEnglish());
#endif

	m_FooterPanel = new CBaseModFooterPanel(this, "FooterPanel");
	// m_hOptionsDialog = NULL;

	m_bWarmRestartMode = false;
	m_ExitingFrameCount = 0;

	m_flBlurScale = 0;
	m_flLastBlurTime = 0;

	m_iBackgroundImageID = -1;
	m_iProductImageID = -1;
	
	// Delay playing the startup music until two frames
	// this allows cbuf commands that occur on the first frame that may start a map
	m_iPlayGameStartupSound = 2;

	m_nProductImageWide = 0;
	m_nProductImageTall = 0;
	m_flMovieFadeInTime = 0.0f;
	m_pBackgroundMaterial = NULL;
	m_pBackgroundTexture = NULL;

	//Make it pausable When we reload, the game stops pausing on +esc
	ConVar *sv_pausable = cvar->FindVar("sv_pausable");
	sv_pausable->SetValue(1);

	m_pVideo = new CVideoBackground( NULL, "VideoBackground" );
	m_pVideo->SetParent( this );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//=============================================================================
CBaseModPanel::~CBaseModPanel()
{
	ReleaseStartupGraphic();

	if ( m_FooterPanel )
	{
//		delete m_FooterPanel;
		m_FooterPanel->MarkForDeletion();
		m_FooterPanel = NULL;
	}

	Assert(m_CFactoryBasePanel == this);
	m_CFactoryBasePanel = 0;

	surface()->DestroyTextureID( m_iBackgroundImageID );
	surface()->DestroyTextureID( m_iProductImageID );

	// Shutdown UI game data
	CUIGameData::Shutdown();
}

vgui::Panel & BaseModUI::CBaseModPanel::GetVguiPanel()
{
	return *(static_cast<vgui::Panel *>(this));
}

//=============================================================================
CBaseModPanel& CBaseModPanel::GetSingleton()
{
	Assert(m_CFactoryBasePanel != 0);
	return *m_CFactoryBasePanel;
}

//=============================================================================
CBaseModPanel* CBaseModPanel::GetSingletonPtr()
{
	return m_CFactoryBasePanel;
}

//=============================================================================
void CBaseModPanel::ReloadScheme()
{
}

void CBaseModPanel::PreloadPanels()
{
	auto pLoadout = GetSingleton().OpenWindow(WT_DM_LOADOUT, NULL, false);
	pLoadout->Activate();
	pLoadout->InvalidateLayout(true, true);
	pLoadout->Hide();

	auto pCreateServer = GetSingleton().OpenWindow(WT_CREATESERVERPANEL, NULL, false);
	pCreateServer->Activate();
	pCreateServer->InvalidateLayout(true, true);
	pCreateServer->Hide();
}

//=============================================================================
CBaseModFrame* CBaseModPanel::OpenWindow(const WINDOW_TYPE & wt, CBaseModFrame * caller, bool hidePrevious, KeyValues *pParameters)
{
	CBaseModFrame *newNav = m_Frames[ wt ].Get();
	bool setActiveWindow = true;

	// Window priority is used to track which windows are visible at all times
	// it is used to resolve the situations when a game requests an error box to popup
	// while a loading progress bar is active.
	// Windows with a higher priority force all other windows to get hidden.
	// After the high-priority window goes away it falls back to restore the low priority windows.
	WINDOW_PRIORITY nWindowPriority = WPRI_NORMAL;

	switch ( wt )
	{
	case WT_PASSWORDENTRY:
		setActiveWindow = false;
		break;
	}

	switch ( wt )
	{
		case WT_GENERICWAITSCREEN:
			nWindowPriority = WPRI_WAITSCREEN;
			break;
		case WT_GENERICCONFIRMATION:
			nWindowPriority = WPRI_MESSAGE;
			break;
		case WT_TRANSITIONSCREEN:
		case WT_PASSWORDENTRY:
		case WT_DM_LOADOUT:
		case WT_CREATESERVERPANEL:
		case WT_LORE_COMPENDIUM:
			nWindowPriority = WPRI_TOPMOST;
			break;
	}

	if ( !newNav )
	{
		switch ( wt )
		{
#if 0
		case WT_ACHIEVEMENTS:
//			m_Frames[wt] = new Achievements(this, "Achievements");
			break;

#ifdef ENABLE_ADDONS
		case WT_ADDONS:
			m_Frames[wt] = new Addons( this, "Addons" );
			break;
#endif

		case WT_ADDONASSOCIATION:
			m_Frames[wt] = new AddonAssociation( this, "AddonAssociation" );
			break;

		case WT_ATTRACTSCREEN:
			m_Frames[ wt ] = new CAttractScreen( this, "AttractScreen" );
			break;

		case WT_AUDIO:
			m_Frames[wt] = new Audio(this, "Audio");
			break;

		case WT_AUDIOVIDEO:
			m_Frames[wt] = new AudioVideo(this, "AudioVideo");
			break;

		case WT_CUSTOMCAMPAIGNS:
			m_Frames[ wt ] = new CustomCampaigns( this, "CustomCampaigns" );
			break;
/*
		case WT_GAMEOPTIONS:
			m_Frames[wt] = new GameOptions(this, "GameOptions");
			break;

		case WT_GAMESETTINGS:
			m_Frames[wt] = new GameSettings(this, "GameSettings");
			break;
*/

		case WT_KEYBOARDMOUSE:
			m_Frames[wt] = new VKeyboard(this, "VKeyboard");
			break;
			
		case WT_MULTIPLAYER:
			m_Frames[wt] = new Multiplayer(this, "Multiplayer");
			break;
		case WT_TRANSITIONSCREEN:
			m_Frames[wt] = new CTransitionScreen( this, "TransitionScreen" );
			break;

		case WT_VIDEO:
			m_Frames[wt] = new Video(this, "Video");
			break;
#endif
		case WT_GENERICCONFIRMATION:
			m_Frames[wt] = new GenericConfirmation(this, "GenericConfirmation");
			break;
		case WT_INGAMEMAINMENU:
			m_Frames[wt] = new InGameMainMenu(this, "InGameMainMenu");
			break;

		case WT_MAINMENU:
			m_Frames[wt] = new MainMenu(this, "MainMenu");
			break;

		case WT_DM_LOADOUT:
			m_Frames[wt] = new DMLoadout(this, "DMLoadout");
			break;

		case WT_CREATESERVERPANEL:
			m_Frames[wt] = new CreateServerPanel(this, "CreateServerPanel");
			break;
		case WT_LORE_COMPENDIUM:
			m_Frames[wt] = new COFLoreCompendium(this, "LoreCompendium");
			break;
		default:
			Assert( false );	// unknown window type
			break;
		}

		//
		// Finish setting up the window
		//

		newNav = m_Frames[wt].Get();
		if ( !newNav )
			return NULL;

		newNav->SetWindowType(wt);
		newNav->SetWindowPriority( nWindowPriority );
		newNav->SetVisible( false );
	}

	newNav->SetDataSettings( pParameters );

	if (setActiveWindow)
	{
		m_ActiveWindow[ nWindowPriority ] = wt;
		newNav->AddActionSignalTarget(this);
		newNav->SetCanBeActiveWindowType(true);
	}
	else if ( nWindowPriority == WPRI_MESSAGE )
	{
		m_ActiveWindow[ nWindowPriority ] = wt;
	}

	//
	// Now the window has been created, set it up
	//

	if ( UI_IsDebug() && (wt != WT_LOADINGPROGRESS) )
	{
		Msg( "[GAMEUI] OnOpen( `%s`, caller = `%s`, hidePrev = %d, setActive = %d, wt=%d, wpri=%d )\n",
			newNav->GetName(), caller ? caller->GetName() : "<NULL>", int(hidePrevious),
			int( setActiveWindow ), wt, nWindowPriority );
		KeyValuesDumpAsDevMsg( pParameters, 1 );
	}

	newNav->SetNavBack(caller);

	if (hidePrevious && caller != 0)
	{
		caller->SetVisible( false );
	}
	else if (caller != 0)
	{
		caller->FindAndSetActiveControl();
		//caller->SetAlpha(128);
	}

	// Check if a higher priority window is open
	if ( GetActiveWindowPriority() > newNav->GetWindowPriority() )
	{
		if ( UI_IsDebug() )
		{
			CBaseModFrame *pOther = m_Frames[ GetActiveWindowType() ].Get();
			Warning( "[GAMEUI] OpenWindow: Another window %p`%s` is having priority %d, deferring `%s`!\n",
				pOther, pOther ? pOther->GetName() : "<<null>>",
				GetActiveWindowPriority(), newNav->GetName() );
		}

		// There's a higher priority window that was open at the moment,
		// hide our window for now, it will get restored later.
		// newNav->SetVisible( false );
	}
	else
	{
		newNav->InvalidateLayout(false, false);
		newNav->OnOpen();
	}

	if ( UI_IsDebug() && (wt != WT_LOADINGPROGRESS) )
	{
		DbgShowCurrentUIState();
	}

	return newNav;
}

///=============================================================================
CBaseModFrame * CBaseModPanel::GetWindow( const WINDOW_TYPE& wt )
{
	return m_Frames[wt].Get();
}

//=============================================================================
WINDOW_TYPE CBaseModPanel::GetActiveWindowType()
{
	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;
			
			return m_ActiveWindow[ k ];
		}
	}
	return WT_NONE;
}

//=============================================================================
WINDOW_PRIORITY CBaseModPanel::GetActiveWindowPriority()
{
	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;

			return WINDOW_PRIORITY(k);
		}
	}
	return WPRI_NONE;
}

//=============================================================================
void CBaseModPanel::SetActiveWindow( CBaseModFrame * frame )
{
	if( !frame )
		return;
	
	m_ActiveWindow[ frame->GetWindowPriority() ] = frame->GetWindowType();

	if ( GetActiveWindowPriority() > frame->GetWindowPriority() )
	{
		if ( UI_IsDebug() )
		{
			CBaseModFrame *pOther = m_Frames[ GetActiveWindowType() ].Get();
			Warning( "[GAMEUI] SetActiveWindow: Another window %p`%s` is having priority %d, deferring `%s`!\n",
				pOther, pOther ? pOther->GetName() : "<<null>>",
				GetActiveWindowPriority(), frame->GetName() );
		}

		// frame->SetVisible( false );
	}
	else
	{
		frame->OnOpen();
	}
}

//=============================================================================
void CBaseModPanel::OnFrameHidden( WINDOW_PRIORITY pri, WINDOW_TYPE wt )
{
	// Do the same as frame closed but dont forget us
	auto hFrame = m_Frames[wt];
	OnFrameClosed(pri, wt);
	m_Frames[wt] = hFrame;
}

//=============================================================================
void CBaseModPanel::OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt )
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnFrameClosed( %d, %d )\n", pri, wt );
		DbgShowCurrentUIState();
	}

	// Mark the frame that just closed as NULL so that nobody could find it
	m_Frames[wt] = NULL;

	if ( m_bClosingAllWindows )
	{
		if ( UI_IsDebug() )
		{
			Msg( "[GAMEUI] Closing all windows\n" );
		}
		return;
	}

	if( pri <= WPRI_NORMAL )
		return;

	for ( int k = 0; k < WPRI_COUNT; ++ k )
	{
		if ( m_ActiveWindow[k] == wt )
			m_ActiveWindow[k] = WT_NONE;
	}

	//
	// We only care to resurrect windows of lower priority when
	// higher priority windows close
	//

	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] == WT_NONE )
			continue;

		CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
		if ( !pFrame )
			continue;

		// pFrame->AddActionSignalTarget(this);

		pFrame->InvalidateLayout(false, false);
		pFrame->OnOpen();
		pFrame->SetVisible( true );
		pFrame->Activate();

		if ( UI_IsDebug() )
		{
			Msg( "[GAMEUI] CBaseModPanel::OnFrameClosed( %d, %d ) -> Activated `%s`, pri=%d\n",
				pri, wt, pFrame->GetName(), pFrame->GetWindowPriority() );
			DbgShowCurrentUIState();
		}

		return;
	}
}

void CBaseModPanel::DbgShowCurrentUIState()
{
	if ( UI_IsDebug() < 2 )
		return;

	Msg( "[GAMEUI] Priorities WT: " );
	for ( int i = 0; i < WPRI_COUNT; ++ i )
	{
		Msg( " %d ", m_ActiveWindow[i] );
	}
	Msg( "\n" );
	for ( int i = 0; i < WT_WINDOW_COUNT; ++ i )
	{
		CBaseModFrame *pFrame = m_Frames[i].Get();
		if ( pFrame )
		{
			Msg( "        %2d. `%s` pri%d vis%d\n",
				i, pFrame->GetName(), pFrame->GetWindowPriority(), pFrame->IsVisible() );
		}
		else
		{
			Msg( "        %2d. NULL\n", i );
		}
	}
}

//=============================================================================
void CBaseModPanel::CloseAllWindows( int ePolicyFlags )
{
	CAutoPushPop< bool > auto_m_bClosingAllWindows( m_bClosingAllWindows, true );

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows( 0x%x )\n", ePolicyFlags );
	}

	// make sure we also close any active flyout menus that might be hanging out.
	FlyoutMenu::CloseActiveMenu();

	for (int i = 0; i < WT_WINDOW_COUNT; ++i)
	{
		CBaseModFrame *pFrame = m_Frames[i].Get();
		if ( !pFrame )
			continue;

		int nPriority = pFrame->GetWindowPriority();

		switch ( nPriority )
		{
		case WPRI_MESSAGE:
			if ( !(ePolicyFlags & CLOSE_POLICY_EVEN_MSGS) )
			{
				if ( UI_IsDebug() )
				{
					Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows() - Keeping msgbox type %d of priority %d.\n", i, nPriority );
				}

				continue;
				m_ActiveWindow[ WPRI_MESSAGE ] = WT_NONE;
			}
			break;

		case WPRI_BKGNDSCREEN:
			if ( ePolicyFlags & CLOSE_POLICY_KEEP_BKGND )
			{
				if ( UI_IsDebug() )
				{
					Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows() - Keeping bkgnd type %d of priority %d.\n", i, nPriority );
				}

				continue;
				m_ActiveWindow[ WPRI_BKGNDSCREEN ] = WT_NONE;
			}
			break;
		}

		// Close the window
		pFrame->Close();
		m_Frames[i] = NULL;
	}

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] After close all windows:\n" );
		DbgShowCurrentUIState();
	}

	m_ActiveWindow[ WPRI_NORMAL ] = WT_NONE;
}

#if defined( _X360 ) && defined( _DEMO )
void CBaseModPanel::OnDemoTimeout()
{
	if ( !engine->IsInGame() && !engine->IsConnected() && !engine->IsDrawingLoadingImage() )
	{
		// exit is terminal and unstoppable
		StartExitingProcess( false );
	}
	else
	{
		engine->ExecuteClientCmd( "disconnect" );
	}
}
#endif

bool CBaseModPanel::ActivateBackgroundEffects()
{
	return true;
}

//=============================================================================
void CBaseModPanel::OnGameUIActivated()
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnGameUIActivated( delay = %d )\n", m_DelayActivation );
	}

	if ( m_DelayActivation )
	{
		return;
	}

	COM_TimestampedLog( "CBaseModPanel::OnGameUIActivated()" );

#if defined( _X360 )
	if ( !engine->IsInGame() && !engine->IsConnected() && !engine->IsDrawingLoadingImage() )
	{
#if defined( _DEMO )
		if ( engine->IsDemoExiting() )
		{
			// just got activated, maybe from a disconnect
			// exit is terminal and unstoppable
			SetVisible( true );
			StartExitingProcess( false );
			return;
		}
#endif
		if ( !GameUI()->IsInLevel() && !GameUI()->IsInBackgroundLevel() )
		{
			// not using a background map
			// start the menu movie and music now, as the main menu is about to open
			// these are very large i/o operations on the xbox
			// they must occur before the installer takes over the DVD
			// otherwise the transfer rate is so slow and we sync stall for 10-15 seconds
			ActivateBackgroundEffects();
		}
		// the installer runs in the background during the main menu
		g_pXboxInstaller->Start();

#if defined( _DEMO )
		// ui valid can now adhere to demo timeout rules
		engine->EnableDemoTimeout( true );
#endif
	}
#endif

	SetVisible( true );

	// This is terrible, why are we directing the window that we open when we are only trying to activate the UI?
	if ( WT_GAMELOBBY == GetActiveWindowType() )
	{
		return;
	}
	else if ( ( !engine->IsConnected() ) /* || GameUI().IsInBackgroundLevel() */ )
	{
		bool bForceReturnToFrontScreen = false;
		WINDOW_TYPE wt = GetActiveWindowType();
		switch ( wt )
		{
		default:
			break;
		case WT_NONE:
		case WT_INGAMEMAINMENU:
		case WT_GENERICCONFIRMATION:
			// bForceReturnToFrontScreen = !g_pMatchFramework->GetMatchmaking()->ShouldPreventOpenFrontScreen();
			bForceReturnToFrontScreen = true; // this used to be some magic about mid-disconnecting-states on PC...
			break;
		}
		if ( !IsPC() || bForceReturnToFrontScreen )
		{
			OpenFrontScreen();
		}
		m_pVideo->SetVisible(true);
	}
	else if ( engine->IsConnected() )
	{
		m_pVideo->SetVisible( false );

		CBaseModFrame *pInGameMainMenu = m_Frames[ WT_INGAMEMAINMENU ].Get();

		if ( !pInGameMainMenu || !pInGameMainMenu->IsAutoDeleteSet() )
		{
			// Prevent in game menu from opening if it already exists!
			// It might be hiding behind a modal window that needs to keep focus
			OpenWindow( WT_INGAMEMAINMENU, 0 );
		}
	}

	PreloadPanels();
}

//=============================================================================
void CBaseModPanel::OnGameUIHidden()
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnGameUIHidden()\n" );
	}

#if defined( _X360 )
	// signal the installer to stop
	g_pXboxInstaller->Stop();
#endif

// 	// We want to check here if we have any pending message boxes and
// 	// if so, then we cannot just simply destroy all the UI elements
// 	for ( int k = WPRI_NORMAL + 1; k < WPRI_LOADINGPLAQUE; ++ k )
// 	{
// 		WINDOW_TYPE wt = m_ActiveWindow[k];
// 		if ( wt != WT_NONE )
// 		{
// 			Msg( "[GAMEUI] CBaseModPanel::OnGameUIHidden() - not destroying UI because of wt %d pri %d\n",
// 				wt, k );
// 			return;
// 		}
// 	}

	SetVisible(false);
	
	// Notify the options dialog that game UI is closing
	// if ( m_hOptionsDialog.Get() )
	// {
	// 	PostMessage( m_hOptionsDialog.Get(), new KeyValues( "GameUIHidden" ), 0.0 );
	// }

	// Notify the in game menu that game UI is closing
	CBaseModFrame *pInGameMainMenu = GetWindow( WT_INGAMEMAINMENU );
	if ( pInGameMainMenu )
	{
		PostMessage( pInGameMainMenu, new KeyValues( "GameUIHidden" ) );
	}

	// Close achievements
	if ( CBaseModFrame *pFrame = GetWindow( WT_ACHIEVEMENTS ) )
	{
		pFrame->Close();
	}
}

void CBaseModPanel::OpenFrontScreen()
{
	WINDOW_TYPE frontWindow = WT_NONE;

	frontWindow = WT_MAINMENU;
	m_pVideo->SetVisible( true );

	if( frontWindow != WT_NONE )
	{
		if( GetActiveWindowType() != frontWindow )
		{
			CloseAllWindows();
			OpenWindow( frontWindow, NULL );
		}
	}
}

//=============================================================================
void CBaseModPanel::OnTick()
{
	if ( s_NavLock > 0 )
	{
		--s_NavLock;
	}

	GetAnimationController()->UpdateAnimations( Plat_FloatTime() );

	CBaseModFrame::RunFrameOnListeners();

	CUIGameData::Get()->RunFrame();

	if ( m_DelayActivation )
	{
		m_DelayActivation--;
		if ( !m_DelayActivation )
		{
			if ( UI_IsDebug() )
			{
				Msg( "[GAMEUI] Executing delayed UI activation\n");
			}
			OnGameUIActivated();
		}
	}

	if ( !(IsInLevel()) && m_bIsIngame)
	{
		m_bIsIngame = false;
		m_pVideo->SetVisible(true);
	}
	else if (IsInLevel() && !m_bIsIngame)
	{
		m_bIsIngame = true;
		m_pVideo->SetVisible(false);
	}

	// MRMODEZ TODO: New fancy music player
	if (IsPC() && m_iPlayGameStartupSound > 0)
	{
		m_iPlayGameStartupSound--;
		if (!m_iPlayGameStartupSound)
		{
			PlayGameStartupSound();
		}
	}

	bool bDoBlur = true;
	WINDOW_TYPE wt = GetActiveWindowType();
	switch ( wt )
	{
	case WT_NONE:
	case WT_MAINMENU:
	case WT_AUDIOVIDEO:
		bDoBlur = false;
		break;
	}
	if ( GetWindow( WT_ATTRACTSCREEN ) /* || ( enginevguifuncs && !enginevguifuncs->IsGameUIVisible() )*/ )
	{
		// attract screen might be open, but not topmost due to notification dialogs
		bDoBlur = false;
	}

	if ( !bDoBlur )
	{
		bDoBlur = false;// GameClientExports()->ClientWantsBlurEffect();
	}

	float nowTime = Plat_FloatTime();
	float deltaTime = nowTime - m_flLastBlurTime;
	if ( deltaTime > 0 )
	{
		m_flLastBlurTime = nowTime;
		m_flBlurScale += (deltaTime && bDoBlur) ? 0.05f : -0.05f;
		m_flBlurScale = clamp( m_flBlurScale, 0, 0.85f );
		//engine->SetBlurFade( m_flBlurScale );
	}
}

void CBaseModPanel::FireGameEvent( IGameEvent *event )
{
	char const *name = event->GetName();
	if ( FStrEq( name, "gameui_activated" ) )
		OnGameUIActivated();
	else if ( FStrEq( name, "gameui_hidden" ) )
		OnGameUIHidden();
}

void CBaseModPanel::SetHelpText( const char* helpText )
{
	if ( m_FooterPanel )
	{
		m_FooterPanel->SetHelpText( helpText );
	}
}

void CBaseModPanel::SetOkButtonEnabled( bool bEnabled )
{
	if ( m_FooterPanel )
	{
		CBaseModFooterPanel::FooterButtons_t buttons = m_FooterPanel->GetButtons();
		if ( bEnabled )
			buttons |= FB_ABUTTON;
		else
			buttons &= ~FB_ABUTTON;
		m_FooterPanel->SetButtons( buttons, m_FooterPanel->GetFormat(), m_FooterPanel->GetHelpTextEnabled() );
	}
}

void CBaseModPanel::SetCancelButtonEnabled( bool bEnabled )
{
	if ( m_FooterPanel )
	{
		CBaseModFooterPanel::FooterButtons_t buttons = m_FooterPanel->GetButtons();
		if ( bEnabled )
			buttons |= FB_BBUTTON;
		else
			buttons &= ~FB_BBUTTON;
		m_FooterPanel->SetButtons( buttons, m_FooterPanel->GetFormat(), m_FooterPanel->GetHelpTextEnabled() );
	}
}

BaseModUI::CBaseModFooterPanel* CBaseModPanel::GetFooterPanel()
{
	// EVIL HACK
	if ( !this )
	{
		Assert( 0 );
		Warning( "CBaseModPanel::GetFooterPanel() called on NULL CBaseModPanel!!!\n" );
		return NULL;
	}
	return m_FooterPanel;
}

void CBaseModPanel::SetLastActiveUserId( int userId )
{
	if ( m_lastActiveUserId != userId )
	{
		DevWarning( "SetLastActiveUserId: %d -> %d\n", m_lastActiveUserId, userId );
	}

	m_lastActiveUserId = userId;
}

int CBaseModPanel::GetLastActiveUserId( )
{
	return m_lastActiveUserId;
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
static void BaseUI_PositionDialog(vgui::PHandle dlg)
{
	if (!dlg.Get())
		return;

	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	dlg->GetSize(wide, tall);

	// Center it, keeping requested size
	dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//=============================================================================
void CBaseModPanel::OnNavigateTo( const char* panelName )
{
	CBaseModFrame* currentFrame = 
		static_cast<CBaseModFrame*>(FindChildByName(panelName, false));

	if (currentFrame && currentFrame->GetCanBeActiveWindowType())
	{
		m_ActiveWindow[ currentFrame->GetWindowPriority() ] = currentFrame->GetWindowType();
	}
}

//=============================================================================
void CBaseModPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(pScheme->GetColor("Blank", Color(0, 0, 0, 0)));

	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );

	char filename[MAX_PATH];
	V_snprintf( filename, sizeof( filename ), "VGUI/loading/BGFX01" ); // TODO: engine->GetStartupImage( filename, sizeof( filename ), screenWide, screenTall );
	m_iBackgroundImageID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_iBackgroundImageID, filename, true, false );

	m_iProductImageID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_iProductImageID, "console/startup_loading", true, false );

	int logoW = 384;
	int logoH = 192;

	bool bIsWidescreen;
#if !defined( _X360 )
	float aspectRatio = (float)screenWide/(float)screenTall;
	bIsWidescreen = aspectRatio >= 1.5999f;
#else
	static ConVarRef mat_xbox_iswidescreen( "mat_xbox_iswidescreen" );
	bIsWidescreen = mat_xbox_iswidescreen.GetBool();
#endif
	if ( !bIsWidescreen )
	{
		// smaller in standard res
		logoW = 320;
		logoH = 160;
	}

	m_nProductImageWide = vgui::scheme()->GetProportionalScaledValue( logoW );
	m_nProductImageTall = vgui::scheme()->GetProportionalScaledValue( logoH );

	char background[MAX_PATH];
	engine->GetMainMenuBackgroundName(background, MAX_PATH);

	if ( aspectRatio >= 1.6f )
	{
		// use the widescreen version
		Q_snprintf( m_szFadeFilename, sizeof( m_szFadeFilename ), "materials/console/%s_widescreen.vtf", background);
	}
	else
	{
		Q_snprintf( m_szFadeFilename, sizeof( m_szFadeFilename ), "materials/console/%s.vtf", background );
	}

	// Active hidden console
	if ( GameConsole()->IsConsoleVisible() )
	{
		GameConsole()->Activate();
	}
}

void CBaseModPanel::DrawColoredText( vgui::HFont hFont, int x, int y, unsigned int color, const char *pAnsiText )
{
	wchar_t szconverted[256];
	int len = g_pVGuiLocalize->ConvertANSIToUnicode( pAnsiText, szconverted, sizeof( szconverted ) );
	if ( len <= 0 )
	{
		return;
	}

	int r = ( color >> 24 ) & 0xFF;
	int g = ( color >> 16 ) & 0xFF;
	int b = ( color >> 8 ) & 0xFF;
	int a = ( color >> 0 ) & 0xFF;

	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawSetTextColor( r, g, b, a );
	vgui::surface()->DrawPrintText( szconverted, len );
}

void CBaseModPanel::DrawCopyStats()
{
#if defined( _X360 )
	int wide, tall;
	GetSize( wide, tall );

	int xPos = 0.1f * wide;
	int yPos = 0.1f * tall;

	// draw copy status
	char textBuffer[256];
	const CopyStats_t *pCopyStats = g_pXboxInstaller->GetCopyStats();	

	V_snprintf( textBuffer, sizeof( textBuffer ), "Version: %d (%s)", g_pXboxInstaller->GetVersion(), XBX_GetLanguageString() );
	DrawColoredText( m_hDefaultFont, xPos, yPos, 0xffff00ff, textBuffer );
	yPos += 20;

	V_snprintf( textBuffer, sizeof( textBuffer ), "DVD Hosted: %s", g_pFullFileSystem->IsDVDHosted() ? "Enabled" : "Disabled" );
	DrawColoredText( m_hDefaultFont, xPos, yPos, 0xffff00ff, textBuffer );
	yPos += 20;

	bool bDrawProgress = true;
	if ( g_pFullFileSystem->IsInstalledToXboxHDDCache() )
	{
		DrawColoredText( m_hDefaultFont, xPos, yPos, 0x00ff00ff, "Existing Image Found." );
		yPos += 20;
		bDrawProgress = false;
	}
	if ( !g_pXboxInstaller->IsInstallEnabled() )
	{
		DrawColoredText( m_hDefaultFont, xPos, yPos, 0xff0000ff, "Install Disabled." );
		yPos += 20;
		bDrawProgress = false;
	}
	if ( g_pXboxInstaller->IsFullyInstalled() )
	{
		DrawColoredText( m_hDefaultFont, xPos, yPos, 0x00ff00ff, "Install Completed." );
		yPos += 20;
	}

	if ( bDrawProgress )
	{
		yPos += 20;
		V_snprintf( textBuffer, sizeof( textBuffer ), "From: %s (%.2f MB)", pCopyStats->m_srcFilename, (float)pCopyStats->m_ReadSize/(1024.0f*1024.0f) );
		DrawColoredText( m_hDefaultFont, xPos, yPos, 0xffff00ff, textBuffer );
		V_snprintf( textBuffer, sizeof( textBuffer ), "To: %s (%.2f MB)", pCopyStats->m_dstFilename, (float)pCopyStats->m_WriteSize/(1024.0f*1024.0f)  );
		DrawColoredText( m_hDefaultFont, xPos, yPos + 20, 0xffff00ff, textBuffer );

		float elapsed = 0;
		float rate = 0;
		if ( pCopyStats->m_InstallStartTime )
		{
			elapsed = (float)(GetTickCount() - pCopyStats->m_InstallStartTime) * 0.001f;
		}
		if ( pCopyStats->m_InstallStopTime )
		{
			elapsed = (float)(pCopyStats->m_InstallStopTime - pCopyStats->m_InstallStartTime) * 0.001f;
		}
		if ( elapsed )
		{
			rate = pCopyStats->m_TotalWriteSize/elapsed;
		}
		V_snprintf( textBuffer, sizeof( textBuffer ), "Progress: %d/%d MB Elapsed: %d secs (%.2f MB/s)", pCopyStats->m_BytesCopied/(1024*1024), g_pXboxInstaller->GetTotalSize()/(1024*1024), (int)elapsed, rate/(1024.0f*1024.0f) );
		DrawColoredText( m_hDefaultFont, xPos, yPos + 40, 0xffff00ff, textBuffer );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're currently playing the game
//-----------------------------------------------------------------------------
bool CBaseModPanel::IsInLevel()
{
	const char *levelName = engine->GetLevelName();
	if( levelName && levelName[0] && !engine->IsLevelMainMenuBackground() )
	{
		return true;
	}

	return false;
}

//=============================================================================
void CBaseModPanel::PaintBackground()
{
	if (!IsInLevel())
	{
		int wide, tall;
		GetSize( wide, tall );

		if ( false /*engine->IsTransitioningToLoad()*/ )
		{
			// ensure the background is clear
			// the loading progress is about to take over in a few frames
			// this keeps us from flashing a different graphic
			surface()->DrawSetColor(0, 0, 0, 255);
			surface()->DrawSetTexture(m_iBackgroundImageID);
			surface()->DrawTexturedRect(0, 0, wide, tall);
		}
		else
		{
			ActivateBackgroundEffects();

			if ( !m_flMovieFadeInTime )
			{
				// do the fade a little bit after the movie starts (needs to be stable)
				// the product overlay will fade out
				m_flMovieFadeInTime = Plat_FloatTime() + TRANSITION_TO_MOVIE_DELAY_TIME;
			}

			float flFadeDelta = RemapValClamped( Plat_FloatTime(), m_flMovieFadeInTime, m_flMovieFadeInTime + TRANSITION_TO_MOVIE_FADE_TIME, 1.0f, 0.0f );
			if ( flFadeDelta > 0.0f )
			{
				if ( !m_pBackgroundMaterial )
				{
					PrepareStartupGraphic();
				}
				DrawStartupGraphic( flFadeDelta );
			}
		}
	}

#if defined( _X360 )
	if ( !GameUI().IsInLevel() && xbox_install_status.GetBool() )
	{
		DrawCopyStats();
	}
#endif
}

IVTFTexture *LoadVTF( CUtlBuffer &temp, const char *szFileName )
{
	if ( !g_pFullFileSystem->ReadFile( szFileName, NULL, temp ) )
		return NULL;

	IVTFTexture *texture = CreateVTFTexture();
	if ( !texture->Unserialize( temp ) )
	{
		Error( "Invalid or corrupt background texture %s\n", szFileName );
		return NULL;
	}
	texture->ConvertImageFormat( IMAGE_FORMAT_RGBA8888, false );
	return texture;
}

void CBaseModPanel::PrepareStartupGraphic()
{
	CUtlBuffer buf;
	// load in the background vtf
	buf.Clear();
	m_pBackgroundTexture = LoadVTF( buf, m_szFadeFilename );
	if ( !m_pBackgroundTexture )
	{
		Error( "Can't find background image '%s'\n", m_szFadeFilename );
		return;
	}

	// Allocate a white material
	m_pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	m_pVMTKeyValues->SetString( "$basetexture", m_szFadeFilename + 10 );
	m_pVMTKeyValues->SetInt( "$ignorez", 1 );
	m_pVMTKeyValues->SetInt( "$nofog", 1 );
	m_pVMTKeyValues->SetInt( "$no_fullbright", 1 );
	m_pVMTKeyValues->SetInt( "$nocull", 1 );
	m_pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	m_pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	m_pBackgroundMaterial = g_pMaterialSystem->CreateMaterial( "__background", m_pVMTKeyValues );
}

void CBaseModPanel::ReleaseStartupGraphic()
{
	if ( m_pBackgroundMaterial )
	{
		m_pBackgroundMaterial->Release();
	}

	if ( m_pBackgroundTexture )
	{
		DestroyVTFTexture( m_pBackgroundTexture );
		m_pBackgroundTexture = NULL;
	}
}

// we have to draw the startup fade graphic using this function so it perfectly matches the one drawn by the engine during load
void DrawScreenSpaceRectangleAlpha( IMaterial *pMaterial, 
							  int nDestX, int nDestY, int nWidth, int nHeight,	// Rect to draw into in screen space
							  float flSrcTextureX0, float flSrcTextureY0,		// which texel you want to appear at destx/y
							  float flSrcTextureX1, float flSrcTextureY1,		// which texel you want to appear at destx+width-1, desty+height-1
							  int nSrcTextureWidth, int nSrcTextureHeight,		// needed for fixup
							  void *pClientRenderable,							// Used to pass to the bind proxies
							  int nXDice, int nYDice,							// Amount to tessellate the mesh
							  float fDepth, float flAlpha )									// what Z value to put in the verts (def 0.0)
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) )
		return;

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( pMaterial, pClientRenderable );

	int xSegments = MAX( nXDice, 1);
	int ySegments = MAX( nYDice, 1);

	CMeshBuilder meshBuilder;

	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, xSegments * ySegments );

	int nScreenWidth, nScreenHeight;
	pRenderContext->GetRenderTargetDimensions( nScreenWidth, nScreenHeight );
	float flLeftX = nDestX - 0.5f;
	float flRightX = nDestX + nWidth - 0.5f;

	float flTopY = nDestY - 0.5f;
	float flBottomY = nDestY + nHeight - 0.5f;

	float flSubrectWidth = flSrcTextureX1 - flSrcTextureX0;
	float flSubrectHeight = flSrcTextureY1 - flSrcTextureY0;

	float flTexelsPerPixelX = ( nWidth > 1 ) ? flSubrectWidth / ( nWidth - 1 ) : 0.0f;
	float flTexelsPerPixelY = ( nHeight > 1 ) ? flSubrectHeight / ( nHeight - 1 ) : 0.0f;

	float flLeftU = flSrcTextureX0 + 0.5f - ( 0.5f * flTexelsPerPixelX );
	float flRightU = flSrcTextureX1 + 0.5f + ( 0.5f * flTexelsPerPixelX );
	float flTopV = flSrcTextureY0 + 0.5f - ( 0.5f * flTexelsPerPixelY );
	float flBottomV = flSrcTextureY1 + 0.5f + ( 0.5f * flTexelsPerPixelY );

	float flOOTexWidth = 1.0f / nSrcTextureWidth;
	float flOOTexHeight = 1.0f / nSrcTextureHeight;
	flLeftU *= flOOTexWidth;
	flRightU *= flOOTexWidth;
	flTopV *= flOOTexHeight;
	flBottomV *= flOOTexHeight;

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	// Dice the quad up...
	if ( xSegments > 1 || ySegments > 1 )
	{
		// Screen height and width of a subrect
		float flWidth  = (flRightX - flLeftX) / (float) xSegments;
		float flHeight = (flTopY - flBottomY) / (float) ySegments;

		// UV height and width of a subrect
		float flUWidth  = (flRightU - flLeftU) / (float) xSegments;
		float flVHeight = (flBottomV - flTopV) / (float) ySegments;

		for ( int x=0; x < xSegments; x++ )
		{
			for ( int y=0; y < ySegments; y++ )
			{
				// Top left
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Top right (x+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Bottom right (x+1), (y+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Bottom left (y+1)
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();
			}
		}
	}
	else // just one quad
	{
		for ( int corner=0; corner<4; corner++ )
		{
			bool bLeft = (corner==0) || (corner==3);
			meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, fDepth );
			meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
			meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
			meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
			meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
			meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}


void CBaseModPanel::DrawStartupGraphic( float flNormalizedAlpha )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	int w = GetWide();
	int h = GetTall();
	int tw = m_pBackgroundTexture->Width();
	int th = m_pBackgroundTexture->Height();
	float depth = 0.5f;
	DrawScreenSpaceRectangleAlpha( m_pBackgroundMaterial, 0, 0, w, h, 0, 0, tw, th, tw, th, NULL, 1, 1, depth, flNormalizedAlpha );
}

void CBaseModPanel::OnCommand(const char *command)
{
	if ( !Q_stricmp( command, "QuitRestartNoConfirm" ) )
	{
		if ( IsX360() )
		{
			StartExitingProcess( false );
		}
	}
#if 0
	else if ( !Q_stricmp( command, "RestartWithNewLanguage" ) )
	{
		if ( !IsX360() )
		{
			const char *pUpdatedAudioLanguage = Audio::GetUpdatedAudioLanguage();

			if ( pUpdatedAudioLanguage[ 0 ] != '\0' )
			{
				char szSteamURL[50];
				char szAppId[50];

				// hide everything while we quit
				SetVisible( false );
				vgui::surface()->RestrictPaintToSinglePanel( GetVPanel() );
				engine->ClientCmd_Unrestricted( "quit\n" );

				// Construct Steam URL. Pattern is steam://run/<appid>/<language>. (e.g. Ep1 In French ==> steam://run/380/french)
				Q_strcpy(szSteamURL, "steam://run/");
				itoa( engine->GetAppID(), szAppId, 10 );
				Q_strcat( szSteamURL, szAppId, sizeof( szSteamURL ) );
				Q_strcat( szSteamURL, "/", sizeof( szSteamURL ) );
				Q_strcat( szSteamURL, pUpdatedAudioLanguage, sizeof( szSteamURL ) );

				// Set Steam URL for re-launch in registry. Launcher will check this registry key and exec it in order to re-load the game in the proper language
				vgui::system()->SetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Source\\Relaunch URL", szSteamURL );
			}
		}
	}
#endif
	else
	{
		BaseClass::OnCommand( command );
	}
}

bool CBaseModPanel::IsReadyToWriteConfig( void )
{
	// For cert we only want to write config files is it has been at least 3 seconds
#ifdef _X360
	static ConVarRef r_host_write_last_time( "host_write_last_time" );
	return ( Plat_FloatTime() > r_host_write_last_time.GetFloat() + 3.05f );
#endif
	return false;
}

const char *CBaseModPanel::GetUISoundName(  UISound_t UISound )
{
	switch ( UISound )
	{
	case UISOUND_BACK:
		return "UI/buttonclickrelease.wav";
	case UISOUND_ACCEPT:
		return "UI/buttonclick.wav";
	case UISOUND_FOCUS:
		return "UI/buttonrollover.wav";
	case UISOUND_CLICK:
		return "UI/buttonclick.wav";
	default:
		return "UI/buttonclick.wav";
	}
	return NULL;
}

void CBaseModPanel::PlayUISound( UISound_t UISound )
{
	const char *pSound = GetUISoundName( UISound );
	if ( pSound )
	{
		vgui::surface()->PlaySound( pSound );
	}
}

//=============================================================================
// Start system shutdown. Cannot be stopped.
// A Restart is cold restart, plays the intro movie again.
//=============================================================================
void CBaseModPanel::StartExitingProcess( bool bWarmRestart )
{
	if ( !IsX360() )
	{
		// xbox only
		Assert( 0 );
		return;
	}

	if ( m_ExitingFrameCount )
	{
		// already fired
		return;
	}

#if defined( _X360 )
	// signal the installer to stop
	g_pXboxInstaller->Stop();
#endif

	// cold restart or warm
	m_bWarmRestartMode = bWarmRestart;

	// the exiting screen will transition to obscure all the game and UI
	OpenWindow( WT_TRANSITIONSCREEN, 0, false );

	// must let a non trivial number of screen swaps occur to stabilize image
	// ui runs in a constrained state, while shutdown is occurring
	m_ExitingFrameCount = 15;

	// exiting cannot be stopped
	// do not allow any input to occur
	g_pInputSystem->DetachFromWindow();

	// start shutting down systems
	engine->StartXboxExitingProcess();
}

void CBaseModPanel::OnSetFocus()
{
	BaseClass::OnSetFocus();
	GameConsole()->Hide();
}

void CBaseModPanel::OnMovedPopupToFront()
{
	GameConsole()->Hide();
}

//-----------------------------------------------------------------------------
// Purpose: Searches for GameStartup*.mp3 files in the sound/ui folder and plays one
//-----------------------------------------------------------------------------
void CBaseModPanel::PlayGameStartupSound()
{
	if (CommandLine()->FindParm("-nostartupsound"))
		return;

	FileFindHandle_t fh;

	CUtlVector<char *> fileNames;

	char path[512];
	Q_snprintf(path, sizeof(path), "sound/ui/gamestartup*.mp3");
	Q_FixSlashes(path);

	char const *fn = g_pFullFileSystem->FindFirstEx(path, "MOD", &fh);
	if (fn)
	{
		do
		{
			char ext[10];
			Q_ExtractFileExtension(fn, ext, sizeof(ext));

			if (!Q_stricmp(ext, "mp3"))
			{
				char temp[512];
				Q_snprintf(temp, sizeof(temp), "ui/%s", fn);

				char *found = new char[strlen(temp) + 1];
				Q_strncpy(found, temp, strlen(temp) + 1);

				Q_FixSlashes(found);
				fileNames.AddToTail(found);
			}

			fn = g_pFullFileSystem->FindNext(fh);

		} while (fn);

		g_pFullFileSystem->FindClose(fh);
	}

	// did we find any?
	if (fileNames.Count() > 0)
	{
		int index = RandomInt(0, fileNames.Count() - 1);
		if (fileNames.IsValidIndex(index) && fileNames[index])
		{
			char found[512];

			// escape chars "*#" make it stream, and be affected by snd_musicvolume
			Q_snprintf(found, sizeof(found), "play *#%s", fileNames[index]);

			engine->ClientCmd_Unrestricted(found);
		}

		fileNames.PurgeAndDeleteElements();
	}
}

void CBaseModPanel::SafeNavigateTo( Panel *pExpectedFrom, Panel *pDesiredTo, bool bAllowStealFocus )
{
	Panel *pOriginalFocus = ipanel()->GetPanel( GetCurrentKeyFocus(), GetModuleName() );
	bool bSomeoneElseHasFocus = pOriginalFocus && (pOriginalFocus != pExpectedFrom);
	bool bActuallyChangingFocus = (pExpectedFrom != pDesiredTo);
	bool bNeedToReturnKeyFocus = !bAllowStealFocus && bSomeoneElseHasFocus && bActuallyChangingFocus;

	pDesiredTo->NavigateTo();

	if ( bNeedToReturnKeyFocus )
	{
		pDesiredTo->NavigateFrom();
		pOriginalFocus->NavigateTo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override the anim hack since its a base panel, to keep consistancy
//-----------------------------------------------------------------------------
void CBaseModPanel::SetSize(int wide, int tall)
{
	ipanel()->SetSize( GetVPanel(), wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: Override the anim hack since its a base panel, to keep consistancy
//-----------------------------------------------------------------------------
void CBaseModPanel::GetSize(int &wide, int &tall)
{
	ipanel()->GetSize( GetVPanel(), wide, tall );
}

CON_COMMAND_F(gamemenurefresh, "Refresh main menu", 0)
{
	CBaseModPanel::GetSingleton().InvalidateLayout(true, true);
}

CON_COMMAND_F(open_charinfo_direct, "Open the character info panel directly to the class you're currently playing.", 0)
{
	// We dont really need class for now but 
	int iClass = TF_CLASS_UNDEFINED;
	int iClassMod = TF_CLASSMOD_NONE;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer )
	{
		iClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
		iClassMod = pPlayer->GetPlayerClass()->GetModifier();
	}

	engine->ClientCmd_Unrestricted("gameui_activate");

	CBaseModPanel::GetSingleton().OpenWindow( WT_DM_LOADOUT, NULL, false );
}