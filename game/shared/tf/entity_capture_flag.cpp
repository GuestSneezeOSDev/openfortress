//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#include "cbase.h"
#include "entity_capture_flag.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "teamplayroundbased_gamerules.h"

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "hudelement.h"
#include "iclientmode.h"
#include "hud_numericdisplay.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "tf_hud_objectivestatus.h"
#include "view.h"
#include "glow_outline_effect.h"

extern CUtlVector<int>	g_Flags;

ConVar cl_flag_return_size( "cl_flag_return_size", "20", FCVAR_CHEAT );

#else
#include "tf_player.h"
#include "tf_team.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "func_respawnroom.h"
#include "datacache/imdlcache.h"

#include "func_respawnflag.h"
#include "func_bomb_target.h"
#include "func_capture_zone.h"
#include "of_flag_detection_zone.h"

extern ConVar tf_flag_caps_per_round;
extern ConVar of_mctf_flag_caps_per_round;

ConVar cl_flag_return_height( "cl_flag_return_height", "82", FCVAR_CHEAT );
ConVar tf_flag_return_on_touch( "tf_flag_return_on_touch", "1", FCVAR_NOTIFY, "if this is set, remote friendly flags return to your base instantly when you touch them.\n" );
ConVar of_capture_on_away( "of_capture_on_away", "1", FCVAR_NOTIFY, "If this is set to 1, players can capture while their flag is not at base.\n" );

#endif


#ifdef CLIENT_DLL

static void RecvProxy_IsDisabled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCaptureFlag *pFlag = (CCaptureFlag *) pStruct;
	bool bIsDisabled = ( pData->m_Value.m_Int > 0 );

	if ( pFlag )
	{
		pFlag->SetDisabled( bIsDisabled ); 
	}
}

#endif

//=============================================================================
//
// CTF Flag tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlag, DT_CaptureFlag )

BEGIN_NETWORK_TABLE( CCaptureFlag, DT_CaptureFlag )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropInt( SENDINFO( m_nType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nFlagStatus ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flResetTime ) ),
	SendPropTime( SENDINFO( m_flReturnTime ) ),
	SendPropTime( SENDINFO( m_flNeutralTime ) ),
	SendPropTime( SENDINFO( m_flSetNeutralTime ) ),
	SendPropTime( SENDINFO( m_flMaxResetTime ) ),
	SendPropEHandle( SENDINFO( m_hPrevOwner ) ),
#else
	RecvPropInt( RECVINFO( m_bDisabled ), 0, RecvProxy_IsDisabled ),
	RecvPropInt( RECVINFO( m_nType ) ),
	RecvPropInt( RECVINFO( m_nFlagStatus ) ),
	RecvPropTime( RECVINFO( m_flResetTime ) ),
	RecvPropTime( RECVINFO( m_flReturnTime ) ),
	RecvPropTime( RECVINFO( m_flNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flSetNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flMaxResetTime ) ),
	RecvPropEHandle( RECVINFO( m_hPrevOwner ) ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CCaptureFlag )

	// Keyfields.
	DEFINE_KEYFIELD( m_nType, FIELD_INTEGER, "GameType" ),
	DEFINE_KEYFIELD( m_flReturnTime, FIELD_FLOAT, "ReturnTime"),
	DEFINE_KEYFIELD( m_flSetNeutralTime, FIELD_FLOAT, "NeutralTime"),
	DEFINE_KEYFIELD( m_Model, FIELD_STRING, "model"),
	DEFINE_KEYFIELD( m_ParticleBlue, FIELD_STRING, "bluetrail"),
	DEFINE_KEYFIELD( m_ParticleRed, FIELD_STRING, "redtrail"),
	DEFINE_KEYFIELD( m_ParticlePaper, FIELD_STRING, "papertrail"),
#ifdef GAME_DLL

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_outputOnPickUp, "OnPickUp" ),
	DEFINE_OUTPUT( m_outputOnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_outputOnTouchSameTeam, "OnTouchSameTeam" ),
#endif

END_DATADESC();

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

#ifdef GAME_DLL
IMPLEMENT_AUTO_LIST( ICaptureFlagAutoList );
#endif

//=============================================================================
//
// CTF Flag functions.
//

