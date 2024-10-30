//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Backwards compatibility for TF2's base_boss entity (not accurate but gets the job done)
//
//=============================================================================//

#ifndef CTFBaseBoss_H
#define CTFBaseBoss_H
#ifdef _WIN32
#pragma once
#endif

//==================================================================================================================
// Base Boss
//==================================================================================================================
class CTFBaseBoss : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CTFBaseBoss, CAI_BaseActor );
	DECLARE_DATADESC();

	CTFBaseBoss();

	void	Spawn( void );
	void	Precache( void );

	bool		m_startDisabled;

	CTFBaseBoss *	GetEntity() { return this; }

	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetMaxHealth( inputdata_t &inputdata );
	void InputAddHealth( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );

	COutputEvent m_outputOnKilled;
	COutputEvent m_outputOnHealthBelow90Percent;
	COutputEvent m_outputOnHealthBelow80Percent;
	COutputEvent m_outputOnHealthBelow70Percent;
	COutputEvent m_outputOnHealthBelow60Percent;
	COutputEvent m_outputOnHealthBelow50Percent;
	COutputEvent m_outputOnHealthBelow40Percent;
	COutputEvent m_outputOnHealthBelow30Percent;
	COutputEvent m_outputOnHealthBelow20Percent;
	COutputEvent m_outputOnHealthBelow10Percent;
};

#endif // CTFBaseBoss_H
