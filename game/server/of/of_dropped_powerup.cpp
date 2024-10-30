//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Dropped Cond Powerup
//
//=============================================================================//


// You're in for a bumpy ride

#include "cbase.h"
#include "of_dropped_powerup.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFDroppedPowerup, DT_DroppedPowerup )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
	SendPropTime( SENDINFO( m_flDespawnTime ) ),
	SendPropTime( SENDINFO( m_flCreationTime ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFDroppedPowerup )
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_dropped_powerup, CTFDroppedPowerup );

PRECACHE_REGISTER( tf_dropped_powerup );

CTFDroppedPowerup::CTFDroppedPowerup()
{
	m_iPowerupID = 0;
	
	szTimerIcon[0] = 0;
	SetTouch( &CTFDroppedPowerup::PackTouch );
}

CTFDroppedPowerup::~CTFDroppedPowerup()
{
	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
	if( pTFPlayer )
	{
		PowerupHandle hHandle;
		hHandle = this;	
		pTFPlayer->m_hPowerups.FindAndRemove( hHandle );
		//StopSound("Powerup.IdleHum");
	}
}

void CTFDroppedPowerup::Spawn( void )
{
	SetModel( STRING( GetModelName() ) );
	//EmitSound("Powerup.IdleHum");
	BaseClass::Spawn();
	SetThink( &CTFDroppedPowerup::FlyThink );
	SetTouch( &CTFDroppedPowerup::PackTouch );
}

void CTFDroppedPowerup::SetInitialVelocity( Vector &vecVelocity ) // Obsolete, because the entity doesn't have physics, left it in since we may use it later on
{ 
	m_vecInitialVelocity = vecVelocity;
}

void CTFDroppedPowerup::FlyThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.01f );
	SetThink( &CTFDroppedPowerup::FlyThink );
}

void CTFDroppedPowerup::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	
	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;	

	CTFPlayer *pTFPlayer = ToTFPlayer( pOther );

	if ( !pTFPlayer )
		return;

	if (pTFPlayer->HasTheFlag() && (m_iPowerupID == TF_COND_INVIS_POWERUP || m_iPowerupID == TF_COND_STEALTHED))
		return;

	if ( pTFPlayer->m_Shared.IsZombie() )
		return;

	bool bSuccess = true;
	
	if ( pTFPlayer->m_Shared.InCond(m_iPowerupID) )  // If we already have this condition, dont pick it up
		return;
	if ( bSuccess )			// If we picked it up
	{
		CSingleUserRecipientFilter filter( pTFPlayer );		// Filter the sound to the person who picked this up
		EmitSound( filter, entindex(), "AmmoPack.Touch" );	// Play the pickup sound
		float flDuration = m_flDespawnTime - gpGlobals->curtime;
		pTFPlayer->m_Shared.AddCond( m_iPowerupID , flDuration ); // Give them the condition
		
		int iRandom = random->RandomInt( 0, 1 );
		pTFPlayer->SpeakConceptIfAllowed( ( iRandom == 1 ) ? MP_CONCEPT_PLAYER_SPELL_PICKUP_RARE : MP_CONCEPT_PLAYER_SPELL_PICKUP_COMMON );
		
		// vecOrigin and vecAngles don't matter,
		// so long as they're reasonable values. (no Inf/NaN)
		Vector vecOrigin(0, 0, 0);
		QAngle vecAngles(0, 0, 0);
		
		CTFDroppedPowerup *pPowerup = static_cast<CTFDroppedPowerup*>( CBaseAnimating::CreateNoSpawn( "tf_dropped_powerup", vecOrigin, vecAngles, pTFPlayer ) );
		if( pPowerup )
		{
			pPowerup->SetModelName( GetModelName() );
			pPowerup->m_nSkin = m_nSkin;
			Q_strncpy( pPowerup->szTimerIcon, szTimerIcon, sizeof( pPowerup->szTimerIcon ) );
			pPowerup->m_iPowerupID = m_iPowerupID;
			pPowerup->m_flCreationTime = m_flCreationTime;
			pPowerup->m_flDespawnTime = m_flDespawnTime;
			pPowerup->SetContextThink( &CBaseEntity::SUB_Remove, m_flDespawnTime, "DieContext" );
		}
		PowerupHandle hHandle;
		hHandle = pPowerup;	
		pTFPlayer->m_hPowerups.AddToTail( hHandle );
		
		IGameEvent *event = gameeventmanager->CreateEvent( "add_powerup_timer" );
		if ( event )
		{
			event->SetInt( "player", pTFPlayer->entindex() );
			event->SetInt( "cond", m_iPowerupID );
			event->SetString( "icon", szTimerIcon );
			gameeventmanager->FireEvent( event );
		}
		
		//StopSound("Powerup.IdleHum");
		UTIL_Remove( this ); // Remove the dropped powerup
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFDroppedPowerup::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}


const char* CTFDroppedPowerup::GetPowerupDroppedLine( void )
{
	switch ( m_iPowerupID )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "CritsDropped";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "InvisibilityDropped";
		break;
		case TF_COND_SHIELD:
		return "ShieldDropped";
		break;
		case TF_COND_HASTE:
		return "HasteDropped";
		break;
		case TF_COND_BERSERK:
		return "BerserkDropped";
		break;
		case TF_COND_JAUGGERNAUGHT:
		return "JauggernaughtDropped"; //change me
		break;
	}
	return "GenericDropped";
}