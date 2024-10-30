//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_IMAGE_MOUSE_OVER_BUTTON_H
#define TF_IMAGE_MOUSE_OVER_BUTTON_H

#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "imagemouseoverbutton.h"
#include "GameEventListener.h"

//===============================================
// CTFImageMouseOverButton - used for class images
//===============================================

template <class T>
class CTFImageMouseOverButton : public CImageMouseOverButton<T>, public CGameEventListener
{
private:
	//DECLARE_CLASS_SIMPLE( CTFImageMouseOverButton, CImageMouseOverButton );

public:
	CTFImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel, vgui::Panel *ptemplateparent = NULL );

	virtual void ApplySettings( KeyValues *inResourceData );

	void UpdateButtonImages( void );

	virtual void FireGameEvent( IGameEvent * event );

private:

	char m_szTeamActiveImageName[TF_TEAM_COUNT][MAX_BG_LENGTH];
	char m_szTeamInactiveImageName[TF_TEAM_COUNT][MAX_BG_LENGTH];
	
	int		m_iBGTeam;
	
	vgui::Panel *pTmpParent;
};

template <class T>
CTFImageMouseOverButton<T>::CTFImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel, vgui::Panel *ptemplateparent ) :
	CImageMouseOverButton<T>( parent, panelName, templatePanel )
{
	pTmpParent = NULL;

	if( ptemplateparent != NULL )
		pTmpParent = ptemplateparent;
	
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_szTeamActiveImageName[i][0] = '\0';
		m_szTeamInactiveImageName[i][0] = '\0';
	}

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;

	ListenForGameEvent( "localplayer_changeteam" );
}

template <class T>
void CTFImageMouseOverButton<T>::ApplySettings( KeyValues *inResourceData )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		Q_strncpy( m_szTeamActiveImageName[i], inResourceData->GetString( VarArgs("teamactiveimage_%d", i), "" ), sizeof( m_szTeamActiveImageName[i] ) );
		Q_strncpy( m_szTeamInactiveImageName[i], inResourceData->GetString( VarArgs("teaminactiveimage_%d", i), "" ), sizeof( m_szTeamInactiveImageName[i] ) );
	}

	CImageMouseOverButton<T>::ApplySettings( inResourceData );
	
	if( pTmpParent && MouseOverButton<T>::GetClassPanel() )
		MouseOverButton<T>::GetClassPanel()->SetParent(pTmpParent);
	
	UpdateButtonImages();
}

template <class T>
void CTFImageMouseOverButton<T>::UpdateButtonImages( void )
{
	// Reset our images
	if ( m_iBGTeam >= 0 && m_iBGTeam < TF_TEAM_COUNT )
	{
		if ( m_szTeamActiveImageName[m_iBGTeam] && m_szTeamActiveImageName[m_iBGTeam][0] )
		{
			this->SetActiveImage( m_szTeamActiveImageName[m_iBGTeam] );
			this->SetActiveImage( vgui::scheme()->GetImage( m_szTeamActiveImageName[m_iBGTeam], CImageMouseOverButton<T>::GetScaleImage() ) );
		}
		if ( m_szTeamInactiveImageName[m_iBGTeam] && m_szTeamInactiveImageName[m_iBGTeam][0] )
		{
			this->SetInactiveImage( m_szTeamInactiveImageName[m_iBGTeam] );
			this->SetInactiveImage( vgui::scheme()->GetImage( m_szTeamInactiveImageName[m_iBGTeam], CImageMouseOverButton<T>::GetScaleImage() ) );
		}
	}
}

template <class T>
void CTFImageMouseOverButton<T>::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
		UpdateButtonImages();
	}
}

#endif //TF_IMAGE_MOUSE_OVER_BUTTON_H