#ifndef C_OFD_TRIGGER_JUMP_H
#define C_OFD_TRIGGER_JUMP_H

class C_OFDTriggerJump : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_OFDTriggerJump, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_OFDTriggerJump();
	~C_OFDTriggerJump();

	virtual void Spawn();
	virtual void StartTouch( CBaseEntity *pOther );

	C_OFDTriggerJump	*m_pNext;
	
protected:
	CountdownTimer m_launchCooldowns[ MAX_PLAYERS + 1 ];
	
	CNetworkVar( Vector, m_vecTarget );
	CNetworkVar( int, m_iSound );
	CNetworkVar( bool, m_bNoCompensation );
	CNetworkVar(bool, m_bNoAirControl);
	CNetworkVar(bool, m_bDontReduceBackwardsAccel);
	CNetworkVar(bool, m_bImSureAboutNoAirControl);

};

extern C_EntityClassList< C_OFDTriggerJump > g_TriggerJumpList;

#endif
