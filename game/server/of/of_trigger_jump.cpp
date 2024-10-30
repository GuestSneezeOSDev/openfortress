#include "cbase.h"
#include "of_trigger_jump.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( COFDTriggerJump )
	DEFINE_KEYFIELD( m_szTarget, FIELD_STRING, "target" ),
	DEFINE_KEYFIELD( m_iSound, FIELD_INTEGER, "do_bounce_sound" ),
	DEFINE_KEYFIELD( m_iSoftLanding, FIELD_INTEGER, "soft_landing" ),
	DEFINE_KEYFIELD( m_bNoCompensation, FIELD_BOOLEAN, "no_compensation" ),
	DEFINE_KEYFIELD( m_bNoAirControl, FIELD_BOOLEAN, "no_aircontrol" ),
	DEFINE_OUTPUT( m_OnJump, "OnJump" ), 
	DEFINE_KEYFIELD( m_szLaunchTarget, FIELD_STRING, "launchTarget" ), // TEMP: for trigger_catapult compatibility
    DEFINE_KEYFIELD( m_bDontReduceBackwardsAccel, FIELD_BOOLEAN, "dont_reducebackwards" ),
	// mappers keep overusing this and its stupid unfun, make them verify they want to use it
	DEFINE_KEYFIELD( m_bImSureAboutNoAirControl, FIELD_BOOLEAN, "no_aircontrol_imsureaboutthis" ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( COFDTriggerJump, DT_OFDTriggerJump )
	SendPropVector( SENDINFO( m_vecTarget ), 0, SPROP_COORD ),
	SendPropInt( SENDINFO( m_iSound ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bNoCompensation ) ),
	SendPropBool( SENDINFO( m_bNoAirControl ) ),
    SendPropBool( SENDINFO( m_bDontReduceBackwardsAccel) ),
	SendPropBool( SENDINFO( m_bImSureAboutNoAirControl ) ),

END_SEND_TABLE()
 
LINK_ENTITY_TO_CLASS( ofd_trigger_jump, COFDTriggerJump );
LINK_ENTITY_TO_CLASS( trigger_catapult, COFDTriggerJump );

IMPLEMENT_AUTO_LIST( IOFDTriggerJumpAutoList );

COFDTriggerJump::COFDTriggerJump(void)
{
	m_pTarget = NULL;
	m_vecTarget = vec3_origin;
	m_iSoftLanding = 0;
	m_bNoCompensation = false;
}

void COFDTriggerJump::Precache(void)
{
	PrecacheScriptSound("JumpPadSound");
}

void COFDTriggerJump::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	m_pTarget = gEntList.FindEntityByName( 0, m_szTarget );

	if ( !m_pTarget )
		m_pTarget = gEntList.FindEntityByName( 0, m_szLaunchTarget ); // trigger_catapult compatibility

	// temp
	m_vecTarget = m_pTarget->GetAbsOrigin();

	//SetTouch( &COFDTriggerJump::OnTouched );
	SetNextThink( TICK_NEVER_THINK );

	//m_bClientSidePredicted = true;
	SetTransmitState( FL_EDICT_PVSCHECK );
}
