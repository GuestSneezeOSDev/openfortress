//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <vgui/ILocalize.h>
#include "iclientmode.h"
#include "c_team.h"
#include "ihudlcd.h"
#include "of_hud_dom.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

extern ConVar of_dom_scorelimit_multiplier;

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudDOM );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudDOM::CTFHudDOM( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDOM" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );

	hudlcd->SetGlobalStat( "(score)", "0" );
	
	m_nScore	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudDOM.res" );

	m_pScore = dynamic_cast<CExLabel *>( FindChildByName( "Score" ) );
	m_pScoreShadow = dynamic_cast<CExLabel *>( FindChildByName( "ScoreShadow" ) );
	//m_pProgressRed = dynamic_cast<CTFImageProgressBar*>(FindChildByName("RedProgress"));
	//m_pProgressBlu = dynamic_cast<CTFImageProgressBar*>(FindChildByName("BluProgress"));

	m_nScore	= -1;
	m_flNextThink = 0.0f;

	UpdateDOMLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudDOM::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}
	
	if ( TFGameRules() && ( TFGameRules()->IsDOMGamemode() || TFGameRules()->IsESCGamemode() ) )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::UpdateDOMLabel( bool bScore )
{
	if ( m_pScore && m_pScoreShadow )
	{
		if ( m_pScore->IsVisible() != bScore )
		{
			m_pScore->SetVisible( bScore );
			m_pScoreShadow->SetVisible( bScore );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the team frag meter in DOM
//-----------------------------------------------------------------------------
void CTFHudDOM::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(score)", "n/a" );

			// turn off our ammo counts
			UpdateDOMLabel( false );

			m_nScore = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int iIndex = GetLocalPlayerIndex();
			int nScore = tf_PR->GetPlayerScore( iIndex );
			
			hudlcd->SetGlobalStat( "(score)", VarArgs( "%d", nScore ) );

			m_nScore = nScore;
			
			UpdateDOMLabel( true );
			wchar_t string1[1024];

			//C_Team* pRedTeam = GetGlobalTeam(TF_TEAM_RED);
			//C_Team* pBluTeam = GetGlobalTeam(TF_TEAM_BLUE);

			/*
			if (m_pProgressRed)
			{
				m_pProgressRed->SetProgress((float)(pRedTeam->Get_Score()) / (float)(TFGameRules()->m_nDomScore_limit));
				m_pProgressRed->Update();
			}
			if (m_pProgressBlu)
			{
				m_pProgressBlu->SetProgress((float)(pBluTeam->Get_Score()) / (float)(TFGameRules()->m_nDomScore_limit));
				m_pProgressBlu->Update();
			}
			*/

			SetDialogVariable( "RedScore", TFGameRules()->m_nDomScore_red );
			SetDialogVariable( "BluScore", TFGameRules()->m_nDomScore_blue );

            SetDialogVariable( "FragLimit", TFGameRules()->m_nDomScore_limit);
			g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Score" ), 1, 1 );

			SetDialogVariable( "scorelabel", string1 );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