CCaptureFlag::CCaptureFlag()
{
#ifdef CLIENT_DLL
	m_pGlowTrailEffect = NULL;
	m_pPaperTrailEffect = NULL;
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#else
	m_hReturnIcon = NULL;
#endif	
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CCaptureFlag::GetItemID( void )
{
	return TF_ITEM_CAPTURE_FLAG;
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::Precache( void )
{
	PrecacheModel( TF_FLAG_MODEL );
#ifdef GAME_DLL
	if ( m_Model != MAKE_STRING ( "" ) )
#endif
		PrecacheModel( STRING( m_Model ) );

	PrecacheScriptSound( TF_CTF_FLAGSPAWN );	
		
	PrecacheParticleSystem( "player_intel_trail_blue" );
	PrecacheParticleSystem( "player_intel_trail_red" );
	PrecacheParticleSystem( "player_intel_trail_mercenary" );
	PrecacheParticleSystem( "player_intel_papertrail" );
	
	PrecacheParticleSystem( STRING( m_ParticleBlue ) );
	PrecacheParticleSystem( STRING( m_ParticleRed ) );
	PrecacheParticleSystem( STRING( m_ParticleMercenary ) );
	PrecacheParticleSystem( STRING( m_ParticlePaper ) );
}

#ifndef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_nOldFlagStatus = m_nFlagStatus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnDataChanged( DataUpdateType_t updateType )
{
	ClientThink();
	if ( m_nOldFlagStatus != m_nFlagStatus )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "flagstatus_update" );
		if ( pEvent )
		{
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Spawn( void )
{
#ifdef GAME_DLL
	if ( TFGameRules()->IsInfGamemode() )
	{
		UTIL_Remove( this );
		return;
	}
#endif

	// Precache the model and sounds.  Set the flag model.
	Precache();
#ifdef GAME_DLL
	if ( m_Model == MAKE_STRING ( "" ) )
		SetModel( TF_FLAG_MODEL );
	else
#endif
		SetModel( STRING( m_Model ) );

	// Set the flag solid and the size for touching.
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetSize( vec3_origin, vec3_origin );

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 24 );

	// use the initial dynamic prop "m_bStartDisabled" setting to set our own m_bDisabled flag
#ifdef GAME_DLL
	m_bDisabled = m_bStartDisabled;
	m_bStartDisabled = false;

	// glow!
	AddGlowEffect();

	// Don't allow the intelligence to fade.
	m_flFadeScale = 0.0f;
#else
	m_bDisabled = false;
#endif

	// Base class spawn.
	BaseClass::Spawn();

#ifdef GAME_DLL
	// Save the starting position, so we can reset the flag later if need be.
	m_vecResetPos = GetAbsOrigin();
	m_vecResetAng = GetAbsAngles();

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	m_bCaptured = false;

#else

	// add this element if it isn't already in the list
	if ( g_Flags.Find( entindex() ) == -1 )
	{
		g_Flags.AddToTail( entindex() );
	}

#endif

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}
	else
	{
		SetDisabled( false );
	}

	AddFlag(FL_OBJECT); //At least the bot will know what this is.
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::Activate( void )
{
	BaseClass::Activate();

	m_iOriginalTeam = GetTeamNumber();
	m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
	
}

void CCaptureFlag::SetParent( CBaseEntity *pParentEntity, int iAttachment )
{
	// Kind of a hack, but, whenver we parent ourselves, instead of returning to a set position
	// We return to our parent's position + our initial offset to it
	if( pParentEntity && !pParentEntity->IsPlayer() )
	{
		m_vecResetPosOffset = pParentEntity->GetAbsOrigin() - GetAbsOrigin();
		m_vecResetAngOffset = pParentEntity->GetAbsAngles() - GetAbsAngles();
		m_pOriginParent = pParentEntity;
	}

	BaseClass::SetParent( pParentEntity, iAttachment );
}

#endif


//-----------------------------------------------------------------------------
// Purpose: Reset the flag position state.
//-----------------------------------------------------------------------------
void CCaptureFlag::Reset( void )
{
#ifdef GAME_DLL
	// Set the flag position.
	if( m_pOriginParent )
	{
		SetAbsOrigin( m_pOriginParent->GetAbsOrigin() - m_vecResetPosOffset );
		SetAbsAngles( m_pOriginParent->GetAbsAngles() - m_vecResetAngOffset );
	}
	else
	{
		SetAbsOrigin( m_vecResetPos );
		SetAbsAngles( m_vecResetAng );
	}
	// No longer dropped, if it was.
	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		ChangeTeam( m_iOriginalTeam );
		m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
	}

	SetMoveType( MOVETYPE_NONE );
	if( m_pOriginParent )
	{
		SetParent( m_pOriginParent );
	}
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::ResetMessage( void )
{
#ifdef GAME_DLL
	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_ENEMY_RETURNED );
				CTeamRecipientFilter filter( iTeam, true );
				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_RETURNED );
			}
			else
			{
				TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_TEAM_RETURNED );
				CTeamRecipientFilter filter( iTeam, true );
				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_RETURNED );
			}
		}

		// Returned sound
		CPASAttenuationFilter filter( this, TF_CTF_FLAGSPAWN );
		EmitSound( filter, entindex(), TF_CTF_FLAGSPAWN );
		
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_TEAM_RETURNED );
			}
			else
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_ENEMY_RETURNED );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagReturned" );

			CTeamRecipientFilter filter( iTeam, true );
			EmitSound( filter, entindex(), TF_INVADE_FLAG_RETURNED );
		}
	}

	// Output.
	m_outputOnReturn.FireOutput(this, this);

	if ( m_hReturnIcon.Get() )
	{
		UTIL_Remove( m_hReturnIcon );
		m_hReturnIcon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	// special thanks to Pelipoika for reverse engineering parts of this: https://forums.alliedmods.net/archive/index.php/t-294659.html

	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN )
		return;

	// The the touch from a live player.
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;

	// Get the touching player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	// Don't let the person who threw this flag pick it up until it hits the ground.
	// This way we can throw the flag to people, but not touch it as soon as we throw it ourselves
	if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer && m_bAllowOwnerPickup == false )
		return;

	if ( pPlayer->GetTeamNumber() == GetTeamNumber() && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
	{
		m_outputOnTouchSameTeam.FireOutput( this, this );

		if (m_nType == TF_FLAGTYPE_CTF)
		{
			if (m_nFlagStatus == TF_FLAGINFO_DROPPED)
			{
				if (tf_flag_return_on_touch.GetBool())
				{
					Reset();
					ResetMessage();
					//CTF_GameStats.Event_PlayerReturnedFlag( pPlayer );
					return;
				}
			}
			return;
		}

		if ( ( m_nType != TF_FLAGTYPE_CTF || m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION ) && m_nType != TF_FLAGTYPE_INVADE )
		{
			if ( !tf_flag_return_on_touch.GetBool() )
				return;

			if ( m_nFlagStatus != TF_FLAGINFO_NONE )
				return;
		}
	}

	if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND || m_nType == TF_FLAGTYPE_TERRITORY_CONTROL )
	{
		if ( pPlayer->GetTeamNumber() != GetTeamNumber() && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
			return;
	}

	// don't do this in invade anymore
	if ( ( /* m_nType == TF_FLAGTYPE_INVADE || */ m_nType == TF_FLAGTYPE_SPECIAL_DELIVERY  ) && GetTeamNumber() != TEAM_UNASSIGNED )
	{
		if ( pPlayer->GetTeamNumber() != GetTeamNumber() && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
			return;
	}

	if ( !TFGameRules()->IsInWaitingForPlayers()/* && pPlayer->IsAllowedToPickUpFlag()*/ )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			PickUp( pPlayer, true );
			return;
		}
		
		// Is the touching player about to teleport?
		if ( pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
			return;
		
		// Can't pickup flag while invulnerable if not playing player destruction
		if ( pPlayer->m_Shared.InCondUber() && m_nType != TF_FLAGTYPE_PLAYER_DESTRUCTION )
			return;
		
		// Can't pickup flag while invisible
		if ( pPlayer->m_Shared.InCondInvis() )
			return;
		
		// If the player isnt 25% visible don't let them pickup flag
		if ( ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) && pPlayer->m_Shared.GetPercentInvisible() > 0.25 ) )
			return;
		
		// Don't let bonked players pickup the flag
		if ( pPlayer->m_Shared.InCond( TF_COND_PHASE ) )
			return;
		
		// Do not allow the player to pick up multiple flags unless the gamemode is player destruction
		if ( !pPlayer->HasTheFlag()
		|| m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION
		|| m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION
		|| tf_flag_return_on_touch.GetBool() )
		{
			if ( !PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() /*, false*/ ) )
			{
				if ( m_nFlagStatus == TF_FLAGINFO_DROPPED )
				{
					if ( pPlayer->GetTeamNumber() == GetTeamNumber() && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY && m_nType != TF_FLAGTYPE_CTF && tf_flag_return_on_touch.GetBool())
					{
						Reset();
						ResetMessage();
						//CTF_GameStats.Event_PlayerReturnedFlag( pPlayer );
						return;
					}
				}
				
				PickUp( pPlayer, true );
			}
		}
		
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

