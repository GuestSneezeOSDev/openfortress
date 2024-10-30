//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf/tf_shareddefs.h"
#include "entity_tfstart.h"
#include "team_control_point.h"
#include "team_control_point_round.h"
#include "team_objectiveresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTFTeamSpawn tables.
//
BEGIN_DATADESC( CTFTeamSpawn )

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_iszRoundBlueSpawn, FIELD_STRING, "round_bluespawn" ),
	DEFINE_KEYFIELD( m_iszRoundRedSpawn, FIELD_STRING, "round_redspawn" ),

	// backwards compatibility for live tf2
	DEFINE_KEYFIELD( m_nMatchSummaryType, FIELD_INTEGER, "MatchSummary" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	// Outputs.

END_DATADESC()

LINK_ENTITY_TO_CLASS( info_player_teamspawn, CTFTeamSpawn );

IMPLEMENT_AUTO_LIST( ITFTeamSpawnAutoList )

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFTeamSpawn::CTFTeamSpawn()
{
	m_bDisabled = false;

	// all are set to false first, and then spawnflags re-enable them (otherwise, you could have civilians/mercenaries spawning in all spawns...)
	m_bScout = false;
	m_bSniper = false;
	m_bSoldier = false;
	m_bDemoman = false;
	m_bMedic = false;
	m_bHeavyweapons = false;
	m_bPyro = false;
	m_bSpy = false;
	m_bEngineer = false;

	// this breaks DM maps so nope
	//m_bMercenary = false;
	m_bCivilian = false;
	m_bJuggernaut = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamSpawn::Spawn( void )
{
	BaseClass::Spawn();

	// don't run these checks if the flags don't exist or equal 0
	if ( !m_spawnflags || m_spawnflags <= 0 )
	{
		m_bScout = true;
		m_bSniper = true;
		m_bSoldier = true;
		m_bDemoman = true;
		m_bMedic = true;
		m_bHeavyweapons = true;
		m_bPyro = true;
		m_bSpy = true;
		m_bEngineer = true;
		//m_bMercenary = true;
		m_bCivilian = true;
		m_bJuggernaut = true;
	}
	else
	{
		if ( HasSpawnFlags( SF_CLASS_SCOUT ) )
		{
			m_bScout = true;
		}
		if ( HasSpawnFlags( SF_CLASS_SNIPER ) )
		{
			m_bSniper = true;
		}
		if ( HasSpawnFlags( SF_CLASS_SOLDIER ) )
		{
			m_bSoldier = true;
		}
		if ( HasSpawnFlags( SF_CLASS_DEMOMAN ) )
		{
			m_bDemoman = true;
		}
		if ( HasSpawnFlags( SF_CLASS_MEDIC ) )
		{
			m_bMedic = true;
		}
		if ( HasSpawnFlags( SF_CLASS_HEAVYWEAPONS ) )
		{
			m_bHeavyweapons = true;
		}
		if ( HasSpawnFlags( SF_CLASS_PYRO ) )
		{
			m_bPyro = true;
		}
		if ( HasSpawnFlags( SF_CLASS_SPY ) )
		{
			m_bSpy = true;
		}
		if ( HasSpawnFlags( SF_CLASS_ENGINEER ) )
		{
			m_bEngineer = true;
		}
		//if ( HasSpawnFlags( SF_CLASS_MERCENARY ) )
		//{
		//	m_bMercenary = true;
		//}
		if ( HasSpawnFlags( SF_CLASS_CIVILIAN ) )
		{
			m_bCivilian = true;
		}
		if ( HasSpawnFlags( SF_CLASS_JUGGERNAUT ) )
		{
			m_bJuggernaut = true;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamSpawn::Activate( void )
{
	BaseClass::Activate();

	Vector mins = g_pGameRules->GetViewVectors()->m_vHullMin;
	Vector maxs = g_pGameRules->GetViewVectors()->m_vHullMax;

	trace_t trace;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	bool bClear = ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
	if ( !bClear )
	{
		// Making this not spam server consoles, sorry not sorry.
		//Warning("Spawnpoint at (%.2f %.2f %.2f) is not clear.\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		DevWarning(1, "Spawnpoint at (%.2f %.2f %.2f) is not clear.\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		//m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTFTeamSpawn::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"TeamNumber: %d", GetTeamNumber() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 teamcolor = g_aTeamColors[ GetTeamNumber() ];
		teamcolor.a = 0;

		if ( m_bDisabled )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"DISABLED" );
			EntityText(text_offset,tempstr,0);
			text_offset++;

			teamcolor.a = 255;
		}

		// Make sure it's empty
		Vector mins = g_pGameRules->GetViewVectors()->m_vHullMin;
		Vector maxs = g_pGameRules->GetViewVectors()->m_vHullMax;

		Vector vTestMins = GetAbsOrigin() + mins;
		Vector vTestMaxs = GetAbsOrigin() + maxs;

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( NULL, vTestMins, vTestMaxs ) )
		{
			NDebugOverlay::Box( GetAbsOrigin(), mins, maxs, teamcolor.r, teamcolor.g, teamcolor.b, teamcolor.a, 0.1);
		}
		else
		{
			NDebugOverlay::Box( GetAbsOrigin(), mins, maxs, 0,255,0, 0, 0.1);
		}

		if ( m_hControlPoint )
		{
			NDebugOverlay::Line( GetAbsOrigin(), m_hControlPoint->GetAbsOrigin(), teamcolor.r, teamcolor.g, teamcolor.b, false, 0.1 );
		}
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputRoundSpawn( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hControlPoint )
		{
			Warning("%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}

	if ( m_iszRoundBlueSpawn != NULL_STRING )
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundBlueSpawn = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, m_iszRoundBlueSpawn ) );
		if ( !m_hRoundBlueSpawn )
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundBlueSpawn) );
		}
	}

	if ( m_iszRoundRedSpawn != NULL_STRING )
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundRedSpawn = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, m_iszRoundRedSpawn ) );
		if ( !m_hRoundRedSpawn )
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundRedSpawn) );
		}
	}
}
