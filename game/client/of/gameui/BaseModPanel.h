//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _BASEMODFACTORYBASEPANEL_H__
#define _BASEMODFACTORYBASEPANEL_H__

#include <GameUI/IBasePanel.h>
#include "ixboxsystem.h"

class COptionsDialog;
class CCreateMultiplayerGameDialog;
class COptionsMouseDialog;
class IMaterial;
class CMatchmakingBasePanel;
class CBackgroundMenuButton;
class CGameMenu;

namespace vgui
{
	class CVideoBackground;
}

enum
{
	DIALOG_STACK_IDX_STANDARD,
	DIALOG_STACK_IDX_WARNING,
	DIALOG_STACK_IDX_ERROR,
};

// must supply some non-trivial time to let the movie startup smoothly
// the attract screen also uses this so it doesn't pop in either
#define TRANSITION_TO_MOVIE_DELAY_TIME	0.5f	// how long to wait before starting the fade
#define TRANSITION_TO_MOVIE_FADE_TIME	1.2f	// how fast to fade

class IVTFTexture;

namespace BaseModUI 
{
	enum UISound_t
	{
		UISOUND_BACK,
		UISOUND_ACCEPT,
		UISOUND_INVALID,
		UISOUND_COUNTDOWN,
		UISOUND_FOCUS,
		UISOUND_CLICK,
		UISOUND_DENY,
	};

	enum WINDOW_TYPE
	{
		WT_NONE = 0,
		WT_ACHIEVEMENTS,
		WT_AUDIO,
		WT_AUDIOVIDEO,
		WT_CLOUD,
		WT_CONTROLLER,
		WT_CONTROLLER_STICKS,
		WT_CONTROLLER_BUTTONS,
		WT_DOWNLOADS,
		WT_GAMELOBBY,
		WT_GAMEOPTIONS,
		WT_GAMESETTINGS,
		WT_GENERICCONFIRMATION,
		WT_INGAMEDIFFICULTYSELECT,
		WT_INGAMEMAINMENU,
		WT_INGAMECHAPTERSELECT,
		WT_INGAMEKICKPLAYERLIST,
		WT_VOTEOPTIONS,
		WT_KEYBOARDMOUSE,
		WT_LOADINGPROGRESSBKGND,
		WT_LOADINGPROGRESS,
		WT_MAINMENU,
		WT_MULTIPLAYER,
		WT_OPTIONS,
		WT_SEARCHINGFORLIVEGAMES,
		WT_SIGNINDIALOG,
		WT_SINGLEPLAYER,
		WT_GENERICWAITSCREEN,
		WT_ATTRACTSCREEN,
		WT_ALLGAMESEARCHRESULTS,
		WT_FOUNDPUBLICGAMES,
		WT_TRANSITIONSCREEN,
		WT_PASSWORDENTRY,
		WT_VIDEO,
		WT_STEAMCLOUDCONFIRM,
		WT_STEAMGROUPSERVERS,
		WT_CUSTOMCAMPAIGNS,
#ifdef ENABLE_ADDONS
		WT_ADDONS,
#endif
		WT_DOWNLOADCAMPAIGN,
		WT_LEADERBOARD,
		WT_ADDONASSOCIATION,
		WT_GETLEGACYDATA,
		WT_DM_LOADOUT,
		WT_CREATESERVERPANEL,
		WT_LORE_COMPENDIUM,
		WT_WINDOW_COUNT // WT_WINDOW_COUNT must be last in the list!
	};

	enum WINDOW_PRIORITY
	{
		WPRI_NONE,
		WPRI_BKGNDSCREEN,
		WPRI_NORMAL,
		WPRI_WAITSCREEN,
		WPRI_MESSAGE,
		WPRI_LOADINGPLAQUE,
		WPRI_TOPMOST,			// must be highest priority, no other windows can obscure
		WPRI_COUNT				// WPRI_COUNT must be last in the list!
	};

	class CBaseModFrame;
	class CBaseModFooterPanel;

	//=============================================================================
	//
	//=============================================================================
	class CBaseModPanel : public vgui::EditablePanel, public CGameEventListener
	{
		DECLARE_CLASS_SIMPLE( CBaseModPanel, vgui::EditablePanel );

	public:
		CBaseModPanel();
		~CBaseModPanel();

		virtual vgui::Panel& GetVguiPanel();
	public:
		virtual void FireGameEvent( IGameEvent *event );

