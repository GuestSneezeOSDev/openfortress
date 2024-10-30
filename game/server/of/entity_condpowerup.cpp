//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch Powerup Spawner.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_condpowerup.h"
#include "of_dropped_powerup.h"
#include "tf_gamerules.h"

#include "tier0/memdbgon.h"

ConVar of_duel_powerup_timers		("of_duel_powerup_timers", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables powerup timers in duel gamemode");
ConVar of_duel_powerup_respawn		( "of_duel_powerup_respawn", "60", FCVAR_NOTIFY | FCVAR_REPLICATED, "Set the time in seconds before a powerup respawns in duel mode.\nOnly takes effect after map change.", 1, 0, 1, 60);

extern ConVar of_powerups;

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the powerupspawner
//-----------------------------------------------------------------------------

// Inputs.
BEGIN_DATADESC( CCondPowerup )
	DEFINE_KEYFIELD( m_iCondition, FIELD_INTEGER, "condID" ),
	DEFINE_KEYFIELD( m_flCondDuration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_iszPowerupModel, FIELD_STRING, "model" ),
	DEFINE_KEYFIELD( m_iszPowerupModelOLD, FIELD_STRING, "powerup_model" ),
	DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),
	DEFINE_KEYFIELD( m_iszTimerIcon, FIELD_STRING, "timericon" ),
	DEFINE_KEYFIELD( m_bDisableShowOutline, FIELD_BOOLEAN, "disable_glow" ),
	DEFINE_KEYFIELD( m_bDisableIdleSound, FIELD_BOOLEAN, "disable_sound"),
	DEFINE_THINKFUNC( AnnouncerThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCondPowerup, DT_CondPowerup )
	SendPropInt( SENDINFO( m_iCondition ) ),
	SendPropBool( SENDINFO( m_bDisableShowOutline ) ),
	SendPropBool( SENDINFO( m_bDisableIdleSound	  ) ),
	SendPropBool( SENDINFO( m_bRespawning ) ),
	SendPropBool( SENDINFO( m_bInitialDelay ) ),
	SendPropTime( SENDINFO( m_flRespawnTick ) ),
	SendPropTime( SENDINFO( fl_RespawnTime ) ),
	SendPropTime( SENDINFO( fl_RespawnDelay ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( dm_powerup_spawner, CCondPowerup );

IMPLEMENT_AUTO_LIST( ICondPowerupAutoList );

CCondPowerup::CCondPowerup()
{
	m_flRespawnTick = 0.0f;
	m_iCondition = 0;
	m_flCondDuration = 5;
	m_iszPickupSound = MAKE_STRING( "AmmoPack.Touch" );
	m_iszPowerupModelOLD = MAKE_STRING( "" );
	m_iszPowerupModel = MAKE_STRING( "" );
	m_iszTimerIcon = MAKE_STRING( "" );
	bWarningTriggered = false;
}

void CCondPowerup::Spawn( void )
{
	Precache();

	if ( m_iszPowerupModel == MAKE_STRING( "" ) ) 
		SetModel( STRING (m_iszPowerupModelOLD )  );
	else 
		SetModel( STRING( m_iszPowerupModel ) );
	
	SetTransmitState( FL_EDICT_ALWAYS );	// Used for the glow effect to always show up

	RegisterThinkContext( "AnnounceThink" );
	SetContextThink( &CCondPowerup::AnnouncerThink, gpGlobals->curtime, "AnnounceThink" );

	BaseClass::Spawn();

	//disable outline
	if (TFGameRules()->IsDuelGamemode())
	{
		m_bDisableShowOutline = true;
		m_bDisableIdleSound = true;
		fl_RespawnTime = of_duel_powerup_respawn.GetFloat();
	}

	if (!m_bDisableIdleSound)
	{
		EmitSound("Powerup.IdleHum");
	}

	AddFlag(FL_OBJECT); // So NPCs will notice it
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the powerupspawner
//-----------------------------------------------------------------------------
void CCondPowerup::Precache( void )
{
	if (m_iszPowerupModel == MAKE_STRING(""))
	{
		PrecacheModel(STRING(m_iszPowerupModelOLD));
	}
	else
	{
		PrecacheModel(STRING(m_iszPowerupModel));
	}

	PrecacheScriptSound( STRING( m_iszPickupSound ) );

	PrecacheScriptSound("Powerup.IdleHum");
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the powerupspawner
//-----------------------------------------------------------------------------
bool CCondPowerup::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;
	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		if (pTFPlayer->HasTheFlag() && (m_iCondition == TF_COND_INVIS_POWERUP || m_iCondition == TF_COND_STEALTHED))
			return false;

		bSuccess = DoPowerupEffect( pTFPlayer );

		if( bSuccess )
		{
			//in duel diffuse sound like a normal pickup (same for respawn)
			if ( TFGameRules()->IsDuelGamemode() )
			{
				EmitSound( STRING( m_iszPickupSound ) );
			}
			else if ( TeamplayRoundBasedRules() )
			{
				if (Q_strcmp(GetPowerupPickupLine(), "None") || Q_strcmp(GetPowerupPickupLineSelf(), "None"))
				{
					TeamplayRoundBasedRules()->BroadcastSoundFFA(pTFPlayer->entindex(), GetPowerupPickupLineSelf(), GetPowerupPickupLine());
				}

				const char *pickupSound = GetPowerupPickupSound();
				if (Q_strcmp(pickupSound, "None"))
				{
					TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, pickupSound, false, 0, pTFPlayer->entindex());
				}
				else
				{
					TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, STRING(m_iszPickupSound), false);
				}
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the powerupspawner
//-----------------------------------------------------------------------------
bool CCondPowerup::DoPowerupEffect( CTFPlayer *pTFPlayer )
{
	// Old maps compatibility
	switch ( m_iCondition )
	{
		case TF_COND_STEALTHED:
			m_iCondition = TF_COND_INVIS_POWERUP;
			break;
		case TF_COND_CRITBOOSTED:
			m_iCondition = TF_COND_CRIT_POWERUP;
			break;
		case 12:
			m_iCondition = TF_COND_SPAWNPROTECT;
			break;
		case 13:
			m_iCondition = TF_COND_SHIELD_CHARGE;
			break;
		case 14:
			m_iCondition = TF_COND_BERSERK;
			break;
		case 15:
			m_iCondition = TF_COND_SHIELD;
			break;
		case TF_COND_SHIELD_DUEL:
			m_iCondition = TF_COND_SHIELD;
			m_flCondDuration = 60.f;
			break;
	}
	
	if ( pTFPlayer->m_Shared.InCond(m_iCondition) )
		return false;

	pTFPlayer->m_Shared.AddCond( m_iCondition , m_flCondDuration );
	pTFPlayer->SpeakConceptIfAllowed( ( random->RandomInt( 0, 1 ) == 1 ) ? MP_CONCEPT_PLAYER_SPELL_PICKUP_RARE : MP_CONCEPT_PLAYER_SPELL_PICKUP_COMMON );
	
	if (!TFGameRules()->IsDuelGamemode())
	{
		Vector vecOrigin;
		QAngle vecAngles;
		CTFDroppedPowerup *pPowerup = static_cast<CTFDroppedPowerup*>(CBaseAnimating::CreateNoSpawn("tf_dropped_powerup", vecOrigin, vecAngles, pTFPlayer));
		if (pPowerup)
		{
			pPowerup->SetModelName(m_iszPowerupModel);
			pPowerup->m_nSkin = m_nSkin;
			Q_strncpy(pPowerup->szTimerIcon, STRING(m_iszTimerIcon), sizeof(pPowerup->szTimerIcon));
			pPowerup->m_iPowerupID = m_iCondition;
			pPowerup->m_flCreationTime = gpGlobals->curtime;
			pPowerup->m_flDespawnTime = gpGlobals->curtime + m_flCondDuration;
			pPowerup->SetContextThink(&CBaseEntity::SUB_Remove, pPowerup->m_flDespawnTime, "DieContext");
		}
		PowerupHandle hHandle;
		hHandle = pPowerup;
		pTFPlayer->m_hPowerups.AddToTail(hHandle);
	}
	
	IGameEvent *event = gameeventmanager->CreateEvent( "add_powerup_timer" );
	if ( event )
	{
		event->SetInt( "player", pTFPlayer->entindex() );
		event->SetInt( "cond", m_iCondition );
		event->SetString( "icon", STRING( m_iszTimerIcon ) );
		gameeventmanager->FireEvent( event );
	}
	return true;
}

CBaseEntity *CCondPowerup::Respawn( void )
{
	CBaseEntity *ret = BaseClass::Respawn();
	m_flRespawnTick = GetNextThink();

	//In duel mode don't show the respawn timers, unless the convar allows it
	if ( TFGameRules()->IsDuelGamemode() && !of_duel_powerup_timers.GetBool())
	{
		m_nRenderFX = kRenderFxNone;
		AddEffects(EF_NODRAW);
	}
	else
	{
		m_nRenderFX = kRenderFxDistort;

		if (!m_bDisableIdleSound)
		{
			StopSound("Powerup.IdleHum");
		}
	}

	return ret;
}

void CCondPowerup::Materialize( void )
{
	//Making powerup visible at the respawn time (duel only)
	//Removing the glitchy effect (all cases)
	if ( !IsDisabled() )
	{
		m_nRenderFX = kRenderFxNone;
		RemoveEffects( EF_NODRAW );
	}

	m_OnRespawn.FireOutput( this, this );

	if (TFGameRules()->IsDuelGamemode())
	{
		EmitSound( STRING( m_iszSpawnSound ) );
	}
	else if (TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_PREROUND)
	{
		const char *respawnSound = GetPowerupRespawnLine();
		if (Q_strcmp(respawnSound, "None"))
		{
			TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, respawnSound);
		}
		else
		{
			TeamplayRoundBasedRules()->BroadcastSound(TEAM_UNASSIGNED, STRING(m_iszSpawnSound), false);
		}
	}

	if (!m_bDisableIdleSound)
	{
		EmitSound("Powerup.IdleHum");
	}

	m_bRespawning = false;
	m_bInitialDelay = false;

	SetTouch( &CItem::ItemTouch );
}

void CCondPowerup::AnnouncerThink()
{
	if ( m_bRespawning && ( m_flRespawnTick - gpGlobals->curtime < 10.0f && !bWarningTriggered ) && TeamplayRoundBasedRules() )
	{
		//in duel mode powerups should not be announced, players should time them
		if ( !TFGameRules()->IsDuelGamemode() )
			TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, GetPowerupPickupIncomingLine() );

		bWarningTriggered = true;
	}
	else if ( m_bRespawning && ( m_flRespawnTick - gpGlobals->curtime > 10.0f && bWarningTriggered ) ) // This fixes the case where you pick up the powerup as soon as it respawns
	{
		bWarningTriggered = false;
	}

	if ( bWarningTriggered && !m_bRespawning )
		bWarningTriggered = false;

	SetNextThink( gpGlobals->curtime + 0.5f, "AnnounceThink" );
}

const char* CCondPowerup::GetPowerupPickupLineSelf( void )
{
	switch ( m_iCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "CritTakenSelf";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "InvisibilityTakenSelf";
		break;
		case TF_COND_SHIELD:
		return "ShieldTakenSelf";
		break;
		case TF_COND_HASTE:
		return "HasteTakenSelf";
		break;
		case TF_COND_BERSERK:
		return "BerserkTakenSelf";
		break;
		case TF_COND_JAUGGERNAUGHT:
		return "JauggernaughtTakenSelf"; //change me
		break;
	}
	return "GenericTakenSelf";
}

const char* CCondPowerup::GetPowerupPickupLine( void )
{
	switch ( m_iCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "CritTaken";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "InvisibilityTaken";
		break;
		case TF_COND_SHIELD:
		return "ShieldTaken";
		break;
		case TF_COND_HASTE:
		return "HasteTaken";
		break;
		case TF_COND_BERSERK:
		return "BerserkTaken";
		break;
		case TF_COND_JAUGGERNAUGHT:
		return "JauggernaughtTaken"; //change me
		break;
	}
	return "GenericTaken";
}

const char* CCondPowerup::GetPowerupPickupSound( void )
{
	switch ( m_iCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
			return "Powerup.Crit";
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
			return "Powerup.Invis";
		case TF_COND_BERSERK:
			return "Powerup.Berserk";
		case TF_COND_SHIELD:
			return "Powerup.Shield";
		case TF_COND_HASTE:
			return "Powerup.Haste";
		case TF_COND_JAUGGERNAUGHT:
			return "Powerup.Jauggernaught"; //change me
		case TF_COND_JETPACK_POWERUP:
			return "Powerup.Jetpack";
	}

	return "Powerup.Generic";
}

const char* CCondPowerup::GetPowerupRespawnLine( void )
{
	switch ( m_iCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "Crits";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "Invisibility";
		break;
		case TF_COND_SHIELD:
		return "Shield";
		break;
		case TF_COND_HASTE:
		return "Haste";
		break;
		case TF_COND_BERSERK:
		return "Berserk";
		break;
		case TF_COND_JAUGGERNAUGHT:
		return "Jauggernaught";
		break;
	}

	return "Generic";
}

const char *CCondPowerup::GetPowerupPickupIncomingLine( void )
{
	switch ( m_iCondition )
	{
		case TF_COND_CRITBOOSTED:
		case TF_COND_CRIT_POWERUP:
		return "CritsIncoming";
		break;
		case TF_COND_STEALTHED:
		case TF_COND_INVIS_POWERUP:
		return "InvisibilityIncoming";
		break;
		case TF_COND_SHIELD:
		return "ShieldIncoming";
		break;
		case TF_COND_INVULNERABLE:
		return "UberIncoming";
		break;
		case TF_COND_HASTE:
		return "HasteIncoming";
		break;
		case TF_COND_BERSERK:
		return "BerserkIncoming";
		break;
		case TF_COND_JAUGGERNAUGHT:
		return "JauggernaughtIncoming";
		break;
	}
	return "PowerupsIncoming";
}