//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF Spawn Point.
//
//=============================================================================//
#ifndef ENTITY_TFSTART_H
#define ENTITY_TFSTART_H

#ifdef _WIN32
#pragma once
#endif

class CTeamControlPoint;
class CTeamControlPointRound;

// what classes can spawn here
#define SF_CLASS_SCOUT			1
#define SF_CLASS_SNIPER			2
#define SF_CLASS_SOLDIER		4
#define SF_CLASS_DEMOMAN		8
#define SF_CLASS_MEDIC			16
#define SF_CLASS_HEAVYWEAPONS	32
#define SF_CLASS_PYRO			64
#define SF_CLASS_SPY			128	
#define SF_CLASS_ENGINEER		256
#define SF_CLASS_MERCENARY		512
#define SF_CLASS_CIVILIAN		1024
#define SF_CLASS_JUGGERNAUT		2048

//=============================================================================
//
// TF team spawning entity.
//
DECLARE_AUTO_LIST( ITFTeamSpawnAutoList )
class CTFTeamSpawn : public CPointEntity, public ITFTeamSpawnAutoList
{
public:
	DECLARE_CLASS( CTFTeamSpawn, CPointEntity );

	CTFTeamSpawn();

	virtual void Spawn( void );
	virtual void Activate( void );

	bool m_bScout;
	bool m_bSniper;
	bool m_bSoldier;
	bool m_bDemoman;
	bool m_bMedic;
	bool m_bHeavyweapons;
	bool m_bPyro;
	bool m_bSpy;
	bool m_bEngineer;
	//bool m_bMercenary;
	bool m_bCivilian;
	bool m_bJuggernaut;

	bool AllowScout( void ) { return m_bScout; }
	bool AllowSniper( void ) { return m_bSniper; }
	bool AllowSoldier( void ) { return m_bSoldier; }
	bool AllowDemoman( void ) { return m_bDemoman; }
	bool AllowMedic( void ) { return m_bMedic; }
	bool AllowHeavyweapons( void ) { return m_bHeavyweapons; }
	bool AllowPyro( void ) { return m_bPyro; }
	bool AllowSpy( void ) { return m_bSpy; }
	bool AllowEngineer( void ) { return m_bEngineer; }
	//bool AllowMercenary( void ) { return m_bMercenary; }
	bool AllowCivilian( void ) { return m_bCivilian; }
	bool AllowJuggernaut( void ) { return m_bJuggernaut; }

	bool IsDisabled( void ) { return m_bDisabled; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	int GetMatchSummary( void ) { return m_nMatchSummaryType; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );

	int DrawDebugTextOverlays(void);

	CHandle<CTeamControlPoint> GetControlPoint( void ) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundBlueSpawn( void ) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRedSpawn( void ) { return m_hRoundRedSpawn; }
	CHandle<CTeamControlPointRound> GetRoundMercenarySpawn(void) { return m_hRoundMercenarySpawn; }

private:
	bool	m_bDisabled;		// Enabled/Disabled?

	int								m_nMatchSummaryType;

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;

	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundMercenarySpawn;

	DECLARE_DATADESC();
};

#endif // ENTITY_TFSTART_H


