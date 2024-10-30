//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "of_announcer_system.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "teamplayroundbased_gamerules.h"

#include "tier0/memdbgon.h"

#ifdef GAME_DLL
BEGIN_DATADESC(CTFAnnouncerSystem)
// Inputs.
DEFINE_KEYFIELD( szAnnouncer , FIELD_SOUNDNAME, "Announcer"), 
DEFINE_KEYFIELD( bForce, FIELD_BOOLEAN, "ForceAnnouncer"), 

// Inputs.
DEFINE_INPUTFUNC( FIELD_STRING, "AnnounceTeam", BroadcastTeamMessage ),
//DEFINE_INPUTFUNC( FIELD_STRING, "AnnounceFFA", BroadcastFFAMessage ),
END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFAnnouncerSystem, DT_AnnouncerSystem )

BEGIN_NETWORK_TABLE( CTFAnnouncerSystem, DT_AnnouncerSystem )
#ifdef GAME_DLL
	SendPropStringT( SENDINFO( szAnnouncer ) ),
	SendPropBool( SENDINFO( bForce ) ),
#else
	RecvPropString( RECVINFO( szAnnouncer ) ),
	RecvPropBool( RECVINFO( bForce ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( of_announcer, CTFAnnouncerSystem );

CTFAnnouncerSystem *g_pAnnouncerEntity = NULL;

CTFAnnouncerSystem* AnnouncerEntity()
{
	return g_pAnnouncerEntity;
}

CTFAnnouncerSystem::CTFAnnouncerSystem()
{
	g_pAnnouncerEntity = this;
}

CTFAnnouncerSystem::~CTFAnnouncerSystem()
{
	if( g_pAnnouncerEntity == this )
		g_pAnnouncerEntity = NULL;
}

#ifdef GAME_DLL
void CTFAnnouncerSystem::BroadcastTeamMessage( inputdata_t &inputdata )
{
	if ( !TeamplayRoundBasedRules() )
		return;
	char sOutputName[MAX_PATH];
	char sTeamName[MAX_PATH];
	char szSound[MAX_PATH];
	Q_strncpy( sOutputName, inputdata.value.String(), sizeof(sOutputName) );
	int iPart = 0;
	int iLastSpace = 0;
	for( int i = 0; i < sizeof(sOutputName); i++ )
	{
		if ( sOutputName[i] != ' ' )
		{
			switch( iPart )
			{
				case 0:
					sTeamName[i] = sOutputName[i];
					break;
				case 1:
					szSound[i-iLastSpace-1] = sOutputName[i];
					break;
			}
		}
		else
		{
			iPart++;
			if ( iPart > 2 )
				break;
			iLastSpace = i;
		}
	}
	int iTeam;
	strlwr(sTeamName);
	if ( !strcmp(sTeamName, "red" ) )
		iTeam = TF_TEAM_RED;
	else if ( !strcmp(sTeamName, "blu" ) || !strcmp(sTeamName, "blue" ) )
		iTeam = TF_TEAM_BLUE;
	else if ( !strcmp(sTeamName, "mercenary" ) || !strcmp(sTeamName, "merc" ) || !strcmp(sTeamName, "dm" ) )
		iTeam = TF_TEAM_MERCENARY;
	else
		iTeam = TEAM_UNASSIGNED;
	DevMsg("Team: %s\n Sound: %s\n",sTeamName,szSound );
	TeamplayRoundBasedRules()->BroadcastSound( iTeam, szSound );
}
/*
void CTFAnnouncerSystem::BroadcastFFAMessage( inputdata_t &inputdata )
{
	if ( !TeamplayRoundBasedRules() )
		return;
	char sOutputName[MAX_PATH];
	char sTeamName[MAX_PATH];
	char szSound[MAX_PATH];
	Q_strncpy( sOutputName, inputdata.value.String(), sizeof(sOutputName) );
	int iPart = 0;
	int iLastSpace = 0;
	for( int i = 0; i < sizeof(sOutputName); i++ )
	{
		if ( sOutputName[i] != ' ' )
		{
			switch( iPart )
			{
				case 0:
					sTeamName[i] = sOutputName[i];
					break;
				case 1:
					szSound[i-iLastSpace] = sOutputName[i];
					break;
			}
		}
		else
		{
			iPart++;
			iLastSpace = i;
		}
	}
	int iTeam;
	strlwr(sTeamName);
	if ( !strcmp(sTeamName, "red" ) )
		iTeam = TF_TEAM_RED;
	else if ( !strcmp(sTeamName, "blu" ) || !strcmp(sTeamName, "blue" ) )
		iTeam = TF_TEAM_BLUE;
	else if ( !strcmp(sTeamName, "mercenary" ) || !strcmp(sTeamName, "merc" ) || !strcmp(sTeamName, "dm" ) )
		iTeam = TF_TEAM_MERCENARY;
	else
		iTeam = TEAM_UNASSIGNED;
	TeamplayRoundBasedRules()->BroadcastSound( iTeam, szSound );
}
*/
#endif