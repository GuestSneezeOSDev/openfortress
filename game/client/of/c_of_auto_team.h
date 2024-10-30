#ifndef C_OF_AUTO_TEAM_H
#define C_OF_AUTO_TEAM_H

#ifdef _WIN32
#pragma once
#endif

class C_TFAutoHumanTeam : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_TFAutoHumanTeam, C_BaseEntity);
	DECLARE_CLIENTCLASS();
	
	C_TFAutoHumanTeam();
	~C_TFAutoHumanTeam();
	virtual void Spawn();
	virtual void OnDataChanged( DataUpdateType_t type );
	int GetHumanTeam(){ return m_iAutoTeam; };
	const char *GetResOverride();
private:
	CNetworkVar( int, m_iAutoTeam );
	char m_szTeamResFile[128];
};

extern C_TFAutoHumanTeam *TFAutoTeam();

#endif