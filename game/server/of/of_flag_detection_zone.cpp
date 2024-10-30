//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#include "cbase.h"
#include "of_flag_detection_zone.h"
#include "entity_capture_flag.h"
#include "tf_player.h"
#include "filters.h"

//=============================================================================
//
// Flag Detection Zone tables.
//

BEGIN_DATADESC( CFlagDetectionZone )

	DEFINE_KEYFIELD( m_iPlayerFilterName,	FIELD_STRING,	"playerfiltername" ),
	DEFINE_FIELD( m_hPlayerFilter,	FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnStartTouchFlag, "OnStartTouchFlag" ),
	DEFINE_OUTPUT( m_outputOnEndTouchFlag, "OnEndTouchFlag" ),
	DEFINE_OUTPUT( m_outputOnDroppedFlag, "OnDroppedFlag" ),
	DEFINE_OUTPUT( m_outputOnPickedUpFlag, "OnPickedUpFlag" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_flagdetectionzone, CFlagDetectionZone );

IMPLEMENT_AUTO_LIST( IFlagDetectionZoneAutoList );

//=============================================================================
//
// Flag Detection Zone functions.
//

CFlagDetectionZone::CFlagDetectionZone()
{
}

void CFlagDetectionZone::Spawn()
{
	InitTrigger();

	if ( m_bDisabled )
		Disable();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFlagDetectionZone::Activate( void )
{ 
	// Get a handle to my filter entity if there is one
	if( m_iPlayerFilterName != NULL_STRING )
	{
		m_hPlayerFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iPlayerFilterName ));
	}

	BaseClass::Activate();
}

void CFlagDetectionZone::StartTouch( CBaseEntity *pOther )
{
	if ( IsDisabled() )
		return;
	
	if( !pOther->IsPlayer() )
		return;
	
	EHANDLE hOther = pOther;

	if( m_hTouchingPlayers.Find( hOther ) == m_hTouchingPlayers.InvalidIndex() )
		m_hTouchingPlayers.AddToTail( hOther );

	if( IsFlagCarrier( pOther ) )
	{
		bool bNewPlayer = false;
		if ( m_hTouchingEntities.Find( hOther ) == m_hTouchingEntities.InvalidIndex() )
		{
			m_hTouchingEntities.AddToTail( hOther );
			bNewPlayer = true;
		}

		m_OnStartTouch.FireOutput( pOther, this );

		if( bNewPlayer && m_hTouchingEntities.Count() == 1 )
		{
			m_outputOnStartTouchFlag.FireOutput( pOther, this );
			m_OnStartTouchAll.FireOutput( pOther, this );
		}
	}
}

void CFlagDetectionZone::EndTouch( CBaseEntity *pOther )
{
	if ( IsDisabled() )
		return;

	EHANDLE hOther = pOther;

	if( pOther->IsPlayer() )
		m_hTouchingPlayers.FindAndRemove( hOther );

	if( IsTouching( pOther ) )
	{
		m_hTouchingEntities.FindAndRemove( hOther );

		m_OnEndTouch.FireOutput(pOther, this);

		// If there are no more entities touching this trigger, fire the lost all touches
		// Loop through the touching entities backwards. Clean out old ones, and look for existing
		bool bFoundOtherTouchee = false;
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			hOther = m_hTouchingEntities[i];

			if( !hOther )
				m_hTouchingEntities.Remove( i );
			else if( hOther->IsPlayer() && !hOther->IsAlive() )
				m_hTouchingEntities.Remove( i );
			else
				bFoundOtherTouchee = true;
		}

		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		// Didn't find one?
		if ( !bFoundOtherTouchee )
		{
			m_outputOnEndTouchFlag.FireOutput( this, this );
			m_OnEndTouchAll.FireOutput( pOther, this);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
//			Override the default trigger to skip trigger flags
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CFlagDetectionZone::PassesTriggerFilters(CBaseEntity *pOther)
{
	bool bOtherIsPlayer = pOther->IsPlayer();

	if ( bOtherIsPlayer )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)pOther;
		if ( !pPlayer->IsAlive() )
			return false;
	}

	CBaseFilter *pFilter = m_hFilter.Get();
	return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the player filter criteria, false if not.
// Input  : pOther - The entity to be filtered, should be player.
//-----------------------------------------------------------------------------
bool CFlagDetectionZone::PassesPlayerTriggerFilters(CBaseEntity *pOther)
{
	bool bOtherIsPlayer = pOther->IsPlayer();

	if ( bOtherIsPlayer )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)pOther;
		if ( !pPlayer->IsAlive() )
			return false;
	}

	CBaseFilter *pFilter = m_hPlayerFilter.Get();
	return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
}

void CFlagDetectionZone::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

void CFlagDetectionZone::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

void CFlagDetectionZone::FlagDropped( CBasePlayer *pPlayer )
{
	EHANDLE hOther = pPlayer;

	if( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() )
	{
		m_outputOnDroppedFlag.FireOutput( pPlayer, this );
		EndTouch( pPlayer );

		m_hTouchingPlayers.AddToTail( hOther );
	}
}

void CFlagDetectionZone::FlagPickedUp( CBasePlayer *pPlayer )
{
	EHANDLE hOther = pPlayer;

	if( m_hTouchingPlayers.Find( hOther ) != m_hTouchingPlayers.InvalidIndex() )
	{
		m_outputOnPickedUpFlag.FireOutput( pPlayer, this );
		StartTouch( pPlayer );
	}
}

bool CFlagDetectionZone::IsFlagCarrier( CBaseEntity *pEntity )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEntity );
	if( !pPlayer )
		return false;

	if( !PassesPlayerTriggerFilters(pPlayer) )
		return false;

	if( pPlayer->HasItem() && pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		// This should never NOT be a capture flag right?, considering the code ^^^^ above
		CCaptureFlag *pFlag = static_cast<CCaptureFlag*>( pPlayer->GetItem() );
		if ( pFlag && !pFlag->IsCaptured() && PassesTriggerFilters(pFlag) )
			return true;
	}

	return false;
}