#ifdef GAME_DLL
	if ( !m_bAllowOwnerPickup )
	{
		if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer ) 
		{
			return;
		}
	}

#endif

	// Call into the base class pickup.
	BaseClass::PickUp( pPlayer, false );

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	
	// Update the parent to set the correct place on the model to attach the flag.
	int iAttachment = pPlayer->LookupAttachment( "flag" );
	if( iAttachment != -1 )
	{
		SetParent( pPlayer, iAttachment );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}

	// Remove the player's disguse if they're a spy
	if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) ||
			pPlayer->m_Shared.InCond( TF_COND_DISGUISING ))
		{
			pPlayer->m_Shared.RemoveDisguise();
		}
	}

	pPlayer->AddGlowEffect();

	// Remove the touch function.
	SetTouch( NULL );

	m_hPrevOwner = pPlayer;
	m_bAllowOwnerPickup = true;

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
		{
			TeamplayRoundBasedRules()->BroadcastSoundFFA( pPlayer->entindex(), TF_CTF_TEAM_STOLEN, TF_CTF_ENEMY_STOLEN );
			CRecipientFilter filter;
			filter.AddAllPlayers();
			filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
			TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_YOUR_FLAG_TAKEN);
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if( iTeam != pPlayer->GetTeamNumber() )
				{
					TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_ENEMY_STOLEN );
					CTeamRecipientFilter filter( iTeam, true );
					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_TAKEN );
				}
				else
				{
					TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_TEAM_STOLEN , true);
					CTeamRecipientFilter filter( iTeam, true );
				
					// exclude the guy who just picked it up
					filter.RemoveRecipient( pPlayer );				
					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_TAKEN );
				}
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_TakeFlagToPoint" );

		if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
		{
			CRecipientFilter filter;
			filter.AddRecipient( pPlayer );
			EmitSound(filter, entindex(), TF_AD_TEAM_STOLEN);
			filter.AddAllPlayers();
			filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
			EmitSound( filter, entindex(), TF_AD_ENEMY_STOLEN );
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_ENEMY_STOLEN );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_TEAM_STOLEN );
				}
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerPickup" );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamPickup" );

		if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
		{
			CRecipientFilter filter;
			filter.AddRecipient( pPlayer );
			EmitSound(filter, entindex(), TF_INVADE_TEAM_STOLEN);
			filter.AddAllPlayers();
			filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
			EmitSound( filter, entindex(), TF_INVADE_ENEMY_STOLEN);
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamPickup" );

					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_ENEMY_STOLEN );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_TEAM_STOLEN );
				}
			}
		}
		// set the flag's team to match the player's team
		ChangeTeam( pPlayer->GetTeamNumber() );
		m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
	}

	SetFlagStatus( TF_FLAGINFO_STOLEN );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_PICKUP );
		event->SetInt( "priority", 8 );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGPICKUP );

	// Output.
	m_outputOnPickUp.FireOutput(pPlayer, this, 0);
	
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if( !pZone->IsDisabled() )
			pZone->FlagPickedUp( pPlayer );
	}

	if ( m_hReturnIcon.Get() )
	{
		UTIL_Remove( m_hReturnIcon );
		m_hReturnIcon = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Capture( CTFPlayer *pPlayer, int nCapturePoint )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

#ifdef GAME_DLL

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
		while( pFlag )
		{
			if ( !of_capture_on_away.GetBool() && !pFlag->IsHome() && pPlayer->GetTeamNumber() == pFlag->GetTeamNumber() )
			{
				TFTeamMgr()->PlayerCenterPrint( pPlayer, "#OF_TEAM_FLAG_AWAY_CANNOT_CAP" );
				return;
			}

			pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( pFlag, "item_teamflag" ) );
		}

		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		bool bRound = false;
		if ( TFGameRules()->IsDMGamemode() && of_mctf_flag_caps_per_round.GetInt() > 0 )
			bRound = true;
		else if ( !TFGameRules()->IsDMGamemode() && tf_flag_caps_per_round.GetInt() > 0 )
			bRound = true;
		
		if ( bRound )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );
			
			bool bToWin = false;
			if ( TFGameRules()->IsDMGamemode() && ( nCaps >= 0 ) && ( of_mctf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
				bToWin = true;
			else if ( !TFGameRules()->IsDMGamemode() && ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
				bToWin = true;
			
			if ( bToWin )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
			{
				CRecipientFilter filter;
				filter.AddRecipient( pPlayer );
				EmitSound(filter, entindex(), TF_CTF_TEAM_CAPTURED);
				TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED);

				filter.AddAllPlayers();
				filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
				EmitSound( filter, entindex(), TF_CTF_ENEMY_CAPTURED);
				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED );
			}
			else
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					if ( iTeam != pPlayer->GetTeamNumber() || iTeam == TF_TEAM_MERCENARY )
					{
						TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_ENEMY_CAPTURED );
						CTeamRecipientFilter filter( iTeam, true );
						TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED );
					}
					else
					{
						TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_TEAM_CAPTURED );
						CTeamRecipientFilter filter( iTeam, true );
						TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED );
					}
				}
			}
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		bool bRewardTeam = false;
		if ( TFGameRules()->IsDMGamemode() && of_mctf_flag_caps_per_round.GetInt() > 0 )
			bRewardTeam = true;
		else if ( !TFGameRules()->IsDMGamemode() && tf_flag_caps_per_round.GetInt() > 0 )
			bRewardTeam = true;	

		// Dont reward the merc team, they're all solo
		bRewardTeam &= pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY;
		
		if ( bRewardTeam )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_CTF_CAPTURED_TEAM_FRAGS );
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		char szNumber[64];
		Q_snprintf( szNumber, sizeof(szNumber), "%d", nCapturePoint );

		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_YouSecuredPoint", szNumber );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "TF_AD_AttackersSecuredPoint", szNumber );

		if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
		{
			CRecipientFilter filter;
			filter.AddRecipient( pPlayer );
			EmitSound(filter, entindex(), TF_AD_TEAM_CAPTURED);

			filter.AddAllPlayers();
			filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
			EmitSound( filter, entindex(), TF_AD_ENEMY_CAPTURED);
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_AttackersSecuredPoint" );

					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_ENEMY_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_TEAM_CAPTURED );
				}
			}
		}

		// Capture sound
		CBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), TF_AD_CAPTURED_SOUND );

		// Reward the player
		if( TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
			pPlayer->IncrementFragCount( TF_AD_CAPTURED_FRAGS );

		// TFTODO:: Reward the team	
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerCapture" );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamCapture" );


		if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
		{
			CRecipientFilter filter;
			filter.AddRecipient( pPlayer );
			EmitSound(filter, entindex(), TF_INVADE_TEAM_CAPTURED);

			filter.AddAllPlayers();
			filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
			EmitSound( filter, entindex(), TF_INVADE_ENEMY_CAPTURED);
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamCapture" );

					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_ENEMY_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_TEAM_CAPTURED );
				}
			}
		}
		// Reward the player
		if( TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
			pPlayer->IncrementFragCount( TF_INVADE_CAPTURED_FRAGS );

		// Reward the team
		if( pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );

		// add the scores (different from other flag types)
		bool bAddScore = false;
		if ( TFGameRules()->IsDMGamemode() && of_mctf_flag_caps_per_round.GetInt() > 0 )
			bAddScore = true;
		else if ( !TFGameRules()->IsDMGamemode() && tf_flag_caps_per_round.GetInt() > 0 )
			bAddScore = true;		
		
		if ( bAddScore )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_CAPTURE );
		event->SetInt( "priority", 9 );
		gameeventmanager->FireEvent( event );
	}

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	// Reset the flag.
	BaseClass::Drop( pPlayer, true );

	Reset();

	pPlayer->TeamFortress_SetSpeed();
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );

	pPlayer->RemoveGlowEffect();
	
	// Output.
	m_outputOnCapture.FireOutput(pPlayer, this, 0);
	
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if ( !pZone->IsDisabled() )
		{
			pZone->FlagDropped( pPlayer );
		}
	}

	m_bCaptured = true;
	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );

	if ( TFGameRules()->InStalemate() )
	{
		// whoever capped the flag is the winner, give them enough caps to win
		CTFTeam *pTeam = pPlayer->GetTFTeam();
		if ( !pTeam )
			return;

		// if we still need more caps to trigger a win, give them to us
		bool bMoreNeeded = false;
		if ( TFGameRules()->IsDMGamemode() && pTeam->GetFlagCaptures() < of_mctf_flag_caps_per_round.GetInt() )
			bMoreNeeded = true;
		else if ( !TFGameRules()->IsDMGamemode() && pTeam->GetFlagCaptures() < tf_flag_caps_per_round.GetInt() )
			bMoreNeeded = true;
		
		if ( bMoreNeeded )
		{
			if ( TFGameRules()->IsDMGamemode() )
				pTeam->SetFlagCaptures( of_mctf_flag_caps_per_round.GetInt() );
			else
				pTeam->SetFlagCaptures( tf_flag_caps_per_round.GetInt() );
		}	
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: A player drops the flag.
//-----------------------------------------------------------------------------
void CCaptureFlag::Drop( CTFPlayer *pPlayer, bool bVisible,  bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	// Call into the base class drop.
	BaseClass::Drop( pPlayer, bVisible );

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL

	if ( bThrown )
	{
		m_bAllowOwnerPickup = false;
		m_flOwnerPickupTime = gpGlobals->curtime + TF_FLAG_OWNER_PICKUP_TIME;
	}

	Vector vecStart = GetAbsOrigin();
	Vector vecEnd = vecStart;
	vecEnd.z -= 8000.0f;
	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trace );
	SetAbsOrigin( trace.endpos );

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		if ( bMessage )
		{
			if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
			{
				TeamplayRoundBasedRules()->BroadcastSoundFFA( pPlayer->entindex(), TF_CTF_TEAM_STOLEN, TF_CTF_ENEMY_DROPPED );
				CRecipientFilter filter;
				filter.AddRecipient( pPlayer );
				TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_YOUR_FLAG_DROPPED);

				filter.AddAllPlayers();
				filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
				TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED);
			}
			else
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					if ( iTeam != pPlayer->GetTeamNumber() )
					{
						TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_ENEMY_DROPPED );
						CTeamRecipientFilter filter( iTeam, true );
						TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_DROPPED );
					}
					else
					{
						TeamplayRoundBasedRules()->BroadcastSound( iTeam, TF_CTF_TEAM_DROPPED );
						CTeamRecipientFilter filter( iTeam, true );
						TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED );
					}
				}
			}
		}
		if (m_flReturnTime)
			SetFlagReturnIn( m_flReturnTime );
		else
			SetFlagReturnIn( TF_CTF_RESET_TIME );
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		if ( bMessage  )
		{
			// Handle messages to the screen.
			TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerFlagDrop" );
			TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_FlagDrop" );

			if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
			{
				CRecipientFilter filter;
				filter.AddRecipient( pPlayer );
				EmitSound(filter, entindex(), TF_INVADE_TEAM_DROPPED);

				filter.AddAllPlayers();
				filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
				EmitSound( filter, entindex(), TF_INVADE_ENEMY_DROPPED);
			}
			else
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					if ( iTeam != pPlayer->GetTeamNumber() )
					{
						TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagDrop" );

						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), TF_INVADE_ENEMY_DROPPED );
					}
					else
					{
						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), TF_INVADE_TEAM_DROPPED );
					}
				}
			}
		}

		if (m_flReturnTime)
			SetFlagReturnIn( m_flReturnTime );
		else
			SetFlagReturnIn( TF_CTF_RESET_TIME );
		if (m_flSetNeutralTime)
			SetFlagNeutralIn( m_flSetNeutralTime );
		else
			SetFlagNeutralIn( TF_INVADE_NEUTRAL_TIME );		
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		if ( bMessage )
		{
			if( pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
			{
				CRecipientFilter filter;
				filter.AddRecipient( pPlayer );
				EmitSound(filter, entindex(), TF_AD_ENEMY_DROPPED);

				filter.AddAllPlayers();
				filter.RemoveRecipientByPlayerIndex( pPlayer->entindex() );
				EmitSound( filter, entindex(), TF_AD_ENEMY_DROPPED);
			}
			else
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					if ( iTeam != pPlayer->GetTeamNumber() || iTeam == TF_TEAM_MERCENARY )
					{
						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), TF_AD_ENEMY_DROPPED );
					}
					else
					{
						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), TF_AD_TEAM_DROPPED );
					}
				}
			}
		}

		if (m_flReturnTime)
			SetFlagReturnIn( m_flReturnTime );
		else
			SetFlagReturnIn( TF_CTF_RESET_TIME );
	}

	// Reset the flag's angles.
	SetAbsAngles( m_vecResetAng );

	pPlayer->RemoveGlowEffect();

	// Reset the touch function.
	SetTouch( &CCaptureFlag::FlagTouch );

	SetFlagStatus( TF_FLAGINFO_DROPPED );

	// reset us if we are in a func_respawnflag
	if( InRespawnFlagZone( GetAbsOrigin() ) )
	{
		Reset();

		ResetMessage();
	}

	// Output.
	m_outputOnDrop.FireOutput(pPlayer, this, 0);
	
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if ( !pZone->IsDisabled() )
		{
			pZone->FlagDropped( pPlayer );
		}
	}

	m_hReturnIcon = CBaseEntity::Create( "item_teamflag_return_icon", GetAbsOrigin() + Vector(0,0,cl_flag_return_height.GetFloat()), vec3_angle, this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDropped( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_DROPPED ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsHome( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_NONE ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsStolen( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_STOLEN ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		AddEffects( EF_NODRAW );

		SetTouch( NULL );
		SetThink( NULL );
	}
	else
	{
		RemoveEffects( EF_NODRAW );

		SetTouch( &CCaptureFlag::FlagTouch );
		SetThink( &CCaptureFlag::Think );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------------------------
// GAME DLL Functions
//-----------------------------------------------------------------------------------------------
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Think( void )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
	{
		SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
		return;
	}

	if ( m_bCaptured )
	{
		m_bCaptured = false;
		SetTouch( &CCaptureFlag::FlagTouch );
	}

	if ( IsDropped() )
	{
		if ( !m_bAllowOwnerPickup )
		{
			if ( m_flOwnerPickupTime && gpGlobals->curtime > m_flOwnerPickupTime )
			{
				m_bAllowOwnerPickup = true;
			}
		}

		if ( m_nType == TF_FLAGTYPE_INVADE )
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
			else if ( m_flNeutralTime && gpGlobals->curtime > m_flNeutralTime )
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagNeutral" );
				}

				// reset the team to the original team setting (when it spawned)
				ChangeTeam( m_iOriginalTeam );
				m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);

				ResetFlagNeutralTime();
			}
		}
		else
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
		}
	}

	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the flag status
