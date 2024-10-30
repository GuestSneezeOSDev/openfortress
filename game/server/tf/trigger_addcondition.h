//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TRIGGER_ADDCONDITION_H
#define TRIGGER_ADDCONDITION_H
#ifdef _WIN32
#pragma once
#endif

class CTriggerAddTFPlayerCondition : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerAddTFPlayerCondition, CBaseTrigger );

public:
	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );

private:
	DECLARE_DATADESC();

	int m_nCondition;
	float m_flDuration;
};

class CTriggerRemoveTFPlayerCondition : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerRemoveTFPlayerCondition, CBaseTrigger );

public:
	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );

private:
	DECLARE_DATADESC();

	int m_nCondition;
};

#endif // TRIGGER_ADDCONDITION_H
