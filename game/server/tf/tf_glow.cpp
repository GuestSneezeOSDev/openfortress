//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Glow effect
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CTFGlow - tf_glow
- m_iMode (Offset 856) (Save|Key)(4 Bytes) - Mode
- m_glowColor (Offset 860) (Save|Key)(4 Bytes) - GlowColor
- m_bDisabled (Offset 864) (Save|Key)(1 Bytes) - StartDisabled
- InputEnable (Offset 0) (Input)(0 Bytes) - Enable
- InputDisable (Offset 0) (Input)(0 Bytes) - Disable
- InputSetGlowColor (Offset 0) (Input)(0 Bytes) - SetGlowColor
*/

#include "cbase.h"
#include "baseanimating.h"
#include "tf_glow.h"

#include "tier0/memdbgon.h"
 
//-----------------------------------------------------------------------------
// Purpose: Glow spawner
//-----------------------------------------------------------------------------

BEGIN_DATADESC(CTFGlow)
	DEFINE_FIELD( m_hglowEntity,	FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_glowColor, FIELD_COLOR32, "glowcolor" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iMode, FIELD_INTEGER, "Mode" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFGlow, DT_CTFGlow )
		SendPropInt( SENDINFO( m_iMode ) ),
		SendPropInt( SENDINFO( m_bDisabled ) ),
		SendPropEHandle(SENDINFO(m_hglowEntity)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_glow, CTFGlow );

CTFGlow::CTFGlow()
{
	m_hglowEntity = NULL;
	m_bDisabled = false;
	m_iMode = 0;
}

void CTFGlow::Spawn( void )
{
	BaseClass::Spawn();

	// this is DIRTY but coloring only wants to work with the m_clrRender keyvalue specifically, and the glow ent doesn't use rendercolor anyway
	m_clrRender = m_glowColor;
}

void CTFGlow::Activate( void ) 
{ 
	BaseClass::Activate();

	if ( !m_bDisabled )
	{
		AddGlow();
	}
}

void CTFGlow::InputSetGlowColor( inputdata_t &inputdata )
{
	color32 clr = inputdata.value.Color32();

	SetRenderColor( clr.r, clr.g, clr.b, clr.a );
}

void CTFGlow::AddGlow( void )
{
	pGlower = gEntList.FindEntityByName( NULL, m_target );

	if ( !pGlower )
	{
		Warning( "tf_glow (%s) doesn't have a valid target!\n", GetDebugName() );
	}

	pProp = dynamic_cast<CBaseAnimating*>( pGlower );

	if ( pProp )
	{
		pProp->SetTransmitState( FL_EDICT_ALWAYS );
		m_hglowEntity = pProp;
	}
}

void CTFGlow::RemoveGlow( void )
{
	m_hglowEntity = NULL;
}


void CTFGlow::InputEnable( inputdata_t &inputdata )
{
	AddGlow();
	m_bDisabled = false;
}

void CTFGlow::InputDisable( inputdata_t &inputdata )
{
	RemoveGlow();
	m_bDisabled = true;
}