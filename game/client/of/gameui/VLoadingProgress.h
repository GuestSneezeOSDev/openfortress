//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VLOADINGPROGRESS_H__
#define __VLOADINGPROGRESS_H__

#include "cbase.h"
#include "basemodui.h"
#include "loadingtippanel.h"

namespace BaseModUI {

class LoadingProgress : public CBaseModFrame, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( LoadingProgress, CBaseModFrame );

public:
	enum LoadingType
	{
		LT_UNDEFINED = 0,
		LT_MAINMENU,
		LT_TRANSITION,
		LT_POSTER,
	};

	enum LoadingWindowType
	{
		LWT_LOADINGPLAQUE,
		LWT_BKGNDSCREEN,
	};

#define	NUM_LOADING_CHARACTERS	4

public:
	LoadingProgress( vgui::Panel *parent, const char *panelName, LoadingWindowType eLoadingType );
	~LoadingProgress();

	virtual void		Close();

	void				SetStatusText(const char *statusText);
	void				SetProgress( float progress );
	float				GetProgress();

	void				SetLoadingType( LoadingType loadingType );
	LoadingType			GetLoadingType();

	bool				IsDrawingProgressBar( void ) { return m_bDrawProgress; }
	
	MESSAGE_FUNC( OnActivate, "activate" );
	MESSAGE_FUNC( OnDeactivate, "deactivate" );
	
	//	Sets loading info like map name, gamemode and similar
	void 				FireGameEvent( IGameEvent *event );

	void				FetchAndSetStatus();
	void				FetchAndSetProgressBar();
	void				CloseIfDisconnected();

protected:
	void				FetchLoadingInfo();
	virtual void		OnThink();
	virtual void		OnCommand(const char *command);
	virtual void		ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void		PaintBackground();

private:
	void				SetupControlStates( void );
	void				SetupPoster( void );
	void				UpdateWorkingAnim();
	void				SetMapBackgroundImage(bool bWidescreen, const char *pMapName);

private:
	bool				m_bActive;

	vgui::ProgressBar	*m_pProTotalProgress;
	vgui::ImagePanel	*m_pWorkingAnim;
	vgui::ImagePanel	*m_pBGImage;
	vgui::ImagePanel	*m_pPoster; 
	vgui::EditablePanel *m_pFooter;
	vgui::Button		*m_pCancelButton;
	LoadingType			m_LoadingType;
	LoadingWindowType	m_LoadingWindowType;

	bool				m_bFullscreenPoster;
	bool				m_bLoadedFully;

	// Poster Data
	KeyValues			*m_pDefaultPosterDataKV;
	bool				m_bValid;

	int					m_textureID_LoadingBar;
	int					m_textureID_LoadingBarBG;
	int					m_textureID_DefaultPosterImage;

	bool				m_bDrawBackground;
	bool				m_bDrawPoster;
	bool				m_bDrawProgress;
	bool				m_bDrawSpinner;

	float				m_flPeakProgress;

	float				m_flLastEngineTime;

	CLoadingTipPanel	*m_pTipPanel;

	vgui::VPANEL				m_nLoadingInfoRef;
	vgui::VPANEL				m_nLoadingProgressRef;

	vgui::ProgressBar			*m_pOriginalProgressBar;
	vgui::Label					*m_pOriginalLabel;

};

};

#endif // __VLOADINGPROGRESS_H__