//-----------------------------------------------------------------------------
void CCaptureFlag::SetFlagStatus( int iStatus )
{ 
	MDLCACHE_CRITICAL_SECTION();

	m_nFlagStatus = iStatus;

	switch ( m_nFlagStatus )
	{
	case TF_FLAGINFO_NONE:
	case TF_FLAGINFO_DROPPED:
		ResetSequence( LookupSequence("spin") );	// set spin animation if it's not being held
		break;
	case TF_FLAGINFO_STOLEN:
		ResetSequence( LookupSequence("idle") );	// set idle animation if it is being held
		break;
	default:
		AssertOnce( false );	// invalid stats
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputRoundActivate( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureFlag::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


#else

void CCaptureFlag::ClientThink()
{
	m_bGlowEnabled = true;
	UpdateGlowEffect();
	BaseClass::ClientThink();

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::GetGlowEffectColor( float *r, float *g, float *b )
{
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			*r = 0.62; *g = 0.21; *b = 0.13;
			break;
		case TF_TEAM_BLUE:
			*r = 0.3; *g = 0.42; *b = 0.5;
			break;
		default:
			*r = 0.76; *g = 0.76; *b = 0.76;
			break;
	}
}
	
float CCaptureFlag::GetReturnProgress()
{
	float flEventTime = max( m_flResetTime.m_Value, m_flNeutralTime.m_Value );

	return ( 1.0 - ( ( flEventTime - gpGlobals->curtime ) / m_flMaxResetTime ) );
}


void CCaptureFlag::Simulate( void )
{
	BaseClass::Simulate();

	ManageTrailEffects();
}

void CCaptureFlag::ManageTrailEffects( void )
{
	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN )
	{
		if ( m_pGlowTrailEffect == NULL )
		{
			const char *pEffectName = NULL;

			if ( GetPrevOwner() && GetPrevOwner() != CBasePlayer::GetLocalPlayer() )
			{
				switch( GetPrevOwner()->GetTeamNumber() )
				{
				case TF_TEAM_BLUE:
					{
						if ( (char *)m_ParticleBlue != NULL )
							pEffectName = (char *)m_ParticleBlue;
						else
							pEffectName = "player_intel_trail_blue";
					}				
				break;
				case TF_TEAM_RED:
					{
						if ( (char *)m_ParticleRed != NULL )
							pEffectName = (char *)m_ParticleRed;
						else
							pEffectName = "player_intel_trail_red";
					}				
				break;
				case TF_TEAM_MERCENARY:
					{
						if ((char *)m_ParticleMercenary != NULL)
							pEffectName = (char *)m_ParticleMercenary;
						else
						pEffectName = "player_intel_trail_mercenary";
					}				
				break;
				default:
					{
						if ((char *)m_ParticleBlue != NULL)
							pEffectName = (char *)m_ParticleBlue;
						else
							pEffectName = "player_intel_trail_blue";
					}				
				break;
				}



				m_pGlowTrailEffect = ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
			}
		}

		if ( GetPrevOwner() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );

			if ( pPlayer )
			{
				if ( pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
				{
					if ( m_pPaperTrailEffect == NULL )
					{
						const char *pEffectName = "player_intel_papertrail";
						if ( m_ParticlePaper != NULL )
							pEffectName = m_ParticlePaper;
						m_pPaperTrailEffect = ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
					}
				}
				else
				{
					if ( m_pPaperTrailEffect )
					{
						ParticleProp()->StopEmission( m_pPaperTrailEffect );
						m_pPaperTrailEffect = NULL;
					}
				}
			}
		}

	}
	else
	{
		if ( m_pGlowTrailEffect )
		{
			ParticleProp()->StopEmission( m_pGlowTrailEffect );
			m_pGlowTrailEffect = NULL;
		}

		if ( m_pPaperTrailEffect )
		{
			ParticleProp()->StopEmission( m_pPaperTrailEffect );
			m_pPaperTrailEffect = NULL;
		}
	}
}


#endif


LINK_ENTITY_TO_CLASS( item_teamflag_return_icon, CCaptureFlagReturnIcon );

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )

BEGIN_NETWORK_TABLE( CCaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
END_NETWORK_TABLE()

CCaptureFlagReturnIcon::CCaptureFlagReturnIcon()
{
#ifdef CLIENT_DLL
	m_pReturnProgressMaterial_Empty = NULL;
	m_pReturnProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCaptureFlagReturnIcon::Spawn( void )
{
	BaseClass::Spawn();

	UTIL_SetSize( this, Vector(-8,-8,-8), Vector(8,8,8) );

	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}

int CCaptureFlagReturnIcon::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_segment_t;


// This defines the properties of the 8 circle segments
// in the circular progress bar.
static progress_segment_t Segments[8] = 
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25,	 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50,	 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75,	 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0,	 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderGroup_t CCaptureFlagReturnIcon::GetRenderGroup( void ) 
{	
	return RENDER_GROUP_TRANSLUCENT_ENTITY;	
}

void CCaptureFlagReturnIcon::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init(  20,  20,  20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCaptureFlagReturnIcon::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );
	
	DrawReturnProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCaptureFlagReturnIcon::DrawReturnProgressBar( void )
{
	CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * > ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetMaxResetTime() <= 0 )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "VGUI/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "VGUI/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;

	switch( pFlag->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		ubColor[0] = 255;
		ubColor[1] = 0;
		ubColor[2] = 0;
		break;
	case TF_TEAM_BLUE:
		ubColor[0] = 0;
		ubColor[1] = 0;
		ubColor[2] = 255;
		break;
	case TF_TEAM_MERCENARY:
		ubColor[0] = 128;
		ubColor[1] = 0;
		ubColor[2] = 128;
		break;
	default:
		ubColor[0] = 100;
		ubColor[1] = 100;
		ubColor[2] = 100;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	float flProgress = pFlag->GetReturnProgress();

	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i=0;i<8;i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( swipe_x - 0.5 ) ) + (vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( Segments[i].vert2x - 0.5 ) ) + (vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif
