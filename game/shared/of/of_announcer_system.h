//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_MUSIC_PLAYER_H
#define TF_MUSIC_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFAnnouncerSystem C_TFAnnouncerSystem
#endif

class CTFAnnouncerSystem : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFAnnouncerSystem, CBaseEntity);
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
	DECLARE_NETWORKCLASS(); 
	
	CTFAnnouncerSystem();
	~CTFAnnouncerSystem();
	CNetworkVar( bool, bForce );
#ifdef GAME_DLL
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}	

	// Input handlers
	void			BroadcastTeamMessage( inputdata_t &inputdata );
//	void			BroadcastFFAMessage( inputdata_t &inputdata );

	CNetworkVar(string_t, szAnnouncer);
#else
	char szAnnouncer[MAX_PATH];
#endif
};

extern CTFAnnouncerSystem* AnnouncerEntity();

#endif // TF_MUSIC_PLAYER_H