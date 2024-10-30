//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//=========================================================
// Purpose: Backwards compatibility for TF2's base_boss entity (not accurate but gets the job done)
//=========================================================
#include "cbase.h"
#include "shareddefs.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

#include "base_boss.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTFBaseBoss )	
	DEFINE_KEYFIELD( m_startDisabled, FIELD_BOOLEAN, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxHealth", InputSetMaxHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),	

	DEFINE_OUTPUT( m_outputOnKilled , "OnKilled" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow90Percent , "OnHealthBelow90Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow80Percent , "OnHealthBelow80Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow70Percent , "OnHealthBelow70Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow60Percent , "OnHealthBelow60Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow50Percent , "OnHealthBelow50Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow40Percent , "OnHealthBelow40Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow30Percent , "OnHealthBelow30Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow20Percent , "OnHealthBelow20Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow10Percent , "OnHealthBelow10Percent" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( base_boss, CTFBaseBoss );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseBoss::CTFBaseBoss()
{
	SetMaxHealth( 1000 );
	SetHealth( 1000 );
	m_startDisabled = false;
}


//=========================================================
// Spawn
//=========================================================
void CTFBaseBoss::Spawn()
{
	char *szModel = (char *)STRING( GetModelName() );

	if (!szModel || !*szModel)
	{
		Warning( "base_boss at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	Precache();

	SetModel( STRING( GetModelName() ) );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	SetBloodColor( DONT_BLEED );

	m_flFieldOfView		= 0.0f; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_SCRIPT;
	
	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	NPCInit();

	SetThink( NULL );
	SetNextThink( -1 );

	if ( m_startDisabled )
	{
		AddFlag( EF_NODRAW );
		SetSolid( SOLID_NONE );
	}
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CTFBaseBoss::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
}	

void CTFBaseBoss::InputEnable( inputdata_t &inputdata )
{
	RemoveFlag( EF_NODRAW );
	SetSolid( SOLID_VPHYSICS );
	m_startDisabled = false;
}

void CTFBaseBoss::InputDisable( inputdata_t &inputdata )
{
	AddFlag( EF_NODRAW );
	SetSolid( SOLID_NONE );
	m_startDisabled = true;
}

void CTFBaseBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_outputOnKilled.FireOutput( info.GetAttacker(), this );

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetMaxHealth( inputdata_t &inputdata )
{
	m_iMaxHealth = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: Add health to the NPC
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputAddHealth( inputdata_t &inputdata )
{
	int iHealth = inputdata.value.Int();
	SetHealth( min( GetMaxHealth(), GetHealth() + iHealth ) );
}

//-----------------------------------------------------------------------------
// Purpose: Remove health from the NPC
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputRemoveHealth( inputdata_t &inputdata )
{
	int iDamage = inputdata.value.Int();

	SetHealth( GetHealth() - iDamage );
	if ( GetHealth() <= 0 )
	{
		m_lifeState = LIFE_DEAD;

		CTakeDamageInfo info( inputdata.pCaller, inputdata.pActivator, vec3_origin, GetAbsOrigin(), iDamage, DMG_GENERIC );
		Event_Killed( info );
	}
}

int CTFBaseBoss::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( !BaseClass::OnTakeDamage_Alive( info ) )
		return 0;

	float m_iCurrentHealth = m_iHealth;

	if ( ( m_iMaxHealth * 0.1 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow10Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.2 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow20Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.3 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow30Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.4 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow40Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.5 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow50Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.6 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow60Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.7 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow70Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.8 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow80Percent.FireOutput( info.GetAttacker(), this );
	}
	else if ( ( m_iMaxHealth * 0.9 ) > m_iCurrentHealth )
	{
		m_outputOnHealthBelow90Percent.FireOutput( info.GetAttacker(), this );
	}

	return 1;
}