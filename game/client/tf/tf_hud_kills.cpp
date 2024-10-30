//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "tf_controls.h"
#include "in_buttons.h"
#include "tf_imagepanel.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "ihudlcd.h"
#include "tf_hud_kills.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "multiplay_gamerules.h"

using namespace vgui;

extern ConVar fraglimit;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar of_disablekillcount( "of_disablekillcount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable killcount showing in your HUD" );

DECLARE_HUDELEMENT( CTFHudKills );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudKills::CTFHudKills( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudKills" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(kills)", "0" );

	m_nKills	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudKills.res" );

	m_pKills = dynamic_cast<CExLabel *>( FindChildByName( "Kills" ) );
	m_pKillsShadow = dynamic_cast<CExLabel *>( FindChildByName( "KillsShadow" ) );

	m_nKills	= -1;
	m_flNextThink = 0.0f;

	UpdateKillLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudKills::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || of_disablekillcount.GetBool() )
	{
		return false;
	}
	if ( TFGameRules() && ( TFGameRules()->IsDMGamemode() || TFGameRules()->IsGGGamemode() ) && !TFGameRules()->DontCountKills() )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::UpdateKillLabel( bool bKills )
{
	if ( m_pKills && m_pKillsShadow )
	{
		if ( m_pKills->IsVisible() != bKills )
		{
			m_pKills->SetVisible( bKills );
			m_pKillsShadow->SetVisible( bKills );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudKills::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(kills)", "n/a" );

			// turn off our ammo counts
			UpdateKillLabel( false );

			m_nKills = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int iIndex = GetLocalPlayerIndex();
			int nKills = tf_PR->GetPlayerScore( iIndex );
			int nGGLevel = tf_PR->GetGGLevel( iIndex );
			
			hudlcd->SetGlobalStat( "(kills)", VarArgs( "%d", nKills ) );
			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
				m_nKills = nGGLevel;
			else
				m_nKills = nKills;
			
			UpdateKillLabel( true );
			SetDialogVariable( "Kills",m_nKills );
			wchar_t string1[1024];
			
			
			
			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
			{
				SetDialogVariable( "FragLimit", TFGameRules()->m_iMaxLevel  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_GGLevel" ), 1, 1 );
			}
			else
			{
				SetDialogVariable( "FragLimit", fraglimit.GetInt()  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Kills" ), 1, 1 );
			}
			SetDialogVariable( "killslabel", string1 );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
