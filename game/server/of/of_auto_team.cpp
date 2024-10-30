// ========= Copyright Open Fortress Developers, CC-BY-NC-SA ============
// Purpose: Auto assigns human players to a specific team
// Author(s): KaydemonLP
// Note: 
//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "of_auto_team.h"

BEGIN_DATADESC(CTFAutoHumanTeam)
DEFINE_KEYFIELD( m_iAutoTeam, FIELD_INTEGER, "AutoTeam"	),
DEFINE_KEYFIELD( m_szTeamResFile, FIELD_STRING, "TeamResFiles"	),
DEFINE_KEYFIELD( m_bExcludeBots, FIELD_BOOLEAN, "ExcludeAllBots" ),
DEFINE_KEYFIELD( m_bDisableSpectating, FIELD_BOOLEAN, "DisableSpectating" ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFAutoHumanTeam, DT_AutoHumanTeam )
	SendPropInt( SENDINFO( m_iAutoTeam ) ),
	SendPropStringT( SENDINFO( m_szTeamResFile ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( of_auto_team, CTFAutoHumanTeam );

CTFAutoHumanTeam *g_AutoTeam = NULL;
CTFAutoHumanTeam *TFAutoTeam()
{
	return g_AutoTeam;
}

CTFAutoHumanTeam::CTFAutoHumanTeam()
{
	m_iAutoTeam = TF_TEAM_RED;
	m_bExcludeBots = false;
	m_bDisableSpectating = false;
	m_szTeamResFile = MAKE_STRING((char*)'\0');
}

void CTFAutoHumanTeam::Spawn( void )
{
	g_AutoTeam = this;
}

CTFAutoHumanTeam::~CTFAutoHumanTeam()
{
	g_AutoTeam = NULL;
}