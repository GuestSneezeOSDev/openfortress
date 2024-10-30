#ifndef OF_AUTO_TEAM_H
#define OF_AUTO_TEAM_H

#ifdef _WIN32
#pragma once
#endif

class CTFAutoHumanTeam : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFAutoHumanTeam, CBaseEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	
	CTFAutoHumanTeam();
	~CTFAutoHumanTeam();
	virtual void Spawn();
	int GetHumanTeam(){ return m_iAutoTeam; };
	bool ShouldExcludeBots(){ return m_bExcludeBots; };
	bool AllowSpectator(){ return !m_bDisableSpectating; };
	
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
private:
	CNetworkVar( int, m_iAutoTeam );
	CNetworkVar( bool, m_bExcludeBots );
	CNetworkVar( bool, m_bDisableSpectating );
	CNetworkVar( string_t, m_szTeamResFile );
};

extern CTFAutoHumanTeam *TFAutoTeam();

#endif