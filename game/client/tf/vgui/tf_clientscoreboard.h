//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SCOREBOARD_H
#define TF_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include "tf_hud_playerstatus.h"
#include "clientscoreboarddialog.h"
#include "of_hud_medals.h"

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTFClientScoreBoardDialog : public CClientScoreBoardDialog
{
private:
	DECLARE_CLASS_SIMPLE( CTFClientScoreBoardDialog, CClientScoreBoardDialog );

public:
	CTFClientScoreBoardDialog( IViewPort *pViewPort );
	virtual ~CTFClientScoreBoardDialog();

	virtual void Reset();
	virtual void Update();
	virtual void ShowPanel( bool bShow );

#if defined( _X360 )
	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
#endif
	
protected:
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PostApplySchemeSettings( vgui::IScheme *pScheme ) {};

private:
	void InitPlayerList( vgui::SectionedListPanel *pPlayerList );
	void RemovePlayerList( vgui::SectionedListPanel *pPlayerList );
	void SetPlayerListImages( vgui::SectionedListPanel *pPlayerList );
	void UpdateTeamInfo();
	void UpdatePlayerList();
	void UpdateSpectatorList();
	void UpdatePlayerDetails();
	void ClearPlayerDetails( bool isDM );
	bool ShouldShowAsSpectator( int iPlayerIndex );

	void FormatMedalName(char *medalName);
	void UpdateLabelValue(const char *name, const int value);
	
	virtual void FireGameEvent( IGameEvent *event );

	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );

	vgui::SectionedListPanel *GetSelectedPlayerList( void );

	vgui::SectionedListPanel	*m_pPlayerListBlue;
	vgui::SectionedListPanel	*m_pPlayerListRed;
	vgui::SectionedListPanel	*m_pPlayerListMercenary;
	CExLabel					*m_pLabelPlayerName;
	vgui::ImagePanel			*m_pImagePanelHorizLine;
	CTFClassImage				*m_pClassImage;
	CTFClassImage				*m_pClassImageColorless;

	int							m_iImageDead;
	int							m_iImageDominated;
	int							m_iImageNemesis;
	
	int							m_iClassIcon[TF_CLASS_COUNT_ALL];

	static const char			*szPlayerDetailsLabels[13];
	
	CPanelAnimationVarAliasType( int, m_iStatusWidth, "status_width", "12", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNemesisWidth, "nemesis_width", "20", "proportional_int" );
};

const wchar_t *GetPointsString( int iPoints );

#endif // TF_SCOREBOARD_H
