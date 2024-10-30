#ifndef FUNC_FLAG_DETECTION_ZONE_H
#define FUNC_FLAG_DETECTION_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//=============================================================================
// Flag detection zone trigger for CTF and related modes
// Mostly a modified copy of CCaptureZone from server/tf/func_capture_zone.cpp
//
DECLARE_AUTO_LIST( IFlagDetectionZoneAutoList );
class CFlagDetectionZone : public CBaseTrigger, public IFlagDetectionZoneAutoList
{
	DECLARE_CLASS( CFlagDetectionZone, CBaseTrigger );

public:

	CFlagDetectionZone();

	void	Spawn();
	void	Activate();
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );
	bool	PassesTriggerFilters(CBaseEntity *pOther);
	bool	PassesPlayerTriggerFilters(CBaseEntity *pOther);

	bool	IsDisabled( void ) { return m_bDisabled; }

	bool	IsFlagCarrier( CBaseEntity *pEntity );

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	void	FlagDropped( CBasePlayer *pPlayer );
	void	FlagPickedUp( CBasePlayer *pPlayer );
private:

	COutputEvent	m_outputOnStartTouchFlag;
	COutputEvent	m_outputOnEndTouchFlag;
	COutputEvent	m_outputOnDroppedFlag;
	COutputEvent	m_outputOnPickedUpFlag;

	// Entities currently being touched by this trigger
	CUtlVector< EHANDLE >	m_hTouchingPlayers;

	string_t	m_iPlayerFilterName;
	CHandle<class CBaseFilter>	m_hPlayerFilter;

	DECLARE_DATADESC();
};

#endif // FUNC_FLAG_DETECTION_ZONE_H