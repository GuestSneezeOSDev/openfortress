//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFImagePanel );

extern ConVar of_coloredhud;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFImagePanel::CTFImagePanel( Panel *parent, const char *name ) : ImagePanel( parent, name )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_szTeamBG[i][0] = '\0';
	}

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;

	m_iPlayerIndex = -1;
	pCompensate = new ImagePanel( this, "CompensatePanel" );
	
	ListenForGameEvent( "localplayer_changeteam" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::ApplySettings( KeyValues *inResourceData )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		Q_strncpy( m_szTeamBG[i], inResourceData->GetString( VarArgs("teambg_%d", i), "" ), sizeof( m_szTeamBG[i] ) );
	}
	
	Q_strncpy( m_szCompensateBG, inResourceData->GetString( "image_compensate" ), sizeof( m_szCompensateBG ) );

	BaseClass::ApplySettings( inResourceData );

	inResourceData->SetInt("zpos", -1);
	pCompensate->ApplySettings( inResourceData );
	pCompensate->SetName("compensate");
	pCompensate->SetVisible(false);
	pCompensate->SetPos(0,0);
	
	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::UpdateBGImage(void)
{
	if ( m_iBGTeam >= 0 && m_iBGTeam < TF_TEAM_COUNT )
	{
		if ( m_szTeamBG[m_iBGTeam] && m_szTeamBG[m_iBGTeam][0] )
		{
			if( m_iPlayerIndex > -1 )
			{
				pCompensate->SetImage( m_szCompensateBG );
				pCompensate->SetVisible(true);
				pCompensate->SetPos(0,0);
				SetImage(m_szTeamBG[m_iBGTeam]);
			}
			else
				SetImage( m_szTeamBG[m_iBGTeam] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
		UpdateBGImage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTFImagePanel::GetDrawColor( void )
{
	Color tempColor = GetFgColor();
	tempColor[3] = GetAlpha();
	
	if( m_iPlayerIndex > -1 )
	{
		C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
		if( tf_PR )
			return tf_PR->GetPlayerColor(m_iPlayerIndex);
	}
	return tempColor;
}

void CTFImagePanel::SetPlayerOwner( int iPlayerIndex )
{
	m_iPlayerIndex = iPlayerIndex;

	UpdateBGImage();
	
	pCompensate->SetPos(0,0);
	pCompensate->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Callback for when the panel size has been changed
//-----------------------------------------------------------------------------
void CTFImagePanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();

	int x, y, w, h;
	pCompensate->GetBounds( x, y, w, h );

	int px, py;
	pCompensate->GetPinOffset( px, py );

	int ox, oy;
	pCompensate->GetResizeOffset( ox, oy );

	int ex;
	int ey;

	bool bResizeHoriz = true;
	bool bResizeVert = true;

	PinCorner_e pinCorner = pCompensate->GetPinCorner();
	if ( pinCorner == PIN_TOPRIGHT || pinCorner == PIN_BOTTOMRIGHT )
	{
		// move along with the right edge
		ex = wide + px;
		x = bResizeHoriz ? ox : ex - w;
	}
	else
	{
		x = px;
		ex = bResizeHoriz ? wide + ox : px + w;
	}

	if ( pinCorner == PIN_BOTTOMLEFT || pinCorner == PIN_BOTTOMRIGHT )
	{
		// move along with the right edge
		ey = tall + py;
		y = bResizeVert ? oy : ey - h;
	}
	else
	{
		y = py;
		ey = bResizeVert ? tall + oy : py + h;
	}

	// Clamp..
	if ( ex < x )
	{
		ex = x;
	}
	if ( ey < y )
	{
		ey = y;
	}

	pCompensate->SetBounds( x, y, ex - x, ey - y );
	pCompensate->InvalidateLayout();

	Repaint();
}

void CTFImagePanel::PaintBackground()
{
	BaseClass::PaintBackground();
	/*
	OK SO BASICALLY
	All the same material uses the same texture id, meaning all of them change this index at once
	meaning that they all change color at once, so this doesnt work
	ive spent way too much time on this to just remove so this stays here if we get some eureka effect
	and figure out how to get it to work
	if( g_pMatSystemSurface && GetImage() )
	{
		IMaterial *pMat = g_pMatSystemSurface->DrawGetTextureMaterial( GetImage()->GetID() );
		if( pMat )
		{
			IMaterialVar *m_pPlayerIndexVar = NULL;
			bool bSuccess;
			m_pPlayerIndexVar = pMat->FindVar( "$playerindex", &bSuccess );
			if( m_pPlayerIndexVar && bSuccess )
			{
				DevMsg("Assigned value %d\n", m_iPlayerIndex);
				m_pPlayerIndexVar->SetIntValue( m_iPlayerIndex );
			}
		}
	}*/
}