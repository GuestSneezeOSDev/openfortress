// ========= Copyright Open Fortress Developers, CC-BY-NC-SA ============
// Purpose: Auto assigns human players to a specific team
// Author(s): KaydemonLP
// Note: 
//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "c_of_auto_team.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFAutoHumanTeam, DT_AutoHumanTeam, CTFAutoHumanTeam )
	RecvPropInt( RECVINFO( m_iAutoTeam ) ),
	RecvPropString( RECVINFO( m_szTeamResFile ) ),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( of_auto_team, C_TFAutoHumanTeam );

C_TFAutoHumanTeam *g_AutoTeam = NULL;
C_TFAutoHumanTeam *TFAutoTeam()
{
	return g_AutoTeam;
}

C_TFAutoHumanTeam::C_TFAutoHumanTeam()
{
	m_szTeamResFile[0] = '\0';
	m_iAutoTeam = TF_TEAM_RED;
}

void C_TFAutoHumanTeam::Spawn( void )
{
	g_AutoTeam = this;
}

void C_TFAutoHumanTeam::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		
	}
}

const char *C_TFAutoHumanTeam::GetResOverride()
{
	if( m_szTeamResFile[0] != '\0' )
		return m_szTeamResFile;
	else
		return NULL;
}

C_TFAutoHumanTeam::~C_TFAutoHumanTeam()
{
	g_AutoTeam = NULL;
}