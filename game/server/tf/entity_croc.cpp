//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The crocodile model that spawns from a func_croc
//
//=============================================================================//

#include "cbase.h"
#include "entity_croc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CEntityCroc )
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_croc, CEntityCroc );

void CEntityCroc::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound("Crocs.Hiss");
	PrecacheScriptSound("Crocs.JumpBite");
	PrecacheScriptSound("Crocs.JumpOut");
	PrecacheScriptSound("Crocs.JumpIn");
	PrecacheScriptSound("Crocs.Growl");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityCroc::Spawn( void )
{
	const char *szModel = STRING( GetModelName() );

	if (!szModel || !*szModel)
	{
		szModel = CROC_MODEL;
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	InitBoneControllers();

	UseClientSideAnimation();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	BaseClass::Spawn();

	m_bSequenceFinished = false;
	m_flPlaybackRate	= 1.0;

	ResetSequenceInfo();

	SetThink( &CEntityCroc::Think );
	SetNextThink( gpGlobals->curtime + 1.0f );

	int iSequence = LookupSequence( "attack" );

	if ( iSequence >= 0 )
	{
		 SetSequence( iSequence );
		 ResetSequenceInfo();
	}
}

void CEntityCroc::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	if ( m_bSequenceFinished )
		UTIL_Remove( this );	
}