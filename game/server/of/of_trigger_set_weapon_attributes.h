//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TRIGGER_SET_WEAPON_ATTRIBUTES_H
#define TRIGGER_SET_WEAPON_ATTRIBUTES_H
#ifdef _WIN32
#pragma once
#endif

class CTriggerWeaponAttributes : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerWeaponAttributes, CBaseTrigger );

public:
	CTriggerWeaponAttributes();

	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );
	bool	KeyValue( const char *szKeyName, const char *szValue );

private:
	DECLARE_DATADESC();

	bool					m_bReplaceAttributes;
	bool					m_bRemoveAttributes;
	string_t				m_szWeaponName;
	CUtlVector<string_t>	m_hAttributes;
	CUtlVector<string_t>	m_hValues;
};

#endif // TRIGGER_ADDCONDITION_H