	public:
		static CBaseModPanel& GetSingleton();
		static CBaseModPanel* GetSingletonPtr();

		void ReloadScheme();

		void PreloadPanels();

		CBaseModFrame* OpenWindow( const WINDOW_TYPE& wt, CBaseModFrame * caller, bool hidePrevious = true, KeyValues *pParameters = NULL );
		CBaseModFrame* GetWindow( const WINDOW_TYPE& wt );

		void OnFrameHidden( WINDOW_PRIORITY pri, WINDOW_TYPE wt );
		void OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt );
		void DbgShowCurrentUIState();

		WINDOW_TYPE GetActiveWindowType();
		WINDOW_PRIORITY GetActiveWindowPriority();
		void SetActiveWindow( CBaseModFrame * frame );

		enum CloseWindowsPolicy_t
		{
			CLOSE_POLICY_DEFAULT = 0,			// will keep msg boxes alive
			CLOSE_POLICY_EVEN_MSGS = 1,			// will kill even msg boxes
			CLOSE_POLICY_EVEN_LOADING = 2,		// will kill even loading screen
			CLOSE_POLICY_KEEP_BKGND = 0x100,	// will keep bkgnd screen
		};
		void CloseAllWindows( int ePolicyFlags = CLOSE_POLICY_DEFAULT );

		bool IsInLevel();
		bool m_bIsIngame;

		void OnGameUIActivated();
		void OnGameUIHidden();
		void OpenFrontScreen();
		void OnTick();

		void SetHelpText( const char* helpText );
		void SetOkButtonEnabled( bool enabled );
		void SetCancelButtonEnabled( bool enabled );

		bool IsReadyToWriteConfig( void );
		const char *GetUISoundName(  UISound_t uiSound );
		void PlayUISound( UISound_t uiSound );
		void StartExitingProcess( bool bWarmRestart );

		CBaseModFooterPanel* GetFooterPanel();
		void SetLastActiveUserId( int userId );
		int GetLastActiveUserId();
		
		MESSAGE_FUNC_CHARPTR( OnNavigateTo, "OnNavigateTo", panelName );

		void SafeNavigateTo( Panel *pExpectedFrom, Panel *pDesiredTo, bool bAllowStealFocus );

		void SetSize(int wide,int tall);	// sets size of panel
		void GetSize(int &wide, int &tall);	// gets size of panel

#if defined( _X360 ) && defined( _DEMO )
		void OnDemoTimeout();
#endif

	protected:
		CBaseModPanel(const CBaseModPanel&);
		CBaseModPanel& operator=(const CBaseModPanel&);

		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PaintBackground();

		void OnCommand(const char *command);
		void OnSetFocus();

		MESSAGE_FUNC( OnMovedPopupToFront, "OnMovedPopupToFront" );

	private:
		void DrawColoredText( vgui::HFont hFont, int x, int y, unsigned int color, const char *pAnsiText );
		void DrawCopyStats();
		bool ActivateBackgroundEffects();

		void PlayGameStartupSound();

		static CBaseModPanel* m_CFactoryBasePanel;

		vgui::DHANDLE< CBaseModFrame > m_Frames[WT_WINDOW_COUNT];
		vgui::DHANDLE< CBaseModFooterPanel > m_FooterPanel;
		WINDOW_TYPE m_ActiveWindow[WPRI_COUNT];
		vgui::HScheme m_UIScheme;
		int m_lastActiveUserId;

		vgui::HFont m_hDefaultFont;

		int	m_iBackgroundImageID;
		int	m_iFadeToBackgroundImageID;
		float m_flMovieFadeInTime;

		int m_DelayActivation;
		int m_ExitingFrameCount;
		bool m_bWarmRestartMode;
		bool m_bClosingAllWindows;

		float m_flBlurScale;
		float m_flLastBlurTime;

		int m_iProductImageID;
		int m_nProductImageWide;
		int m_nProductImageTall;

		char m_szFadeFilename[ MAX_PATH ];
		IMaterial *m_pBackgroundMaterial;
		KeyValues *m_pVMTKeyValues;

		int m_iPlayGameStartupSound;

		void PrepareStartupGraphic();
		void ReleaseStartupGraphic();
		void DrawStartupGraphic( float flNormalizedAlpha );
		IVTFTexture			*m_pBackgroundTexture;

		vgui::CVideoBackground		*m_pVideo;

	};
};

#endif
