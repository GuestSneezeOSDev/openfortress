//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for Open Foreskin.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "bots/bot.h"
#include "tf_gamerules.h"
#include "of_items_game.h"
#include "tf_gamestats.h"
#include "viewport_panel_names.h"
#include "tf_team.h"
#include "tf_viewmodel.h"
#include "in_buttons.h"
#include "entity_capture_flag.h"
#include "effect_dispatch_data.h"
#include "tf_weapon_builder.h"
#include "tf_obj.h"
#include "tf_ammo_pack.h"
#include "entity_weapon_spawner.h"
#include "datacache/imdlcache.h"
#include "particle_system.h"
#include "IEffects.h"
#include "tf_weapon_pda.h"
#include "sceneentity.h"
#include "fmtstr.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_minigun.h"
#include "func_respawnroom.h"
#include "tf_weapon_medigun.h"
#include "hl2orange.spa.h"
#include "te_tfblood.h"
#include "of_weapon_physcannon.h"
#include "eventqueue.h"
#include "tf_weaponbase_melee.h"
#include "of_dropped_powerup.h"
#include "basegrenade_shared.h"
#include "dt_utlvector_send.h"
#include "team_control_point_master.h"
#include "gamevars_shared.h"
#include "tf_player_resource.h"
#include "of_auto_team.h"
#include "filesystem.h"
#include "ai_basenpc.h"
#include "entity_weapon_spawner.h"
#include "of_shared_schemas.h"
#include "of_loadout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DAMAGE_FORCE_SCALE_SELF				9


extern bool IsInCommentaryMode( void );

extern ConVar	sk_player_head;
extern ConVar	sk_player_chest;
extern ConVar	sk_player_stomach;
extern ConVar	sk_player_arm;
extern ConVar	sk_player_leg;

extern ConVar	tf_spy_invis_time;
extern ConVar	tf_spy_invis_unstealth_time;
extern ConVar	tf_stalematechangeclasstime;

extern ConVar	of_merc_only_arsenal;
extern ConVar	of_infiniteammo;

extern ConVar	of_threewave;
extern ConVar	of_crit_multiplier;

extern ConVar	of_randomizer;
extern ConVar	of_randomizer_setting;

extern ConVar	of_headshots;

extern ConVar	of_disable_drop_weapons;

EHANDLE g_pLastSpawnPoints[TF_TEAM_COUNT];

ConVar tf_playerstatetransitions	( "tf_playerstatetransitions", "-2", FCVAR_CHEAT, "tf_playerstatetransitions <ent index or -1 for all>. Show player state transitions." );
ConVar tf_playergib					( "tf_playergib", "1", FCVAR_PROTECTED, "Allow player gibbing." );

ConVar tf_weapon_ragdoll_velocity_min	( "tf_weapon_ragdoll_velocity_min", "100", FCVAR_CHEAT );
ConVar tf_weapon_ragdoll_velocity_max	( "tf_weapon_ragdoll_velocity_max", "150", FCVAR_CHEAT );
ConVar tf_weapon_ragdoll_maxspeed		( "tf_weapon_ragdoll_maxspeed", "300", FCVAR_CHEAT );

ConVar tf_damageforcescale_other		( "tf_damageforcescale_other", "6.0", FCVAR_CHEAT );
// Lowered from 10 originally
// Since we use the predamagescale damage now, we dont need the icnreased force
ConVar tf_damageforcescale_self_soldier	( "tf_damageforcescale_self_soldier", "6.0", FCVAR_CHEAT );
ConVar tf_damagescale_self_soldier		( "tf_damagescale_self_soldier", "0.60", FCVAR_CHEAT );

ConVar tf_damage_lineardist				( "tf_damage_lineardist", "0", FCVAR_CHEAT );
ConVar tf_damage_range					( "tf_damage_range", "0.5", FCVAR_CHEAT );

ConVar tf_max_voice_speak_delay			( "tf_max_voice_speak_delay", "1.5", FCVAR_REPLICATED , "Max time delay between a player's voice commands." );

ConVar of_forcespawnprotect	( "of_forcespawnprotect", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "Manually define how long the spawn protection lasts." );
ConVar of_instantrespawn	( "of_instantrespawn", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "Instant respawning." );
ConVar of_dropweapons		( "of_dropweapons", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "1- Allow manual weapon dropping.\n2- Thrown weapons hurt enemies." );
ConVar of_healonkill		( "of_healonkill", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Amount of health gained after a kill." );

ConVar of_resistance		( "of_resistance", "0.33", FCVAR_REPLICATED | FCVAR_NOTIFY , "Defines the resistance of the Shield powerup." );

ConVar of_fullammo			("of_fullammo", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Weapons have full ammo when dropped.");

ConVar of_selfdamage		( "of_selfdamage", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Whether or not you should deal self damage with explosives.",true, -1, true, 1  );

ConVar of_knockback_all("of_knockback_all", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Multiplies damage impulse for all damage types.");
ConVar of_knockback_bullets("of_knockback_bullets", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Multiplies damage impulse for bullets.");
ConVar of_knockback_explosives("of_knockback_explosives", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Multiplies damage impulse for explosions.");
ConVar of_knockback_melee("of_knockback_melee", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Multiplies damage impulse for melee.");

ConVar of_startloadout("of_startloadout", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Equips players with the normal spawn loadout.");

ConVar of_zombie_dropitems( "of_zombie_dropitems", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Should zombies drop small ammopacks + healthkits on death?." );

ConVar of_spawn_with_weapon( "of_spawn_with_weapon", "", FCVAR_ARCHIVE | FCVAR_NOTIFY, "For bot behaviour debugging: players will only spawn with the specified weapon classname." );

ConVar of_spread_infection("of_spread_infection", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Whether or not you can spread infection by touching teammates.");

ConVar of_armor_light_absorption("of_armor_light_absorption", "0.3", FCVAR_REPLICATED | FCVAR_NOTIFY, "Sets the amount LIGHT ARMOR damage absorption.");
ConVar of_armor_med_absorption("of_armor_med_absorption", "0.6", FCVAR_REPLICATED | FCVAR_NOTIFY, "Sets the amount MED ARMOR damage absorption.");
ConVar of_armor_heavy_absorption("of_armor_heavy_absorption", "0.8", FCVAR_REPLICATED | FCVAR_NOTIFY, "Sets the amount HEAVY ARMOR damage absorption.");

extern ConVar of_armor;
extern ConVar of_grenades;
extern ConVar of_allowteams;
extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_maxunlag;
extern ConVar tf_damage_disablespread;
extern ConVar tf_gravetalk;
extern ConVar tf_spectalk;
extern ConVar of_allow_special_teams;
extern ConVar of_spawnprotecttime;
extern ConVar friendlyfire;
extern ConVar of_juggernaught_wintime;
extern ConVar of_armor_on_spawn;
extern ConVar of_berserk_knockback;
extern ConVar of_teamplay_collision;

ConVar of_dynamic_color_update	( "of_dynamic_color_update", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Updates player color immediately." );
// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
		m_iPlayerIndex = TF_PLAYER_INDEX_NONE;
	}

	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropInt( SENDINFO( m_iPlayerIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
    Vector vecEyePos = pPlayer->EyePosition();
	CPVSFilter filter( vecEyePos );
	if ( !IsCustomPlayerAnimEvent( event ) && ( event != PLAYERANIMEVENT_SNAP_YAW ) && ( event != PLAYERANIMEVENT_VOICE_COMMAND_GESTURE ) )
	{
		filter.RemoveRecipient( pPlayer );
	}

	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_iPlayerIndex = pPlayer->entindex();
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < (1<<ANIMATION_SEQUENCE_BITS) );
	//Assert( (1<<ANIMATION_SEQUENCE_BITS) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//=================================================================================
//
// Ragdoll Entity
//
class CTFRagdoll : public CBaseAnimatingOverlay
{
public:

	DECLARE_CLASS( CTFRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	CTFRagdoll()
	{
		m_iPlayerIndex.Set( TF_PLAYER_INDEX_NONE );
		m_bGib = false;
		m_bBurning = false;
		m_bFlagOnGround = false;
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
		m_iDamageCustom = 0;
		UseClientSideAnimation();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar( bool, m_bGib );
	CNetworkVar( bool, m_bBurning );
	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iClass );
	CNetworkVar( int, m_iDamageCustom );

	CNetworkVar( unsigned short, m_iGoreHead );
	CNetworkVar( unsigned short, m_iGoreLeftArm );
	CNetworkVar( unsigned short, m_iGoreRightArm );
	CNetworkVar( unsigned short, m_iGoreLeftLeg );
	CNetworkVar( unsigned short, m_iGoreRightLeg );
	
	CNetworkVar( bool, m_bFlagOnGround );
	CNetworkVar( bool, m_bDissolve );
};

LINK_ENTITY_TO_CLASS( tf_ragdoll, CTFRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTFRagdoll, DT_TFRagdoll )
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1,  SPROP_COORD ),
	SendPropInt( SENDINFO( m_iPlayerIndex ), 7, SPROP_UNSIGNED ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f ),
	SendPropInt( SENDINFO( m_nForceBone ) ),
	SendPropBool( SENDINFO( m_bGib ) ),
	SendPropBool( SENDINFO( m_bBurning ) ),
	SendPropInt( SENDINFO( m_iTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), 4, SPROP_UNSIGNED ),	
	SendPropInt( SENDINFO( m_iDamageCustom ) ),
	SendPropInt( SENDINFO( m_iGoreHead ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iGoreLeftArm ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iGoreRightArm ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iGoreLeftLeg ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iGoreRightLeg ), 2, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bFlagOnGround ) ),
	SendPropBool( SENDINFO( m_bDissolve ) ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: Filters updates to a variable so that only non-local players see
// the changes.  This is so we can send a low-res origin to non-local players
// while sending a hi-res one to the local player.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendNonLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient( objectID - 1 );
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalDataTable );

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_PlayerObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;

	// If this fails, then SendProxyArrayLength_PlayerObjects didn't work.
	Assert( iElement < pPlayer->GetObjectCount() );

	CBaseObject *pObject = pPlayer->GetObject(iElement);

	EHANDLE hObject;
	hObject = pObject;

	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

int SendProxyArrayLength_PlayerObjects( const void *pStruct, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;
	int iObjects = pPlayer->GetObjectCount();
	Assert( iObjects <= MAX_OBJECTS_PER_PLAYER );
	return iObjects;
}

BEGIN_DATADESC( CTFPlayer )
	DEFINE_INPUTFUNC( FIELD_STRING,		"SetCustomModel",			 SetCustomModel ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"SetCustomArmModel",		 SetCustomArmModel ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AddMoney",					 AddMoney ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetMoney",					 SetMoney ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"StripWeapons",				 InputStripWeapons ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"SpeakResponseConcept",		 InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"IgnitePlayer",				 InputIgnitePlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"ExtinguishPlayer",			 InputExtinguishPlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"PoisonPlayer",				 InputPoisonPlayer),
	DEFINE_INPUTFUNC( FIELD_VOID,		"DePoisonPlayer",			 InputDePoisonPlayer),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN,	"SetZombie",				 InputSetZombie ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN,	"SetTeamNoKill",			 InputSetTeamNoKill ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"SetAttributeOfWeapon",		 InputSetAttributeOfWeapon ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"AddAttributeToWeapon",		 InputAddAttributeToWeapon ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"RemoveAttributeFromWeapon", InputRemoveAttributeFromWeapon ),
    DEFINE_INPUTFUNC( FIELD_INTEGER,    "SetArmor",                  InputSetArmor),
    DEFINE_KEYFIELD ( m_Shared.m_iArmor , FIELD_INTEGER, "tfcarmor" ), // this works?
END_DATADESC()

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropArray2( 
		SendProxyArrayLength_PlayerObjects,
		SendPropInt("player_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_PlayerObjectList), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"
		),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
//	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

END_SEND_TABLE()

// all players except the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

END_SEND_TABLE()


//============

LINK_ENTITY_TO_CLASS( player, CTFPlayer );
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST( CTFPlayer, DT_TFPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_nModelIndex" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// cs_playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropBool( SENDINFO(m_bSaveMeParity)  ),
	SendPropBool( SENDINFO(m_bChatting) ),
	SendPropBool( SENDINFO(m_bRetroMode)  ),
	SendPropBool( SENDINFO(m_bIsRobot) ),
	SendPropBool( SENDINFO(m_bHauling) ),

	// This will create a race condition will the local player, but the data will be the same so.....
	SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	SendPropEHandle(SENDINFO(m_hItem)),

	SendPropVector( SENDINFO( m_vecPlayerColor ) ),
	
	SendPropVector( SENDINFO( m_vecViewmodelOffset ) ),
	SendPropVector( SENDINFO( m_vecViewmodelAngle ) ),
	
	SendPropBool( SENDINFO( m_bCentered ) ),
	SendPropBool( SENDINFO( m_bMinimized ) ),
	
	// Ragdoll.
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropDataTable( SENDINFO_DT( m_PlayerClass ), &REFERENCE_SEND_TABLE( DT_TFPlayerClassShared ) ),
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_TFPlayerShared ) ),

	// Data that only gets sent to the local player
	SendPropDataTable( "tflocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "tfnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropBool( SENDINFO( m_iSpawnCounter ) ),
	
	SendPropBool( SENDINFO( m_bResupplied ) ),
	
	SendPropInt( SENDINFO( m_iAccount ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iMegaOverheal ) ),
END_SEND_TABLE()



// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );

// Hint callbacks
bool HintCallbackNeedsResources_Sentrygun( CBasePlayer *pPlayer )
{
	return ( pPlayer->GetAmmoCount( TF_AMMO_METAL ) > CalculateObjectCost( OBJ_SENTRYGUN ) );
}
bool HintCallbackNeedsResources_Dispenser( CBasePlayer *pPlayer )
{
	return ( pPlayer->GetAmmoCount( TF_AMMO_METAL ) > CalculateObjectCost( OBJ_DISPENSER ) );
}
bool HintCallbackNeedsResources_Teleporter( CBasePlayer *pPlayer )
{
	return ( pPlayer->GetAmmoCount( TF_AMMO_METAL ) > CalculateObjectCost( OBJ_TELEPORTER ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer::CTFPlayer()
{
	m_pBotController = NULL;
	m_pSenses = NULL;

	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	item_list = 0;

	m_Shared.SetTFCArmor( 0 );

	m_hItem = NULL;
	m_hTauntScene = NULL;

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_pStateInfo = NULL;
	m_lifeState = LIFE_DEAD; // Start "dead".
	m_iMaxSentryKills = 0;
	m_iLevelProgress = 0;
	m_flNextNameChangeTime = 0;

	m_flNextTimeCheck = gpGlobals->curtime;
	m_flSpawnTime = 0;

	SetViewOffset( TF_PLAYER_VIEW_OFFSET );

	m_Shared.Init( this );

	m_iLastSkin = -1;

	m_bHudClassAutoKill = false;
	m_bMedigunAutoHeal = false;

	m_vecLastDeathPosition = Vector( FLT_MAX, FLT_MAX, FLT_MAX );

	m_vecPlayerColor.Init( 1.0f, 1.0f, 1.0f );
	
	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );

	ResetScores();

	m_flLastAction = gpGlobals->curtime;

	m_bInitTaunt = false;

	m_bSpeakingConceptAsDisguisedSpy = false;
	
	m_iAccount = 0;
	
    ConVarRef scissor( "r_flashlightscissor" );
    scissor.SetValue( "0" );
	
	m_bDied = false;
	m_bGotKilled = false;

	m_bWinDeath = false;

	m_bHauling = false;
	
	m_bPuppet = false;
	
	m_bResupplied = false;
	
	m_purgatoryDuration.Invalidate();
	m_lastCalledMedic.Invalidate();
	
	m_chzVMCosmeticGloves = NULL;
	m_chzVMCosmeticSleeves = NULL;

	m_bIsJuggernaught = false;
	m_iJuggernaughtScore = 0;
	m_iJuggernaughtTimer = 0;

	m_iShieldDamage = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFPlayerThink()
{
	if ( m_pStateInfo && m_pStateInfo->pfnThink )
	{
		(this->*m_pStateInfo->pfnThink)();
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if ( IsAlive() && m_flNextRandomExpressionTime >= 0 && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();
		m_iszExpressionScene = NULL_STRING;
		UpdateExpression();
	}
	
	if ( of_dynamic_color_update.GetBool() )
		UpdatePlayerColor();

	TauntEffectThink();

	if ( TFGameRules() )
		TFGameRules()->EntityLimitPrevention();

	// Check to see if we are in the air and taunting.  Stop if so.
/*	if ( GetGroundEntity() == NULL && m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if( m_hTauntScene.Get() )
		{
			StopScriptedScene( this, m_hTauntScene );
			m_Shared.m_flTauntRemoveTime = 0.0f;
			m_hTauntScene = NULL;
		}
	}
*/

	//midair medal, record player airtime
	if (!GetGroundEntity() && GetWaterLevel() <= WL_Feet) //always go here when player is in the air
	{
		if (!m_fAirStartTime)
			m_fAirStartTime = gpGlobals->curtime;
	}
	else
	{
		m_fAirStartTime = 0.f;
        m_Shared.SetLaunchedFromJumppad(false);
	}

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::MedicRegenThink( void )
{
	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		if ( IsAlive() )
		{
			// Heal faster if we haven't been in combat for a while
			float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageTime();
			float flScale = RemapValClamped( flTimeSinceDamage, 5, 10, 1.0, 3.0 );

			int iHealAmount = ceil( TF_MEDIC_REGEN_AMOUNT * flScale);
			TakeHealth( iHealAmount, DMG_GENERIC );
		}

		SetContextThink( &CTFPlayer::MedicRegenThink, gpGlobals->curtime + TF_MEDIC_REGEN_TIME, "MedicRegenThink" );
	}
}

// Used for Infection, on both survivors and zombies
void CTFPlayer::ZombieRegenThink( void )
{
	if ( IsAlive() )
	{
		if ( m_Shared.IsZombie() )
			TakeHealth( 4, DMG_GENERIC );
		else
		{
			TakeHealth( 1, DMG_GENERIC );
		}
	}

	SetContextThink( &CTFPlayer::ZombieRegenThink, gpGlobals->curtime + 1, "ZombieRegenThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFPlayer::~CTFPlayer()
{
	DestroyRagdoll();
	m_PlayerAnimState->Release();
}


CTFPlayer *CTFPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CTFPlayer::s_PlayerEdict = ed;
	return (CTFPlayer*)CreateEntityByName( className );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateTimers( void )
{
	m_Shared.ConditionThink();
	m_Shared.InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PreThink()
{
	// Update timers.
	UpdateTimers();
	
	// copied from hl2player
	if  (IsInAVehicle() )
	{
		UpdateClientData();	
		CheckTimeBasedDamage();
		UpdateTimers();
		WaterMove();

		m_vecTotalBulletForce = vec3_origin;
		CheckForIdle();
		return;
	}
	
	// Pass through to the base class think.
	BaseClass::PreThink();


	// Reset bullet force accumulator, only lasts one frame, for ragdoll forces from multiple shots.
	m_vecTotalBulletForce = vec3_origin;

	CheckForIdle();
}

ConVar mp_idledealmethod( "mp_idledealmethod", "1", FCVAR_GAMEDLL, "Deals with Idle Players. 1 = Sends them into Spectator mode then kicks them if they're still idle, 2 = Kicks them out of the game;" );
ConVar mp_idlemaxtime( "mp_idlemaxtime", "3", FCVAR_GAMEDLL, "Maximum time a player is allowed to be idle (in minutes)" );
ConVar mp_idleallowhost( "mp_idleallowhost", "0", FCVAR_GAMEDLL, "Allow the host of a listen server to be team changed on idle, mostly for testing");

void CTFPlayer::CheckForIdle( void )
{
	if ( m_afButtonLast != m_nButtons )
		m_flLastAction = gpGlobals->curtime;

	if ( mp_idledealmethod.GetInt() )
	{
		if ( IsHLTV() )
			return;

		if ( IsFakeClient() )
			return;

		//Don't mess with the host on a listen server (probably one of us debugging something)
        if ( engine->IsDedicatedServer() == false && entindex() == 1 && !mp_idleallowhost.GetBool())
			return;

		if ( m_bIsIdle == false )
		{
			if ( StateGet() == TF_STATE_OBSERVER || StateGet() != TF_STATE_ACTIVE )
				return;
		}
		
		float flIdleTime = mp_idlemaxtime.GetFloat() * 60;

		if ( TFGameRules()->InStalemate() )
		{
			flIdleTime = mp_stalemate_timelimit.GetInt() * 0.5f;
		}
		
		if ( (gpGlobals->curtime - m_flLastAction) > flIdleTime )
		{
			bool bKickPlayer = false;

			ConVarRef mp_allowspectators( "mp_allowspectators" );
			if ( mp_allowspectators.IsValid() && ( mp_allowspectators.GetBool() == false ) )
			{
				// just kick the player if this server doesn't allow spectators
				bKickPlayer = true;
			}
			else if ( mp_idledealmethod.GetInt() == 1 )
			{
				//First send them into spectator mode then kick him.
				if ( m_bIsIdle == false )
				{
					DevMsg("ForceChangeTeam was executed...");
					ForceChangeTeam(TEAM_SPECTATOR);
					m_flLastAction = gpGlobals->curtime;
					m_bIsIdle = true;
					return;
				}
				else
				{
					bKickPlayer = true;
				}
			}
			else if ( mp_idledealmethod.GetInt() == 2 )
			{
				bKickPlayer = true;
			}

            if (engine->IsDedicatedServer() == false && entindex() == 1) //still dont kick the host
                bKickPlayer = false;

			if ( bKickPlayer == true )
			{
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_idle_kick", GetPlayerName() );
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", GetUserID() ) );
				m_flLastAction = gpGlobals->curtime;
			}
		}
	}
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CTFPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOn( void )
{
	AddEffects( EF_DIMLIGHT );
	if( IsAlive() ) EmitSound( "HL2Player.FlashLightOn" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	if( IsAlive() ) EmitSound( "HL2Player.FlashLightOff" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PostThink()
{
	BaseClass::PostThink();

	// check if our guy is chatting for the particle
	m_bChatting = (m_nButtons & IN_TYPING) != 0;
	
	ProcessSceneEvents();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	// Update our last nav area
	// Only really needed for bots but might be useful for regular players as well
	UpdateLastKnownArea();

	if ( TFGameRules()->IsJugGamemode() )
	{
		if ( IsJuggernaught() )
		{
			if ( gpGlobals->curtime > ( m_iJuggernaughtTimer + 1 ) )
			{
				m_iJuggernaughtTimer = gpGlobals->curtime + 1;
				m_iJuggernaughtScore++;
			}
		}

		//MOVE THIS TO GAMERULES
		if ( m_iJuggernaughtScore == of_juggernaught_wintime.GetInt() )
			TFGameRules()->SetWinningTeam(TF_TEAM_MERCENARY, WINREASON_JUGGERNAUGHT_TIMER, true, true, false);;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Precache()
{
	// Precache the player models and gibs.
	// this is handled in CTFGameRules instead now
	//PrecachePlayerModels();

	// Precache the player sounds.
	PrecacheScriptSound( "Player.Spawn" );
	PrecacheScriptSound( "Player.Gib" );
	PrecacheScriptSound( "TFPlayer.Pain" );
	PrecacheScriptSound( "TFPlayer.CritHit" );
	PrecacheScriptSound( "TFPlayer.LegShot" );
	PrecacheScriptSound( "TFPlayer.CritPain" );
	PrecacheScriptSound( "TFPlayer.CritDeath" );
	PrecacheScriptSound( "TFPlayer.FreezeCam" );
	PrecacheScriptSound("TFPlayer.MeleeReflectImpact");
	PrecacheScriptSound( "TFPlayer.Drown" );
	PrecacheScriptSound( "TFPlayer.AttackerPain" );
	PrecacheScriptSound( "TFPlayer.SaveMe" );
	PrecacheScriptSound( "TFPlayer.MedicChargedDeath" );
	PrecacheScriptSound( "TFPlayer.FlameOut" );
	PrecacheScriptSound( "Camera.SnapShot" );

	PrecacheScriptSound( "Game.YourTeamLost" );
	PrecacheScriptSound( "Game.YourTeamWon" );
	PrecacheScriptSound( "Game.SuddenDeath" );
	PrecacheScriptSound( "Game.Stalemate" );
	PrecacheScriptSound( "TV.Tune" );

	PrecacheScriptSound( "Player.ZombieLunge" );
	PrecacheScriptSound( "Game.Infection.YourTeamLost" );
	PrecacheScriptSound( "Game.Infection.YourTeamWon" );
	PrecacheScriptSound( "Game.Infection.Infected" );
	PrecacheScriptSound( "Game.Infection.Warmup" );
	PrecacheScriptSound( "Game.Infection.Begin" );

	// Precache particle systems
	PrecacheParticleSystem( "crit_text" );
	PrecacheParticleSystem( "headshot_of" );
	PrecacheParticleSystem( "cig_smoke" );
	PrecacheParticleSystem( "speech_mediccall" );
	PrecacheParticleSystem( "speech_typing" );
	PrecacheParticleSystem( "player_recent_teleport_red" );
	PrecacheParticleSystem( "player_recent_teleport_blue" );
	PrecacheParticleSystem( "player_recent_teleport_dm" );
	PrecacheParticleSystem( "particle_nemesis_red" );
	PrecacheParticleSystem( "particle_nemesis_blue" );
	PrecacheParticleSystem( "particle_nemesis_dm" );
	PrecacheParticleSystem( "spy_start_disguise_red" );
	PrecacheParticleSystem( "spy_start_disguise_blue" );
	PrecacheParticleSystem( "spy_start_disguise_dm" );
	PrecacheParticleSystem( "burningplayer_red" );
	PrecacheParticleSystem( "burningplayer_blue" );
	PrecacheParticleSystem( "burningplayer_dm" );
	PrecacheParticleSystem( "blood_spray_red_01" );
	PrecacheParticleSystem( "blood_spray_red_01_far" );
	PrecacheParticleSystem( "blood_trail_red_01_boom" );
	PrecacheParticleSystem( "tfc_sniper_mist" );
	PrecacheParticleSystem( "water_blood_impact_red_01" );
	PrecacheParticleSystem( "blood_impact_red_01" );
	PrecacheParticleSystem( "water_playerdive" );
	PrecacheParticleSystem( "water_playeremerge" );
	PrecacheParticleSystem( "electrocuted_gibbed_red" );
	PrecacheParticleSystem( "electrocuted_gibbed_blue" );
	PrecacheParticleSystem( "electrocuted_gibbed_dm" );
	PrecacheParticleSystem( "electrocutedplayer_red" );
	PrecacheParticleSystem( "electrocutedplayer_blue" );
	PrecacheParticleSystem( "electrocutedplayer_dm" );

	// should hopefully stop stuttering
	PrecacheParticleSystem( "ExplosionCore_wall" );
	PrecacheParticleSystem( "ExplosionCore_MidAir" );
	PrecacheParticleSystem( "ExplosionCore_MidAir_underwater" );
	PrecacheParticleSystem( "mlg_explosion_primary" );

	PrecacheScriptSound( "HL2Player.FlashLightOn" );
	PrecacheScriptSound( "HL2Player.FlashLightOff" );

	// needed so the stickybomb launcher charging plays...
	PrecacheScriptSound( "Weapon_StickyBombLauncher.ChargeUp" );
	
	PrecacheScriptSound( "HeartbeatLoop" );
	PrecacheScriptSound( "Weapon_General.CritPower" );
	PrecacheScriptSound( "Weapon_General.ProjectileReflect");
	
	for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		PrecacheScriptSound ( UTIL_VarArgs( "%s.Jumpsound", g_aPlayerClassNames_NonLocalized[i] ) );

	// needed to suppress console spam about weapon spawners
	PrecacheMaterial( "VGUI/flagtime_full" );
	PrecacheMaterial( "VGUI/flagtime_empty" );

	BaseClass::Precache();
}

extern const char *s_aPlayerClassFiles[];

//-----------------------------------------------------------------------------
// Purpose: Precache the player models and player model gibs.
//-----------------------------------------------------------------------------
void CTFPlayer::PrecachePlayerModels( void )
{
	// DEPRECATED, this is now done in TFGameRules instead
#if 0
	int i;
	for ( i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		const unsigned char *pKey = NULL;

		if ( g_pGameRules )
		{
			pKey = g_pGameRules->GetEncryptionKey();
		}

		KeyValues *kvFinal = ReadEncryptedKVFile( filesystem, s_aPlayerClassFiles[i], pKey );
		if( !kvFinal )
			continue;
		
		const char *pszModel = kvFinal->GetString("model");
		if ( pszModel && pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}

		const char *pszModelArm = kvFinal->GetString("arm_model");
		if ( pszModelArm && pszModelArm[0] )
		{
			PrecacheModel( pszModelArm );
		}
		
		PrecacheScriptSound( kvFinal->GetString("sound_death") );
		PrecacheScriptSound( kvFinal->GetString("sound_crit_death") );
		PrecacheScriptSound( kvFinal->GetString("sound_melee_death") );
		PrecacheScriptSound( kvFinal->GetString("sound_explosion_death") );
		
		// TFC
		KeyValues* kvModifier = kvFinal->FindKey("TFC");
		if( kvModifier )
		{
			pszModel = kvModifier->GetString("model");
			if ( pszModel && pszModel[0] )
			{
				int iModel = PrecacheModel( pszModel );
				PrecacheGibsForModel( iModel );
			}

			pszModelArm = kvModifier->GetString("arm_model");
			if ( pszModelArm && pszModelArm[0] )
			{
				PrecacheModel( pszModelArm );
			}
			
			PrecacheScriptSound( kvModifier->GetString("sound_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
		}
		
		kvModifier = kvFinal->FindKey("Zombie");
		if( kvModifier )
		{
			pszModel = kvModifier->GetString("model");
			if ( pszModel && pszModel[0] )
			{
				int iModel = PrecacheModel( pszModel );
				PrecacheGibsForModel( iModel );
			}

			pszModelArm = kvModifier->GetString("arm_model");
			if ( pszModelArm && pszModelArm[0] )
			{
				PrecacheModel( pszModelArm );
			}	
			
			PrecacheScriptSound( kvModifier->GetString("sound_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
			PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
		}
	}
	PrecacheModel(DM_SHIELD_MODEL);

	KeyValues* pItemsGame = new KeyValues( "items_game" );
	pItemsGame->LoadFromFile( filesystem, "scripts/items/items_game.txt" );
	if ( pItemsGame )
	{
		KeyValues* pCosmetics = pItemsGame->FindKey( "Cosmetics" );
		if ( pCosmetics )
		{
			for ( KeyValues *pCosmetic = pCosmetics->GetFirstSubKey(); pCosmetic; pCosmetic = pCosmetic->GetNextKey() )
			{
				if ( pCosmetic )
				{
					if( Q_stricmp(pCosmetic->GetString( "Model" ), "BLANK") )
						PrecacheModel( pCosmetic->GetString( "Model" ) );
				}
			}
		}
	}
	pItemsGame->deleteThis();
//	const char *pszArmModel = GetPlayerClassData(i)->m_szArmModelName;
//	if ( pszArmModel && pszArmModel[0] )
//	{
//		PrecacheModel( pszArmModel );
//	}	

	PrecacheModel( "models/player/spy_mask.mdl" );

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( i = 1; i < ARRAYSIZE(g_pszBDayGibs); i++ )
		{
			PrecacheModel( g_pszBDayGibs[i] );
		}
		PrecacheModel(BDAY_HAT_MODEL);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToPlay( void )
{
	// Make sure real players have their loadout registered
	if( !IsFakeClient() )
	{
		COFPlayerLoadout *pLoadout = LoadoutManager()->GetLoadout(this);
		if( !pLoadout )
		{
			if( LoadoutRequestTimer.IsElapsed() || !LoadoutRequestTimer.HasStarted() )
			{
				// TODO: If it causes issues
				// We can remove this command and make it a proper function on the client
				// triggering it via gameevents
				engine->ClientCommand( this->edict(), "send_loadout_to_server" );

				LoadoutRequestTimer.Start( OF_LOADOUT_REQUEST_TIMER );
			}
			return false;
		}
	}

	return GetTeamNumber() > LAST_SHARED_TEAM && GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToSpawn( void )
{
	if ( IsClassMenuOpen() )
	{
		return false;
	}

	if ((TFGameRules() && TFGameRules()->IsArenaGamemode()) && (m_Shared.GetTFLives() <= 0))
	{
		if (!(TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->InSetup()))
		{
			return false;
		}
	}

	return ( StateGet() != TF_STATE_DYING );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
//			when they next finish picking a class.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGainInstantSpawn( void )
{
		return ( GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED || IsClassMenuOpen() || of_instantrespawn.GetBool() );
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores
//-----------------------------------------------------------------------------
void CTFPlayer::ResetScores( void )
{
	CTF_GameStats.ResetPlayerStats( this );
	RemoveNemesisRelationships();
	BaseClass::ResetScores();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	SetWeaponBuilder( NULL );
	
	m_iMaxSentryKills = 0;
	CTF_GameStats.Event_MaxSentryKills( this, 0 );

	StateEnter( TF_STATE_WELCOME );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Spawn()
{
	MDLCACHE_CRITICAL_SECTION();

	if ( TFGameRules() && TFGameRules()->IsInfGamemode() )
	{
		// Players on BLU are set to zombies, otherwise, set to survivor
		m_Shared.SetZombie( GetTeamNumber() == TF_TEAM_BLUE );
	}

	//DevMsg("Lives = %i \n", m_Shared.GetTFLives());

	if ( !IsPlayerClass( TF_CLASS_MEDIC ) )
		SetContextThink( NULL, gpGlobals->curtime, "MedicRegenThink" );

	if ( TFGameRules() && !TFGameRules()->IsInfGamemode() )
		SetContextThink( NULL, gpGlobals->curtime, "ZombieRegenThink" );

	m_flSpawnTime = gpGlobals->curtime;
	UpdateModel();

	SetMoveType( MOVETYPE_WALK );
    if (GetTeamNumber() != TEAM_SPECTATOR)
        SetRenderMode(kRenderNormal);//dont think this code is even called when youre spec but idk

	m_hSuperWeapons.Purge();
	m_hPowerups.Purge();
	ClearSlots();
	
	m_iMegaOverheal = 0;
	
	BaseClass::Spawn();
		
	SetLives(m_Shared.GetTFLives());
	
//	DestroyViewModels();
	
	CreateViewModel( TF_VIEWMODEL_WEAPON );
	CreateViewModel( TF_VIEWMODEL_ARMS );
	CreateViewModel( TF_VIEWMODEL_COSMETICS );
	
	// Make sure it has no model set, in case it had one before
	if ( IsPlayerClass ( TF_CLASS_SPY ) )
	{
		CreateViewModel( TF_VIEWMODEL_SPYWATCH );
		
		if ( GetViewModel( TF_VIEWMODEL_SPYWATCH ) )
			GetViewModel( TF_VIEWMODEL_SPYWATCH )->SetModel( "" );
	}
	
	GetViewModel(TF_VIEWMODEL_ARMS)->SetModel( GetPlayerClass()->GetArmModelName() );

	GetViewModel(TF_VIEWMODEL_ARMS)->m_nSkin = GetTeamNumber() - 2;
	
	if ( GetViewModel( TF_VIEWMODEL_COSMETICS ) )
		GetViewModel( TF_VIEWMODEL_COSMETICS )->SetModel( "" );

	m_Shared.bWatchReady = false; // Ok so fuck the watch, because for some reason it deploys on equip, this is later used for shit in tf_weapon_invis
	
	CreateViewModel();
	
	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on.
	// So if we're in the welcome state, call its enter function to reset 
	if ( m_Shared.InState( TF_STATE_WELCOME ) )
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if ( m_Shared.InState( TF_STATE_DYING ) )
	{
		StateTransition( TF_STATE_ACTIVE );
	}

	// If they're spawning into the world as fresh meat, give them items and stuff.
	if ( m_Shared.InState( TF_STATE_ACTIVE ) )
	{
		// remove our disguise each time we spawn
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			m_Shared.RemoveDisguise();
		}

		if ( m_bDied )
			EmitSound( "Player.Spawn" );
		else
		{
			CSingleUserRecipientFilter filter( this );	// We only play your own Spawn sound on round start
			EmitSound( filter, entindex(), "Player.Spawn" );
		}

		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );
		InitClass();
		m_Shared.RemoveAllCond( NULL ); // Remove conc'd, burning, rotting, hallucinating, etc.

		UpdateSkin( GetTeamNumber() );
		TeamFortress_SetSpeed();

		// Prevent firing for a second so players don't blow their faces off
		SetNextAttack( gpGlobals->curtime + 1.0 );

		DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

		// Force a taunt off, if we are still taunting, the condition should have been cleared above.
		if( m_hTauntScene.Get() )
		{
			StopScriptedScene( this, m_hTauntScene );
			m_Shared.m_flTauntRemoveTime = 0.0f;
			m_hTauntScene = NULL;
		}

		// turn on separation so players don't get stuck in each other when spawned
		// don't attempt separation if we are in deathmatch
		if ( TFGameRules() && !TFGameRules()->IsDMGamemode() )
		{
			m_Shared.SetSeparation(true);
			m_Shared.SetSeparationVelocity(vec3_origin);
		}

		RemoveTeleportEffect();
	
		//If this is true it means I respawned without dying (changing class inside the spawn room) but doesn't necessarily mean that my healers have stopped healing me
		//This means that medics can still be linked to me but my health would not be affected since this condition is not set.
		//So instead of going and forcing every healer on me to stop healing we just set this condition back on. 
		//If the game decides I shouldn't be healed by someone (LOS, Distance, etc) they will break the link themselves like usual.
		if ( m_Shared.GetNumHealers() > 0 )
		{
			m_Shared.AddCond( TF_COND_HEALTH_BUFF );
		}

		//in duel spawn protection the first spawn only
		if ( ( TFGameRules()->IsDMGamemode() || of_forcespawnprotect.GetBool() == 1 ) && !( TFGameRules()->IsDuelGamemode() && int( MaxSpeed() ) != 1 ) )
			m_Shared.AddCond( TF_COND_SPAWNPROTECT , of_spawnprotecttime.GetFloat() );

		m_Shared.SetSpawnEffect( V_atoi(engine->GetClientConVarValue(entindex(), "of_respawn_particle")) );
		
		if ( !m_bSeenRoundInfo )
		{
			TFGameRules()->ShowRoundInfoPanel( this );
			m_bSeenRoundInfo = true;
		}

		if ( IsInCommentaryMode() && !IsFakeClient() )
		{
			// Player is spawning in commentary mode. Tell the commentary system.
			CBaseEntity *pEnt = NULL;
			variant_t emptyVariant;
			while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "commentary_auto" )) != NULL )
			{
				pEnt->AcceptInput( "MultiplayerSpawned", this, this, emptyVariant, 0 );
			}
		}
	}
	
	m_purgatoryDuration.Invalidate();
	m_lastCalledMedic.Invalidate();

	CTF_GameStats.Event_PlayerSpawned( this );

	m_iSpawnCounter = !m_iSpawnCounter;
	m_bAllowInstantSpawn = false;

	m_bWinDeath = false;

	m_bHauling = false;

	m_Shared.SetSpyCloakMeter( 100.0f );

	m_Shared.ClearDamageEvents();
	ClearDamagerHistory();

	m_flLastDamageTime = 0;

	m_flNextVoiceCommandTime = gpGlobals->curtime;

	ClearZoomOwner();
	SetFOV( this , 0 );

	SetViewOffset( GetClassEyeHeight() );

	ClearExpression();
	m_flNextSpeakWeaponFire = gpGlobals->curtime;

	m_bIsIdle = false;
	//m_flPowerPlayTime = 0.0;

	m_Shared.m_flNextLungeTime = 0.0f;

	UpdatePlayerColor();

	// Gore
	m_iGoreHead = 0;
	m_iGoreLeftArm = 0;
	m_iGoreRightArm = 0;
	m_iGoreLeftLeg = 0;
	m_iGoreRightLeg = 0;

	// This makes the surrounding box always the same size as the standing collision box
	// helps with parts of the hitboxes that extend out of the crouching hitbox, eg with the
	// heavyweapons guy
	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	if( m_bDied )
	{
		if(m_bGotKilled)
		{
			CFmtStrN<128> modifiers( "gotkilled:true" );
			SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_RESPAWN, modifiers );
		}
		else
			SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_RESPAWN );
	}

	m_bGotKilled = false;
	m_bDied = false;

	m_iTaunt = -1;
	
	UpdateModel();

	// the Civilian glows to his own teammates in Escort
	if ( TFGameRules() && TFGameRules()->IsESCGamemode() )
	{
		if ( IsPlayerClass( TF_CLASS_CIVILIAN ) )
		{
			SetTransmitState( FL_EDICT_ALWAYS );
			//AddGlowEffect();
		}
		else
		{
			//RemoveGlowEffect();
		}
	}
	
	UpdateCosmetics();
	
	AI_CriteriaSet pTempSet;
	ModifyOrAppendCriteria(pTempSet);
	char szMutator[64];
	Q_strncpy( szMutator, pTempSet.GetValue(pTempSet.FindCriterionIndex("playermutator")), sizeof(szMutator));
	strlwr(szMutator);

	m_bIsRobot = FStrEq(szMutator, "robot");

	//reset medal related values
	m_bHadPowerup = false;
	m_iPowerupKills = 0;
	m_iEXKills = 0;
	m_fEXTime = 0;
	m_fAirStartTime = 0.f;
	m_iSpreeKills = 0;
	m_iImpressiveCount = 0;
	TFGameRules()->ResetDeathInflictor(entindex());
	if (GetBotController())
	{
		GetBotController()->Spawn();
	}
}

void CTFPlayer::UpdateCosmetics()
{
	m_flCosmetics.Purge();
	NetworkStateChanged();
	
	bool bIsTempBot = GetBotController() && static_cast<CBot*>(GetBotController())->RemoveOnDeath();

    if( GetPlayerClass()->IsClass( TF_CLASS_MERCENARY ) && GetTeamNumber() != TEAM_SPECTATOR || bIsTempBot )
	{
		KeyValues *kvDesiredCosmetics = new KeyValues( "DesiredCosmetics" );

		CUtlVector<float> flDesiredCosmetics;
		COFPlayerLoadout *pLoadout = LoadoutManager()->GetLoadout( this );

		// Drop a pack with their leftover ammo
		// This is terrible but it works for now
		if( pLoadout ) //|| bIsTempBot )
		{
			int index = GetPlayerClass()->GetClassIndex();
			FOR_EACH_DICT(pLoadout->m_ClassLoadout[index].m_Cosmetics, i)
			{
				flDesiredCosmetics.AddToTail(pLoadout->m_ClassLoadout[index].m_Cosmetics[i]);
			}
		}
		else
		{
			// Uncomment this to test all blank loadout slots
			// Q_strncpy(szDesired,"0 25 27 13 40 17 28", sizeof(szDesired));
			int iCosmeticCount = random->RandomInt( 0, 5 );
			int iMaxCosNum = GetItemSchema()->GetCosmeticCount();
			for( int i = 0; i < iCosmeticCount; i++ )
			{	
				flDesiredCosmetics.AddToTail( random->RandomInt( 0, iMaxCosNum ) );
			}
		}

		bool bArmCosmetic = false;
		bool bGloveCosmetic = false;
		
		for( int i = 0; i < flDesiredCosmetics.Count(); i++ )
		{
			COFCosmeticInfo *pCosmetic = GetItemSchema()->GetCosmetic( (int)( flDesiredCosmetics[i] ) );
			if( !pCosmetic )
				continue;
			
			float flCosmetic = flDesiredCosmetics[i];
			// This is real hacky but more often than not the float is missing 1 for some reason
			// So just add that back, it gets rounded down anyways so we dont need to worry about it
			flCosmetic += 0.001;

			flCosmetic = flCosmetic - (int)flCosmetic;
			
			flCosmetic *= 100;
			
			int iStyle = (int)flCosmetic;

			pCosmetic = pCosmetic->GetStyle(iStyle);
			
			const char *szRegion = pCosmetic->m_szRegion;
			kvDesiredCosmetics->SetString( szRegion, UTIL_VarArgs( "%f", flDesiredCosmetics[i] ) );

			if ( !Q_stricmp( szRegion, "gloves" ) )
			{
				m_chzVMCosmeticGloves = pCosmetic->m_szViewModel ? pCosmetic->m_szViewModel : "models/weapons/c_models/cosmetics/merc/gloves/default.mdl";
				if ( GetViewModel( TF_VIEWMODEL_ARMS ) )
				{
					bGloveCosmetic = true;
					if ( m_chzVMCosmeticGloves )
						GetViewModel( TF_VIEWMODEL_ARMS )->SetModel( m_chzVMCosmeticGloves );

					int iVisibleTeam = 0;
					int iTeamCount = 1;
					if( pCosmetic->m_bTeamSkins )
					{
						iTeamCount = 3;
						iVisibleTeam = GetTeamNumber();
						

						iVisibleTeam = iVisibleTeam - 2;
					}
					if( pCosmetic->m_bUsesBrightskins )
					{
						iTeamCount++;
						if( V_atoi( engine->GetClientConVarValue(entindex(), "of_tennisball") ) == 1 )
							iVisibleTeam = iTeamCount - 1;
					}
					
					GetViewModel( TF_VIEWMODEL_ARMS )->m_nSkin = iVisibleTeam < 0 ? 0 : iVisibleTeam;

					int iSkin = iTeamCount * pCosmetic->m_iSkinOffset;
					
					GetViewModel( TF_VIEWMODEL_ARMS )->m_nSkin += iSkin;
				}
			}
			else if ( !Q_stricmp(szRegion, "suit") )
			{
				m_chzVMCosmeticSleeves = pCosmetic->m_szViewModel ? pCosmetic->m_szViewModel : "models/weapons/c_models/cosmetics/merc/sleeves/default.mdl";		
				if ( GetViewModel( TF_VIEWMODEL_COSMETICS ) )
				{
					bArmCosmetic = true;
					if ( m_chzVMCosmeticSleeves )
						GetViewModel( TF_VIEWMODEL_COSMETICS )->SetModel( m_chzVMCosmeticSleeves );
					
					int iVisibleTeam = 0;
					int iTeamCount = 1;
					if( pCosmetic->m_bTeamSkins )
					{
						iTeamCount = 3;
						iVisibleTeam = GetTeamNumber();
						

						iVisibleTeam = iVisibleTeam - 2;
					}
					if( pCosmetic->m_bUsesBrightskins )
					{
						iTeamCount++;
						if( V_atoi( engine->GetClientConVarValue(entindex(), "of_tennisball") ) == 1 )
							iVisibleTeam = iTeamCount - 1;
					}
					
					GetViewModel( TF_VIEWMODEL_COSMETICS )->m_nSkin = iVisibleTeam < 0 ? 0 : iVisibleTeam;

					int iSkin = iTeamCount * pCosmetic->m_iSkinOffset;
					
					GetViewModel( TF_VIEWMODEL_COSMETICS )->m_nSkin += iSkin;
				}
			}
			// undone: causes too much stuttering, its now done in tf_gamerules precache instead
			//const char *pModel = pCosmetic->GetString( "Model" , "models/error.mdl" );
			//PrecacheModel( pModel );
		}

		if( V_atoi(engine->GetClientConVarValue(entindex(), "of_disable_viewmodel_cosmetics")) == 1 )
		{
			bArmCosmetic = false;
			bGloveCosmetic = false;
		}
		
		if( m_Shared.IsZombie() )
		{
			bArmCosmetic = false;
			bGloveCosmetic = false;
		}

		if( !bArmCosmetic && GetViewModel( TF_VIEWMODEL_COSMETICS ) )
			GetViewModel( TF_VIEWMODEL_COSMETICS )->SetModel( "models/weapons/c_models/cosmetics/merc/sleeves/default.mdl" );

		if( !bGloveCosmetic && GetViewModel( TF_VIEWMODEL_ARMS ) )
			GetViewModel( TF_VIEWMODEL_ARMS )->SetModel( "models/weapons/c_models/cosmetics/merc/gloves/default.mdl" );

		if( m_Shared.IsZombie() )
		{
			GetViewModel(TF_VIEWMODEL_COSMETICS)->SetModel("");
			GetViewModel(TF_VIEWMODEL_ARMS)->SetModel("models/weapons/c_models/c_zombie_merc_arms.mdl");
		}

		KeyValues *pHat = kvDesiredCosmetics->GetFirstValue();
		for( pHat; pHat != NULL; pHat = pHat->GetNextValue() ) // Loop through all the keyvalues
		{
			m_flCosmetics.AddToTail(pHat->GetFloat());
			NetworkStateChanged();
		}

		if( kvDesiredCosmetics )
			kvDesiredCosmetics->deleteThis();
	}
	
	GetTFPlayerResource()->UpdatePlayerCosmetics( this );
}

void CTFPlayer::ForceEquipCosmetics( const float *flCosmetics, int iCount, int iClass )
{
	if( !LoadoutManager() )
		return;

	if( !flCosmetics )
		return;

	COFPlayerLoadout *pLoadout = LoadoutManager()->GetLoadout( this );

	if( !pLoadout )
		return;

	for( int i = 0; i < iCount; i++ )
	{
		float flCosmetic = flCosmetics[i];

		COFCosmeticInfo *pCosmetic = GetItemSchema()->GetCosmetic((int)flCosmetic);

		if( !pCosmetic )
			continue;

		const char *szCategory = pCosmetic->m_szRegion;

		unsigned short handle = pLoadout->m_ClassLoadout[iClass].m_Cosmetics.Find(szCategory);

		if( handle == pLoadout->m_ClassLoadout[iClass].m_Cosmetics.InvalidIndex() )
			pLoadout->m_ClassLoadout[iClass].m_Cosmetics.Insert( szCategory, flCosmetic );
		else
			pLoadout->m_ClassLoadout[iClass].m_Cosmetics[handle] = flCosmetic;
	}
}

void CTFPlayer::ForceEquipCosmetics( const char *szCosmetics, int iClass )
{
	if( !szCosmetics )
		return;

	CCommand cosmetics;
	cosmetics.Tokenize(szCosmetics);

	float *flCosmetics = new float[cosmetics.ArgC()];

	for( int i = 0; i < cosmetics.ArgC(); i++ )
	{
		flCosmetics[i] = atof(cosmetics[i]);
	}

	ForceEquipCosmetics( flCosmetics, cosmetics.ArgC(), iClass );

	delete[] flCosmetics;
}

void CTFPlayer::EquipRandomCosmetics( int iClass, int iCount )
{
	if( iCount > GetItemSchema()->GetCategoryCount() )
		iCount = GetItemSchema()->GetCategoryCount();

	float *flCosmetics = new float[iCount];

	int i = 0;
	while( i < iCount )
	{
		int iIndex = random->RandomInt( 0, GetItemSchema()->GetCosmeticCount() );
		COFCosmeticInfo *pCurrentCosmetic = GetItemSchema()->GetCosmetic( iIndex );
		if( !pCurrentCosmetic )
			continue;

		bool bInvalid = false;
		for( int y = 0; y < i; y++ )
		{
			COFCosmeticInfo *pEquippedCosmetic = GetItemSchema()->GetCosmetic( (int)flCosmetics[y] );

			if( pCurrentCosmetic->m_szRegion == pEquippedCosmetic->m_szRegion )
			{
				bInvalid = true;
				break;
			}
		}

		if( bInvalid )
			continue;

		flCosmetics[i] = iIndex;
		flCosmetics[i] += (float)(random->RandomInt( 0, pCurrentCosmetic->m_StyleInfo.Count())) / 100.0f;
		i++;
	}

	ForceEquipCosmetics( flCosmetics, iCount, iClass );

	delete[] flCosmetics;
}

void CTFPlayer::ClearSlots()
{
	// Cant be assed to clear this a different way ¯\_(ツ)_/¯ 
	// If you know a cleaner solution feel free to use it - Kay
	memset( m_hWeaponInSlot, NULL, sizeof(m_hWeaponInSlot[0][0]) * 200 );	
}

//-----------------------------------------------------------------------------
// Purpose: Removes all nemesis relationships between this player and others
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveNemesisRelationships()
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != this )
		{
			// set this player to be not dominating anyone else
			m_Shared.SetPlayerDominated( pTemp, false );

			// set no one else to be dominating this player
			pTemp->m_Shared.SetPlayerDominated( this, false );
		}
	}	
	// reset the matrix of who has killed whom with respect to this player
	CTF_GameStats.ResetKillHistory( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayer::Regenerate( void )
{
	// We may have been boosted over our max health. If we have, 
	// restore it after we reset out class values.
	int iCurrentHealth = GetHealth();
	m_bRegenerating = true;
	
	// Set initial health and armor based on class.
	SetMaxHealth( GetPlayerClass()->GetMaxHealth() );

	if ( 
		( TFGameRules()->m_bArmorOnSpawn && of_armor_on_spawn.GetInt() < 0)
		|| of_armor_on_spawn.GetInt() > 0
		)
	{
		m_Shared.SetTFCArmor(GetPlayerClass()->GetMaxArmor());
	}

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	SetHealth( GetMaxHealth() );

	if( TFGameRules()->IsMutator( ARSENAL ) &&
		( !of_merc_only_arsenal.GetBool() || m_PlayerClass.IsClass(TF_CLASS_MERCENARY) ) )
	{
		TFPlayerClassData_t *pData = m_PlayerClass.GetData();
		ManageArsenalWeapons(pData);

		SelectDefaultWeapon();
	}
	
	// RestockAmmo
	RestockClips( 1.0f );
	RestockAmmo( 1.0f );
	RestockMetal( 1.0f );
	RestockCloak( 1.0f );
	RestockSpecialEffects( 1.0f );
	
	UpdatePlayerColor();
	UpdateCosmetics();
	
	m_bResupplied = !m_bResupplied;
	
	m_bRegenerating = false;
	if ( iCurrentHealth > GetHealth() )
		SetHealth( iCurrentHealth );

	if ( m_Shared.InCond(TF_COND_BURNING))
		m_Shared.RemoveCond(TF_COND_BURNING);

	if (m_Shared.InCond(TF_COND_POISON))
		m_Shared.RemoveCond(TF_COND_POISON);

	if (m_Shared.InCond(TF_COND_TRANQ))
		m_Shared.RemoveCond(TF_COND_TRANQ);

	if (m_Shared.InCond(TF_COND_PIERCED_LEGS))
		m_Shared.RemoveCond(TF_COND_PIERCED_LEGS);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::InitClass( void )
{

	// Set initial health and armor based on class.
	SetMaxHealth( GetPlayerClass()->GetMaxHealth() );

	if (
		(TFGameRules()->m_bArmorOnSpawn && of_armor_on_spawn.GetInt() < 0)
		|| of_armor_on_spawn.GetInt() > 0
		)
	{
		m_Shared.SetTFCArmor(GetPlayerClass()->GetMaxArmor());
	}
	else
	{
		m_Shared.SetTFCArmor(0);
	}

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	SetHealth( GetMaxHealth() );

	// Give default items for class.
	GiveDefaultItems();

	UpdatePlayerColor();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CreateViewModel( int iViewModel )
{
	Assert( iViewModel >= 0 && iViewModel < MAX_VIEWMODELS );
	if ( GetViewModel( iViewModel ) )
		return;
	
	CTFViewModel *pViewModel = ( CTFViewModel * )CreateEntityByName( "tf_viewmodel" );
	if ( pViewModel )
	{
		pViewModel->SetAbsOrigin( GetAbsOrigin() );
		pViewModel->SetOwner( this );
		pViewModel->SetIndex( iViewModel );
		pViewModel->RemoveEffects(EF_BONEMERGE);
		DispatchSpawn( pViewModel );
		pViewModel->FollowEntity( this, false );
		pViewModel->SetParent( NULL );
		pViewModel->SetLightingOrigin( GetLightingOrigin() );
		m_hViewModel.Set( iViewModel, pViewModel );
	}
	
	CTFViewModel *vmhands = ( CTFViewModel * )CreateEntityByName( "tf_viewmodel" );
	if ( vmhands && iViewModel+2 != 3 )
	{
		vmhands->SetAbsOrigin( GetAbsOrigin() );
		vmhands->SetOwner( this );
		vmhands->SetParent( this );
		vmhands->SetIndex( iViewModel+2 );
		DispatchSpawn(vmhands);
		vmhands->m_nSkin = m_nSkin;
		vmhands->SetLocalOrigin( vec3_origin );
		vmhands->FollowEntity( pViewModel, true );
		vmhands->SetModel( GetPlayerClass()->GetArmModelName() );
		vmhands->SetLightingOrigin( pViewModel->GetLightingOrigin() );
		m_hViewModel.Set( iViewModel+2, vmhands );

		CTFViewModel *vmsleeves = (CTFViewModel *)CreateEntityByName("tf_viewmodel");
		if (vmsleeves && iViewModel + 3 != 4)
		{
			vmsleeves->SetAbsOrigin(GetAbsOrigin());
			vmsleeves->SetOwner(this);
			vmsleeves->SetParent(this);
			vmsleeves->SetIndex(iViewModel + 3);
			DispatchSpawn(vmsleeves);
			vmsleeves->m_nSkin = m_nSkin;
			vmsleeves->SetLocalOrigin(vec3_origin);
			vmsleeves->FollowEntity(pViewModel, true);
			vmsleeves->SetModel(GetPlayerClass()->GetArmModelName());
			vmsleeves->SetLightingOrigin(pViewModel->GetLightingOrigin());
			m_hViewModel.Set(iViewModel + 3, vmsleeves);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the view model for the player's off hand
//-----------------------------------------------------------------------------
CBaseViewModel *CTFPlayer::GetOffHandViewModel()
{
	// off hand model is slot 1
	return GetViewModel( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends the specified animation activity to the off hand view model
//-----------------------------------------------------------------------------
void CTFPlayer::SendOffHandViewModelActivity( Activity activity )
{
	CBaseViewModel *pViewModel = GetOffHandViewModel();
	if ( pViewModel )
	{
		int sequence = pViewModel->SelectWeightedSequence( activity );
		pViewModel->SendViewModelMatchingSequence( sequence );
	}
}

void CTFPlayer::AddResponseCriteria( const char *szName, const char *szValue )
{
	custom_criteria_t pNew;
	Q_strncpy( pNew.szName, szName, sizeof(pNew.szName) );
	Q_strncpy( pNew.szValue, szValue, sizeof(pNew.szValue) );

	m_hCustomCriteria.AddToTail(pNew);
}

void CTFPlayer::AddResponseCriteria( char *szName, char *szValue )
{
	custom_criteria_t pNew;
	Q_strncpy( pNew.szName, szName, sizeof(pNew.szName) );
	Q_strncpy( pNew.szValue, szValue, sizeof(pNew.szValue) );

	m_hCustomCriteria.AddToTail(pNew);
}

//-----------------------------------------------------------------------------
// Purpose: Set the player up with the default weapons, ammo, etc.
//-----------------------------------------------------------------------------
void CTFPlayer::GiveDefaultItems()
{
	// Get the player class data.
	TFPlayerClassData_t *pData = m_PlayerClass.GetData();

	RemoveAllAmmo();

	// Give ammo. Must be done before weapons, so weapons know the player has ammo for them.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		GiveAmmo( pData->m_aAmmoMax[iAmmo], iAmmo );
	}
	
	// give only melees to zombies in infection
	if ( m_Shared.IsZombie() )
	{
		ManageRegularWeapons( pData );
	}
	else
	{
		// Give weapons.
		if ( of_randomizer.GetBool() && ManageRandomizerWeapons( pData ) )
		{
			// Its nothing ¯\_(ツ)_/¯
		}
		else if ( of_spawn_with_weapon.GetString() && !FStrEq( of_spawn_with_weapon.GetString(), "" ) )
			ManageCustomSpawnWeapons( pData );
		else if ( TFGameRules()->IsGGGamemode() )
			ManageGunGameWeapons( pData );
		else if ( TFGameRules()->IsMutator( INSTAGIB ) || TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) )
			ManageInstagibWeapons( pData );
		else if ( TFGameRules()->IsMutator( CLAN_ARENA ) || TFGameRules()->IsMutator( UNHOLY_TRINITY ) )
			ManageClanArenaWeapons( pData );
		else if ( TFGameRules()->IsMutator( ROCKET_ARENA ) )
			ManageRocketArenaWeapons( pData );
		else if (TFGameRules()->IsMutator(ARSENAL))
		{
			if( of_merc_only_arsenal.GetBool() && !m_PlayerClass.IsClass(TF_CLASS_MERCENARY))
			{
				ManageRegularWeapons(pData);
			}
			else
			{
				ManageArsenalWeapons(pData);
			}
		}
		else if ( TFGameRules()->IsMutator( ETERNAL ) )
			ManageEternalWeapons( pData );
		// Last one
		else if( TFGameRules()->GetPlayerInventorySystem(this) == OF_INVENTORY_SLOTS )
			ManageSlotSystemWeapons( pData );
		else
			ManageRegularWeapons( pData );
		
		if ( of_threewave.GetBool() )
			Manage3WaveWeapons( pData );
	}

	if ( !m_Shared.IsZombie() )
	{
		// Give a builder weapon for each object the player class is allowed to build
		switch ( GetPlayerClass()->GetClassIndex() )
		{
			case TF_CLASS_ENGINEER:
			case TF_CLASS_SPY:
				CUtlVector<int> hBuildings;
				for( int i = 0; i < TF_PLAYER_BUILDABLE_COUNT; i++ )
				{
					int iBuildingID = GetPlayerClass()->GetData()->m_aBuildable[i];
					if( iBuildingID == OBJ_LAST )
						break;

					if( GetObjectInfo( iBuildingID )->m_bVisibleInWeaponSelection )
						hBuildings.AddToTail( iBuildingID );
				}
				if( hBuildings.Count() )
					ManageBuilderWeapons( &hBuildings );
				break;
		}
	}
	
	// Moved all the Active/Last weapon stuff here
	if( m_bRegenerating == false )
		SelectDefaultWeapon();
}

void CTFPlayer::SelectDefaultWeapon( void )
{
	SetActiveWeapon( NULL );
	Weapon_SetLast( NULL );

	bool bDMBuckets = TFGameRules()->UsesDMBuckets();
	bDMBuckets &= !TFGameRules()->IsGGGamemode();
	bDMBuckets &= !TFGameRules()->IsMutator( ARSENAL );
	
	if( bDMBuckets )
	{
		for( int i = ARRAYSIZE(m_hWeaponInSlot)-1; i >= 0; i-- )
		{
			for( int y = ARRAYSIZE(m_hWeaponInSlot[i])-1; y >= 0; y-- )
			{
				if( !GetActiveWeapon() )
					Weapon_Switch( GetWeaponInSlot(i, y) );

				else if( GetWeaponInSlot(i, y) )
				{
					Weapon_SetLast( GetWeaponInSlot(i, y) );
					break;
				}
			}
			if( GetLastWeapon() )
				break;
		}
	}
	else
	{
		for( int i = 0; i < ARRAYSIZE(m_hWeaponInSlot); i++ )
		{
			for( int y = 0; y < ARRAYSIZE(m_hWeaponInSlot[i]); y++ )
			{
				if( !GetActiveWeapon() )
					Weapon_Switch( GetWeaponInSlot(i, y) );

				else if( GetWeaponInSlot(i, y) )
				{
					Weapon_SetLast( GetWeaponInSlot(i, y) );
					break;
				}
			}
			if( GetLastWeapon() )
				break;
		}
	}
}

void CTFPlayer::StripWeapons( void )
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		if ( pWeapon )
		{
			Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
		}
	}
	ClearSlots();
}

bool CTFPlayer::OwnsWeaponID( int ID )
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		if ( pWeapon && pWeapon->GetWeaponID() == ID )
		{
			return true;
		}
	}
	return false;
}

int CTFPlayer::GetCarriedWeapons( void )
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	int WeaponCount = 0;
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		//If we have a weapon in this slot, count up
		if ( pWeapon )
		{
			WeaponCount++;
		}
	}
	return WeaponCount;
}

int CTFPlayer::RestockClips( float PowerupSize )
{
	int bSuccess = false;
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		//If we have a weapon in this slot, count up
		if ( pWeapon )
		{
			if ( pWeapon->m_iClip1 < pWeapon->GetMaxClip1() )
			{
				pWeapon->m_iClip1 += pWeapon->GetMaxClip1() * PowerupSize;
				bSuccess = pWeapon->GetMaxClip1() * PowerupSize;
				if ( pWeapon->m_iClip1 > pWeapon->GetMaxClip1() )
				{
					bSuccess -= pWeapon->m_iClip1 - pWeapon->GetMaxClip1();
					pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
				}
				
			}
			
		}
	}
	return bSuccess;
}

int CTFPlayer::RestockAmmo( float PowerupSize, bool bIgnoreModifiers )
{
	int bSuccess = 0;
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		//If we have a weapon in this slot, count up
		if ( pWeapon )
			bSuccess += RestockAmmo( PowerupSize * (bIgnoreModifiers ? 1.0f : pWeapon->GetTFWpnData().m_flPickupMultiplier), pWeapon );
	}
	return bSuccess;
}

int CTFPlayer::RestockAmmo( float PowerupSize, CTFWeaponBase *pWeapon )
{
	int bSuccess = 0;
	int iClipDifference = pWeapon->GetMaxClip1() - pWeapon->m_iClip1;

	if( pWeapon->m_iReserveAmmo < pWeapon->GetMaxReserveAmmo() + iClipDifference )
	{
		pWeapon->m_iReserveAmmo += pWeapon->GetMaxReserveAmmo() * PowerupSize;
		bSuccess += pWeapon->GetMaxReserveAmmo() * PowerupSize;
		if ( pWeapon->m_iReserveAmmo > pWeapon->GetMaxReserveAmmo() + iClipDifference )
		{
			bSuccess -= pWeapon->m_iReserveAmmo - (pWeapon->GetMaxReserveAmmo() + iClipDifference);
			pWeapon->m_iReserveAmmo = pWeapon->GetMaxReserveAmmo() + iClipDifference;
		}
	}
	// Fallback in case we picked up ammo for an empty clip, but then used a ressuply cabinet
	else if( pWeapon->m_iReserveAmmo > pWeapon->GetMaxReserveAmmo() + iClipDifference )
		pWeapon->m_iReserveAmmo = pWeapon->GetMaxReserveAmmo() + iClipDifference;

	return bSuccess;
}

int CTFPlayer::RestockSpecialEffects( float PowerupSize )
{
	int bSuccess = 0;
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		if( pWeapon && pWeapon->IsMeleeWeapon() )
		{
			CTFWeaponBaseMelee *pMelee = dynamic_cast<CTFWeaponBaseMelee*>( pWeapon );
			if ( pMelee )
			{
				pMelee->SetShieldChargeMeter( PowerupSize );
				bSuccess = PowerupSize;
			}
		}
	}
	return bSuccess;
}

int CTFPlayer::RestockMetal( float PowerupSize )
{
	int bSuccess = false;
	int iMaxMetal = GetPlayerClass()->GetData()->m_aAmmoMax[TF_AMMO_METAL];
	if ( GiveAmmo( ceil(iMaxMetal * PowerupSize), TF_AMMO_METAL, true ) )
		bSuccess = ( ceil(iMaxMetal * PowerupSize), TF_AMMO_METAL, true );
	return bSuccess;
}
int CTFPlayer::RestockCloak( float PowerupSize )
{
	int bSuccess = false;
	
	float flCloak = m_Shared.GetSpyCloakMeter();
	if ( flCloak < 100.0f )
	{
		m_Shared.SetSpyCloakMeter( min( 100.0f, flCloak + PowerupSize * 100.0f ) );
		bSuccess = min( 100.0f, flCloak + PowerupSize * 100.0f );
	}	
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageBuilderWeapons( CUtlVector<int> *hBuildings )
{
	FOR_EACH_VEC( *hBuildings, i )
	{
		int iObjectID = hBuildings->Element(i);

		if( iObjectID == OBJ_LAST )
			break;

		// Objects visible in weapon selection are treated as their own weapons, ex: Sapper
		if( GetObjectInfo(iObjectID)->m_bVisibleInWeaponSelection )
		{
			if (!GetObjectInfo(iObjectID)->m_pObjectName)
				return;
			CTFWeaponBuilder *pObj = (CTFWeaponBuilder *)WeaponSchema_OwnsThisID(GetItemSchema()->GetWeaponID(GetObjectInfo(iObjectID)->m_pObjectName));
			// We own it already so just reset it
			if( pObj )
			{
				pObj->WeaponReset();
				continue;
			}

			pObj = (CTFWeaponBuilder *)GiveNamedItem( "tf_weapon_builder", iObjectID );
			pObj->SetSchemaName( GetObjectInfo(iObjectID)->m_pObjectName );
			pObj->m_iBuildableObjects.AddToTail(iObjectID);
			pObj->SetSubType( iObjectID );
			pObj->GiveTo(this);
		}
		else
		{
			CTFWeaponBuilder *pObj = (CTFWeaponBuilder*) WeaponSchema_OwnsThisID(GetItemSchema()->GetWeaponID("tf_weapon_builder"));
		
			if( !pObj )
			{
				pObj = (CTFWeaponBuilder *)GiveNamedItem( "tf_weapon_builder" );
				pObj->GiveTo(this);
			}

			if( !pObj->m_iBuildableObjects.HasElement(iObjectID) )
			{
				pObj->m_iBuildableObjects.AddToTail( iObjectID );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateGunGameLevel()
{
	if( GGLevel() < TFGameRules()->m_iMaxLevel - 1 )
		TFGameRules()->BroadcastSoundFFA( entindex(), "GG.RankUp" );
	else
		TFGameRules()->BroadcastSoundFFA( entindex(), "GG.YouGotKnife", "GG.EnemyGotKnife" );

	m_Shared.RemoveCond( TF_COND_AIMING );
	m_Shared.RemoveCond( TF_COND_AIMING_TFC );
	m_Shared.RemoveCond( TF_COND_AIMING_SCOPE_ONLY );
	m_Shared.RemoveCond( TF_COND_ZOOMED );
	TeamFortress_SetSpeed();
	int level = GGLevel();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	CTFWeaponBase *pNewWeapon = (CTFWeaponBase *)GiveNamedItem( STRING(TFGameRules()->m_iszWeaponName[level]) );
	for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
	{
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		if ( pWeapon && pNewWeapon && (pWeapon->GetGGLevel()< level || pWeapon->GetSlot() == pNewWeapon->GetSlot()) && !pWeapon->NeverStrip() )
		{
			Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
			pWeapon = NULL;
		}
	}
	if ( pNewWeapon )
	{
		pNewWeapon->GiveTo(this);
		pNewWeapon->SetGGLevel(level);
		Weapon_Switch(pNewWeapon, pNewWeapon->GetSlot() );
	}
	else
	{
		SwitchToNextBestWeapon( NULL );
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRegularWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	if ( of_startloadout.GetInt() > 0 )
	{
		for ( int iWeapon = 0; iWeapon < GetDesiredWeaponCount( pData ); iWeapon++ )
		{
			int iWeaponID = GetDesiredWeapon( iWeapon, pData );
			if ( iWeaponID != TF_WEAPON_NONE )
			{
				const char *pszWeaponName = GetItemSchema()->GetWeaponName(iWeaponID);
				if( !pszWeaponName )
					continue;

				pWeapon = (CTFWeaponBase *)WeaponSchema_OwnsThisID( iWeaponID );
				if ( pWeapon )
				{
					pWeapon->ChangeTeam( GetTeamNumber() );
					pWeapon->GiveDefaultAmmo();

					if ( m_bRegenerating == false )
					{
						pWeapon->WeaponReset();
					}
				}
				else
				{
					if( !Q_stricmp(pszWeaponName, "") )
						continue;
					pWeapon = (CTFWeaponBase *)GiveNamedItem( pszWeaponName );
					if( !Q_stricmp(pszWeaponName, "") )
						continue;
					if ( pWeapon )
					{
						pWeapon->DefaultTouch( this );
					}
				}
			}
			else
			{
				//I shouldn't have any weapons in this slot, so get rid of it
				CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
				//Don't nuke builders since they will be nuked if we don't need them later.
				if ( pCarriedWeapon && pCarriedWeapon->GetWeaponID() != TF_WEAPON_BUILDER )
				{
					Weapon_Detach( pCarriedWeapon );
					UTIL_Remove( pCarriedWeapon );
				}
			}
		}
	}
}

int CTFPlayer::GetDesiredWeaponCount( TFPlayerClassData_t *pData )
{
	if( TFGameRules() && TFGameRules()->m_hLogicLoadout.Count() )
	{
		for( int i = 0; i < TFGameRules()->m_hLogicLoadout.Count(); i++ )
		{
			if ( ( TFGameRules()->m_hLogicLoadout[i]->m_iClass == 0 || TFGameRules()->m_hLogicLoadout[i]->m_iClass == GetPlayerClass()->GetClassIndex() ) && TFGameRules()->m_hLogicLoadout[i]->hWeaponNames.Count() )
				return TFGameRules()->m_hLogicLoadout[i]->hWeaponNames.Count();
		}
	}
	return pData->m_iWeaponCount;
}

int CTFPlayer::GetDesiredWeapon( int iWeapon, TFPlayerClassData_t *pData )
{
	if( TFGameRules() && TFGameRules()->m_hLogicLoadout.Count() )
	{
		for( int i = 0; i < TFGameRules()->m_hLogicLoadout.Count(); i++ )
		{
			if ( ( TFGameRules()->m_hLogicLoadout[i]->m_iClass == 0 || TFGameRules()->m_hLogicLoadout[i]->m_iClass == GetPlayerClass()->GetClassIndex() ) && TFGameRules()->m_hLogicLoadout[i]->hWeaponNames.Count())
				return TFGameRules()->m_hLogicLoadout[i]->hWeaponNames[iWeapon];
		}
	}
	return pData->m_aWeapons[iWeapon];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageInstagibWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );

	for ( int iWeapon = 0; iWeapon < GetCarriedWeapons()+5; ++iWeapon )
	{
		
		pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
			
		if ( pWeapon && pWeapon->GetWeaponID() != TF_WEAPON_RAILGUN )
		{
			if ( TFGameRules()->IsMutator( INSTAGIB ) )
			{
				if ( pWeapon && pWeapon->GetWeaponID() != TF_WEAPON_CROWBAR )
				{
					Weapon_Detach( pWeapon );
					UTIL_Remove( pWeapon );
				}
			}
			else
			{
				Weapon_Detach( pWeapon );
				UTIL_Remove( pWeapon );
			}
		}
		else 
		{
			pWeapon = (CTFWeaponBase *)GiveNamedItem( "tf_weapon_railgun" );
			if ( pWeapon )
			{
				pWeapon->DefaultTouch( this );
			}
			if ( TFGameRules()->IsMutator( INSTAGIB ) )
			{
				pWeapon = (CTFWeaponBase *)GiveNamedItem( "tf_weapon_crowbar" );
				if ( pWeapon )
				{
					pWeapon->DefaultTouch( this );
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageGunGameWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	
	pWeapon = (CTFWeaponBase *)GiveNamedItem( "tf_weapon_crowbar" );
	CTFWeaponBase *pNewWeapon = (CTFWeaponBase *)GiveNamedItem( STRING(TFGameRules()->m_iszWeaponName[GGLevel()]) );
	if ( pWeapon && pNewWeapon )
	{
		if ( pWeapon->GetSlot() != pNewWeapon->GetSlot() )
		{
				pWeapon->SetGGLevel(999);
				pWeapon->DefaultTouch( this );
		}
		else
		{
			UTIL_Remove( pWeapon );
		}
	}

	if ( pNewWeapon )
	{
		pNewWeapon->SetGGLevel(GGLevel());
		pNewWeapon->DefaultTouch( this );
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageClanArenaWeapons(TFPlayerClassData_t *pData)
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(0);

	/*
	██╗  ██╗ █████╗  ██████╗██╗  ██╗
	██║  ██║██╔══██╗██╔════╝██║ ██╔╝
	███████║███████║██║     █████╔╝ 
	██╔══██║██╔══██║██║     ██╔═██╗ 
	██║  ██║██║  ██║╚██████╗██║  ██╗
	╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝
	Seriously, please submit a pull request or something about this
	*/

	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_crowbar");
	if (pWeapon)
		pWeapon->DefaultTouch(this);

	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_railgun");
	if (pWeapon)
		pWeapon->DefaultTouch(this);
	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_lightning_gun");
	if (pWeapon)
		pWeapon->DefaultTouch(this);
	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_rocketlauncher_dm");
	if (pWeapon)	
		pWeapon->DefaultTouch(this);

	if ( TFGameRules()->IsMutator( CLAN_ARENA ) )
	{
		pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_pistol_mercenary");
		if (pWeapon)
			pWeapon->DefaultTouch(this);
		pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_supershotgun");
		if (pWeapon)
			pWeapon->DefaultTouch(this);
		pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_shotgun");
		if (pWeapon)
			pWeapon->DefaultTouch(this);
		pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_nailgun");
		if (pWeapon)
			pWeapon->DefaultTouch(this);
		pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_grenadelauncher_mercenary");
		if (pWeapon)
			pWeapon->DefaultTouch(this);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRocketArenaWeapons(TFPlayerClassData_t *pData)
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(0);

	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_lead_pipe"); // Market Gardener and Rockets? Oh boy
	if( pWeapon )
		pWeapon->DefaultTouch(this);
	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_rocketlauncher_dm");
	if( pWeapon )
		pWeapon->DefaultTouch(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageArsenalWeapons(TFPlayerClassData_t *pData)
{
	StripWeapons();
	COFPlayerLoadout *pLoadout = LoadoutManager()->GetLoadout( this );

	CUtlMap<int,int> hDesiredWeapons;
	SetDefLessFunc( hDesiredWeapons );

	if( !IsFakeClient() && pLoadout )
	{
		int index = TF_CLASS_MERCENARY;//GetPlayerClass()->GetClassIndex();

		bool bFirst = true;
		FOR_EACH_VEC( pLoadout->m_ClassLoadout[index].m_Weapons, i )
		{
			hDesiredWeapons.Insert( i, pLoadout->m_ClassLoadout[index].m_Weapons[i] );
		}
	}
	else
	{
		int iMaxWpnNum = GetItemSchema()->GetWeaponCount() - 1;
		int iLastWep = -1;
		for( int i = 0; i < 2; i++ )
		{
			if( !i )
			{
				iLastWep = random->RandomInt( 0, iMaxWpnNum );
				hDesiredWeapons.Insert( i, iLastWep );
			}
			else
			{
				int iNewWep = -1;
				do
				{
					iNewWep = random->RandomInt( 0, iMaxWpnNum );
				}
				while( iNewWep == iLastWep );
				hDesiredWeapons.Insert( i, iNewWep );
			}
		}
	}
	
	KeyValues *kvDesiredWeapons = new KeyValues("DesiredWeapons");
	
	kvDesiredWeapons->SetInt( "1", GetItemSchema()->GetWeaponID( "tf_weapon_assaultrifle" ) );
	kvDesiredWeapons->SetInt( "2", GetItemSchema()->GetWeaponID( "tf_weapon_pistol_mercenary" ) );
	kvDesiredWeapons->SetInt( "3",  GetItemSchema()->GetWeaponID( "tf_weapon_crowbar" ) );
	
	FOR_EACH_MAP( hDesiredWeapons, i )
	{
		COFSchemaWeaponInfo *pWeapon = GetItemSchema()->GetWeapon(hDesiredWeapons[i]);
		
		if( !pWeapon )
			continue;
		
		if( !pWeapon->m_bShowInLoadout )
			continue;
		
		int iWeaponSlot = -1;
		iWeaponSlot = pWeapon->m_iWeaponSlot[TF_CLASS_MERCENARY];

		int iLoadoutSlot = hDesiredWeapons.Key(i) + 1;
		
		if( (iLoadoutSlot == 3 && iWeaponSlot != 3) || (iWeaponSlot > -1 && iWeaponSlot != i) )
			continue;

		if( kvDesiredWeapons )
			kvDesiredWeapons->SetInt( UTIL_VarArgs( "%d", iLoadoutSlot ), hDesiredWeapons[i] );
	}
	
	KeyValues *pWeapon = kvDesiredWeapons->GetFirstValue();
	for( pWeapon; pWeapon != NULL; pWeapon = pWeapon->GetNextValue() ) // Loop through all the keyvalues
	{
		CTFWeaponBase *pGivenWeapon;

		const char *pszName = GetItemSchema()->GetWeapon(pWeapon->GetInt())->m_szWeaponName;

		if( Weapon_OwnsThisType( pszName, 0 ) )
			continue;

		pGivenWeapon = CreateWeaponNoGive( pszName );

		if( pGivenWeapon )
		{
			pGivenWeapon->SetLocalOrigin( GetLocalOrigin() );
			pGivenWeapon->SetSlotOverride( atoi(pWeapon->GetName()) - 1 );
			pGivenWeapon->SetPositionOverride( 0 );

			if( !pGivenWeapon->IsMarkedForDeletion() ) 
			{
				pGivenWeapon->Touch( this );
			}
			pGivenWeapon->DefaultTouch( this );
		}


	}

	if( kvDesiredWeapons )
		kvDesiredWeapons->deleteThis();

	// Remove chainsaw charging condition to fix an exploit with changing weapons and resupplying
	if ( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		if ( !Weapon_OwnsThisID( TF_WEAPON_CHAINSAW ) )
			m_Shared.RemoveCond( TF_COND_SHIELD_CHARGE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageEternalWeapons(TFPlayerClassData_t *pData)
{
	StripWeapons();
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(0);

	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_chainsaw");
	if (pWeapon)
		pWeapon->DefaultTouch(this);

	pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_eternalshotgun");
	if (pWeapon)
		pWeapon->DefaultTouch(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Manage3WaveWeapons(TFPlayerClassData_t *pData)
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GiveNamedItem("tf_weapon_grapple");
	if (pWeapon)
		pWeapon->DefaultTouch(this);
}

bool CTFPlayer::ManageRandomizerWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();
	KeyValues* pRandomizer = new KeyValues( "Randomizer" );
	pRandomizer->LoadFromFile( filesystem, "cfg/randomizer.cfg" );
	if ( !pRandomizer )
		return false;
	
	KeyValues *pWeaponPresets = pRandomizer->FindKey( "weapon_presets" );
	if ( !pWeaponPresets )
		return false;
	
	KeyValues *pSetting = pWeaponPresets->FindKey( of_randomizer_setting.GetString() );
	if ( !pSetting )
		return false;
	
	bool bPrimary = false;
	bool bSecondary = false;
	bool bMelee = false;
	
	for( int i = 0; i < 3; i++ )
	{
		bool bFound = false;
		int  iFailsafe = 0;
		int  iAllWeapons = TF_WEAPON_COUNT;
		while ( !bFound )
		{
			int iWeaponID = random->RandomInt( 0, iAllWeapons-1 );
			if ( pSetting->FindKey( g_aWeaponNames[ iWeaponID ] ) )
			{
				CTFWeaponBase *pWeapon = (CTFWeaponBase *)GiveNamedItem( g_aWeaponNames[ iWeaponID ] );
				if ( !pWeapon )
				{
					iFailsafe++;
					continue;
				}
				WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWeapon->GetClassname() );
				CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
				if ( pWeaponInfo )
				{					
					switch ( pWeaponInfo->iSlot )
					{
						case 0:
							if ( !bPrimary )
							{
								bPrimary = true;
								bFound = true;
							}
							break;
						case 1:
							if ( !bSecondary )
							{
								bSecondary = true;
								bFound = true;
							}
							break;
						case 2:
							if ( !bMelee )
							{
								bMelee = true;
								bFound = true;
							}
							break;
						default:
							if ( !bPrimary )
							{
								bPrimary = true;
								bFound = true;
							}
							else if ( !bSecondary )
							{
								bSecondary = true;
								bFound = true;
							}
							else
								bMelee = true;
								bFound = true;
							break;
					}
				}
				if ( bFound )
					pWeapon->DefaultTouch( this );
				else
					UTIL_Remove ( pWeapon );
				iFailsafe++;
			}
		}
	}
	
	return true;
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageCustomSpawnWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();

	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GiveNamedItem( of_spawn_with_weapon.GetString() );

	if ( pWeapon )
	{
		pWeapon->DefaultTouch( this );
	}

	// OFBOT: I'm not sure why but bots end up A posing when giving only 1 weapon, but normal players don't, this is a quick fix for it
	if ( GetActiveWeapon() == nullptr )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );

		for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
		{
			pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
			if ( pWeapon )
			{
				Weapon_Switch( pWeapon );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageSlotSystemWeapons( TFPlayerClassData_t *pData )
{
	StripWeapons();
	int iSlot = 0;
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	if ( of_startloadout.GetInt() > 0 )
	{
		for ( int iWeapon = 0; iWeapon < GetDesiredWeaponCount( pData ); iWeapon++ )
		{
			int iWeaponID = GetDesiredWeapon( iWeapon, pData );
			if ( iWeaponID != TF_WEAPON_NONE )
			{
				const char *pszWeaponName = GetItemSchema()->GetWeaponName(iWeaponID);
				if( !pszWeaponName )
					continue;

				pWeapon = (CTFWeaponBase *)WeaponSchema_OwnsThisID( iWeaponID );
				if ( pWeapon )
				{
					pWeapon->ChangeTeam( GetTeamNumber() );
					pWeapon->GiveDefaultAmmo();

					if ( m_bRegenerating == false )
					{
						pWeapon->WeaponReset();
					}
				}
				else
				{
					if( !Q_stricmp(pszWeaponName, "") )
						continue;

					pWeapon = CreateWeaponNoGive( pszWeaponName );

					if( pWeapon )
					{
						pWeapon->SetLocalOrigin( GetLocalOrigin() );
						pWeapon->SetSlotOverride(iSlot);
						pWeapon->SetPositionOverride(0);

						if( !pWeapon->IsMarkedForDeletion() ) 
						{
							pWeapon->Touch( this );
						}
						pWeapon->DefaultTouch( this );

						iSlot++;
					}
				}
			}
			else
			{
				//I shouldn't have any weapons in this slot, so get rid of it
				CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
				//Don't nuke builders since they will be nuked if we don't need them later.
				if ( pCarriedWeapon && pCarriedWeapon->GetWeaponID() != TF_WEAPON_BUILDER )
				{
					Weapon_Detach( pCarriedWeapon );
					UTIL_Remove( pCarriedWeapon );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a spawn point for the player.
//-----------------------------------------------------------------------------

// enable info player start and info player deathmatch for checking
CBaseEntity *FindPlayerStart(const char *pszClassName);

CBaseEntity* CTFPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot = g_pLastSpawnPoints[ GetTeamNumber() ];
	const char *pSpawnPointName = "";

	switch( GetTeamNumber() )
	{
	case TF_TEAM_RED:
	case TF_TEAM_BLUE:
	case TF_TEAM_MERCENARY:
		{
			pSpawnPointName = "info_player_teamspawn";

			bool bFind = false;

			// in deathmatch players need to spawn further away from people for balance
			// if the game isn't deathmatch we don't want to do this so go back to normal
			// zombies (in infection) also use DM spawning algorithms
			if ( ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) || m_Shared.IsZombie() )
			{
				// Find the first spawn point
				pSpot = gEntList.FindEntityByClassname( NULL, pSpawnPointName );
				bFind = SelectDMSpawnSpots( pSpawnPointName, pSpot );
			}
			else
			{
				bFind = SelectSpawnSpot( pSpawnPointName, pSpot );
			}

			if ( bFind )
			{
				g_pLastSpawnPoints[ GetTeamNumber() ] = pSpot;
			}

			// need to save this for later so we can apply and modifiers to the armor and grenades...after the call to InitClass() //by tf2team
			m_pSpawnPoint = dynamic_cast<CTFTeamSpawn*>( pSpot );

			break;
		}
	case TEAM_SPECTATOR:
	case TEAM_UNASSIGNED:
	default:
		{
			pSpot = CBaseEntity::Instance( INDEXENT(0) );
			break;		
		}
	}

	if ( !pSpot )
	{
		pSpawnPointName = "info_player_teamspawn";

		// There's a rare chance if there is too many players then the DM spawning will fail, so fallback to normal spawning
		if ( SelectSpawnSpot( pSpawnPointName, pSpot ) )
		{
			g_pLastSpawnPoints[ GetTeamNumber() ] = pSpot;

			m_pSpawnPoint = dynamic_cast<CTFTeamSpawn*>( pSpot );
		}

		// still nothing? then just spawn at the centre of the map
		if ( !pSpot )
		{
			Warning( "Player Spawn: no valid spawn point was found for class %s on team %i found, even though at least one spawn entity exists.\n", 
				GetPlayerClassData( GetPlayerClass()->GetClassIndex(), 0 )->m_szLocalizableName, GetTeamNumber() );

			pSpot = CBaseEntity::Instance( INDEXENT(0) );
		}
	}

	return pSpot;
} 

//-----------------------------------------------------------------------------
// Purpose: Spawning for normal gameplay
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Sometimes the first spot can be NULL????
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	bool bIgnorePlayers = false;

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if ( TFGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// this is here because the DM spawning system may fall back to the normal spawning system if the player count overflows the spawnpoints
				// telefragging
				if ( ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) && !m_Shared.IsZombie() )
				{
					CBaseEntity *pList[ 32 ];
					Vector mins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
					Vector maxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;
					int targets = UTIL_EntitiesInBox( pList, 32, mins, maxs, FL_CLIENT );

					for ( int i = 0; i < targets; i++ )
					{
						// don't telefrag ourselves
						CBaseEntity *ent = pList[ i ];
						if ( ent != this && ent->IsAlive() && ( ent->GetTeamNumber() != GetTeamNumber() || ent->GetTeamNumber() == TF_TEAM_MERCENARY ) )
						{
							// special damage type to bypass uber or spawn protection in DM
							CTakeDamageInfo info( pSpot, this, 1000, DMG_ACID | DMG_BLAST, TF_DMG_CUSTOM_TELEFRAG );
							ent->TakeDamage( info );
						}
					}
				}

				// Found a valid spawn point.
				return true;
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		if ( pSpot == pFirstSpot && !bIgnorePlayers )
		{
			// Loop through again, ignoring players
			bIgnorePlayers = true;
			pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
		}
	} 
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot ); 

	return false;
}

// Really expensive function that gets if one spawnpoint is farther away from any all players than the other
int DistanceToPlayerSort(CBaseEntity* const *p1, CBaseEntity* const *p2)
{
	long double flClosestDistance1 = -1.0;
	long double flClosestDistance2 = -1.0;
	
	bool bFirst = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if( !pPlayer )
			continue;

		if ( !pPlayer->IsAlive() )
			continue;
		
		if ( !pPlayer->IsReadyToPlay() )
			continue;
		
		if( bFirst )
		{
			bFirst = false;
			flClosestDistance1 = pPlayer->GetAbsOrigin().DistToSqr((*p1)->GetAbsOrigin());
			flClosestDistance2 = pPlayer->GetAbsOrigin().DistToSqr((*p2)->GetAbsOrigin());
		}

		long double flTemp = pPlayer->GetAbsOrigin().DistToSqr((*p1)->GetAbsOrigin());
		if( flClosestDistance1 > flTemp )
			flClosestDistance1 = flTemp;

		flTemp = pPlayer->GetAbsOrigin().DistToSqr((*p2)->GetAbsOrigin());

		if( flClosestDistance2 > flTemp )
			flClosestDistance2 = flTemp;
	}

	// check the priority
	if( flClosestDistance2 > flClosestDistance1 )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Spawning for deathmatch
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectDMSpawnSpots( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for other players. We
	// don't collide with our team members, so we should be fine.

	// Find furthest spawn point
	CBaseEntity *pFirstSpot = pSpot;
	CBaseEntity *pFurthest = NULL;

	CUtlVector<CBaseEntity*> hSpawnPoints;

	// Check the distance from all other players

	// Are players in the map?
	bool bPlayers = TFGameRules() && TFGameRules()->CountActivePlayers() > 1;

	do
	{
		if ( !pSpot )
		{
			pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			continue;
		}

		// Check to see if this is a valid team spawn (player is on this team, etc.).
		if ( TFGameRules()->IsSpawnPointValid( pSpot, this, true ) )
		{
			// Check for a bad spawn entity.
			if ( pSpot->GetAbsOrigin() == vec3_origin )
			{
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
				continue;
			}

			// No players then just pick one
			if ( !bPlayers )
			{
				pFurthest = pSpot;
				break;
			}
			else
			{
				hSpawnPoints.AddToTail(pSpot);
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );

	if( bPlayers && hSpawnPoints.Count() )
	{
		// Sort to get the farthest spawn points
		hSpawnPoints.Sort(DistanceToPlayerSort);

		// Get one of the 3 farthest spawn points
		int iRand = random->RandomInt(0, min(2, hSpawnPoints.Count()));

		pFurthest = hSpawnPoints[iRand];
	}
	else
		pFurthest = pFirstSpot;

	pSpot = pFurthest;

	if ( pSpot )
	{
		// zombies don't telefrag

		// Why not? - Kay
		if ( !m_Shared.IsZombie() )
		{
			// telefragging
			CBaseEntity *pList[ 32 ];
			Vector mins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
			Vector maxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;
			int targets = UTIL_EntitiesInBox( pList, 32, mins, maxs, FL_CLIENT );

			for ( int i = 0; i < targets; i++ )
			{
				// don't telefrag ourselves
				CBaseEntity *ent = pList[ i ];
				if ( ent != this && ent->IsAlive() && ( ent->GetTeamNumber() != GetTeamNumber() || ent->GetTeamNumber() == TF_TEAM_MERCENARY ) )
				{
					ent->m_iHealth = 0;
					// special damage type to bypass uber or spawn protection in DM
					CTakeDamageInfo info(pSpot, this, 0, DMG_PREVENT_PHYSICS_FORCE | (DMG_BLAST | DMG_ALWAYSGIB), TF_DMG_CUSTOM_TELEFRAG);
					ent->Event_Killed(info);
					if( ent->IsPlayer() )
						ToBasePlayer(ent)->Event_Dying(info);
				}
			}	
		}

		// Found a valid spawn point.
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerAreaCapture *CTFPlayer::GetControlPointStandingOn( void )
{
	touchlink_t *root = (touchlink_t *)GetDataObject( TOUCHLINK );
	if ( root )
	{
		touchlink_t *next = root->nextLink;
		while ( next != root )
		{
			CBaseEntity *pEntity = next->entityTouched;
			if ( !pEntity )
				return NULL;

			if ( pEntity->IsSolidFlagSet( FSOLID_TRIGGER ) && pEntity->IsBSPModel() )
			{
				CTriggerAreaCapture *pCapArea = dynamic_cast<CTriggerAreaCapture *>( pEntity );
				if ( pCapArea )
					return pCapArea;
			}

			next = next->nextLink;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsCapturingPoint( void )
{
	CTriggerAreaCapture *pCapArea = GetControlPointStandingOn();
	if ( pCapArea )
	{
		CTeamControlPoint *pPoint = pCapArea->GetControlPoint();
		if ( pPoint && TFGameRules()->TeamMayCapturePoint( GetTeamNumber(), pPoint->GetPointIndex() ) &&
			TFGameRules()->PlayerMayCapturePoint( this, pPoint->GetPointIndex() ) )
		{
			return pPoint->GetOwner() != GetTeamNumber();
		}
	}

	return false;
}

/*
//-----------------------------------------------------------------------------
// Purpose: Return a CTFNavArea casted instance
//-----------------------------------------------------------------------------
CTFNavArea *CTFPlayer::GetLastKnownArea( void ) const
{
	return (CTFNavArea *)m_lastNavArea;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::OnNavAreaChanged( CNavArea *newArea, CNavArea *oldArea )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( !newArea || !oldArea )
		return;

	NavAreaCollector collector( true );
	newArea->ForAllPotentiallyVisibleAreas( collector );

	const CUtlVector<CNavArea *> *areas = &collector.m_area;

	FOR_EACH_VEC( *areas, i ) {
		CTFNavArea *area = static_cast<CTFNavArea *>( ( *areas )[i] );
		if ( area )
		area->IncreaseDanger( GetTeamNumber(), ( area->GetCenter() - GetAbsOrigin() ).Length() );
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetAutoTeam( void )
{
	CTFTeam *pBlue = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	CTFTeam *pRed  = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	CTFTeam *pMercenary = TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY );

	bool dm = ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() );
	
	if ( pBlue && pRed && !dm )
	{
		int iBluePlayers = pBlue->GetNumPlayers();
		int iRedPlayers = pRed->GetNumPlayers();

		if( !IsBot() || !IsBotOfType( BOT_TYPE_INSOURCE ) )
		{
			iBluePlayers -= pBlue->GetNumQuotaBots();
			iRedPlayers -= pRed->GetNumQuotaBots();
		}

		if( iBluePlayers < iRedPlayers )
		{
			return TF_TEAM_BLUE;
		}
		else if ( iRedPlayers < iBluePlayers )
		{
			return TF_TEAM_RED;
		}
		else
		{
			int iTeam = RandomInt ( 0 , 1 );
			if ( iTeam == 0 )
				return TF_TEAM_RED;
			else
				return TF_TEAM_BLUE;
		}
	}
	else if ( pMercenary && dm )
	{
		return TF_TEAM_MERCENARY;
	}

	return TEAM_SPECTATOR;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam( const char *pTeamName, bool bNoKill )
{
	if( !TFGameRules() )
		return;

	// civ can't change teams
	if ( TFGameRules()->IsESCGamemode() && IsPlayerClass( TF_CLASS_CIVILIAN ) )
		return;
	
	//CTFBot *pBot = dynamic_cast<CTFBot*>(this);
	bool bIsTempBot = 
		GetBotController() && static_cast<CBot*>(GetBotController())->RemoveOnDeath()
	// Also dont force tf_bot_add bots if we're excluding them
	|| (TFAutoTeam() && TFAutoTeam()->ShouldExcludeBots() && IsFakeClient() );
	
	bool bJoinedDuel = false;

	//add player to the duel queue if they're not in it
	if(TFGameRules()->IsDuelGamemode() && TFGameRules()->GetDuelQueuePos(this) == -1)
	{
		Msg("player %s with index %d was placed in the queue for the first time\n", GetPlayerName(), entindex());
		TFGameRules()->PlaceIntoDuelQueue(this);
		bJoinedDuel = true;
		UpdatePlayerColor();
	}

	//force spectating if player is not one of the two duelers
	bool bSpectate = !Q_strcmp(pTeamName, "spectate");

	if ( !bSpectate && TFGameRules()->IsDMGamemode() )
	{
		if (!TFGameRules()->IsTeamplay())
		{
			if (TFGameRules()->IsClassSelectAllowed(TF_TEAM_MERCENARY))
			{
				ShowViewPortPanel(PANEL_CLASS);
			}
			else
			{
				SetDesiredPlayerClassIndex(TFGameRules()->GetForcedClassIndex(TF_TEAM_MERCENARY));

				if (bJoinedDuel)
				{
					UpdateCosmetics();

					int iDesiredClass = GetDesiredPlayerClassIndex();

					int iModifier = TFGameRules()->GetPlayerClassMod(this);

					if (GetPlayerClass()->GetClassIndex() != iDesiredClass || GetPlayerClass()->GetModifier() != iModifier)
						GetPlayerClass()->Init(iDesiredClass, iModifier);
				}
			}
			
			if (!of_allowteams.GetBool())
			{
				int iTeam = TF_TEAM_MERCENARY;
				if (TFAutoTeam() && !bIsTempBot)
					iTeam = TFAutoTeam()->GetHumanTeam();

				ChangeTeam(iTeam, false);
				return;
			}
		}
	}
	
	int iTeam = TEAM_INVALID;
	bool bForceTeam = false;
	// If we're not a bot, and spectating is disabled, force the team
	if( TFAutoTeam() && !bIsTempBot && (!bSpectate || !TFAutoTeam()->AllowSpectator()) )
	{
		iTeam = TFAutoTeam()->GetHumanTeam();
		bForceTeam = true;
	}
	else if( !Q_strcmp(pTeamName, "auto") )
	{
		iTeam = GetAutoTeam();
	}
	else if( bSpectate )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		// we can't join DM team in every other gamemode unless of_allow_special_teams is set to 1
		if( !bForceTeam && !( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) && !of_allow_special_teams.GetBool() && ( stricmp( pTeamName, "mercenary" ) == 0 ) )
		{
			if ( !TFGameRules()->IsInfGamemode() ) // infection uses Mercenary team for joining, this is handled down below
				return;
		}

		for ( int i = TEAM_SPECTATOR; i < TF_TEAM_COUNT; ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				if ( iTeam == TF_TEAM_NPC )
					iTeam = TEAM_INVALID;

				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs("Invalid team \"%s\".", pTeamName ) );
		return;
	}

	if ( iTeam == TEAM_SPECTATOR )
	{
		// Prevent this is the cvar is set
		if ( !mp_allowspectators.GetInt() && !IsHLTV() && !bForceTeam )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return;
		}

		if ( HasTheFlag() )
			DropFlag();
		
		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() && !bNoKill ) 
		{
			CommitSuicide( false, true );
		}

		ChangeTeam( TEAM_SPECTATOR, false );

		// do we have fadetoblack on? (need to fade their screen back in)
		if ( mp_fadetoblack.GetBool() )
		{
			color32_s clr = { 0,0,0,255 };
			UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
		}
	}
	else
	{
		if ( iTeam == GetTeamNumber() )
			return;	// we wouldn't change the team

		// if this join would unbalance the teams, refuse
		// come up with a better way to tell the player they tried to join a full team!
		// Bots MUST join a team
		if ( !bForceTeam && TFGameRules()->WouldChangeUnbalanceTeams( iTeam, GetTeamNumber() ) && !IsFakeClient() )
		{
			ShowViewPortPanel( PANEL_TEAM );
			return;
		}

		if ( TFGameRules()->IsInfGamemode() )
		{
			// join red team if infection hasn't begun, otherwise join blue
			if( TFGameRules()->InSetup() || TFGameRules()->IsInWaitingForPlayers() || !TFGameRules()->GetInfectionRoundTimer() )
			{
				if( GetTeamNumber() == TF_TEAM_RED )
					return;

				ChangeTeam( TF_TEAM_RED, false );
				if( TFGameRules()->IsClassSelectAllowed(TF_TEAM_RED) )
				{
					ShowViewPortPanel( PANEL_CLASS );
					return;
				}
			}
			else
			{
				if( GetTeamNumber() == TF_TEAM_BLUE )
					return;

				ChangeTeam( TF_TEAM_BLUE, false );
				if ( TFGameRules()->IsClassSelectAllowed(TF_TEAM_BLUE) )
				{
					ShowViewPortPanel( PANEL_CLASS );
					return;
				}
			}

			SetDesiredPlayerClassIndex(TF_CLASS_MERCENARY);
			return;
		}
		else
		{
			if ( !bForceTeam && ( TFGameRules()->IsDMGamemode() && TFGameRules()->IsTeamplay() ) && !of_allow_special_teams.GetBool() )
			{
				if ( iTeam == TF_TEAM_MERCENARY )
					iTeam = random->RandomInt( TF_TEAM_RED, TF_TEAM_BLUE );
			}

			if ( HasTheFlag() )
				DropFlag();

			ChangeTeam( iTeam, bNoKill );
		}

		if ( bNoKill )
			return;

		if ( TFGameRules()->IsDMGamemode() )
		{
			if (TFGameRules()->IsClassSelectAllowed(iTeam))
			{
				ShowViewPortPanel(PANEL_CLASS);
			}
			else
			{
				SetDesiredPlayerClassIndex(TFGameRules()->GetForcedClassIndex(iTeam));
			}
		}	
		else
		{
			ShowViewPortPanel( PANEL_CLASS );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without using the game menus
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam_NoMenus( const char *pTeamName )
{
	Assert( IsX360() );

	Msg( "Client command HandleCommand_JoinTeam_NoMenus: %s\n", pTeamName );

	// Only expected to be used on the 360 when players leave the lobby to start a new game
	if ( !IsInCommentaryMode() )
	{
		Assert( GetTeamNumber() == TEAM_UNASSIGNED );
		Assert( IsX360() );
	}

	int iTeam = TEAM_SPECTATOR;
	if ( Q_stricmp( pTeamName, "spectate" ) )
	{
		for ( int i = 0; i < TF_TEAM_COUNT; ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	ForceChangeTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Player has been forcefully changed to another team
//-----------------------------------------------------------------------------
void CTFPlayer::ForceChangeTeam( int iTeamNum )
{
	int iNewTeam = iTeamNum;

	if ( iNewTeam == TF_TEAM_AUTOASSIGN )
	{
		iNewTeam = GetAutoTeam();
	}

	if ( !GetGlobalTeam( iNewTeam ) )
	{
		Warning( "CTFPlayer::ForceChangeTeam( %d ) - invalid team index.\n", iNewTeam );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iNewTeam == iOldTeam )
		return;

	TeamFortress_RemoveProjectiles();
	RemoveAllObjects();

	// don't let cheeky people remove their domination after switching to spectator in DM
	if ( !TFGameRules()->IsDMGamemode() && TFGameRules()->IsTeamplay() )
	{
		RemoveNemesisRelationships();
	}

    ChangeTeam( iNewTeam, true );

	if ( iNewTeam == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iNewTeam == TEAM_SPECTATOR )
	{
		m_bIsIdle = false;
		StateTransition( TF_STATE_OBSERVER );
		RemoveAllWeapons();
		DestroyViewModels();
		DestroyRagdoll();
	}

	// Don't modify living players in any way
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleFadeToBlack( void )
{
	if ( mp_fadetoblack.GetBool() )
	{
		color32_s clr = { 0,0,0,255 };
		UTIL_ScreenFade( this, clr, 0.75, 0, FFADE_OUT | FFADE_STAYOUT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ChangeTeam( int iTeamNum, bool bNoKill )
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CTFPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iTeamNum == iOldTeam )
		return;

	TeamFortress_RemoveProjectiles();
	RemoveAllObjects();

	// don't let cheeky people remove their domination after switching to spectator in DM
	if ( !TFGameRules()->IsDMGamemode() && TFGameRules()->IsTeamplay() )
	{
		RemoveNemesisRelationships();
	}

	BaseClass::ChangeTeam( iTeamNum );

	int iDesiredClass = GetDesiredPlayerClassIndex();

	int iModifier = TFGameRules()->GetPlayerClassMod( this );

	GetPlayerClass()->Init( iDesiredClass, iModifier );

	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iTeamNum == TEAM_SPECTATOR )
	{
		m_bIsIdle = false;

		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();
        DestroyRagdoll();
        UpdateCosmetics();

        SetRenderMode(kRenderNone);

	}
	else // active player
	{
		if ( !bNoKill && !IsDead() && (iOldTeam == TF_TEAM_RED || iOldTeam == TF_TEAM_BLUE || iOldTeam == TF_TEAM_MERCENARY ) )
		{
			// Kill player if switching teams while alive
			CommitSuicide( false, true );
		}
		else if ( IsDead() && iOldTeam < FIRST_GAME_TEAM )
		{
			SetObserverMode( OBS_MODE_CHASE );
			HandleFadeToBlack();
		}

		// let any spies disguising as me know that I've changed teams
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTemp && pTemp != this )
			{
				if ( ( pTemp->m_Shared.GetDisguiseTarget() == this ) || // they were disguising as me and I've changed teams
 					 ( !pTemp->m_Shared.GetDisguiseTarget() && pTemp->m_Shared.GetDisguiseTeam() == iTeamNum ) ) // they don't have a disguise and I'm joining the team they're disguising as
				{
					// choose someone else...
					pTemp->m_Shared.FindDisguiseTarget();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinClass( const char *pClassName )
{
	// can only join a class after you join a valid team
	if ( GetTeamNumber() <= LAST_SHARED_TEAM )
		return;
	
	// In case we don't get the class menu message before the spawn timer
	// comes up, fake that we've closed the menu.TF_CLASS_COUNT_ALL 
	SetClassMenuOpen( false );
	if( TFGameRules()->InStalemate() || (TFGameRules()->IsArenaGamemode() && (m_Shared.GetTFLives() > 0)) )
	{
		if ( IsAlive() && !TFGameRules()->CanChangeClassInStalemate() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_stalemate_cant_change_class" );
			return;
		}
	}

	bool bShouldNotRespawn = false;

	if ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && ( TFGameRules()->GetWinningTeam() != GetTeamNumber() ) )
	{
		m_bAllowInstantSpawn = false;
		bShouldNotRespawn = true;
	}

	if( !SetClass( pClassName ) )
		return;

	// We can respawn instantly if:
	//	- We're inside a respawn room
	//	- We're in the stalemate grace period
	//  - We're in Infection warmup

	bool bInRespawnRoom = PointInRespawnRoom( this, WorldSpaceCenter() );
	if( bInRespawnRoom && !IsAlive() )
	{
		// If we're not spectating ourselves, ignore respawn rooms. Otherwise we'll get instant spawns
		// by spectating someone inside a respawn room.
		bInRespawnRoom = (GetObserverTarget() == this);
	}

	bool bInStalemateClassChangeTime = false;

	if( TFGameRules()->IsInfGamemode() && (TFGameRules()->InSetup() || TFGameRules()->IsInWaitingForPlayers() ) )
	{
		bInRespawnRoom = true;
	}
	if( TFGameRules()->InStalemate() || (TFGameRules()->IsArenaGamemode() && (m_Shared.GetTFLives() > 0)) )
	{
		// Stalemate overrides respawn rules. Only allow spawning if we're in the class change time.
		bInStalemateClassChangeTime = TFGameRules()->CanChangeClassInStalemate();
		bInRespawnRoom = false;
		
	}
	if( bShouldNotRespawn == false && ( bInRespawnRoom || bInStalemateClassChangeTime ) )
	{
		// Don't instantly respawn if we are trying to instantly respawn from death
		// Prevents bugs with respawning on the same frame as you die by changing class
		// Bots frequently encountered this problem
		if ( m_iHealth > 0 )
			ForceRespawn();

		if( TFGameRules()->IsDuelGamemode() )
		{
			UpdateCosmetics();

			int iDesiredClass = GetDesiredPlayerClassIndex();

			if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
				GetPlayerClass()->Init( iDesiredClass, TFGameRules()->GetPlayerClassMod( this ) );
		}
		return;
	}

	if( FStrEq( pClassName, "random" ) )
	{
		if( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_asrandom" );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_asrandom" );
		}
	}
	else
	{
		if ( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_as", GetPlayerClassData( GetDesiredPlayerClassIndex(), 0 )->m_szLocalizableName );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_as", GetPlayerClassData(GetDesiredPlayerClassIndex(), 0 )->m_szLocalizableName );
		}
	}

	if ( IsAlive() && ( GetHudClassAutoKill() == true ) && bShouldNotRespawn == false )
	{
		CommitSuicide( false, true );
	}

	if( TFGameRules()->IsDuelGamemode() )
	{
		UpdateCosmetics();

		int iDesiredClass = GetDesiredPlayerClassIndex();

		if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
			GetPlayerClass()->Init( iDesiredClass, TFGameRules()->GetPlayerClassMod( this ) );
	}	
}

bool CTFPlayer::SetClass( const char *pClassName )
{
	int iClassIndex = TF_CLASS_UNDEFINED;
	if ( stricmp( pClassName, "random" ) != 0 )
	{
		int i = 0;

		for( i = TF_CLASS_SCOUT ; i < TF_CLASS_COUNT_ALL ; i++ )
		{
			if ( stricmp( pClassName, GetPlayerClassData( i, 0 )->m_szClassName ) == 0 )
			{
				iClassIndex = i;
				break;
			}
		}
	}
	else
	{
		iClassIndex = TF_CLASS_RANDOM;
	}

	return SetClass( iClassIndex );
}

bool CTFPlayer::SetClass( int iClassIndex )
{
	if (!IsClassAllowed(iClassIndex))
		return false;

	// The player has selected Random class...so let's pick one for them.
	if ( iClassIndex == TF_CLASS_RANDOM )
		iClassIndex = GetRandomClass();

	// joining the same class?
	if ( iClassIndex != TF_CLASS_RANDOM && iClassIndex == GetDesiredPlayerClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// were a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.

		// OFBOT todo : disabled as this causes problems with bots and DM
		/*if ( m_bAllowInstantSpawn && !IsAlive() ) // bots must skip this behaviour as they change class on the same frame they die, therefore causing some networking bugs here)
		{
			ForceRespawn();
		}
		*/
		return false;
	}

	SetDesiredPlayerClassIndex( iClassIndex );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_changeclass" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "class", iClassIndex );

		gameeventmanager->FireEvent( event );
	}

	return true;
}

int CTFPlayer::GetRandomClass()
{
	int iForcedClassIndex = TFGameRules()->GetForcedClassIndex(GetTeamNumber());

	// If a class is being forced, just use it instead of picking randomly
	if (iForcedClassIndex != -1) {
		return iForcedClassIndex;
	}

	int iClassIndex = TF_CLASS_UNDEFINED;
	int iRandomIterationCount = 0;
	do {
		// Don't let them be the same class twice in a row
		iClassIndex = random->RandomInt(TF_FIRST_NORMAL_CLASS, TF_CLASS_COUNT_ALL - 1);

		// Don't spend too much time on randomising
		// If it takes too long, just go through all classes and select the first
		if( iRandomIterationCount > 12 )
		{
			for (int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++)
			{
				if ( IsClassAllowed(i) )
				{
					iClassIndex = i;
					break;
				}
			}

			// If the only allowed class is random (meaning none are allowed), just randomly pick a normal class
			if (iClassIndex == TF_CLASS_RANDOM) {
				iClassIndex = random->RandomInt(TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS);
			}

			break;
		}

		iRandomIterationCount++;
	} while( iClassIndex == GetPlayerClass()->GetClassIndex() ||  // don't select the same class
		!IsClassAllowed(iClassIndex) );

	return iClassIndex;
}

bool CTFPlayer::IsClassAllowed( int iClassIndex )
{
	if (GetBotController() && GetBotType() == BOT_TYPE_LOCKDOWN)
	{
		return true;
	}

	return TFGameRules()->IsClassAllowed(GetTeamNumber(), iClassIndex);
}

void CTFPlayer::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	// Prevent player moving when taunting
	if( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		ucmd->forwardmove = 0.0f;
		ucmd->sidemove = 0.0f;
		ucmd->upmove = 0.0f;
		ucmd->buttons = 0;
		ucmd->weaponselect = 0;
	}
	
	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon )
{
	if ( !weapon )
		return GetAbsOrigin();

	const QAngle &angles = EyeAngles();

	float initVel = weapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) ? TF_PIPEBOMB_MIN_CHARGE_VEL : weapon->GetProjectileSpeed();
	//CALL_ATTRIB_HOOK_FLOAT( initVel, mult_projectile_range );

	return EstimateProjectileImpactPosition( angles.x, angles.y, initVel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge )
{
	float flMaxVel = TF_PIPEBOMB_MAX_CHARGE_VEL;
	float flMinVel = TF_PIPEBOMB_MIN_CHARGE_VEL;

	if( GetActiveTFWeapon() )
	{
		flMinVel = GetActiveTFWeapon()->GetProjectileSpeed();
		flMaxVel = flMinVel * (8 / 3);
	}

	float initVel = charge * ( flMaxVel - flMinVel ) + flMinVel;
	//CALL_ATTRIB_HOOK_FLOAT( initVel, mult_projectile_range );

	return EstimateProjectileImpactPosition( pitch, yaw, initVel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateProjectileImpactPosition( float pitch, float yaw, float initVel )
{
	Vector vecForward, vecRight, vecUp;
	QAngle angles( pitch, yaw, 0.0f );
	AngleVectors( angles, &vecForward, &vecRight, &vecUp );

	Vector vecSrc = Weapon_ShootPosition();
	vecSrc += vecForward * 16.0f + vecRight * 8.0f + vecUp * -6.0f;

	const float initVelScale = 0.9f;
	Vector      vecVelocity = initVelScale * ( ( vecForward * initVel ) + ( vecUp * 200.0f ) );

	Vector      pos = vecSrc;
	Vector      lastPos = pos;

	extern ConVar sv_gravity;
	const float g = sv_gravity.GetFloat();

	Vector alongDir = vecForward;
	alongDir.AsVector2D().NormalizeInPlace();

	float alongVel = vecVelocity.AsVector2D().Length();

	trace_t                        trace;
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	const float timeStep = 0.01f;
	const float maxTime = 5.0f;

	float t = 0.0f;
	do
	{
		float along = alongVel * t;
		float height = vecVelocity.z * t - 0.5f * g * Square( t );

		pos.x = vecSrc.x + alongDir.x * along;
		pos.y = vecSrc.y + alongDir.y * along;
		pos.z = vecSrc.z + height;

		UTIL_TraceHull( lastPos, pos, -Vector( 8, 8, 8 ), Vector( 8, 8, 8 ), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

		if ( trace.DidHit() )
			break;

		lastPos = pos;
		t += timeStep;
	} while ( t < maxTime );

	return trace.endpos;
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	m_flLastAction = gpGlobals->curtime;

	if ( FStrEq( pcmd, "addcond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TF_COND_LAST-1 );

				CTFPlayer *pTargetPlayer = this;
				if ( args.ArgC() >= 4 )
				{
					// Find the matching netname
					for ( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
						if ( pPlayer )
						{
							if ( Q_strstr( pPlayer->GetPlayerName(), args[3] ) )
							{
								pTargetPlayer = ToTFPlayer(pPlayer);
								break;
							}
						}
					}
				}

				if ( args.ArgC() >= 3 )
				{
					float flDuration = atof( args[2] );
					pTargetPlayer->m_Shared.AddCond( iCond, flDuration );
				}
				else
				{
					pTargetPlayer->m_Shared.AddCond( iCond );
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "removecond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TF_COND_LAST-1 );
				m_Shared.RemoveCond( iCond );
			}

		}
		return true;
	}
	else if ( FStrEq( pcmd, "taunt" ) )
	{
		if (args.ArgC() == 2)
		{
			int iTaunt = atoi(args[1]);
			Taunt( iTaunt );
		}
		else
			Taunt( 0 );

		return true;
	}
	else if ( FStrEq( pcmd, "build" ) )
	{
		if ( IsHauling() )
			return true;

		if ( args.ArgC() == 3 )
		{
			// player wants to build something
			int iBuilding = atoi( args[1] );
			int iAltMode = atoi( args[2] );

			if ( iAltMode >= 0 )
				StartBuildingObjectOfType( iBuilding, iAltMode );
			else
				StartBuildingObjectOfType( iBuilding, 0 );
		}
		else if ( args.ArgC() == 2 )
		{
			// player wants to build something
			int iBuilding = atoi( args[1] );

			StartBuildingObjectOfType( iBuilding, 0 );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "destroy" ) )
	{
		if ( IsHauling() )
			return true;

		if ( args.ArgC() == 3 )
		{
			// player wants to destroy something
			int iBuilding = atoi( args[ 1 ] );
			int iAltMode = atoi( args[2] );
			if ( iAltMode >= 0)
				DetonateOwnedObjectsOfType( iBuilding, iAltMode );
			else
				DetonateOwnedObjectsOfType( iBuilding, 0 );
		}
		else if ( args.ArgC() == 2 )
		{
			// player wants to build something
			int iBuilding = atoi( args[1] );

			DetonateOwnedObjectsOfType( iBuilding, 0 );
		}

		return true;
	}
#ifdef _DEBUG

	else if ( FStrEq( pcmd, "burn" ) ) 
	{
		m_Shared.Burn( this, 0 );
		return true;
	}
#endif

	else if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( args.ArgC() >= 2 )
		{
			HandleCommand_JoinTeam( args[1], false );
		}
		return true;
	}
	if ( FStrEq( pcmd, "jointeam_nokill" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam( args[1], true );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nomenus" ) )
	{
		if ( IsX360() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoMenus( args[1] );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "closedwelcomemenu" ) )
	{
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
		{
			ShowViewPortPanel( PANEL_TEAM, true );
		}
		else if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
		{
			ShowViewPortPanel( PANEL_CLASS, true );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		if ( args.ArgC() >= 2 )
		{
			HandleCommand_JoinClass( args[1] );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "disguise" ) ) 
	{
		if ( args.ArgC() >= 3 )
		{
			if ( CanDisguise() )
			{
				int nClass = atoi( args[ 1 ] );
				int nTeam = atoi( args[ 2 ] );
				
				// intercepting the team value and reassigning what gets passed into Disguise()
				// because the team numbers in the client menu don't match the #define values for the teams
				m_Shared.Disguise( ( nTeam == 1 ) ? TF_TEAM_BLUE : TF_TEAM_RED, nClass );
			}
		}
		return true;
	}
	else if (FStrEq( pcmd, "lastdisguise" ) )
	{
		// disguise as our last known disguise. desired disguise will be initted to something sensible
		if ( CanDisguise() )
		{
			// disguise as the previous class, if one exists
			int nClass = m_Shared.GetDesiredDisguiseClass();

			//If we pass in "random" or whatever then just make it pick a random class.
			if ( args.ArgC() > 1 )
			{
				nClass = TF_CLASS_UNDEFINED;
			}

			if ( nClass == TF_CLASS_UNDEFINED )
			{
				// they haven't disguised yet, pick a nice one for them.
				// exclude some undesirable classes
				do
				{
					nClass = random->RandomInt( TF_FIRST_NORMAL_CLASS, TF_CLASS_MERCENARY );
				} while( nClass == TF_CLASS_SCOUT || nClass == TF_CLASS_MERCENARY );
			}

			m_Shared.Disguise( ( GetTeamNumber() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE, nClass );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playgesture" ) )
	{
		if ( args.ArgC() == 1 )
		{
			Warning( "mp_playgesture: Gesture activity or sequence must be specified!\n" );
			return true;
		}

		if ( sv_cheats->GetBool() )
		{
			if ( !PlayGesture( args[1] ) )
			{
				Warning( "mp_playgesture: unknown sequence or activity name \"%s\"\n", args[1] );
				return true;
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "mp_playanimation" ) )
	{
		if ( args.ArgC() == 1 )
		{
			Warning( "mp_playanimation: Activity or sequence must be specified!\n" );
			return true;
		}

		if ( sv_cheats->GetBool() )
		{
			if ( !PlaySpecificSequence( args[1] ) )
			{
				Warning( "mp_playanimation: Unknown sequence or activity name \"%s\"\n", args[1] );
				return true;
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
		SetClassMenuOpen( true );
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		SetClassMenuOpen( false );
		return true;
	}
	else if ( FStrEq( pcmd, "pda_click" ) )
	{
		// player clicked on the PDA, play attack animation

		CTFWeaponBase *pWpn = GetActiveTFWeapon();

		CTFWeaponPDA *pPDA = dynamic_cast<CTFWeaponPDA *>(pWpn);

		if ( pPDA && !m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "extendfreeze" ) )
	{
		m_flDeathTime += 2.0f;
		return true;
	}
	else if ( FStrEq( pcmd, "show_motd" ) )
	{
		KeyValues *data = new KeyValues( "data" );
		data->SetString( "title", "#TF_Welcome" );	// info panel title
		data->SetString( "type", "1" );				// show userdata from stringtable entry
		data->SetString( "msg",	"motd" );			// use this stringtable entry
		data->SetString( "cmd", "mapinfo" );		// exec this command if panel closed

		ShowViewPortPanel( PANEL_INFO, true, data );

		data->deleteThis();
	}
	/*	else if ( FStrEq( pcmd, "condump_on" ) )
	{
		if ( !PlayerHasPowerplay() )
		{
			Msg("Console dumping on.\n");
			return true;
		}
		else 
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0; i < GetTeam()->GetNumPlayers(); i++ )
				{
					CTFPlayer *pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer(i) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( true );
					}
				}
				return true;
			}
			else
			{
				if ( SetPowerplayEnabled( true ) )
					return true;
			}
		}
	}
	else if ( FStrEq( pcmd, "condump_off" ) )
	{
		if ( !PlayerHasPowerplay() )
		{
			Msg("Console dumping off.\n");
			return true;
		}
		else
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0; i < GetTeam()->GetNumPlayers(); i++ )
				{
					CTFPlayer *pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer(i) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( false );
					}
				}
				return true;
			}
			else
			{
				if ( SetPowerplayEnabled( false ) )
					return true;
			}
		}
	}*/

	return BaseClass::ClientCommand( args );
}


void CTFPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	// can't pick up what you're standing on
	if ( GetGroundEntity() == pObject )
		return;
	
	if ( bLimitMassAndSize == true )
	{
		if ( CBasePlayer::CanPickupObject( pObject, 9001, 9001 ) == false )
			 return;
	}

	// Can't be picked up if NPCs are on me
	if ( pObject->HasNPCsOnIt() )
		return;

	PlayerPickupObject( this, pObject );
}

float CTFPlayer::GetHeldObjectMass( IPhysicsObject *pHeldObject )
{
	float mass = PlayerPickupGetHeldObjectMass( m_hUseEntity, pHeldObject );
	if ( mass == 0.0f )
	{
		mass = PhysCannonGetHeldObjectMass( GetActiveWeapon(), pHeldObject );
	}
	return mass;
}

//-----------------------------------------------------------------------------
// Purpose: Force the player to drop any physics objects he's carrying
//-----------------------------------------------------------------------------
void CTFPlayer::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis )
{
	if ( PhysIsInCallback() )
	{
		variant_t value;
		g_EventQueue.AddEvent( this, "ForceDropPhysObjects", value, 0.01f, pOnlyIfHoldingThis, this );
		return;
	}

#ifdef HL2_EPISODIC
	if ( hl2_episodic.GetBool() )
	{
		CBaseEntity *pHeldEntity = PhysCannonGetHeldEntity( GetActiveWeapon() );
		if( pHeldEntity && pHeldEntity->ClassMatches( "grenade_helicopter" ) )
		{
			return;
		}
	}
#endif

	// Drop any objects being handheld.
	ClearUseEntity();

	// Then force the physcannon to drop anything it's holding, if it's our active weapon
	PhysCannonForceDrop( GetActiveWeapon(), NULL );
}

void CTFPlayer::InputForceDropPhysObjects( inputdata_t &data )
{
	ForceDropOfCarriedPhysObjects( data.pActivator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetClassMenuOpen( bool bOpen )
{
	m_bIsClassMenuOpen = bOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsClassMenuOpen( void )
{
	return m_bIsClassMenuOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayGesture( const char *pGestureName )
{
	Activity nActivity = (Activity)LookupActivity( pGestureName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pGestureName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlaySpecificSequence( const char *pAnimationName )
{
	Activity nActivity = (Activity)LookupActivity( pAnimationName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pAnimationName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanDisguise( void )
{
	if ( !IsAlive() )
		return false;

	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
		return false;

	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		HintMessage( HINT_CANNOT_DISGUISE_WITH_FLAG );
		return false;
	}

	// no disguising in infection or DM
	if ( TFGameRules() && ( ( TFGameRules()->IsInfGamemode() || TFGameRules()->IsCoopEnabled() ) || ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) ) )
		return false;

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAutoswitch( void )
{
	int bShouldSwitch = 0;

	if ( !( GetFlags() & FL_FAKECLIENT ) )
		bShouldSwitch = V_atoi(engine->GetClientConVarValue(entindex(), "of_autoswitchweapons"));
	
	if ( bShouldSwitch > 1 )
		return true;
	
	if ( GetNextAttack() > gpGlobals->curtime || m_nButtons & IN_ATTACK || bShouldSwitch < 1 || m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;
	
	return true;

}

//-----------------------------------------------------------------------------
// Purpose: Override to add weapon to the hud
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );
	
	// should we switch to this item?
	if ( CanAutoswitch() )
	{
		Weapon_Switch( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DetonateOwnedObjectsOfType( int iType, int iAltMode )
{
	int i;
	int iNumObjects = GetObjectCount();
	for ( i=0;i<iNumObjects;i++ )
	{
		CBaseObject *pObj = GetObject(i);

		if ( pObj && pObj->GetType() == iType && pObj->GetAltMode() == iAltMode )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_DETONATED_OBJECT, pObj->GetResponseRulesModifier() );
			pObj->DetonateObject();

			const CObjectInfo *pInfo = GetObjectInfo( iType );

			if ( pInfo )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (weapon \"%s\") (objectowner \"%s<%i><%s><%s>\") (attacker_position \"%d %d %d\")\n",   
					GetPlayerName(),
					GetUserID(),
					GetNetworkIDString(),
					GetTeam()->GetName(),
					pInfo->m_pObjectName,
					"pda_engineer",
					GetPlayerName(),
					GetUserID(),
					GetNetworkIDString(),
					GetTeam()->GetName(),
					(int)GetAbsOrigin().x, 
					(int)GetAbsOrigin().y,
					(int)GetAbsOrigin().z );
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StartBuildingObjectOfType( int iType, int iAltMode )
{
	// early out if we can't build this type of object
	if ( CanBuild( iType, iAltMode ) != CB_CAN_BUILD )
		return;

	const char *tf_builder = "tf_weapon_builder";
	const char *pszWeaponName = GetObjectInfo(iType)->m_bVisibleInWeaponSelection ? GetObjectInfo(iType)->m_pObjectName : tf_builder;

	CTFWeaponBuilder *pBuilder =  (CTFWeaponBuilder*) WeaponSchema_OwnsThisID( GetItemSchema()->GetWeaponID(pszWeaponName) );

	// Is this the builder that builds the object we're looking for?
	if ( pBuilder )
	{
		pBuilder->SetSubType( iType, iAltMode );

		if ( GetActiveTFWeapon() == pBuilder )
		{
			SetActiveWeapon( NULL );
		}

		// try to switch to this weapon
		Weapon_Switch( pBuilder );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject *CTFPlayer::GetObjectOfType( int iType, int iMode )
{
	FOR_EACH_VEC( m_aObjects, i )
	{
		CBaseObject *obj = (CBaseObject *)m_aObjects[i].Get();
		if ( obj->ObjectType() == iType && obj->GetAltMode() == iMode )
			return obj;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage != DAMAGE_YES )
		return;

	CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		// Prevent team damage here so blood doesn't appear
		if ( info.GetAttacker()->IsPlayer() )
		{
			if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) && friendlyfire.GetBool() == false )
				return;
		}
	}

	// Save this bone for the ragdoll.
	m_nForceBone = ptr->physicsbone;

	SetLastHitGroup( ptr->hitgroup );

	// Ignore hitboxes for all weapons except the sniper rifle

	CTakeDamageInfo info_modified = info;

	// headshots only!
	if ( ptr->hitgroup != HITGROUP_HEAD && of_headshots.GetInt() >= 2 )
		return;

	m_iGoreHead = 0;
	m_iGoreLeftArm = 0;
	m_iGoreRightArm = 0;
	m_iGoreLeftLeg = 0;
	m_iGoreRightLeg = 0;

	if ( info_modified.GetDamageType() & DMG_USE_HITLOCATIONS || of_headshots.GetBool() == 1 )
	{
		switch ( ptr->hitgroup )
		{
		case HITGROUP_HEAD:
			{
				CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();

				bool bCritical = true;
				
				int iCanHeadshot = -1;
				if( pWpn )
				{
					iCanHeadshot = pWpn->GetTFWpnData().m_iCanHeadshot;
					pWpn->GetAttributeValue_Int("set can headshot", iCanHeadshot);
				}
				if ((pWpn && !pWpn->CanFireCriticalShot(true)) || (iCanHeadshot == 0 && of_headshots.GetBool() == 0))
				{
					bCritical = false;
				}

				if (pWpn->GetWeaponID() == TF_WEAPON_FLAMETHROWER)
				{
					bCritical = false;
				}
		
				if ( bCritical )
				{
					if (!pWpn->GetTFWpnData().m_b150Headshot)
					{
						info_modified.AddDamageType(DMG_CRITICAL);
						if (info_modified.GetDamageCustom() != TF_DMG_CUSTOM_CRIT_POWERUP || TF_DAMAGE_CRIT_MULTIPLIER >= of_crit_multiplier.GetFloat())
							info_modified.SetDamageCustom(TF_DMG_CUSTOM_HEADSHOT);

						//DevMsg("Hit a headshot, crit moment.\n");
					}
					else if (pWpn->GetTFWpnData().m_b150Headshot)
					{

						if (info_modified.GetDamageCustom() != TF_DMG_CUSTOM_CRIT_POWERUP || TF_DAMAGE_CRIT_MULTIPLIER >= of_crit_multiplier.GetFloat())
							info_modified.SetDamageCustom(TF_DMG_CUSTOM_RAILGUN_HEADSHOT);
						//DevMsg("Hit a railgun headshot, fake crit moment.\n");
					}

					// play the critical shot sound to the shooter	
					if ( pWpn )
					{
						pWpn->WeaponSound( BURST );
					}
				}

				break;
			}
		case HITGROUP_RIGHTLEG:
		case HITGROUP_LEFTLEG:
		{
			CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();

			if (pWpn && pWpn->GetTFWpnData().m_bCanPieceLegs)
			{
				CTakeDamageInfo info;

				m_Shared.FuckUpLegs(pAttacker, pWpn->GetTFWpnData().m_flPiecedLegsEffectDuration, pWpn->GetTFWpnData().m_flSpeedReduction);
				info_modified.SetDamageCustom(TF_DMG_CUSTOM_LEGSHOT);
			}
			break;
		}
		default:
			break;
		}
	}

	if (pAttacker != NULL)
	{
		CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();

		if (pWpn != NULL)
		{
			if (GetEnemyTeam(pAttacker) == pAttacker->GetTeamNumber() && !m_Shared.InCondUber())
			{

				if (pWpn->GetTFWpnData().m_bCanTranq)
				{
					m_Shared.Tranq(pAttacker, pWpn->GetTFWpnData().m_flTranqEffectDuration, pWpn->GetTFWpnData().m_flSpeedReduction, pWpn->GetTFWpnData().m_flWeaponSpeedReduction);
				}
				if (pWpn->GetTFWpnData().m_bCanPoison)
				{
					m_Shared.Poison(pAttacker, pWpn->GetTFWpnData().m_flPoisonEffectDuration);
				}
				if (pWpn->GetTFWpnData().m_bCanIgnite)
				{
					m_Shared.Burn(pAttacker, pWpn->GetTFWpnData().m_flAfterBurnEffectDuration);
				}
			}
		}
	}
	/*
	m_HeadBodygroup = FindBodygroupByName("head");
	m_LeftArmBodygroup = FindBodygroupByName("leftarm");
	m_RightArmBodygroup = FindBodygroupByName("rightarm");
	m_LeftLegBodygroup = FindBodygroupByName("leftleg");
	m_RightLegBodygroup = FindBodygroupByName("rightleg");

	// This shot will be fatal so start blowing the limbs off
	// These are sent to the client which will do whatever it needs to do
	*/

	//DevMsg( 1, "CURRENT DAMAGE IS %.2f \n", info_modified.GetDamage() );

	if (info_modified.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT || info_modified.GetDamageType() & DMG_CRITICAL || info_modified.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)
	{
		switch ( ptr->hitgroup )
		{
		case HITGROUP_HEAD:
			m_iGoreHead = 3;
			break;
		case HITGROUP_LEFTARM:
			m_iGoreLeftArm = 3;
			break;
		case HITGROUP_RIGHTARM:
			m_iGoreRightArm = 3;
			break;
		case HITGROUP_LEFTLEG:
			m_iGoreLeftLeg = 3;
			break;
		case HITGROUP_RIGHTLEG:
			m_iGoreRightLeg = 3;
			break;
		default:
			break;
		}
	}
	else
	{
		switch ( ptr->hitgroup )
		{
		case HITGROUP_HEAD:
			m_iGoreHead = 2;
			break;
		case HITGROUP_LEFTARM:
			m_iGoreLeftArm = 2;
			break;
		case HITGROUP_RIGHTARM:
			m_iGoreRightArm = 2;
			break;
		case HITGROUP_LEFTLEG:
			m_iGoreLeftLeg = 2;
			break;
		case HITGROUP_RIGHTLEG:
			m_iGoreRightLeg = 2;
			break;
		default:
			break;
		}
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// no impact effects
	}
	else if (m_Shared.InCondUber() || (m_Shared.InCond(TF_COND_SHIELD) || m_Shared.InCond(TF_COND_SHIELD_DUEL)))
	{ 
		// Make bullet impacts
		g_pEffects->Ricochet( ptr->endpos - (vecDir * 8), -vecDir );
	}
	else
	{	
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		CDisablePredictionFiltering disabler;

		// This does smaller splotches on the guy and splats blood on the world.
		TraceBleed( info_modified.GetDamage(), vecDir, ptr, info_modified.GetDamageType() );
	}

	AddMultiDamage( info_modified, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	int bResult = false;

	// If the bit's set, add over the max health
	if ( bitsDamageType & DMG_IGNORE_MAXHEALTH )
	{
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~(bitsDamageType & ~iTimeBasedDamage);
		m_iHealth += flHealth;
		bResult = true;
	}
	else
	{
		float flMaxHealth = GetPlayerClass()->GetMaxHealth();
		
		// don't want to add more than we're allowed to have
		if (flHealth > flMaxHealth - m_iHealth)
			flHealth = flMaxHealth - m_iHealth;

		if (flHealth <= 0)
			bResult = false;
		else
			bResult = BaseClass::TakeHealth(flHealth, bitsDamageType);
	}

	return bResult;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::TakeArmorTFC(int iArmorTFC)
{
	int iArmorToAdd = iArmorTFC;
	int iMaxArmor = GetPlayerClass()->GetMaxArmor();

		// don't want to add more than we're allowed to have
	if (iArmorToAdd > iMaxArmor - m_Shared.GetTFCArmor())
		iArmorToAdd = iMaxArmor - m_Shared.GetTFCArmor();

	if (iArmorToAdd <= 0)
		return 0;

	m_Shared.SetTFCArmor(iArmorToAdd + (m_Shared.GetTFCArmor() ) );

	return iArmorToAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFWeaponRemove( int iWeaponID )
{
	// find the weapon that matches the id and remove it
	int i;
	for (i = 0; i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWeapon = ( CTFWeaponBase *)GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponID() != iWeaponID )
			continue;

		RemovePlayerItem( pWeapon );
		UTIL_Remove( pWeapon );
	}
}

/* Ivory: this is used for nothing
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::DropCurrentWeapon( void )
{
	if( !m_Shared.GetActiveTFWeapon() )
		return false;
	
	int Clip = m_Shared.GetActiveTFWeapon()->m_iClip1;
	int ReserveAmmo = m_Shared.GetActiveTFWeapon()->m_iReserveAmmo;
	
// akimbo pickups have NOT pewished
	if ( m_Shared.GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PISTOL_AKIMBO )
	{
		CTFWeaponBase *pTFPistol = (CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_PISTOL_MERCENARY );
		DropWeapon( pTFPistol, true, false, (float)Clip / 2.0f, ReserveAmmo );
		pTFPistol = NULL;
		UTIL_Remove ( m_Shared.GetActiveTFWeapon() );
	}
	else
	{
		DropWeapon( m_Shared.GetActiveTFWeapon(), true, false, Clip, ReserveAmmo );
		UTIL_Remove ( m_Shared.GetActiveTFWeapon() );
	}

	if ( GetLastWeapon() )
		Weapon_Switch( GetLastWeapon() );
	else
		SwitchToNextBestWeapon( m_Shared.GetActiveTFWeapon() );

	return true;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropFlag( void )
{
	if ( HasItem() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );
		if ( pFlag )
		{
			pFlag->Drop( this, true, true );
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
				event->SetInt( "priority", 8 );

				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EHANDLE CTFPlayer::TeamFortress_GetDisguiseTarget( int nTeam, int nClass )
{
	if ( nTeam == GetTeamNumber() || nTeam == TF_SPY_UNDEFINED )
	{
		// we're not disguised as the enemy team
		return NULL;
	}

	CBaseEntity *pLastTarget = m_Shared.GetDisguiseTarget(); // don't redisguise self as this person
	
	// Find a player on the team the spy is disguised as to pretend to be
	CTFPlayer *pPlayer = NULL;

	// Loop through players
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			if ( pPlayer == pLastTarget )
			{
				// choose someone else, we're trying to rid ourselves of a disguise as this one
				continue;
			}

			// First, try to find a player with the same color AND skin
			if ( pPlayer->GetTeamNumber() == nTeam && pPlayer->GetPlayerClass()->GetClassIndex() == nClass )
			{
				return pPlayer;
			}
		}
	}

	// we didn't find someone with the same skin, so just find someone with the same color
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() == nTeam )
			{
				return pPlayer;
			}
		}
	}

	// we didn't find anyone
	return NULL;
}

static float DamageForce( const Vector &size, float damage, float scale )
{ 
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

//-----------------------------------------------------------------------------
// Purpose: Apply damage knockback when damage is zero
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyDamageKnockback(const CTakeDamageInfo &info, bool noKnockback, Vector *dir)
{
	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}
	g_vecAttackDir = vecDir;

	if (dir)
		*dir = vecDir;

	if (noKnockback)
		return;

	if ( ( info.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) == 0 )
	{
		if ( info.GetInflictor() && ( GetMoveType() == MOVETYPE_WALK ) && 
		   ( !info.GetAttacker()->IsSolidFlagSet( FSOLID_TRIGGER ) ) && 
		   ( !m_Shared.InCond( TF_COND_DISGUISED ) ) )	
		{
			Vector vecForce;
			vecForce.Init();
			if ( info.GetAttacker() == this )
			{
				float flDamageForce = info.GetDamageForForceCalc();
				vecForce = vecDir * -DamageForce( WorldAlignSize(), flDamageForce, tf_damageforcescale_self_soldier.GetFloat() );
			}
			else
			{
				// Sentryguns push a lot harder
				if ( (info.GetDamageType() & DMG_BULLET) && info.GetInflictor()->IsBaseObject() )
				{
					vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), 16 );
				}
				else
				{
					float flDamageMult = 1.0f;
					flDamageMult *= tf_damageforcescale_other.GetFloat();
					flDamageMult *= of_knockback_all.GetFloat();
					if( m_Shared.InCond(TF_COND_BERSERK) )
					{
						flDamageMult *= of_berserk_knockback.GetFloat();
					}

					vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), flDamageMult );

					if (info.GetDamageType() & DMG_BLAST)
						vecForce *= of_knockback_explosives.GetFloat();
					if (info.GetDamageType() & DMG_BULLET)
						vecForce *= of_knockback_bullets.GetFloat();
					if (info.GetDamageType() & DMG_CLUB)
						vecForce *= of_knockback_melee.GetFloat();

					if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
						vecForce *= 0.5; // Heavies take less push from non sentryguns

				}
			}

			if ( m_Shared.IsZombie() )
				vecForce *= 0.66f; // Zombies take less push overall

			ApplyAbsVelocityImpulse( vecForce );
		}
	}
}

ConVar tf_debug_damage( "tf_debug_damage", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	if ( (GetFlags() & FL_GODMODE) || IsInCommentaryMode() )
		return 0;

	if ( m_debugOverlays & OVERLAY_BUDDHA_MODE ) 
	{
		if ((m_iHealth - info.GetDamage()) <= 0)
		{
			m_iHealth = 1;
			return 0;
		}
	}

	// Early out if there's no damage and no knockback
	if ( !info.GetDamage() && !info.GetDamageForForceCalc() )
	{
		return 0;
	}

	if( !IsAlive() )
		return 0;

	int iHealthBefore = GetHealth();

	bool bDebug = tf_debug_damage.GetBool();
	if ( bDebug )
	{
		Warning( "%s taking damage from %s, via %s. Damage: %.2f\n", GetDebugName(), info.GetInflictor() ? info.GetInflictor()->GetDebugName() : "Unknown Inflictor", info.GetAttacker() ? info.GetAttacker()->GetDebugName() : "Unknown Attacker", info.GetDamage() );
	}

	// Make sure the player can take damage from the attacking entity
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player can't take damage from that attacker.\n" );
		}
		return 0;
	}

	AddDamagerToHistory( info.GetAttacker() );

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();
	m_LastDamageType = info.GetDamageType();

	if ( IsPlayerClass( TF_CLASS_SPY ) && !( info.GetDamageType() & DMG_FALL ) )
	{
		m_Shared.NoteLastDamageTime( m_lastDamageAmount );
	}
	
	// Save damage force for ragdolls.
	m_vecTotalBulletForce = info.GetDamageForce();
	m_vecTotalBulletForce.x = clamp( m_vecTotalBulletForce.x, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.y = clamp( m_vecTotalBulletForce.y, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.z = clamp( m_vecTotalBulletForce.z, -15000.0f, 15000.0f );

	int bTookDamage = 0;
 
	int bitsDamage = inputInfo.GetDamageType();
	int bitsCustomDamage = inputInfo.GetDamageCustom();

	CBaseEntity *pAttacker = info.GetAttacker();

	bool bIsAlly = false;

	// If we're invulnerable, only apply knockback unless certain special conditions are met
	if ( m_Shared.InCondUber() )
	{
		bool bAllowDamage = false;

		// in deathmatch, ubers needs to be destroyed on spawning players, same for func_croc
		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG || info.GetDamageCustom() == TF_DMG_CUSTOM_CROC )
			bAllowDamage = true;

		// check to see if our attacker is a trigger_hurt (and allow it to kill us even if we're invuln)
		CBaseEntity *pInflictor = info.GetInflictor();

		if ( pAttacker && pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) )
		{
			CTriggerHurt *pTrigger = dynamic_cast<CTriggerHurt *>( pAttacker );
			if ( pTrigger )
				bAllowDamage = true;
		}

		 // same check for teleporter
		if ( pInflictor && pInflictor->IsBaseObject() )
		{
			CBaseObject *pObject = assert_cast<CBaseObject *>( pInflictor );
			if ( pObject->AltMode() == 1 )
				bAllowDamage = true;
		}

		if ( !bAllowDamage )
		{
			//if no damage just apply knockback, if attack coming from ally only do so under certain circumstances
			bIsAlly = pAttacker && pAttacker != this && pAttacker->IsPlayer() && pAttacker->GetTeamNumber() != TF_TEAM_MERCENARY && pAttacker->GetTeamNumber() == GetTeamNumber();

			ApplyDamageKnockback( info, bIsAlly && !of_teamplay_knockback.GetBool() );

			// Even if not taking damage complain about burining
			if ( !(bitsDamage & DMG_BURN ) )
				SpeakConceptIfAllowed( MP_CONCEPT_HURT );

			return 0;
		}
	}

	// explosive weapons deal no self damage in certain game modes
	int iSelfDamageVar = of_selfdamage.GetInt();
	if ( (bitsDamage & DMG_HALF_FALLOFF) && pAttacker == this && iSelfDamageVar < 1 )
	{
		if (!iSelfDamageVar)
		{
			info.CopyDamageToBaseDamage();
			ApplyDamageKnockback(info);
			return 0;
		}
		else
		{
			int iMutator = TFGameRules()->GetMutator();
			if (iMutator == CLAN_ARENA || iMutator == UNHOLY_TRINITY || iMutator == ROCKET_ARENA)
			{
				info.CopyDamageToBaseDamage();
				ApplyDamageKnockback(info);
				return 0;
			}
		}
	}

	CTFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
	CTFWeaponBase *pWeapon = pTFAttacker ? pTFAttacker->GetActiveTFWeapon() : NULL;
	int iWeaponID = pWeapon ? pWeapon->GetWeaponID() : 0;

	if( pWeapon )
		pWeapon->m_flDamageBuildup = min( pWeapon->m_flDamageBuildup + 1.0f, pWeapon->GetTFWpnData().m_iHitsForConsecutiveDamage );

	// if this is our own rocket and we are in midair, scale down the damage
	if ( info.GetAttacker() == this && !GetGroundEntity() )
	{
		float flDamage = info.GetDamage() * tf_damagescale_self_soldier.GetFloat();
		info.SetDamage(flDamage);
	}
	
	bool bIsPlayer = pAttacker && pAttacker->IsPlayer();

	// If we're not damaging ourselves, apply randomness
	if ( pAttacker != this && !(bitsDamage & (DMG_DROWN | DMG_FALL)) ) 
	{
		float flDamage = 0.f;

		if ( bitsDamage & DMG_CRITICAL )
		{
			if ( bDebug )
			{
				Warning( "    CRITICAL!\n");
			}
			
			if ( bitsCustomDamage == TF_DMG_CUSTOM_CRIT_POWERUP )
				flDamage = info.GetDamage() * of_crit_multiplier.GetFloat();
			else
				flDamage = info.GetDamage() * TF_DAMAGE_CRIT_MULTIPLIER;

			// Show the attacker, unless the target is a disguised spy
			if ( bIsPlayer && !m_Shared.InCond(TF_COND_DISGUISED) )
			{
				CEffectData	data;
				if (bitsCustomDamage == TF_DMG_CUSTOM_HEADSHOT)
				{
					data.m_nHitBox = GetParticleSystemIndex("headshot_of");
				}
				else
				{
					data.m_nHitBox = GetParticleSystemIndex("crit_text");
				}
				data.m_vOrigin = WorldSpaceCenter() + Vector(0,0,32);
				data.m_vAngles = vec3_angle;
				data.m_nEntIndex = 0;

				CSingleUserRecipientFilter filter( (CBasePlayer*)pAttacker );
				te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );

				EmitSound_t params;
				params.m_flSoundTime = 0;
				params.m_pSoundName = "TFPlayer.CritHit";
				EmitSound( filter, pAttacker->entindex(), params );

				//DevMsg("Normal crit hit, show crit text.\n");
			}

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
			{
				SpeakConceptIfAllowed( MP_CONCEPT_HURT, "damagecritical:1" );
			}

		}
		else
		{
			float flRandomDamage = info.GetDamage() * tf_damage_range.GetFloat();
			if ( tf_damage_lineardist.GetBool() )
			{
				float flBaseDamage = info.GetDamage() - flRandomDamage;
				flDamage = flBaseDamage + RandomFloat( 0, flRandomDamage * 2 );
			}
			else
			{
				float flMin = 0.25;
				float flMax = 0.75;
				float flCenter = 0.5;

				if ( bitsDamage & DMG_USEDISTANCEMOD )
				{
					float flDistance = max( 1.0, (WorldSpaceCenter() - pAttacker->WorldSpaceCenter()).Length() );
					float flOptimalDistance = 512.0;
					// Rocket launcher & Scattergun have different short range bonuses
					if ( bIsPlayer && iWeaponID == TF_WEAPON_ASSAULTRIFLE )
						flDistance = min(flDistance, 512);

					flCenter = RemapValClamped( flDistance / flOptimalDistance, 0.0, 2.0, 1.0, 0.0 );
					if ( (bitsDamage & DMG_NOCLOSEDISTANCEMOD) && flCenter > 0.5 )
						flCenter = RemapVal( flCenter, 0.5, 1.0, 0.5, 0.65 );

					flMin = max( 0.0, flCenter - 0.25 );
					flMax = min( 1.0, flCenter + 0.25 );

					if ( bDebug )
					{
						Warning("    RANDOM: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
					}
				}
				
				//Msg("Range: %.2f - %.2f\n", flMin, flMax );
				float flRandomVal = tf_damage_disablespread.GetBool() ? flCenter : RandomFloat( flMin, flMax );

				if ( flRandomVal > 0.5 && bIsPlayer && pWeapon )
				{
					switch ( iWeaponID )
					{
					// Rocket launcher only has half the bonus of the other weapons at short range
					case TF_WEAPON_ROCKETLAUNCHER:
					case TF_WEAPON_ROCKETLAUNCHER_DM:
					case TF_WEAPON_SUPER_ROCKETLAUNCHER:
						flRandomDamage *= 0.5f;
						break;
							
					// Scattergun gets 50% bonus of other weapons at short range
					case TF_WEAPON_SCATTERGUN:
						flRandomDamage *= 1.5f;
						break;

					// SSG gets 100% bonus of other weapons at short range
					case TF_WEAPON_SUPERSHOTGUN:
						flRandomDamage *= (pWeapon->GetTFWpnData().m_flSuperShotgunRampUp);
						break;

					default:
						break;
					}
				}

				float flOut = SimpleSplineRemapValClamped( flRandomVal, 0, 1, -flRandomDamage, flRandomDamage );
				flDamage = info.GetDamage() + flOut;
			}
			
			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
				SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}

		info.SetDamage( flDamage );
	}

	if (bitsCustomDamage == TF_DMG_CUSTOM_LEGSHOT)
	{
		info.SetDamage(info.GetDamage() * 0.5);

		if (bIsPlayer && !m_Shared.InCond(TF_COND_DISGUISED))
		{
			CEffectData	data;
			data.m_nHitBox = GetParticleSystemIndex("tfc_sniper_mist");
			data.m_vOrigin = WorldSpaceCenter() + Vector(0, 0, 0);
			data.m_vAngles = vec3_angle;
			data.m_nEntIndex = 0;

			CSingleUserRecipientFilter filter((CBasePlayer*)pAttacker);
			te->DispatchEffect(filter, 0.0, data.m_vOrigin, "ParticleEffect", data);

			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.Legshot";
			EmitSound(filter, pAttacker->entindex(), params);
		}
	}

	if (bitsCustomDamage == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)
	{
		info.SetDamage(info.GetDamage() * 1.875);

		if (bIsPlayer && !m_Shared.InCond(TF_COND_DISGUISED))
		{
			CEffectData	data;
			data.m_nHitBox = GetParticleSystemIndex("headshot_of");
			data.m_vOrigin = WorldSpaceCenter() + Vector(0, 0, 32);
			data.m_vAngles = vec3_angle;
			data.m_nEntIndex = 0;

			CSingleUserRecipientFilter filter((CBasePlayer*)pAttacker);
			te->DispatchEffect(filter, 0.0, data.m_vOrigin, "ParticleEffect", data);

			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.CritHit";
			EmitSound(filter, pAttacker->entindex(), params);

			//DevMsg("Railgun's headshot, show fake crits.\n");
		}
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_MINIGUN && TFGameRules()->IsInfGamemode() )
	{
		// this deals less damage in infection
		info.SetDamage( info.GetDamage() * 0.5 );
	}

	////////////////
	///ARMOR AREA///
	////////////////

	info.CopyDamageToBaseDamage();

	if ((of_armor.GetInt() == 1) || (of_armor.GetInt() == -1))
	{

		if (m_Shared.GetTFCArmor() > 0)
		{
			if (!((info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB) || (info.GetDamageCustom() == TF_DMG_CUSTOM_POISON)) && !(info.GetDamageType() & (DMG_FALL | DMG_DROWN)))
			{
				float flArmorRatio = 0.5f;

				switch (GetPlayerClass()->GetArmorType())
				{
				case ARMOR_LIGHT:
					flArmorRatio = of_armor_light_absorption.GetFloat();
					//DevMsg("Light damage absorption\n");
					break;
				case ARMOR_MEDIUM:
					flArmorRatio = of_armor_med_absorption.GetFloat();
					//DevMsg("Med damage absorption\n");
					break;
				case ARMOR_HEAVY:
					flArmorRatio = of_armor_heavy_absorption.GetFloat();
					//DevMsg("Heavy damage absorption\n");
					break;
				}

				float flNewDamage = info.GetDamage() * (1.0f - flArmorRatio); //Non-armor Damage that wasn't absorpted

				float flCurrentArmor = m_Shared.m_iArmor;
				float flRemovedArmor = (info.GetDamage() - flNewDamage);


				if (flRemovedArmor > flCurrentArmor)
				{
					flNewDamage = info.GetDamage() - flCurrentArmor;
					flRemovedArmor = flCurrentArmor;

					m_Shared.SetTFCArmor(0);
				}
				else
				{
					flRemovedArmor = (int)(flRemovedArmor + 0.5);
					m_Shared.SetTFCArmor(flCurrentArmor - flRemovedArmor);
				}

				info.SetDamage((int)(flNewDamage));
			}
		}
	}

	////////////////
	///ARMOR AREA///
	////////////////

	// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
	bTookDamage = CBaseCombatCharacter::OnTakeDamage( info );

	// Early out if the base class took no damage
	if ( !bTookDamage )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player failed to take the damage.\n" );
		}
		return 0;
	}

	if ( bDebug )
	{
		Warning( "    DEALT: Player took %.2f damage.\n", info.GetDamage() );
		Warning( "    HEALTH LEFT: %d\n", GetHealth() );
	}

	// Send the damage message to the client for the hud damage indicator
	// Don't do this for damage types that don't use the indicator
	if ( !(bitsDamage & (DMG_DROWN | DMG_FALL | DMG_BURN) ) )
	{
		// Try and figure out where the damage is coming from
		Vector vecDamageOrigin = info.GetReportedPosition();

		// If we didn't get an origin to use, try using the attacker's origin
		if ( vecDamageOrigin == vec3_origin && info.GetInflictor() )
		{
			vecDamageOrigin = info.GetInflictor()->GetAbsOrigin();
		}

		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( clamp( (int)info.GetDamage(), 0, 255 ) );
			WRITE_VEC3COORD( vecDamageOrigin );
		MessageEnd();
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( info.GetInflictor() && info.GetInflictor()->edict() )
	{
		m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();
	}

	m_DmgTake += (int)info.GetDamage();

	// Reset damage time countdown for each type of time based damage player just sustained
	int iDamage = 0;
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
			m_rgbTimeBasedDamage[i] = 0;
	}

	// No view punch sounds with teamplay knockback enabled but no friendlyfire
	bool bNoPunchSound = of_teamplay_knockback.GetBool() && !friendlyfire.GetBool();
	if (!bIsAlly)
		bIsAlly = pAttacker && pAttacker != this && pAttacker->IsPlayer() && pAttacker->GetTeamNumber() != TF_TEAM_MERCENARY && pAttacker->GetTeamNumber() == GetTeamNumber();

	if ( !bNoPunchSound || ( bNoPunchSound && !bIsAlly ) )
	{
		// Display any effect associate with this damage type
		DamageEffect( info.GetDamage(), bitsDamage );

		m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
		m_bitsHUDDamage = -1;  // make sure the damage bits get resent

		if ( ! (bitsDamage & DMG_DISSOLVE ) )
			m_Local.m_vecPunchAngle.SetX(-2);

		PainSound(info);

		PlayFlinch(info);

		int iHealthBoundary = GetMaxHealth() * 0.25;
		if ( GetHealth() <= iHealthBoundary && iHealthBefore > iHealthBoundary )
			ClearExpression();
	}

	CTF_GameStats.Event_PlayerDamage( this, info, iHealthBefore - GetHealth() );

	if (GetBotController())
	{
		GetBotController()->OnTakeDamage(inputInfo);
	}

	return bTookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DamageEffect(float flDamage, int fDamageType)
{
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );

	if (fDamageType & DMG_CRUSH)
	{
		//Red damage indicator
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_DROWN)
	{
		//Red damage indicator
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_SLASH)
	{
		if ( !bDisguised )
		{
			// If slash damage shoot some blood
			SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
		}
	}
	else if ( fDamageType & DMG_BULLET )
	{
		if ( !bDisguised )
		{
			EmitSound( "Flesh.BulletImpact" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DismemberRandomLimbs( void )
{
	int iGore = 0;

	if ( m_iGoreHead < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 1 )
			iGore = 2;

		if ( m_iGoreHead < iGore )
			m_iGoreHead = iGore;
	}
	if ( m_iGoreLeftArm < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 1 )
			iGore = 2;

		if ( m_iGoreLeftArm < iGore )
			m_iGoreLeftArm = iGore;
	}
	if ( m_iGoreRightArm < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 1 )
			iGore = 2;

		if ( m_iGoreRightArm < iGore )
			m_iGoreRightArm = iGore;
	}
	if ( m_iGoreLeftLeg < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 1 )
			iGore = 2;

		if ( m_iGoreLeftLeg < iGore )
			m_iGoreLeftLeg = iGore;
	}
	if ( m_iGoreRightLeg < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 1 )
			iGore = 2;

		if ( m_iGoreRightLeg < iGore )
			m_iGoreRightLeg = iGore;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS )
	{
		// coop needs to return false
		if ( TFGameRules() && TFGameRules()->IsCoopEnabled() )
			return false;

		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return of_teamplay_collision.GetBool();
			break;
		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return of_teamplay_collision.GetBool();
			break;
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//---------------------------------------
// Is the player the passed player class?
//---------------------------------------
bool CTFPlayer::IsPlayerClass( int iClass ) const
{
	const CTFPlayerClass *pClass = &m_PlayerClass;

	if ( !pClass )
		return false;

	return ( pClass->IsClass( iClass ) );
}

// copied from hl2player
void CTFPlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	BaseClass::LeaveVehicle( vecExitPoint, vecExitAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state or if we are still in unassigned team
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) || !m_Shared.InState( TF_STATE_ACTIVE ) )
		return;

	// Civilian can't suicide in escort to prevent griefing
	if ( TFGameRules() && TFGameRules()->IsESCGamemode() && IsPlayerClass( TF_CLASS_CIVILIAN )  )
		return;

	// Don't suicide during the "bonus time" if we're not on the winning team
	if ( !bForce && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		( ( GetTeamNumber() != TFGameRules()->GetWinningTeam() ) || ( TFGameRules()->GetWinningTeam() == TF_TEAM_MERCENARY && !m_Shared.IsTopThree() ) ) )
	{
		return;
	}
	
	m_iSuicideCustomKillFlags = TF_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();
	bool bIsAlly = pAttacker && pAttacker != this && pAttacker->IsPlayer() && pAttacker->GetTeamNumber() != TF_TEAM_MERCENARY && pAttacker->GetTeamNumber() == GetTeamNumber();

	// Apply knockback, but not if it's coming for an ally and friendly knockback is off
	// Grab the vector of the incoming attack. 
	// (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}
	ApplyDamageKnockback( info, bIsAlly && !of_teamplay_knockback.GetBool(), &vecDir );
	
	// Do the damage.
	m_bitsDamageType |= info.GetDamageType();

	int iOldHealth = m_iHealth;
	bool bIgniting = false;

	// Apply a damage force unless player is protected from that damage
	bool bNoFriendlyFire = bIsAlly && !friendlyfire.GetBool();
	if ( m_takedamage != DAMAGE_EVENTS_ONLY && !bNoFriendlyFire )
	{
		// Start burning if we took ignition damage
		bIgniting = ( ( info.GetDamageType() & DMG_IGNITE ) && ( GetWaterLevel() < WL_Waist ) );

		if ( info.GetDamage() == 0.0f )
			return 0;

		//Scale damage if player has a shield powerup, duel has a fixed multiplier
		float flScaledDamage = info.GetDamage();
		if ( m_Shared.InCondShield() )
		{
			if (TFGameRules()->IsDuelGamemode())
			{
				flScaledDamage *= 0.5f;
				m_iShieldDamage += flScaledDamage;

				if (m_iShieldDamage >= 150)
					m_Shared.RemoveCondShield();
			}
			else
			{
				flScaledDamage *= of_resistance.GetFloat();
			}
		}

		// Take damage - round to the nearest integer.
		m_iHealth -= (flScaledDamage + 0.5f);
		
		if( (flScaledDamage + 0.5f) > 0 )
		{
			//adjust the overheal according to damage taken
			// health is 400
			// Armored health is 150
			// Default hp is 150
			// Default + armor = 300
			// therefor, Overheal = 100
			int iOverheal = iOldHealth - m_Shared.GetDefaultHealth() - m_iMegaOverheal;
			// Damage is 200, so the overheal absorbs the 100 which leaves the megaheal to take 100
			// If our megaheal is 0, overheal is 100, damage is 40
			// 40 - 100 = -60, we dont want this, so if its less than 0 dont add anything to megaheal
			
			// My brain was on airplane mode trying to figure out my own formula
			// Yes i wrote this equasion before i even thought about all of this, i dont know how either
			m_iMegaOverheal = max( 0, 
			m_iMegaOverheal - max( 0,
			((flScaledDamage + 0.5f) - iOverheal)
			));
		}
		
		if( m_iHealth <= m_Shared.GetDefaultHealth() )
			m_iMegaOverheal = 0;
	}

	m_flLastDamageTime = gpGlobals->curtime;
	
	if ( !pAttacker )
		return 0;

	if ( bIgniting )
		m_Shared.Burn( ToTFPlayer( pAttacker ), 0 );

	// Fire a global game event - "player_hurt"
	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "health", max( 0, m_iHealth ) );
		event->SetInt( "damageamount", ( iOldHealth - m_iHealth ) );
		event->SetBool("crit", (info.GetDamageType() & DMG_CRITICAL) != 0);
		
		// HLTV event priority, not transmitted
		event->SetInt( "priority", 5 );	

		CTFPlayer *pPlayer = ToTFPlayer(pAttacker);

		event->SetInt( "attacker", pPlayer ? pPlayer->GetUserID() : 0 );
		event->SetInt( "victim_index", entindex() );
		event->SetInt( "attacker_index", pAttacker->entindex() );

		if (pPlayer && pPlayer->m_iImpressiveCount == 2)
		{
			event->SetBool("impressive", true);
			pPlayer->m_iImpressiveCount = 0;
		}

        gameeventmanager->FireEvent( event );
	}
	
	if ( pAttacker != this && pAttacker->IsPlayer() )
	{
		ToTFPlayer( pAttacker )->RecordDamageEvent( info, (m_iHealth <= 0) );
	}

	//No bleeding while invul or disguised.
	bool bBleed = !m_Shared.InCond(TF_COND_DISGUISED);
	if ( bBleed && pAttacker->IsPlayer() )
	{
		CTFWeaponBase *pWeapon = ToTFPlayer( pAttacker )->GetActiveTFWeapon();
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
			bBleed = false;
	}
	
	// DMG_GENERIC and DMG_DISSOLVE must not bleed
	if ( info.GetDamageType() & (DMG_GENERIC | DMG_DISSOLVE ) )
		bBleed = false;

	if ( bBleed && !bNoFriendlyFire )
	{
		Vector vDamagePos = info.GetDamagePosition();

		if ( vDamagePos == vec3_origin )
			vDamagePos = WorldSpaceCenter();

		CPVSFilter filter( vDamagePos );
		TE_TFBlood( filter, 0.0, vDamagePos, -vecDir, entindex() );
	}

	// Done.
	return 1;
}

void CTFPlayer::ResetShieldDamage()
{
	m_iShieldDamage = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people who damaged player
//-----------------------------------------------------------------------------
void CTFPlayer::AddDamagerToHistory( EHANDLE hDamager )
{
	// sanity check: ignore damager if it is on our team and on ourself.  (Catch-all for 
	// damaging self in rocket jumps, etc.)
	CTFPlayer *pDamager = ToTFPlayer( hDamager );
		if ( !pDamager || pDamager == this || ( InSameTeam( pDamager ) && pDamager->GetTeamNumber() != TF_TEAM_MERCENARY ) )
			return;
	
	// If this damager is different from the most recent damager, shift the
	// damagers down and drop the oldest damager.  (If this damager is already
	// the most recent, we will just update the damage time but not remove
	// other damagers from history.)
	if ( m_DamagerHistory[0].hDamager != hDamager )
	{
		for ( int i = 1; i < ARRAYSIZE( m_DamagerHistory ); i++ )
		{
			m_DamagerHistory[i] = m_DamagerHistory[i-1];
		}		
	}	
	// set this damager as most recent and note the time
	m_DamagerHistory[0].hDamager = hDamager;
	m_DamagerHistory[0].flTimeDamage = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Clears damager history
//-----------------------------------------------------------------------------
void CTFPlayer::ClearDamagerHistory()
{
	for ( int i = 0; i < ARRAYSIZE( m_DamagerHistory ); i++ )
	{
		m_DamagerHistory[i].Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGib(const CTakeDamageInfo &info)
{
	// Check to see if we should allow players to gib.
	if ( !tf_playergib.GetBool() )
		return false;

	if ( tf_playergib.GetInt() == 2 || TFGameRules()->IsMutator( INSTAGIB ) || TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) )
		return true;

	if ( info.GetDamageType() & DMG_ALWAYSGIB )
		return true;

	if ( info.GetDamageType() & DMG_NEVERGIB )
		return false;

	CTFWeaponBase *pWeapon =(CTFWeaponBase *)( info.GetWeapon() );
	if( pWeapon && pWeapon->GetTFWpnData().m_bGibOnOverkill && info.GetDamage() > GetMaxHealth() )
		return true;
	if (pWeapon && pWeapon->GetTFWpnData().m_bGibOnHeadshot && ((info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT) || (info.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)))
		return true;

	if ( ( info.GetDamageType() & DMG_BLAST )
		|| ( info.GetDamageType() & DMG_HALF_FALLOFF ) )
	{
		// if the player fatal damage surpassed the player's health by 20, then gib them (live tf2 behavior)
		if ( m_iHealth < -19 || ( info.GetDamageType() & DMG_CRITICAL ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	if ( pVictim->IsPlayer() )
	{
		CTFPlayer *pTFVictim = ToTFPlayer(pVictim);
		CTFPlayer *pTFAttacker = ToTFPlayer(info.GetAttacker());
		if ( pTFAttacker )
		{
			if ( pTFVictim != pTFAttacker )
				pTFVictim->GotKilled();
		}
		
		// Custom death handlers
		const char *pszCustomDeath = "customdeath:none";
		const char *pszDamageType = "damagetype:none";
		if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT || info.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON )
		{
			pszCustomDeath = "customdeath:taunt_heavy";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN )
		{
			pszCustomDeath = "customdeath:taunt_pyro";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_FENCING )
		{
			pszCustomDeath = "customdeath:taunt_spy";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_BOND)
		{
			pszCustomDeath = "customdeath:taunt_mercenary";
		}

		// Revenge handler
		const char *pszDomination = "domination:none";
		if ( pTFVictim->GetDeathFlags() & (TF_DEATH_REVENGE|TF_DEATH_ASSISTER_REVENGE) )
		{
			pszDomination = "domination:revenge";
		}
		else if ( pTFVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			pszDomination = "domination:dominated";
		}
		
		if ( info.GetDamageType() & DMG_BLAST )
		{
			pszDamageType = "blast";
		}
		
		CFmtStrN<128> modifiers( "%s,%s,victimclass:%s,damagetype:%s", pszCustomDeath, pszDomination, g_aPlayerClassNames_NonLocalized[ pTFVictim->GetPlayerClass()->GetClassIndex() ], pszDamageType);
		SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );
	}
	else
	{
		if ( pVictim->IsBaseObject() )
		{
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( pVictim );
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_OBJECT, pObject->GetResponseRulesModifier() );
		}
		// check npcs too!
		else if ( pVictim->IsNPC() )
		{
		// Custom death handlers
		const char *pszCustomDeath = "customdeath:none";
		if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT || info.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON )
		{
			pszCustomDeath = "customdeath:taunt_heavy";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN )
		{
			pszCustomDeath = "customdeath:taunt_pyro";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_FENCING )
		{
			pszCustomDeath = "customdeath:taunt_spy";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_BOND)
		{
			pszCustomDeath = "customdeath:taunt_mercenary";
		}

		const char *pszDomination = "domination:none";

		CFmtStrN<128> modifiers("%s,%s,victimclass:%s", pszCustomDeath, pszDomination, g_aPlayerClassNames_NonLocalized[TF_CLASS_UNDEFINED]);
		SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	SpeakConceptIfAllowed( MP_CONCEPT_DIED );

	StateTransition( TF_STATE_DYING );	// Transition into the dying state.
	
	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTFPlayer *pPlayerAttacker = NULL;

	if (TFGameRules() && TFGameRules()->IsArenaGamemode()) // If you killed someone and it wasnt yourself
	{
		int iShitWayOfDoingThis = (m_Shared.GetTFLives() - 1);
		m_Shared.SetTFLives(iShitWayOfDoingThis);
		SetLives(iShitWayOfDoingThis);
	}

	//Medal shenanigans
	if ( pAttacker && pAttacker->IsPlayer() )
	{
		pPlayerAttacker = ToTFPlayer( info.GetAttacker() );

		if ( pPlayerAttacker != this )
		{
			//Powerup Massacre
			pPlayerAttacker->m_iPowerupKills += m_Shared.InPowerupCond() ? 1 : 0; //count kills while holding powerup

			//Excellent
			if (pPlayerAttacker->m_iEXKills >= 9) //reset after achieving the highest EX medal
				pPlayerAttacker->m_iEXKills = 0;
			pPlayerAttacker->m_iEXKills = gpGlobals->curtime < pPlayerAttacker->m_fEXTime + 3.f ? pPlayerAttacker->m_iEXKills + 1 : 0;	//kills must me done within 3 seconds from one another, otherwise counter resets
			pPlayerAttacker->m_fEXTime = gpGlobals->curtime;

			//Killing Spree
			pPlayerAttacker->m_iSpreeKills++;
		}
	}
	
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	// we want the ragdoll to burn if the player was burning and was not a pryo (who only burns momentarily)
	bool bBurning = m_Shared.InCond( TF_COND_BURNING ) && ( TF_CLASS_PYRO != GetPlayerClass()->GetClassIndex() );
	// no playing death anims in midair, as it looks awkward
	bool bFlagOnGround = ( ( GetFlags() & FL_ONGROUND ) != NULL );

	// Remove all conditions...
	if (m_Shared.InPowerupCond())
		m_bHadPowerup = true;
	m_Shared.RemoveAllCond( NULL );

	//remove hook as precaution
	m_Shared.SetHook(NULL);

	// Reset our model if we were disguised
	if ( bDisguised )
	{
		UpdateModel();
	}

	if ( m_bIsJuggernaught )
	{
		m_bIsJuggernaught = false; 
		m_Shared.OnRemoveJauggernaught();
	}

	RemoveTeleportEffect();
	
	// Stop being invisible
	m_Shared.RemoveCondInvis();

	// Detonate any hauled object
	if ( m_bHauling )
	{
		CTFWeaponBuilder *pBuilder = static_cast<CTFWeaponBuilder*>( Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );

		if ( pBuilder )	
		{
			CBaseObject *obj = pBuilder->m_hObjectBeingBuilt.Get();

			if ( obj && obj->GetType() != OBJ_ATTACHMENT_SAPPER )
			{
				obj->m_takedamage = true;

				// give credits to them for destroying this object
				if ( pAttacker )
				{
					CTakeDamageInfo info( pInflictor, pAttacker, GetActiveTFWeapon(), GetAbsOrigin(), GetAbsOrigin(), 5000, DMG_BULLET );
					obj->Killed( info );
				}
			}
		}
	}

	// Drop a pack with their leftover ammo
	// This is terrible but it works for now
	if( !GetBotController() || !static_cast<CBot*>(GetBotController())->RemoveOnDeath() )
	{
		DropAmmoPack();

		int Clip = -2;
		int Reserve = -2;
		CTFWeaponBase *pTFWeapon = m_Shared.GetActiveTFWeapon();

		if( pTFWeapon )
		{
			if( !of_fullammo.GetBool() )
			{
				Clip = pTFWeapon->m_iClip1;
				Reserve = pTFWeapon->m_iReserveAmmo;
			}

			if( !pTFWeapon->GetTFWpnData().m_bAlwaysDrop )
			{
				DropWeapon(pTFWeapon, false, false, Clip, Reserve);
			}
		}
		
		Clip = -1;
		Reserve = -1;
		for ( int i = 0; i < m_hSuperWeapons.Count(); i++ )
		{
			if ( m_hSuperWeapons[i] )
			{
				CTFWeaponBase *pSuperWeapon = m_hSuperWeapons[i].Get();
				if ( pSuperWeapon && pSuperWeapon != GetActiveWeapon() )
				{
					Clip = pSuperWeapon->m_iClip1;
					Reserve = pSuperWeapon->m_iReserveAmmo;
					DropWeapon( pSuperWeapon, false, false ,Clip, Reserve );
				}
			}
		}	
		m_hSuperWeapons.Purge();
		
		for ( int i = 0; i < m_hPowerups.Count(); i++ )
		{
			if ( m_hPowerups[i] )
			{
				m_hPowerups[i]->SetAbsOrigin( GetAbsOrigin() );
				m_hPowerups[i]->SetAbsAngles( QAngle( 0, 0, 0 ) );
				m_hPowerups[i]->SetTouch( &CTFDroppedPowerup::PackTouch );
				m_hPowerups[i]->SetThink( &CTFDroppedPowerup::FlyThink );
				m_hPowerups[i]->SetOwnerEntity( NULL );
				DispatchSpawn( m_hPowerups[i] );
				m_hPowerups[i]->SetNextThink( gpGlobals->curtime ); // Set the next think to happen imidiatley
				TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, m_hPowerups[i]->GetPowerupDroppedLine() );
			}
		}
		m_hPowerups.Purge();
	}

	// If the player has a capture flag and was killed by another player, award that player a defense
	if ( HasItem() && pPlayerAttacker && ( pPlayerAttacker != this ) )
	{
		CCaptureFlag *pCaptureFlag = dynamic_cast<CCaptureFlag *>( GetItem() );
		if ( pCaptureFlag )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", pPlayerAttacker->entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DEFEND );
				event->SetInt( "priority", 8 );
				gameeventmanager->FireEvent( event );
			}
			CTF_GameStats.Event_PlayerDefendedPoint( pPlayerAttacker );
		}
	}

	// Remove all items...
	RemoveAllItems( true );

	for ( unsigned int iWeapon = 0; iWeapon < m_iWeaponCount; ++iWeapon )
	{
		CTFWeaponBase *pTFWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );
		if ( pTFWeapon )
			pTFWeapon->WeaponReset();
	}

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if (pWeapon)
	{
		pWeapon->SendViewModelAnim(ACT_IDLE);
		pWeapon->Holster();
		SetActiveWeapon( NULL );
	}

	ClearZoomOwner();

	m_vecLastDeathPosition = GetAbsOrigin();

	CTakeDamageInfo info_modified = info;

	// Ragdoll, gib, or death animation.
	bool bRagdoll = true;
	bool bGib = false;
	// See if we should gib.
	if ( ShouldGib( info ) )
	{
		bGib = true;
		bRagdoll = false;
	}
	/*
	else
	// See if we should play a custom death animation.
	{
		if ( PlayDeathAnimation( info, info_modified ) )
		{
			bRagdoll = false;
		}
	}
	*/

	int iDamageType = info.GetDamageType();
	if ( iDamageType & DMG_FALL ) // fall damage
	{
		// begone legs!
		if ( m_iGoreRightLeg < 2 )
			m_iGoreRightLeg = 2;
		if ( m_iGoreLeftLeg < 2 )
			m_iGoreLeftLeg = 2;
	}

	if ( iDamageType & DMG_BLAST || iDamageType & DMG_NERVEGAS ) // explosives or sawblade
		DismemberRandomLimbs();

	if ( info_modified.GetDamageCustom() == TF_DMG_CUSTOM_DECAPITATION_BOSS )
		m_iGoreHead = 2;

	// show killer in death cam mode
	// chopped down version of SetObserverTarget without the team check
	if( pPlayerAttacker )
	{
		// See if we were killed by a sentrygun. If so, look at that instead of the player
		if ( pInflictor && pInflictor->IsBaseObject() )
		{
			// Catches the case where we're killed directly by the sentrygun (i.e. bullets)
			// Look at the sentrygun
			m_hObserverTarget.Set( pInflictor ); 
		}
		// See if we were killed by a projectile emitted from a base object. The attacker
		// will still be the owner of that object, but we want the deathcam to point to the 
		// object itself.
		else if ( pInflictor && pInflictor->GetOwnerEntity() && 
					pInflictor->GetOwnerEntity()->IsBaseObject() )
		{
			m_hObserverTarget.Set( pInflictor->GetOwnerEntity() );
		}
		else
		{
			// Look at the player
			m_hObserverTarget.Set( pAttacker ); 
		}
		
		// I was supposed to kill him, I'll go after you now
		if ( this == TFGameRules()->GetIT() )
			TFGameRules()->SetIT( pPlayerAttacker );

		// reset fov to default
		SetFOV( this, 0 );
	}
	// look at our npc
	else if ( pAttacker && pAttacker->IsNPC() )
	{
		m_hObserverTarget.Set( pAttacker );
	}
	else if ( pAttacker && pAttacker->IsBaseObject() )
	{
		// Catches the case where we're killed by entities spawned by the sentrygun (i.e. rockets)
		// Look at the sentrygun. 
		m_hObserverTarget.Set( pAttacker ); 
	}
	else
	{
		m_hObserverTarget.Set( NULL );
	}

	if ( info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE )
	{
		// if this was suicide, recalculate attacker to see if we want to award the kill to a recent damager
		info_modified.SetAttacker( TFGameRules()->GetDeathScorer( pAttacker, pInflictor, this ) );
	}
	else if ( !pAttacker || pAttacker->IsBSPModel() || this == pAttacker )
	{
		// if this was suicide or environmental death, recalculate attacker
		CBasePlayer *pPlayerDamager = TFGameRules()->GetRecentDamager( this, NULL, TF_TIME_DEATH_KILL_CREDIT );

		if ( pPlayerDamager )
		{
			info_modified.SetAttacker( pPlayerDamager );
			info_modified.SetInflictor( NULL );

			info_modified.SetDamageType( DMG_GENERIC );
			info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );

			info_modified.SetWeapon( NULL );
		}
	}

	// create particles and sounds for a full medic kill
	if ( IsPlayerClass( TF_CLASS_MEDIC ) && pAttacker )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

		if ( pWpn )
		{
			CWeaponMedigun *pMedigun = dynamic_cast <CWeaponMedigun*>( pWpn );

			if ( pMedigun && pMedigun->GetChargeLevel() > 0.99f )
			{
				if ( GetTeamNumber() == 2 )
					DispatchParticleEffect( "electrocuted_gibbed_red", GetAbsOrigin(), vec3_angle );
				else if ( GetTeamNumber() == 3 )
					DispatchParticleEffect( "electrocuted_gibbed_blue", GetAbsOrigin(), vec3_angle );
				else
					DispatchParticleEffect( "electrocuted_gibbed_dm", GetAbsOrigin(), vec3_angle );

				EmitSound( "TFPlayer.MedicChargedDeath" );
			}
		}
	}

	bool bDoGunGameCheck = pPlayerAttacker && TFGameRules() && TFGameRules()->IsGGGamemode() && pPlayerAttacker != this;

	// We have to increase our level before BaseClass::Event_Killed since we check if someone won during it
	// So the winning level up has to be done here
	// 
	// However, we dont want to switch weapons here, since that would cause issues with the killfeed and similar
	bool bGiveNextGunGameGun = false;
	if( bDoGunGameCheck )
	{
		if ( pPlayerAttacker->GGLevel() == TFGameRules()->m_iMaxLevel-1 )
			pPlayerAttacker->IncrementLevelProgress(TFGameRules()->m_iRequiredKills);
		else
			pPlayerAttacker->IncrementLevelProgress(1);

		if ( pPlayerAttacker->GetLevelProgress() >= TFGameRules()->m_iRequiredKills )
		{
			pPlayerAttacker->SetLevelProgress(0);
			pPlayerAttacker->IncrementGGLevel(1);
			bGiveNextGunGameGun = true;
		}
	}

	BaseClass::Event_Killed( info_modified );

	// We give the new weapon after Event_Killed so that it doesn't accidentally get used instead
	// of the previous weapon in calls, particularly clientside ones
	if( bGiveNextGunGameGun )
	{
		if ( pPlayerAttacker->GGLevel() < TFGameRules()->m_iMaxLevel )
			pPlayerAttacker->UpdateGunGameLevel();
	}
	
	bool bDissolve = false;
	if ( iDamageType & DMG_DISSOLVE )
		bDissolve = true;
	
	if (((TF_DMG_CUSTOM_HEADSHOT == info.GetDamageCustom()) || (TF_DMG_CUSTOM_RAILGUN_HEADSHOT == info.GetDamageCustom())) && pInflictor )
		CTF_GameStats.Event_Headshot( pPlayerAttacker );
	else if ( TF_DMG_CUSTOM_BACKSTAB == info.GetDamageCustom() && pInflictor )
		CTF_GameStats.Event_Backstab( pPlayerAttacker );

	// Create the ragdoll entity.
	if ( bGib || bRagdoll )
		CreateRagdollEntity( bGib, bBurning, bDissolve, bFlagOnGround, info.GetDamageCustom() );

	// Don't overflow the value for this.
	m_iHealth = 0;

	// If we died in sudden death, explode our buildings
	//if ( IsPlayerClass( TF_CLASS_ENGINEER ) && TFGameRules()->InStalemate() )
	if ( TFGameRules() && TFGameRules()->InStalemate() )
	{
		for (int i = GetObjectCount()-1; i >= 0; i--)
		{
			CBaseObject *obj = GetObject(i);
			Assert( obj );

			if ( obj )
			{
				obj->DetonateObject();
			}		
		}
	}

	// Reward killer with health
	if ( pPlayerAttacker )
		pPlayerAttacker->TakeHealth( of_healonkill.GetFloat(), DMG_GENERIC );

	DestroyViewModels();

	if ( TFGameRules() && TFGameRules()->IsInfGamemode() && GetTeamNumber() == TF_TEAM_RED )
	{
		if ( TFGameRules()->GetInfectionRoundTimer() && !TFGameRules()->InSetup() && !TFGameRules()->IsInWaitingForPlayers() )
		{
			ChangeTeam( TF_TEAM_BLUE, false );

			if (!TFGameRules()->IsClassSelectAllowed(TF_TEAM_BLUE))
				SetDesiredPlayerClassIndex(TFGameRules()->GetForcedClassIndex(TF_TEAM_BLUE));

			// PLACEHOLDER
			CBroadcastRecipientFilter filter;
			EmitSound( filter, entindex(), "Game.Infection.Infected" );

			if( TeamplayRoundBasedRules() && GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers() > 2 && random->RandomInt( 1, 4 ) == 1 )
				TeamplayRoundBasedRules()->BroadcastSound( TEAM_UNASSIGNED, "SomeoneDied" );
		}
	}

	if ( TFGameRules() && TFGameRules()->IsESCGamemode() && IsPlayerClass( TF_CLASS_CIVILIAN ) )
	{
		CBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), "Esc.Death" );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "The Hunted was eliminated! :(\n" );

		// RED gets 25 poinst out of 100 for killing Civilian in Escort gamemode on a PL map
		// Don't do this if he suicides by reaching the goal!
		if ( TFGameRules()->IsPayloadOverride() && !m_bWinDeath )
		{
			int amount = 25;

			int dom_score_red_old = TFGameRules()->m_nDomScore_red;
			int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

			// is the resulting score gonna be over the limit? clamp it
            if ( ( dom_score_red_old + amount ) > TFGameRules()->m_nDomScore_limit)
                TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_limit;
			else
				TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_red + amount;

			TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
		}
	}

	if ( pAttacker && pInflictor == pAttacker && pAttacker->IsBSPModel() )
	{
		CTFPlayer *pPlayerDamager = TFGameRules()->GetRecentDamager( this, NULL, TF_TIME_DEATH_KILL_CREDIT );

		if ( pPlayerDamager )
		{
			// I'm not sure what live tf2 uses this for
			IGameEvent *event = gameeventmanager->CreateEvent( "environmental_death" );

			if ( event )
			{
				event->SetInt( "killer", pPlayerDamager->GetUserID() );
				event->SetInt( "victim", GetUserID() );

				// HLTV event priority, not transmitted
				event->SetInt( "priority", 9 );

				gameeventmanager->FireEvent( event );
			}
		}
	}
	if( GetBotController() )
	{
		GetBotController()->OnDeath(info);
	}

	m_bDied = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// moved to tf_player_shared
/*
bool CTFPlayer::PlayDeathAnimation( const CTakeDamageInfo &info, CTakeDamageInfo &info_modified )
{
	//if ( SelectWeightedSequence( ACT_DIESIMPLE ) == -1 )
	//	return false;

	// Get the attacking player.
	CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
		return false;

	bool bPlayDeathAnim = false;

	// Check for a sniper headshot. (Currently only on Heavy.)
	if ( ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT ) )
	{
		//if ( GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) )
		//{
			bPlayDeathAnim = true;
		//}
	}
	// Check for a spy backstab. (Currently only on Sniper.)
	else if ( ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB ) )
	{
		//if ( GetPlayerClass()->IsClass( TF_CLASS_SNIPER ) )
		//{
			bPlayDeathAnim = true;
		//}
	}

	// Play death animation?
	if ( bPlayDeathAnim )
	{
		info_modified.SetDamageType( info_modified.GetDamageType() | DMG_REMOVENORAGDOLL | DMG_PREVENT_PHYSICS_FORCE );

		SetAbsVelocity( vec3_origin );
		DoAnimationEvent( PLAYERANIMEVENT_DIE );

		int sequence = LookupSequence( "primary_death_headshot" );	

		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, sequence );

		// No ragdoll yet.
		if ( m_hRagdoll.Get() )
		{
			UTIL_Remove( m_hRagdoll );
		}
	}

	return bPlayDeathAnim;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//			&vecOrigin - 
//			&vecAngles - 
//-----------------------------------------------------------------------------
bool CTFPlayer::CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles )
{
	// Look up the hand and weapon bones.
	int iHandBone = LookupBone( "weapon_bone" );
	if ( iHandBone == -1 )
		return false;

	GetBonePosition( iHandBone, vecOrigin, vecAngles );

	// Draw the position and angles.
	Vector vecDebugForward2, vecDebugRight2, vecDebugUp2;
	AngleVectors( vecAngles, &vecDebugForward2, &vecDebugRight2, &vecDebugUp2 );

	/*
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugForward2 * 25.0f ), 255, 0, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugRight2 * 25.0f ), 0, 255, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugUp2 * 25.0f ), 0, 0, 255, false, 30.0f ); 
	*/

	VectorAngles( vecDebugUp2, vecAngles );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: If we don't let players drop ammo boxes, we don't need this code..
//-----------------------------------------------------------------------------
void CTFPlayer::AmmoPackCleanUp( void )
{
	// If we have more than 3 ammo packs out now, destroy the oldest one.
	int iNumPacks = 0;
	CTFAmmoPack *pOldestBox = NULL;

	// Cycle through all ammobox in the world and remove them
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_ammo_pack" );
	while ( pEnt )
	{
		CBaseEntity *pOwner = pEnt->GetOwnerEntity();
		if (pOwner == this)
		{
			CTFAmmoPack *pThisBox = dynamic_cast<CTFAmmoPack *>( pEnt );
			Assert( pThisBox );
			if ( pThisBox )
			{
				iNumPacks++;

				// Find the oldest one
				if ( pOldestBox == NULL || pOldestBox->GetCreationTime() > pThisBox->GetCreationTime() )
				{
					pOldestBox = pThisBox;
				}
			}
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_ammo_pack" );
	}

	// If they have more than 3 packs active, remove the oldest one
	if ( iNumPacks > 3 && pOldestBox )
	{
		UTIL_Remove( pOldestBox );
	}
}		

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropAmmoPack( void )
{
	// zombies have a special function for this
	if ( m_Shared.IsZombie() )
	{
		DropZombieAmmoHealth();
		return;
	}

	// survivors drop nothing in Infected
	if ( TFGameRules()->IsInfGamemode() )
		return;

	// We want the ammo packs to look like the player's weapon model they were carrying.
	// except if they are melee or building weapons
	CTFWeaponBase *pWeapon = NULL;
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();

	if ( !pActiveWeapon || pActiveWeapon->GetTFWpnData().m_bDontDrop )
	{
		// Don't drop this one, find another one to drop

		int iWeight = -1;

		// find the highest weighted weapon
		for (int i = 0;i < WeaponCount(); i++) 
		{
			CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetTFWpnData().m_bDontDrop )
				continue;

			int iThisWeight = pWpn->GetTFWpnData().iWeight;

			if ( iThisWeight > iWeight )
			{
				iWeight = iThisWeight;
				pWeapon = pWpn;
			}
		}
	}
	else
	{
		pWeapon = pActiveWeapon;
	}

	// If we didn't find one, bail
	if ( !pWeapon )
		return;

	// We need to find bones on the world model, so switch the weapon to it.
	const char *pszWorldModel = "models/items/ammopack_small.mdl";
	PrecacheModel( pszWorldModel );
	pWeapon->SetModel( pszWorldModel );


	// Find the position and angle of the weapons so the "ammo box" matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
		return;

	// Fill the ammo pack with unused player ammo, if out add a minimum amount.
	int iPrimary = max( 5, pWeapon->GetMaxReserveAmmo() );
	int iSecondary = max( 5, pWeapon->GetMaxReserveAmmo() );
	int iMetal = 100;	

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, vecPackAngles, this, pszWorldModel );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		// Remove all of the players ammo.
		RemoveAllAmmo();

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( iPrimary, TF_AMMO_PRIMARY );
		pAmmoPack->GiveAmmo( iSecondary, TF_AMMO_SECONDARY );
		pAmmoPack->GiveAmmo( iMetal, TF_AMMO_METAL );

		Vector vecRight, vecUp;
		AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );

		// Calculate the initial impulse on the weapon.
		Vector vecImpulse( 0.0f, 0.0f, 0.0f );
		vecImpulse += vecUp * random->RandomFloat( -0.25, 0.25 );
		vecImpulse += vecRight * random->RandomFloat( -0.25, 0.25 );
		VectorNormalize( vecImpulse );
		vecImpulse *= random->RandomFloat( tf_weapon_ragdoll_velocity_min.GetFloat(), tf_weapon_ragdoll_velocity_max.GetFloat() );
		vecImpulse += GetAbsVelocity();

		// Cap the impulse.
		float flSpeed = vecImpulse.Length();
		if ( flSpeed > tf_weapon_ragdoll_maxspeed.GetFloat() )
		{
			VectorScale( vecImpulse, tf_weapon_ragdoll_maxspeed.GetFloat() / flSpeed, vecImpulse );
		}

		if ( pAmmoPack->VPhysicsGetObject() )
		{
			// We can probably remove this when the mass on the weapons is correct!
			pAmmoPack->VPhysicsGetObject()->SetMass( 25.0f );
			AngularImpulse angImpulse( 0, random->RandomFloat( 0, 100 ), 0 );
			pAmmoPack->VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
		}

		pAmmoPack->SetInitialVelocity( vecImpulse );

		if ( GetTeamNumber() == TF_TEAM_RED )
			pAmmoPack->m_nSkin = 0;
		else if ( GetTeamNumber() == TF_TEAM_BLUE)
			pAmmoPack->m_nSkin = 1;
		else
			pAmmoPack->m_nSkin = 2;
		
		// Give the ammo pack some health, so that trains can destroy it.
		pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pAmmoPack->m_takedamage = DAMAGE_YES;		
		pAmmoPack->SetHealth( 900 );
		
		pAmmoPack->SetBodygroup( 1, 1 );
	
		// Clean up old ammo packs if they exist in the world
		AmmoPackCleanUp();	
	}	
	pWeapon->SetModel( pWeapon->GetViewModel() );
}


//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
void CC_DropWeapon( void )
{
	CTFPlayer *pPlayer = ToTFPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	if (!of_dropweapons.GetBool())
		return;

	if (pPlayer->DropWeaponCooldownElapsed())
	{
		if (of_randomizer.GetBool())
			return;

		CTFWeaponBase *pWeapon = pPlayer->m_Shared.GetActiveTFWeapon();

		if (!pWeapon || !pWeapon->CanDropManualy() || pPlayer->m_Shared.GetHook())
			return;

		int Clip = pWeapon->m_iClip1;
		int ReserveAmmo = pWeapon->m_iReserveAmmo;

		// akimbo pickups have NOT pewished
		if (pWeapon->GetWeaponID() == TF_WEAPON_PISTOL_AKIMBO)
		{
			CTFWeaponBase *pTFPistol = (CTFWeaponBase *)pPlayer->Weapon_OwnsThisID(TF_WEAPON_PISTOL_MERCENARY);
			pPlayer->DropWeapon(pTFPistol, true, false, (float)Clip / 2.0f, ReserveAmmo);
			pTFPistol = NULL;
			UTIL_Remove(pWeapon);
		}
		else
		{
			pPlayer->DropWeapon(pWeapon, true, false, Clip, ReserveAmmo);
			UTIL_Remove(pWeapon);
		}

		CBaseCombatWeapon *pLastWeapon = pPlayer->GetLastWeapon();

		if (pLastWeapon)
			pPlayer->Weapon_Switch(pLastWeapon);
		else
			pPlayer->SwitchToNextBestWeapon(pWeapon);

		if (of_dropweapons.GetInt() == 2)
			pPlayer->setDropWeaponCooldown();
	}
}
static ConCommand dropweapon("dropweapon", CC_DropWeapon, "Drop your weapon.");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_GiveHealthAmmo( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );

	if ( pPlayer )
	{
		pPlayer->RefillHealthAmmo();
	}
}
static ConCommand givecurrentammo("givehealthammo", CC_GiveHealthAmmo, "Give full supply of health and ammo\n", FCVAR_CHEAT );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( buy, "Buy weapon.\n\tArguments: <item_name>" )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer 
		/*&& (  TFGameRules()->CanBuy() not yet implemented) */
		&& args.ArgC() >= 2 )
	{
		char item_to_give[ 256 ];
		Q_strncpy( item_to_give, args[1], sizeof( item_to_give ) );
		Q_strlower( item_to_give );

		string_t iszItem = AllocPooledString( item_to_give );	// Make a copy of the classname
		
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( item_to_give );			  //Get the weapon info
		Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );											  //Is it valid?
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) ); // Cast to TF Weapon info
		Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn" );		
		if( pWeaponInfo && pWeaponInfo->m_bBuyable )
		{
			int WeaponID = AliasToWeaponID( item_to_give );
			int WeaponCost = pWeaponInfo->m_iCost;
			if( !pPlayer->OwnsWeaponID( WeaponID ) && WeaponCost <= pPlayer->m_iAccount )
			{
				CTFWeaponBase *pGivenWeapon = (CTFWeaponBase *)pPlayer->GiveNamedItem( STRING(iszItem) );  // Create the specified weapon 
				CTFWeaponBase *pOverlappingWeapon = pPlayer->GetWeaponInSlot( pGivenWeapon->GetSlot(), pGivenWeapon->GetPosition() );
				if( pOverlappingWeapon )
				{
					int Clip = pOverlappingWeapon->m_iClip1;
					int ReserveAmmo = pOverlappingWeapon->m_iReserveAmmo;

					pPlayer->DropWeapon(pOverlappingWeapon, true, false, Clip, ReserveAmmo);
					UTIL_Remove( pOverlappingWeapon );
				}
				pPlayer->m_iAccount -= WeaponCost;
				pGivenWeapon->GiveTo( pPlayer );
			}
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( give_money, "Give yourself a certain amount of currency.\n\tArguments: <amount>" )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer 
		&& (gpGlobals->maxClients == 1 || sv_cheats->GetBool()) 
		&& args.ArgC() >= 2 )
	{
		int amount = atof( args[1] );
		
		pPlayer->m_iAccount += amount;

	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	if( iSlot < 0 || iSlotPos < 0 )
		return NULL;

	// Just doing > since its faster than >= :lilacstare:
	if( iSlot > MAX_WEAPON_SLOTS - 1 || iSlotPos > MAX_WEAPON_POSITIONS - 1 )
		return NULL;

	if( m_hWeaponInSlot[iSlot][iSlotPos] )
		return m_hWeaponInSlot[iSlot][iSlotPos];

	return NULL;
}

bool CTFPlayer::SetWeaponInSlot( WeaponHandle hWeapon, int iSlot, int iSlotPos )
{
	Assert( iSlot < MAX_WEAPON_SLOTS && iSlotPos < MAX_WEAPON_POSITIONS && "Slots out of bounds in CTFPlayer::EquipWeaponInSlot" );
	Assert( iSlot > -1 && iSlotPos > -1 && "Slots out of bounds in CTFPlayer::EquipWeaponInSlot" );

	if( iSlot < 0 || iSlot >= MAX_WEAPON_SLOTS )
		return false;

	if( iSlotPos < 0 || iSlotPos >= MAX_WEAPON_POSITIONS )
		return false;

	m_hWeaponInSlot[iSlot][iSlotPos] = hWeapon;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we can pickup the weapon, Used in the 3 slot weapon system in DM
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPickupWeapon( CTFWeaponBase *pCarriedWeapon, CTFWeaponBase *pWeapon )
{
	return ( pCarriedWeapon->GetSlot() == pWeapon->GetSlot()  	//The Weapons Occupy the same slot
	&& pCarriedWeapon != pWeapon ); 							//and they're not the same
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropWeapon( CTFWeaponBase *pActiveWeapon, bool bThrown, bool bDissolve, int Clip, int Reserve )
{
	if( !bThrown && of_disable_drop_weapons.GetBool() )
		return;
	
	if( of_randomizer.GetBool() )
		return;

	// We want the ammo packs to look like the player's weapon model they were carrying.
	// except if they are melee or building weapons
	CTFWeaponBase *pWeapon = NULL;

	if ( !pActiveWeapon ||
		( pActiveWeapon &&
		( pActiveWeapon->GetTFWpnData().m_bDontDrop 
		|| pActiveWeapon->GetWeaponID() == TF_WEAPON_BUILDER 
		|| pActiveWeapon->GetWeaponID() == TF_WEAPON_GRAPPLE 
		|| pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_DESTROY ) ) )
	{
		// Don't drop this one, find another one to drop
		int iWeight = -1;

		// find the highest weighted weapon
		for ( int i = 0; i < WeaponCount(); i++ ) 
		{
			CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetTFWpnData().m_bDontDrop )
				continue;

			int iThisWeight = pWpn->GetTFWpnData().iWeight;

			if ( iThisWeight > iWeight )
			{
				iWeight = iThisWeight;
				pWeapon = pWpn;
			}
		}
	}
	else
	{
		pWeapon = pActiveWeapon;
	}

	// If we didn't find one, bail
	if ( !pWeapon )
		return;

	if( pWeapon->GetTFWpnData().m_szAkimboOfWeapon[0] != '\0' )
	{
		const char *szClassName = pWeapon->GetTFWpnData().m_szAkimboOfWeapon;
		COFSchemaWeaponInfo *pWpnInfo = GetItemSchema()->GetWeapon( pWeapon->GetTFWpnData().m_szAkimboOfWeapon );
		if( pWpnInfo )
		{
			const char *pszClass = pWpnInfo->m_szWeaponClass;
			if( pszClass )
				szClassName = pszClass;
		}

		CTFWeaponBase *pAkimbo = (CTFWeaponBase*)CreateEntityByName( szClassName );
		if( pAkimbo )
		{
			pAkimbo->SetupSchemaItem( pWeapon->GetTFWpnData().m_szAkimboOfWeapon );
			DispatchSpawn(pAkimbo);

			DropWeapon( pAkimbo, bThrown, bDissolve, Clip / 2, Reserve / 2 );
			// Only drop two if we don't own the base weapon ( ie we're in 3 wep and only have the akimbos, not the pistol )
			if( !Weapon_OwnsThisType(pWeapon->GetTFWpnData().m_szAkimboOfWeapon, 0) )
				DropWeapon( pAkimbo, bThrown, bDissolve, Clip / 2, Reserve / 2 );

			UTIL_RemoveImmediate(pAkimbo);
			pWeapon->SetModel(pWeapon->GetViewModel());
			return;
		}
	}

	// We need to find bones on the world model, so switch the weapon to it.
	const char *pszWorldModel = pWeapon->GetWorldModel();
	pWeapon->SetModel( pszWorldModel );
	
	// Find the position and angle of the weapons so the "ammo box" matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
		return;
	
	// Create the ammo pack.
	bool bThrownHurt = bThrown && of_dropweapons.GetInt() == 2;

	QAngle angLaunch = QAngle( 0, 0 ,0 ); // the angle we will launch ourselves from
	Vector vecLaunch;					  // final velocity used to launch the items

	CWeaponSpawner *pDroppedWeapon = dynamic_cast< CWeaponSpawner* >( CBaseEntity::CreateNoSpawn( "dm_weapon_spawner", WorldSpaceCenter(), angLaunch, this ) );

	if( pDroppedWeapon )
	{
		pDroppedWeapon->SetWeaponName( pWeapon->GetSchemaName() );
		pDroppedWeapon->SetWeaponModel( pszWorldModel );

		DroppedWeaponInfo_t *pNewInfo = new DroppedWeaponInfo_t();
		pNewInfo->m_hAttributes.SetSize( pWeapon->m_Attributes.Count() );
		FOR_EACH_VEC( pWeapon->m_Attributes, i )
			pNewInfo->m_hAttributes[i] = pWeapon->m_Attributes.Element(i);

		pNewInfo->m_iReserveAmmo = Reserve;
		pNewInfo->m_iClip = Clip;

		pNewInfo->m_bThrown = bThrown;

		pDroppedWeapon->SetupDroppedWeapon( pNewInfo );

		pDroppedWeapon->Activate();
		pDroppedWeapon->Spawn();
		// Calculate the initial impulse on the weapon.
		AngularImpulse angImp;
		Vector vecImpulse( 0.0f, 0.0f, 0.0f );
		
		if( bThrown )
		{
			angImp = AngularImpulse(200, 200, 200);

			Vector vecMuzzlePos = Weapon_ShootPosition();
			Vector vecEndPos;
			EyeVectors(&vecEndPos);
			vecEndPos = vecMuzzlePos + vecEndPos * 256.f;

			trace_t tr;
			UTIL_TraceLine(vecMuzzlePos, vecEndPos, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

			vecImpulse = tr.endpos - pDroppedWeapon->GetAbsOrigin();
			VectorNormalize(vecImpulse);
			Vector pVelocity = GetAbsVelocity();
			pVelocity = Vector(pVelocity.x, pVelocity.y, 60.f);

			//Throw like you mean it
			if( bThrownHurt )
			{
				vecImpulse = vecImpulse * 1200.f + pVelocity;
				vecImpulse.z += 60.f;
			}
			else
			{
				vecImpulse = vecImpulse * 300.f + pVelocity;
				vecImpulse.z += 150.f;
			}
		}
		else
		{
			angImp = AngularImpulse(0, random->RandomFloat(0, 100), 0);

			Vector vecRight, vecUp;
			AngleVectors(EyeAngles(), NULL, &vecRight, &vecUp);

			vecImpulse += vecUp * random->RandomFloat(-0.25, 0.25);
			vecImpulse += vecRight * random->RandomFloat(-0.25, 0.25);
			VectorNormalize(vecImpulse);
			vecImpulse *= random->RandomFloat(tf_weapon_ragdoll_velocity_min.GetFloat(), tf_weapon_ragdoll_velocity_max.GetFloat());
			vecImpulse += GetAbsVelocity();

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if( flSpeed > tf_weapon_ragdoll_maxspeed.GetFloat() )
				VectorScale(vecImpulse, tf_weapon_ragdoll_maxspeed.GetFloat() / flSpeed, vecImpulse);
		}

		pDroppedWeapon->DropSingleInstance( vecImpulse, this, pDroppedWeapon->IsSuperWeapon() ? 60.0f : 30.0f, 1.0f, true );

		//if VPhysics Object exists apply velocity on it, otherwise apply it to the entity
		if (pDroppedWeapon->VPhysicsGetObject())
		{
			// We can probably remove this when the mass on the weapons is correct!
			pDroppedWeapon->VPhysicsGetObject()->SetMass(25.f);
			pDroppedWeapon->VPhysicsGetObject()->SetVelocityInstantaneous(&vecImpulse, &angImp);
		}
		else
		{
			pDroppedWeapon->SetAbsVelocity(vecImpulse);
		}

		if ( GetTeamNumber() == TF_TEAM_RED )
			pDroppedWeapon->m_nSkin = 0;
		else if ( GetTeamNumber() == TF_TEAM_BLUE)
			pDroppedWeapon->m_nSkin = 1;
		else
			pDroppedWeapon->m_nSkin = 2;
		
		//if( pWeapon->GetTeamNumber() > LAST_SHARED_TEAM )
		//	pDroppedWeapon->SetTeamNum( pWeapon->GetTeamNumber() );
		//else
		//	pDroppedWeapon->SetTeamNum( TEAM_UNASSIGNED );
		
		// Give the ammo pack some health, so that trains can destroy it.
		pDroppedWeapon->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pDroppedWeapon->m_takedamage = DAMAGE_YES;		
		pDroppedWeapon->SetHealth( 900 );
		//pDroppedWeapon->m_iReserveAmmo = Reserve;
		//pDroppedWeapon->m_iClip = Clip;
		pDroppedWeapon->SetBodygroup( 1, 1 );

		// Failsafe in case the active weapon is a super weapon that's JUST been drained from all ammo
		bDissolve = bDissolve || (pDroppedWeapon->IsSuperWeapon() && (pDroppedWeapon->GetInfo()->m_iReserveAmmo <= 0
                        && pDroppedWeapon->GetInfo()->m_iClip <= 0));

		if ( bDissolve )
		{
			pDroppedWeapon->SetTouch( NULL );
			pDroppedWeapon->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}		
	}	
	pWeapon->SetModel( pWeapon->GetViewModel() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::setDropWeaponCooldown()
{
	DropTimer.Start(1.25f);
}
bool CTFPlayer::DropWeaponCooldownElapsed()
{
	return DropTimer.IsElapsed();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayerDeathThink( void )
{
	//overridden, do nothing
}

//-----------------------------------------------------------------------------
// Purpose: Remove the tf items from the player then call into the base class
//          removal of items.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems( bool removeSuit )
{
	// If the player has a capture flag, drop it.
	if ( HasItem() )
	{
		GetItem()->Drop( this, true );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
			event->SetInt( "priority", 8 );
			gameeventmanager->FireEvent( event );
		}
	}

	if ( m_hOffHandWeapon.Get() )
	{ 
		HolsterOffHandWeapon();

		// hide the weapon model
		// don't normally have to do this, unless we have a holster animation
		CBaseViewModel *vm = GetViewModel( 1 );
		if ( vm )
		{
			vm->SetWeaponModel( NULL, NULL );
		}

		m_hOffHandWeapon = NULL;
	}

	Weapon_SetLast( NULL );
	UpdateClientData();
}

void CTFPlayer::ClientHearVox( const char *pSentence )
{
	//TFTODO: implement this.
}

bool CTFPlayer::IsRetroModeOn( void )
{
	if  ( 
		( TFGameRules()->IsRetroMode( RETROMODE_BLUE_ONLY ) && GetTeamNumber() == TF_TEAM_BLUE) ||
		( TFGameRules()->IsRetroMode( RETROMODE_RED_ONLY ) && GetTeamNumber() == TF_TEAM_RED) ||
		( TFGameRules()->IsRetroMode( RETROMODE_ON ) ) 
		)
	{
		m_bRetroMode = true;
		return true;
	}

	m_bRetroMode = false;
	return false;
}

bool CTFPlayer::IsRobot()
{
	return m_bIsRobot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateModel( void )
{
	SetModel( GetPlayerClass()->GetModelName() );
	m_PlayerAnimState->OnNewModel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateArmModel( void )
{
    auto viewmodel = GetViewModel(TF_VIEWMODEL_ARMS);
    Assert(viewmodel);
    if (!viewmodel)
    {
        Warning("Couldn't get viewmodel for arm model!\n");
        return;
    }
    auto armModel = GetPlayerClass()->GetArmModelName();
    Assert(armModel);
    if (!armModel)
    {
        Warning("Couldn't get arm model name!\n");
        return;
    }
    viewmodel->SetModel(armModel);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSkin - 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateSkin( int iTeam )
{
	// The player's skin is team - 2.
	int iSkin = iTeam - 2;

	// Check to see if the skin actually changed.
	if ( iSkin != m_iLastSkin )
	{
		m_nSkin = iSkin;
		m_iLastSkin = iSkin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Utility function for live-changing classes (without respawning player)
//-----------------------------------------------------------------------------
void CTFPlayer::UpdatePlayerClass( int iPlayerClass, bool bRefreshWeapons )
{
	if ( iPlayerClass != TF_CLASS_UNDEFINED )
	{
		TeamFortress_RemoveEverythingFromWorld();

		GetPlayerClass()->SetClass( iPlayerClass );

		GetPlayerClass()->SetModifier( TFGameRules()->GetPlayerClassMod( this ) );
		
		m_Shared.FadeInvis( 0.1 );

		RemoveTeleportEffect();

		SetHealth( GetPlayerClass()->GetMaxHealth() );
	}
	
	if ( bRefreshWeapons )
	{
		StripWeapons();
		GiveDefaultItems();
	}

	UpdateModel();
	UpdateArmModel();
	UpdateSkin( GetTeamNumber() );
	UpdateCosmetics();
}

void CTFPlayer::BecomeJuggernaught()
{
	m_bIsJuggernaught = true;

	m_Shared.OnAddJauggernaught();
}

//=========================================================================
// Displays the state of the items specified by the Goal passed in
void CTFPlayer::DisplayLocalItemStatus( CTFGoal *pGoal )
{
#if 0
	for (int i = 0; i < 4; i++)
	{
		if (pGoal->display_item_status[i] != 0)
		{
			CTFGoalItem *pItem = Finditem(pGoal->display_item_status[i]);
			if (pItem)
				DisplayItemStatus(pGoal, this, pItem);
			else
				ClientPrint( this, HUD_PRINTTALK, "#Item_missing" );
		}
	}
#endif
}

//=========================================================================
// Called when the player connects to the server.
//=========================================================================
void CTFPlayer::TeamFortress_ClientConnected( void )
{
//	LoadoutManager()->RegisterPlayer( this );
}

//=========================================================================
// Called when the player disconnects from the server.
//=========================================================================
void CTFPlayer::TeamFortress_ClientDisconnected( void )
{
	// this is awful!!!!!!!!!
	CParticleSystem *pParticle = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( pParticle != NULL )
	{
		pParticle->KeyValue( "start_active", "1" );
		pParticle->KeyValue( "effect_name", "blood_trail_red_01_boom" );
		pParticle->SetLocalOrigin( GetAbsOrigin() );
		DispatchSpawn( pParticle );
		pParticle->Activate();

		pParticle->SetThink( &CBaseEntity::SUB_Remove );
		pParticle->SetNextThink( gpGlobals->curtime + 1.0f );
	}
	CParticleSystem *pParticle2 = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( pParticle2 != NULL )
	{
		pParticle2->KeyValue( "start_active", "1" );
		pParticle2->KeyValue( "effect_name", "tfc_sniper_mist" );
		pParticle2->SetLocalOrigin( GetAbsOrigin() );
		DispatchSpawn( pParticle2 );
		pParticle2->Activate();

		pParticle2->SetThink( &CBaseEntity::SUB_Remove );
		pParticle2->SetNextThink( gpGlobals->curtime + 1.0f );
	}

	CTakeDamageInfo info( this, this, 0, DMG_CRITICAL );
	DeathSound( info );

	EmitSound( "Player.Gib" );

	RemoveNemesisRelationships();

	//if it's duel mode remove this player from the queue
	if( TFGameRules()->IsDuelGamemode() )
	{
		//if player leaves in the mid of a duel (ragequit) it resigns,
		//player who did not leave wins and match ends
		if( TFGameRules()->IsRageQuitter(this) )
			TFGameRules()->DuelRageQuit(this);
		else
			TFGameRules()->RemoveFromDuelQueue(this);

		// Why would you return here?? - Kay
		// TODO: Test with this commented
		// If it break, we need to unregister the player from the loadout system here as well
//		return;
	}

	// Drop a pack with their leftover ammo if we haven't already
	if( IsAlive() )
		DropAmmoPack();

	//Drop the weapon if we haven't already
	CTFWeaponBase *pTFWeapon = m_Shared.GetActiveTFWeapon();
	if( pTFWeapon && IsAlive() )
	{
		int Clip = -1;
		int Reserve = -1;
		if (!of_fullammo.GetBool() || pTFWeapon->GetTFWpnData().m_bAlwaysDrop)
		{
			Clip = pTFWeapon->m_iClip1;
			Reserve = pTFWeapon->m_iReserveAmmo;
		}

		// akimbo pickups have NOT pewished
		if( pTFWeapon->GetWeaponID() == TF_WEAPON_PISTOL_AKIMBO )
		{
			CTFWeaponBase *pTFPistol = (CTFWeaponBase *)Weapon_OwnsThisID(TF_WEAPON_PISTOL_MERCENARY);
			DropWeapon(pTFPistol, false, false, Clip, Reserve);
			DropWeapon(pTFPistol, false, false, Clip, Reserve);
			pTFPistol = NULL;
		}
		else
		{
			DropWeapon(pTFWeapon, false, false, Clip, Reserve);
		}
	}

	TeamFortress_RemoveEverythingFromWorld();
	RemoveAllWeapons();

	LoadoutManager()->RemovePlayer( this );
}

//=========================================================================
// Removes everything this player has (buildings, grenades, etc.) from the world
// AKA RemoveAllOwnedEntitiesFromWorld
//=========================================================================
SERVERONLY_DLL_EXPORT void CTFPlayer::TeamFortress_RemoveEverythingFromWorld( void )
{
	TeamFortress_RemoveProjectiles();

	// Destroy any buildables - this should replace TeamFortress_RemoveBuildings
	RemoveAllObjects();
}

//=========================================================================
// Removes all rockets the player has fired into the world
// (this prevents a team kill cheat where players would fire rockets 
// then change teams to kill their own team)
//=========================================================================
void CTFPlayer::TeamFortress_RemoveProjectiles( void )
{
	// ficool2: there must be a better way to do this... wtf
	
	// TODO: We could potentially go through every entity
	// check if its owned by this player
	// then add a base entity check that by default, returns false
	// then every entity we want to remove from the player, we simply set that to auto give us true
	// This would auto remove every projectile thats based on an already existing projectile
	// and would also avoid having to go through every entity multiple times like the below functions do
	// Downside would be that it still requires some manual setup
	// but definitely more intuitive and less repetitive than this
	// Also, we can't just remove EVERY owned entity, since that COULD be something like the flag
	// Which would be game breaking if removed - Kay

	RemoveOwnedEnt( "tf_projectile_syringe" );
	RemoveOwnedEnt( "tf_projectile_nail" );
	RemoveOwnedEnt( "tf_projectile_tranq" );
	RemoveOwnedEnt( "tf_projectile_pipe_remote" );
	RemoveOwnedEnt( "tf_projectile_pipe" );
	RemoveOwnedEnt( "tf_projectile_pipe_dm" );
	RemoveOwnedEnt( "tf_projectile_rocket" );
	RemoveOwnedEnt( "tf_projectile_bfg" );
}


//=========================================================================
// Remove all of an ent owned by this player
//=========================================================================
void CTFPlayer::RemoveOwnedEnt( const char *pEntName, bool bGrenade )
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, pEntName );
	while ( pEnt )
	{
		// if the player owns this entity, remove it
		bool bOwner = (pEnt->GetOwnerEntity() == this);

		if ( !bOwner && bGrenade )
		{
			CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade*>(pEnt);
			Assert( pGrenade );
			if ( pGrenade )
			{
				bOwner = (pGrenade->GetThrower() == this);
			}
		}

		if ( bOwner )
		{
			pEnt->SetThink( &BaseClass::SUB_Remove );
			pEnt->SetNextThink( gpGlobals->curtime );
			pEnt->SetTouch( NULL );
			pEnt->AddEffects( EF_NODRAW );
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, pEntName );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::NoteWeaponFired()
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

//=============================================================================
//
// Player state functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPlayerStateInfo *CTFPlayer::StateLookupInfo( int nState )
{
	// This table MUST match the 
	static CPlayerStateInfo playerStateInfos[] =
	{
		{ TF_STATE_ACTIVE,				"TF_STATE_ACTIVE",				&CTFPlayer::StateEnterACTIVE,				NULL,	NULL },
		{ TF_STATE_WELCOME,				"TF_STATE_WELCOME",				&CTFPlayer::StateEnterWELCOME,				NULL,	&CTFPlayer::StateThinkWELCOME },
		{ TF_STATE_OBSERVER,			"TF_STATE_OBSERVER",			&CTFPlayer::StateEnterOBSERVER,				NULL,	&CTFPlayer::StateThinkOBSERVER },
		{ TF_STATE_DYING,				"TF_STATE_DYING",				&CTFPlayer::StateEnterDYING,				NULL,	&CTFPlayer::StateThinkDYING },
	};

	for ( int iState = 0; iState < ARRAYSIZE( playerStateInfos ); ++iState )
	{
		if ( playerStateInfos[iState].m_nPlayerState == nState )
			return &playerStateInfos[iState];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnter( int nState )
{
	m_Shared.m_nPlayerState = nState;
	m_pStateInfo = StateLookupInfo( nState );

	if ( tf_playerstatetransitions.GetInt() == -1 || tf_playerstatetransitions.GetInt() == entindex() )
	{
		if ( m_pStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", nState );
	}

	// Initialize the new state.
	if ( m_pStateInfo && m_pStateInfo->pfnEnterState )
	{
		(this->*m_pStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateLeave( void )
{
	if ( m_pStateInfo && m_pStateInfo->pfnLeaveState )
	{
		(this->*m_pStateInfo->pfnLeaveState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateTransition( int nState )
{
	StateLeave();
	StateEnter( nState );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterWELCOME( void )
{
	PickWelcomeObserverPoint();  
	
	StartObserverMode( OBS_MODE_FIXED );

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );		

	PhysObjectSleep();

	if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR, false );
	}
	else if ( (TFGameRules() && TFGameRules()->IsLoadingBugBaitReport()) )
	{
		m_bSeenRoundInfo = true;
		
		ChangeTeam( TF_TEAM_BLUE, false );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
	else if ( IsInCommentaryMode() )
	{
		m_bSeenRoundInfo = true;
	}
	else
	{
		if ( !IsX360() )
		{
			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", "#TF_Welcome" );	// info panel title
			data->SetString( "type", "1" );				// show userdata from stringtable entry
			data->SetString( "msg",	"motd" );			// use this stringtable entry
			data->SetString( "cmd", "mapinfo" );		// exec this command if panel closed

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		else
		{
			ShowViewPortPanel( PANEL_MAPINFO, true );
		}

		m_bSeenRoundInfo = false;
	}

	m_bIsIdle = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkWELCOME( void )
{
	if ( IsInCommentaryMode() && !IsFakeClient() )
	{
		ChangeTeam( TF_TEAM_BLUE, false );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveEffects( EF_NODRAW | EF_NOSHADOW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	m_flLastAction = gpGlobals->curtime;
	m_bIsIdle = false;

	// If we're a Medic, start thinking to regen myself
	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
		SetContextThink( &CTFPlayer::MedicRegenThink, gpGlobals->curtime + TF_MEDIC_REGEN_TIME, "MedicRegenThink" );

	if ( TFGameRules() && TFGameRules()->IsInfGamemode() )
		SetContextThink( &CTFPlayer::ZombieRegenThink, gpGlobals->curtime + 1, "ZombieRegenThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverMode(int mode)
{
	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;

	// Skip OBS_MODE_POI as we're not using that.
	if ( mode == OBS_MODE_POI )
	{
		mode++;
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if( GetTeamNumber() > TEAM_SPECTATOR )
	{
		if ( IsDead() && ( mode > OBS_MODE_FIXED ) && mp_fadetoblack.GetBool() )
		{
			mode = OBS_MODE_CHASE;
		}
		else if ( mode == OBS_MODE_ROAMING )
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	m_flLastAction = gpGlobals->curtime;

	switch ( mode )
	{
	case OBS_MODE_NONE:
	case OBS_MODE_FIXED :
	case OBS_MODE_DEATHCAM :
		SetFOV( this, 0 );	// Reset FOV
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_NONE );
		break;

	case OBS_MODE_CHASE :
	case OBS_MODE_IN_EYE :	
		// update FOV and viewmodels
		SetObserverTarget( m_hObserverTarget );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_ROAMING :
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
		
	case OBS_MODE_FREEZECAM:
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
	}

	CheckObserverSettings();

	return true;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterOBSERVER( void )
{
	// Always start a spectator session in chase mode
	m_iObserverLastMode = OBS_MODE_CHASE;

	if( m_hObserverTarget == NULL )
	{
		// find a new observer target
		CheckObserverSettings();
	}

	if ( !m_bAbortFreezeCam )
	{
		FindInitialObserverTarget();
	}

	StartObserverMode( m_iObserverLastMode );

	PhysObjectSleep();

	m_bIsIdle = false;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		HandleFadeToBlack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkOBSERVER()
{
	// Make sure nobody has changed any of our state.
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterDYING( void )
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlayedFreezeCamSound = false;
	m_bAbortFreezeCam = false;
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to observer mode once the dying process is over
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkDYING( void )
{
	// If we have a ragdoll, it's time to go to deathcam
	if ( !m_bAbortFreezeCam && m_hRagdoll && 
		(m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD) && 
		GetObserverMode() != OBS_MODE_FREEZECAM )
	{
		if ( GetObserverMode() != OBS_MODE_DEATHCAM )
		{
			StartObserverMode( OBS_MODE_DEATHCAM );	// go to observer mode
		}
		RemoveEffects( EF_NODRAW | EF_NOSHADOW );	// still draw player body
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = (m_flDeathTime + TF_DEATH_ANIMATION_TIME + flTimeInFreeze );
	if ( !m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this )
	{
		// Start the sound so that it ends at the freezecam lock on time
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if ( gpGlobals->curtime >= flFreezeSoundTime )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.FreezeCam";
			EmitSound( filter, entindex(), params );

			m_bPlayedFreezeCamSound = true;
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) )	// allow x seconds death animation / death cam
	{
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			if ( !m_bAbortFreezeCam && gpGlobals->curtime < flFreezeEnd )
			{
				if ( GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					StartObserverMode( OBS_MODE_FREEZECAM );
					PhysObjectSleep();
				}
				return;
			}
		}

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack)
			if ( m_bAbortFreezeCam && !mp_fadetoblack.GetBool() )
			{
				if ( m_hObserverTarget == NULL )
				{
					// find a new observer target
					CheckObserverSettings();
				}

				FindInitialObserverTarget();
				SetObserverMode( OBS_MODE_CHASE );
				ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(OBS_MODE_CHASE) );
			}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if( gpGlobals->curtime < flFreezeEnd && ( !TFGameRules()->IsDMGamemode() || 
		(
			   !( m_nButtons & IN_USE		) 
			&& !( m_nButtons & IN_FORWARD	)
			&& !( m_nButtons & IN_BACK		)
			&& !( m_nButtons & IN_RIGHT		)
			&& !( m_nButtons & IN_MOVELEFT	)
			&& !( m_nButtons & IN_MOVERIGHT	)
			&& !( m_nButtons & IN_FORWARD	)
		)
		) )
			return;

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		AddEffects( EF_NOINTERP );

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );

		StateTransition( TF_STATE_OBSERVER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AttemptToExitFreezeCam( void )
{
	float flFreezeTravelTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() + 0.5;
	if ( gpGlobals->curtime < flFreezeTravelTime )
		return;

	m_bAbortFreezeCam = true;
}

class CIntroViewpoint : public CPointEntity
{
	DECLARE_CLASS( CIntroViewpoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	int			m_iIntroStep;
	float		m_flStepDelay;
	string_t	m_iszMessage;
	string_t	m_iszGameEvent;
	float		m_flEventDelay;
	int			m_iGameEventData;
	float		m_flFOV;
};

BEGIN_DATADESC( CIntroViewpoint )
	DEFINE_KEYFIELD( m_iIntroStep,	FIELD_INTEGER,	"step_number" ),
	DEFINE_KEYFIELD( m_flStepDelay,	FIELD_FLOAT,	"time_delay" ),
	DEFINE_KEYFIELD( m_iszMessage,	FIELD_STRING,	"hint_message" ),
	DEFINE_KEYFIELD( m_iszGameEvent,	FIELD_STRING,	"event_to_fire" ),
	DEFINE_KEYFIELD( m_flEventDelay,	FIELD_FLOAT,	"event_delay" ),
	DEFINE_KEYFIELD( m_iGameEventData,	FIELD_INTEGER,	"event_data_int" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_intro_viewpoint, CIntroViewpoint );

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
	if ( iCount <= 0 )
	{
        return 0;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, iAmmoIndex ) )
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
	{
		return 0;
	}

	int iMax = m_PlayerClass.GetData()->m_aAmmoMax[iAmmoIndex];
	int iAdd = min( iCount, iMax - GetAmmoCount(iAmmoIndex) );
	if ( iAdd < 1 )
	{
		return 0;
	}

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	CBaseCombatCharacter::GiveAmmo( iAdd, iAmmoIndex );
	return iAdd;
}


//-----------------------------------------------------------------------------
// Purpose: Reset player's information and force him to spawn
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRespawn()
{
	CTF_GameStats.Event_PlayerForceRespawn( this );

	m_flSpawnTime = gpGlobals->curtime;

	int iDesiredClass = GetDesiredPlayerClassIndex();

	if ( iDesiredClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	if( iDesiredClass == TF_CLASS_RANDOM 
		// TODO: Add a special flag to civ players in escort to check for that instead
		// This is kinda stinky - Kay
		|| (!IsClassAllowed( iDesiredClass ) && !TFGameRules()->IsESCGamemode()) )
	{
		iDesiredClass = GetRandomClass();
	}

	if ( HasTheFlag() )
	{
		DropFlag();
	}

	if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
	{
		// clean up any pipebombs/buildings in the world (no explosions)
		TeamFortress_RemoveEverythingFromWorld();

		GetPlayerClass()->Init( iDesiredClass, TFGameRules()->GetPlayerClassMod( this ) );

		CTF_GameStats.Event_PlayerChangedClass( this );
	}

	m_Shared.RemoveAllCond( NULL );

	RemoveAllItems( true );

	// prevent a team switch exploit here
	TeamFortress_RemoveProjectiles();

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// TODO: move this into conditions
	RemoveTeleportEffect();

	// remove invisibility very quickly	
	m_Shared.FadeInvis( 0.1 );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	StateTransition( TF_STATE_ACTIVE );
	
	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CTFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTFPlayer::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 101:
		{
			if( sv_cheats->GetBool() )
			{
				extern int gEvilImpulse101;
				gEvilImpulse101 = true;

				GiveAllItems();

				gEvilImpulse101 = false;
			}
		}
		break;

	default:
		{
			BaseClass::CheatImpulseCommands( iImpulse );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetWeaponBuilder( CTFWeaponBuilder *pBuilder )
{
	m_hWeaponBuilder = pBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder *CTFPlayer::GetWeaponBuilder( void )
{
	Assert( 0 );
	return m_hWeaponBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this player is building something
//-----------------------------------------------------------------------------
bool CTFPlayer::IsBuilding( void )
{
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
		return pBuilder->IsBuilding();
		*/

	return false;
}

void CTFPlayer::RemoveBuildResources( int iAmount )
{
	if ( of_infiniteammo.GetBool() != 1 )
		RemoveAmmo( iAmount, TF_AMMO_METAL );
}

void CTFPlayer::AddBuildResources( int iAmount )
{
	GiveAmmo( iAmount, TF_AMMO_METAL );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CTFPlayer::GetObject( int index )
{
	return (CBaseObject *)( m_aObjects[index].Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all the player's objects
//			If bForceAll is set, remove all of them immediately.
//			Otherwise, make them all deteriorate over time.
//			If iClass is passed in, don't remove any objects that can be built 
//			by that class. If bReturnResources is set, the cost of any destroyed 
//			objects will be returned to the player.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllObjects( void )
{
	// Remove all the player's objects
	for ( int i = GetObjectCount()-1; i >= 0; i-- )
	{
		CBaseObject *obj = GetObject( i );
		Assert( obj );

		if ( obj )
		{
			obj->DetonateObject();

			if ( obj )
				UTIL_Remove( obj );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Player has started building an object
//-----------------------------------------------------------------------------
int	CTFPlayer::StartedBuildingObject( int iObjectType )
{
	// Deduct the cost of the object
	int iCost = CalculateObjectCost( iObjectType );
	if ( iCost > GetBuildResources() )
	{
		// Player must have lost resources since he started placing
		return 0;
	}

	RemoveBuildResources( iCost );

	// If the object costs 0, we need to return non-0 to mean success
	if ( !iCost )
		return 1;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Player has aborted building something
//-----------------------------------------------------------------------------
void CTFPlayer::StoppedBuilding( int iObjectType )
{
	/*
	int iCost = CalculateObjectCost( iObjectType );

	AddBuildResources( iCost );

	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->StoppedBuilding( iObjectType );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Object has been built by this player
//-----------------------------------------------------------------------------
void CTFPlayer::FinishedObject( CBaseObject *pObject )
{
	AddObject( pObject );
	CTF_GameStats.Event_PlayerCreatedBuilding( this, pObject );

	/*
	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->FinishedObject();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this player's object list.
//-----------------------------------------------------------------------------
void CTFPlayer::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::AddObject adding object %p:%s to player %s\n", gpGlobals->curtime, pObject, pObject->GetClassname(), GetPlayerName() ) );

	// Make a handle out of it
	CHandle<CBaseObject> hObject;
	hObject = pObject;

	bool alreadyInList = PlayerOwnsObject( pObject );
	Assert( !alreadyInList );
	if ( !alreadyInList )
	{
		m_aObjects.AddToTail( hObject );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Object built by this player has been destroyed
//-----------------------------------------------------------------------------
void CTFPlayer::OwnedObjectDestroyed( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::OwnedObjectDestroyed player %s object %p:%s\n", gpGlobals->curtime, 
		GetPlayerName(),
		pObject,
		pObject->GetClassname() ) );

	RemoveObject( pObject );

	// Tell our builder weapon so it recalculates the state of the build icons
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->RecalcState();
	}
	*/
}


//-----------------------------------------------------------------------------
// Removes an object from the player
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::RemoveObject %p:%s from player %s\n", gpGlobals->curtime, 
		pObject,
		pObject->GetClassname(),
		GetPlayerName() ) );

	Assert( pObject );

	int i;
	for ( i = m_aObjects.Count(); --i >= 0; )
	{
		// Also, while we're at it, remove all other bogus ones too...
		if ( (!m_aObjects[i].Get()) || (m_aObjects[i] == pObject))
		{
			m_aObjects.FastRemove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// See if the player owns this object
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerOwnsObject( CBaseObject *pObject )
{
	return ( m_aObjects.Find( pObject ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayFlinch( const CTakeDamageInfo &info )
{
	// Don't play flinches if we just died. 
	if ( !IsAlive() )
		return;

	// No pain flinches while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	// No pain sounds with teamplay knockback enabled but no friendlyfire
	if ( of_teamplay_knockback.GetBool() && !friendlyfire.GetBool() )
	{
		CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );

		if ( pAttacker )
		{
			if ( info.GetAttacker()->IsPlayer() && info.GetAttacker()->GetTeamNumber() != TF_TEAM_MERCENARY && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
			{
				return;
			}
		}
	}

	PlayerAnimEvent_t flinchEvent;

	switch ( LastHitGroup() )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchEvent = PLAYERANIMEVENT_FLINCH_HEAD;
		break;
	case HITGROUP_LEFTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchEvent = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	}

	DoAnimationEvent( flinchEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the crit sound that players that get crit hear
//-----------------------------------------------------------------------------
float CTFPlayer::PlayCritReceivedSound( void )
{
	float flCritPainLength = 0;
	// Play a custom pain sound to the guy taking the damage
	CSingleUserRecipientFilter receiverfilter( this );
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "TFPlayer.CritPain";
	params.m_pflSoundDuration = &flCritPainLength;
	EmitSound( receiverfilter, entindex(), params );

	return flCritPainLength;
}
//-----------------------------------------------------------------------------
// Purpose: Plays the Leg Shot sound that players that get shot in the leg hear
//-----------------------------------------------------------------------------
float CTFPlayer::PlayLegShotReceivedSound(void)
{
	float flLegShotPainLength = 0;
	// Play a custom pain sound to the guy taking the damage
	CSingleUserRecipientFilter receiverfilter(this);
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "TFPlayer.LegShot";
	params.m_pflSoundDuration = &flLegShotPainLength;
	EmitSound(receiverfilter, entindex(), params);

	return flLegShotPainLength;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PainSound( const CTakeDamageInfo &info )
{
	// Don't make sounds if we just died. DeathSound will handle that.
	if ( !IsAlive() )
		return;

	// No pain sounds with teamplay knockback enabled but no friendlyfire
	if ( of_teamplay_knockback.GetBool() && !friendlyfire.GetBool() )
	{
		CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );

		if ( pAttacker )
		{
			if ( info.GetAttacker()->IsPlayer() && info.GetAttacker()->GetTeamNumber() != TF_TEAM_MERCENARY && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
			{
				return;
			}
		}
	}

	// no pain sounds while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	if ( m_flNextPainSoundTime > gpGlobals->curtime )
		return;

	if ( info.GetDamageType() & DMG_DROWN )
	{
		EmitSound( "TFPlayer.Drown" );
		return;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		// Looping fire pain sound is done in CTFPlayerShared::ConditionThink
		return;
	}

	float flPainLength = 0;

	bool bAttackerIsPlayer = ( info.GetAttacker() && info.GetAttacker()->IsPlayer() );

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	
	if ( pExpresser )
		pExpresser->AllowMultipleScenes();

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter( GetAbsOrigin() );

	if ( bAttackerIsPlayer && ( info.GetAttacker() && info.GetAttacker() != this ) )
	{
		filter.RemoveRecipient( ToBasePlayer( info.GetAttacker() ) );
	}

	// play a crit sound to the victim ( us )
	if ((info.GetDamageType() & DMG_CRITICAL) || (info.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT))
	{
		flPainLength = PlayCritReceivedSound();

		// remove us from hearing our own pain sound if we hear the crit sound
		filter.RemoveRecipient( this );
		//DevMsg("Play crit sound to person shot.\n");
	}

	if ((info.GetDamageCustom() == TF_DMG_CUSTOM_LEGSHOT) && !((info.GetDamageType() & DMG_CRITICAL) || (info.GetDamageCustom() == TF_DMG_CUSTOM_RAILGUN_HEADSHOT)))
	{
		flPainLength = PlayLegShotReceivedSound();

		//DevMsg("Play LegPieced sound to person shot in the leg. Can not play if shot is also a crit or a railgun headshot\n");
	}
	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &filter ) )
	{
		flPainLength = max( GetSceneDuration( szResponse ), flPainLength );
	}

	// speak a louder pain concept to just the attacker
	if ( bAttackerIsPlayer && ( info.GetAttacker() && info.GetAttacker() != this ) )
	{
		CSingleUserRecipientFilter attackerFilter( ToBasePlayer( info.GetAttacker() ) );
		SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_ATTACKER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &attackerFilter );
	}

	if ( pExpresser )
		pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Don't make death sounds when choosing a class
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
		return;

	TFPlayerClassData_t *pData = GetPlayerClass()->GetData();
	if ( !pData )
		return;
	
	char *pszDeathSound;
	int iSize = 0;

	if ( m_LastDamageType & DMG_FALL ) // Did we die from falling?
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
		return;
	}
	else if ( m_LastDamageType & DMG_BLAST )
	{
		pszDeathSound = pData->m_szExplosionDeathSound;
		iSize = sizeof(pData->m_szExplosionDeathSound);
	}
	else if ((m_LastDamageType & DMG_CRITICAL))
	{
		pszDeathSound = pData->m_szCritDeathSound;
		iSize = sizeof(pData->m_szCritDeathSound);
		
		PlayCritReceivedSound();

		//DevMsg("Crit Sound, Last sound for death.\n");
	}
	else if ((m_LastDamageType == TF_DMG_CUSTOM_RAILGUN_HEADSHOT))
	{
		pszDeathSound = pData->m_szCritDeathSound;
		iSize = sizeof(pData->m_szCritDeathSound);

		PlayCritReceivedSound();
	}
	else if ( m_LastDamageType & DMG_CLUB )
	{
		pszDeathSound = pData->m_szMeleeDeathSound;
		iSize = sizeof(pData->m_szMeleeDeathSound);
	}
	else
	{
		pszDeathSound = pData->m_szDeathSound;
		iSize = sizeof(pData->m_szDeathSound);
	}
	
	AI_CriteriaSet pTempSet;
	ModifyOrAppendCriteria(pTempSet);
	char szMutator[64];
	Q_strncpy( szMutator, pTempSet.GetValue(pTempSet.FindCriterionIndex("playermutator")), sizeof(szMutator));
	strlwr(szMutator);

	char szResult[128];
	if( FStrEq(szMutator, "robot") )
	{
		char szSoundScript[124];
		Q_strncpy(szSoundScript, pszDeathSound, sizeof(szSoundScript));
		int iLen = sizeof(szSoundScript);
		int y = 0;
		for( int i = 0; i < iLen; i++ )
		{
			szResult[y] = szSoundScript[i];
			if( szSoundScript[i] == '.' )
			{
				y++;
				szResult[y] = 'M';
				y++;
				szResult[y] = 'V';
				y++;
				szResult[y] = 'M';
				y++;
				szResult[y] = '_';
			}
			y++;
		}

		pszDeathSound = szResult;
	}
	
	EmitSound( pszDeathSound );
}

//-----------------------------------------------------------------------------
// Purpose: called when this player burns another player
//-----------------------------------------------------------------------------
void CTFPlayer::OnBurnOther( CTFPlayer *pTFPlayerVictim )
{
#define ACHIEVEMENT_BURN_TIME_WINDOW	30.0f
#define ACHIEVEMENT_BURN_VICTIMS	5
	// add current time we burned another player to head of vector
	m_aBurnOtherTimes.AddToHead( gpGlobals->curtime );

	// remove any burn times that are older than the burn window from the list
	float flTimeDiscard = gpGlobals->curtime - ACHIEVEMENT_BURN_TIME_WINDOW;
	for ( int i = 1; i < m_aBurnOtherTimes.Count(); i++ )
	{
		if ( m_aBurnOtherTimes[i] < flTimeDiscard )
		{
			m_aBurnOtherTimes.RemoveMultiple( i, m_aBurnOtherTimes.Count() - i );
			break;
		}
	}

	// see if we've burned enough players in time window to satisfy achievement
	if ( m_aBurnOtherTimes.Count() >= ACHIEVEMENT_BURN_VICTIMS )
	{
		CSingleUserRecipientFilter filter( this );
		UserMessageBegin( filter, "AchievementEvent" );
		WRITE_BYTE( ACHIEVEMENT_TF_BURN_PLAYERSINMINIMIMTIME );
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetTFTeam( void )
{
	CTFTeam *pTeam = dynamic_cast<CTFTeam *>( GetTeam() );
	Assert( pTeam );
	return pTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetOpposingTFTeam( void )
{
	int iTeam = GetTeamNumber();
	if ( iTeam == TF_TEAM_RED )
	{
		return TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	}
	else if ( iTeam == TF_TEAM_BLUE)
	{
		return TFTeamMgr()->GetTeam( TF_TEAM_RED );
	}
	else
	{
		return TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Give this player the "i just teleported" effect for 12 seconds
//-----------------------------------------------------------------------------
void CTFPlayer::TeleportEffect( void )
{
	m_Shared.AddCond( TF_COND_TELEPORTED );

	// Also removed on death
	SetContextThink( &CTFPlayer::RemoveTeleportEffect, gpGlobals->curtime + 12, "TFPlayer_TeleportEffect" );
}

//-----------------------------------------------------------------------------
// Purpose: Remove the teleporter effect
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveTeleportEffect( void )
{
	m_Shared.RemoveCond( TF_COND_TELEPORTED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( void )
{
	CreateRagdollEntity( false, false, false, false, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity to pass to the client.
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( bool bGib, bool bBurning, bool bDissolve, bool bFlagOnGround, int iDamageCustom)
{
	// If we already have a ragdoll destroy it.
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTFRagdoll*>( CreateEntityByName( "tf_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->m_nForceBone = m_nForceBone;
		Assert( entindex() >= 1 && entindex() <= MAX_PLAYERS );
		pRagdoll->m_iPlayerIndex.Set( entindex() );
		pRagdoll->m_bGib = bGib;
		pRagdoll->m_bBurning = bBurning;
		pRagdoll->m_iTeam = GetTeamNumber();
		pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
		pRagdoll->m_iDamageCustom = iDamageCustom;
		pRagdoll->m_iGoreHead = m_iGoreHead;
		pRagdoll->m_iGoreLeftArm = m_iGoreLeftArm;
		pRagdoll->m_iGoreRightArm = m_iGoreRightArm;
		pRagdoll->m_iGoreLeftLeg = m_iGoreLeftLeg;
		pRagdoll->m_iGoreRightLeg = m_iGoreRightLeg;
		pRagdoll->m_bFlagOnGround = bFlagOnGround;
		pRagdoll->m_bDissolve = bDissolve;

		if ( bDissolve )
			pRagdoll->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}

	// Turn off the player.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );
	SetMoveType( MOVETYPE_NONE );

	// Add additional gib setup.
	if ( bGib )
	{
		m_nRenderFX = kRenderFxRagdoll;
	}
	// Save ragdoll handle.
	m_hRagdoll = pRagdoll;
}

// Purpose: Destroy's a ragdoll, called with a player is disconnecting.
//-----------------------------------------------------------------------------
void CTFPlayer::DestroyRagdoll( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_FrameUpdate( void )
{
	BaseClass::Weapon_FrameUpdate();

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		m_hOffHandWeapon->Operator_FrameUpdate( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::Weapon_HandleAnimEvent( pEvent );

	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) 
{

}

//-----------------------------------------------------------------------------
// Purpose: Notify everything around me some combat just happened
// Input  : pWeapon - the weapon that was just fired
//-----------------------------------------------------------------------------
/*
void CTFPlayer::OnMyWeaponFired( CBaseCombatWeapon *pWeapon )
{
	if ( !pWeapon )
		return;

	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	switch ( ( (CTFWeaponBase *)pWeapon )->GetWeaponID() )
	{
		case TF_WEAPON_WRENCH:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_MEDIGUN:
		//case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		//case TF_WEAPON_LUNCHBOX:
		//case TF_WEAPON_LUNCHBOX_DRINK:
		//case TF_WEAPON_BUFF_ITEM:
		//case TF_WEAPON_PDA_SPY_BUILD:
			return;

		default:
		{
			if ( !GetLastKnownArea() )
				return;

			CUtlVector<CTFNavArea *> nearby;
			CollectSurroundingAreas( &nearby, GetLastKnownArea() );

			FOR_EACH_VEC( nearby, i ) 
			{
				nearby[i]->OnCombat();
			}
		}
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: Remove invisibility, called when player attacks
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveInvisibility( void )
{
	if ( !m_Shared.InCond( TF_COND_STEALTHED ) )
		return;

	// remove quickly
	m_Shared.FadeInvis( 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: Remove disguise
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveDisguise( void )
{
	// remove quickly
	if ( m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		m_Shared.RemoveDisguise();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SaveMe( void )
{
	if ( !IsAlive() || IsPlayerClass( TF_CLASS_UNDEFINED ) || GetTeamNumber() < TF_TEAM_RED )
		return;

	m_bSaveMeParity = !m_bSaveMeParity;
}

//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
void CC_DropItem( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer )
	{
		pPlayer->DropFlag();
	}
}
static ConCommand dropitem( "dropitem", CC_DropItem, "Drop the flag." );

class CObserverPoint : public CPointEntity
{
	DECLARE_CLASS( CObserverPoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual void Activate( void )
	{
		BaseClass::Activate();

		if ( m_iszAssociateTeamEntityName != NULL_STRING )
		{
			m_hAssociatedTeamEntity = gEntList.FindEntityByName( NULL, m_iszAssociateTeamEntityName );
			if ( !m_hAssociatedTeamEntity )
			{
				Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName) );
			}
		}
	}

	bool CanUseObserverPoint( CTFPlayer *pPlayer )
	{
		if ( m_bDisabled )
			return false;

		if ( m_hAssociatedTeamEntity && ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM ) )
		{
			// If we don't own the associated team entity, we can't use this point
			if ( m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
				return false;
		}

		// Only spectate observer points on control points in the current miniround
		if ( g_pObjectiveResource->PlayingMiniRounds() && m_hAssociatedTeamEntity )
		{
			CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>(m_hAssociatedTeamEntity.Get());
			if ( pPoint )
			{
				bool bInRound = g_pObjectiveResource->IsInMiniRound( pPoint->GetPointIndex() );
				if ( !bInRound )
					return false;
			}
		}

		return true;
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void InputEnable( inputdata_t &inputdata )
	{
		m_bDisabled = false;
	}
	void InputDisable( inputdata_t &inputdata )
	{
		m_bDisabled = true;
	}
	bool IsDefaultWelcome( void ) { return m_bDefaultWelcome; }

public:
	bool		m_bDisabled;
	bool		m_bDefaultWelcome;
	EHANDLE		m_hAssociatedTeamEntity;
	string_t	m_iszAssociateTeamEntityName;
	float		m_flFOV;
};

BEGIN_DATADESC( CObserverPoint )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome" ),
	DEFINE_KEYFIELD( m_iszAssociateTeamEntityName,	FIELD_STRING,	"associated_team_entity" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_observer_point,CObserverPoint);

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
//			Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CTFPlayer::BuildObservableEntityList( void )
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Add all the map-placed observer points
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		m_hObservableEntities.AddToTail( pObserverPoint );

		if ( m_hObserverTarget.Get() == pObserverPoint )
		{
			iCurrentIndex = (m_hObservableEntities.Count()-1);
		}

		pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}

	// Add all the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			m_hObservableEntities.AddToTail( pPlayer );

			if ( m_hObserverTarget.Get() == pPlayer )
			{
				iCurrentIndex = (m_hObservableEntities.Count()-1);
			}
		}
	}

	// Add all my objects
	int iNumObjects = GetObjectCount();
	for ( int i = 0; i < iNumObjects; i++ )
	{
		CBaseObject *pObj = GetObject(i);
		if ( pObj )
		{
			m_hObservableEntities.AddToTail( pObj );

			if ( m_hObserverTarget.Get() == pObj )
			{
				iCurrentIndex = (m_hObservableEntities.Count()-1);
			}
		}
	}
	
	return iCurrentIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 
	int startIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count()-1;

	startIndex += iDir;
	if (startIndex > iMax)
		startIndex = 0;
	else if (startIndex < 0)
		startIndex = iMax;

	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNextObserverTarget(bool bReverse)
{
	int startIndex = GetNextObserverSearchStartPoint( bReverse );

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 

	int iMax = m_hObservableEntities.Count()-1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if ( startIndex > iMax )
	{
		currentIndex = startIndex = 1;
	}

	do
	{
		CBaseEntity *nextTarget = m_hObservableEntities[currentIndex];

		if ( IsValidObserverTarget( nextTarget ) )
			return nextTarget;	
 
		currentIndex += iDir;

		// Loop through the entities
		if (currentIndex > iMax)
		{
			currentIndex = 0;
		}
		else if (currentIndex < 0)
		{
			currentIndex = iMax;
		}
	} while ( currentIndex != startIndex );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsValidObserverTarget(CBaseEntity * target)
{
	if ( target && !target->IsPlayer() )
	{
		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
		if ( pObsPoint && !pObsPoint->CanUseObserverPoint( this ) )
			return false;

		// so zombies can't cheat
		if ( TFGameRules()->IsInfGamemode() && target->GetTeamNumber() == TF_TEAM_RED )
			return false;
		
		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		switch ( mp_forcecamera.GetInt() )	
		{
		case OBS_ALLOW_ALL		:	break;
		case OBS_ALLOW_TEAM		:	if ( target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber() )
										return false;
									break;
		case OBS_ALLOW_NONE		:	return false;
		}

		return true;
	}

	return BaseClass::IsValidObserverTarget( target );
}


void CTFPlayer::PickWelcomeObserverPoint( void )
{
	//Don't just spawn at the world origin, find a nice spot to look from while we choose our team and class.
	CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );

	while ( pObserverPoint )
	{
		if ( IsValidObserverTarget( pObserverPoint ) )
		{
			SetObserverTarget( pObserverPoint );
		}

		if ( pObserverPoint->IsDefaultWelcome() )
			break;

		pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverTarget(CBaseEntity *target)
{
	ClearZoomOwner();
	SetFOV( this, 0 );
		
	if ( !BaseClass::SetObserverTarget(target) )
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
	if ( pObsPoint )
	{
		SetViewOffset( vec3_origin );
		JumptoPosition( target->GetAbsOrigin(), target->EyeAngles() );
		SetFOV( pObsPoint, pObsPoint->m_flFOV );
	}

	m_flLastAction = gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
//			Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNearestObservableTarget( Vector vecOrigin, float flMaxDist )
{
	CTeam *pTeam = GetTeam();
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = (flMaxDist * flMaxDist);
	int iNumPlayers = pTeam->GetNumPlayers();

	if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
	{
		iNumPlayers = gpGlobals->maxClients;
	}


	for ( int i = 0; i < iNumPlayers; i++ )
	{
		CTFPlayer *pPlayer = NULL;

		if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		}
		else
		{
			pPlayer = ToTFPlayer( pTeam->GetPlayer(i) );
		}

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget(pPlayer) )
			continue;

		float flDistSqr = ( pPlayer->GetAbsOrigin() - vecOrigin ).LengthSqr();

		if ( flDistSqr < flCurDistSqr )
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			if ( !bFoundClass || pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;

				if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
				{
					bFoundClass = true;
				}
			}
		}
		else if ( !bFoundClass )
		{
			if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;
				bFoundClass = true;
			}
		}
	}

	//if ( !bFoundClass && IsPlayerClass( TF_CLASS_ENGINEER ) )
    if ( !bFoundClass )
	{
		// let's spectate our sentry instead, we didn't find any other players to spec
		int iNumObjects = GetObjectCount();
		for ( int i = 0; i < iNumObjects; i++ )
		{
			CBaseObject *pObj = GetObject(i);

			if ( pObj && pObj->GetType() == OBJ_SENTRYGUN )
			{
				pReturnTarget = pObj;
			}
		}
	}		

	return pReturnTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FindInitialObserverTarget( void )
{
	// If we're on a team (i.e. not a pure observer), try and find
	// a target that'll give the player the most useful information.
	if ( GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			// Has our forward cap point been contested recently?
			int iFarthestPoint = TFGameRules()->GetFarthestOwnedControlPoint( GetTeamNumber(), false );
			if ( iFarthestPoint != -1 )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Does it have an associated viewpoint?
					CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
					while ( pObserverPoint )
					{
						CObserverPoint *pObsPoint = assert_cast<CObserverPoint *>(pObserverPoint);
						if ( pObsPoint && pObsPoint->m_hAssociatedTeamEntity == pMaster->GetControlPoint(iFarthestPoint) )
						{
							if ( IsValidObserverTarget( pObsPoint ) )
							{
								m_hObserverTarget.Set( pObsPoint );
								return;
							}
						}

						pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
					}
				}
			}

			// Has the point beyond our farthest been contested lately?
			iFarthestPoint += (ObjectiveResource()->GetBaseControlPointForTeam( GetTeamNumber() ) == 0 ? 1 : -1);
			if ( iFarthestPoint >= 0 && iFarthestPoint < MAX_CONTROL_POINTS )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Try and find a player near that cap point
					CBaseEntity *pCapPoint = pMaster->GetControlPoint(iFarthestPoint);
					if ( pCapPoint )
					{
						CBaseEntity *pTarget = FindNearestObservableTarget( pCapPoint->GetAbsOrigin(), 1500 );
						if ( pTarget )
						{
							m_hObserverTarget.Set( pTarget );
							return;
						}
					}
				}
			}
		}
	}

	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget( GetAbsOrigin(), FLT_MAX );
	if ( pTarget )
	{
		m_hObserverTarget.Set( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ValidateCurrentObserverTarget( void )
{
	// If our current target is a dead player who's gibbed / died, refind as if 
	// we were finding our initial target, so we end up somewhere useful.
	if ( m_hObserverTarget && m_hObserverTarget->IsPlayer() )
	{
		CBasePlayer *player = ToBasePlayer( m_hObserverTarget );

		if ( player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING )
		{
			// Once we're past the pause after death, find a new target
			if ( (player->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				FindInitialObserverTarget();
			}

			return;
		}
	}
	// check added for npcs too
	if ( m_hObserverTarget && ( m_hObserverTarget->IsBaseObject() || m_hObserverTarget->IsNPC() ) )
	{
		if ( m_iObserverMode == OBS_MODE_IN_EYE )
		{
			// ForceObserverMode( OBS_MODE_CHASE );
			m_iObserverMode = OBS_MODE_CHASE;
			SetObserverTarget( m_hObserverTarget );
			SetMoveType( MOVETYPE_OBSERVER );

			CheckObserverSettings();
		}
	}

	BaseClass::ValidateCurrentObserverTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Touch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
		CheckUncoveringSpies( pPlayer );
		SpreadPoison(pPlayer);
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has seen through an enemy spy's disguise
//-----------------------------------------------------------------------------
void CTFPlayer::CheckUncoveringSpies( CTFPlayer *pTouchedPlayer )
{
	// Only uncover enemies
	if ( m_Shared.IsAlly( pTouchedPlayer ) )
	{
		return;
	}

	// Only uncover if they're stealthed
	if ( !pTouchedPlayer->m_Shared.InCondInvis() )
	{
		return;
	}

	// pulse their invisibility
	pTouchedPlayer->m_Shared.OnSpyTouchedByEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has been infected
//-----------------------------------------------------------------------------
void CTFPlayer::SpreadPoison(CTFPlayer *pTouchedPlayer)
{
	// Only spread poison among teammates
	if (of_spread_infection.GetBool())
	{
		if (m_Shared.IsAlly(pTouchedPlayer) && pTouchedPlayer->m_Shared.InCond(TF_COND_POISON))
			m_Shared.Poison(pTouchedPlayer->m_Shared.m_hPoisonAttacker, pTouchedPlayer->m_Shared.GetConditionDuration(TF_COND_POISON));
	}
}

enum
{
	TF_TAUNT_HADUKEN = 0,
	TF_TAUNT_POW,
	TF_TAUNT_FENCING,
	TF_TAUNT_BOND
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Taunt( int iTaunt )
{
	if ( !IsAlive() )
		return;

	if ( m_Shared.InCond( TF_COND_TAUNTING ) || m_Shared.InCond( TF_COND_DISGUISED ) ||
		 m_Shared.InCond( TF_COND_STEALTHED ) || m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		return;

	// Check to see if we are in water (above our waist).
//	if ( GetWaterLevel() > WL_Waist )
//		return;

	// Check to see if we are on the ground.
//	if ( GetGroundEntity() == NULL )
//		return;

	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();
	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if (iTaunt == 0)
	{
		m_bInitTaunt = true;
		if (SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_TAUNT, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME))
		{
			DevMsg("szResponse: %s\n", szResponse);
			// Get the duration of the scene.
			float flDuration = GetSceneDuration(szResponse) + 0.2f;

			// Set player state as taunting.
			m_Shared.AddCond(TF_COND_TAUNTING);
			m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + flDuration;

			m_angTauntCamera = EyeAngles();

			// Slam velocity to zero.
			//		SetAbsVelocity( vec3_origin );

			if (V_stricmp(szResponse, "scenes/player/pyro/low/taunt02.vcd") == 0)
			{
				SetTauntEffect(TF_TAUNT_HADUKEN, 2.0f);
			}
			else if (V_stricmp(szResponse, "scenes/player/heavy/low/taunt03_v1.vcd") == 0 || V_stricmp(szResponse, "scenes/player/heavy/low/taunt03.vcd") == 0 || V_stricmp(szResponse, "scenes/player/heavy/low/taunt03_v2.vcd") == 0)
			{
				SetTauntEffect(TF_TAUNT_POW, 1.8f);
			}
			else if (V_stricmp(szResponse, "scenes/player/mercenary/low/taunt_bond.vcd") == 0)
			{
				SetTauntEffect(TF_TAUNT_BOND, 1.6f);
			}
			else if (V_stricmp(szResponse, "scenes/player/spy/low/taunt03_v1.vcd") == 0 || V_stricmp(szResponse, "scenes/player/spy/low/taunt03_v2.vcd") == 0)
			{
				SetTauntEffect(TF_TAUNT_FENCING, 1.8f);
			}
		}
	}
	else
	{
		m_bInitTaunt = true;

		int iDoThisTaunt;

		switch (iTaunt)
		{
			default:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT;
				break;
			case 1:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_KNUCKLES;
				break;
			case 2:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_SO_CLOSE;
				break;
			case 3:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_GROOVY;
				break;
			case 4:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_DEAD_MEAT;
				break;
			case 5:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_BOND;
				break;
			case 6:
				iDoThisTaunt = MP_CONCEPT_PLAYER_TAUNT_BOMB_SCARE;
				break;
		}
		

		if (SpeakConceptIfAllowed(iDoThisTaunt, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME))
		{
			DevMsg("szResponse: %s\n", szResponse);
			// Get the duration of the scene.
			float flDuration = GetSceneDuration(szResponse) + 0.2f;

			// Set player state as taunting.
			m_Shared.AddCond(TF_COND_TAUNTING);
			m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + flDuration;

			m_angTauntCamera = EyeAngles();

			if (V_stricmp(szResponse, "scenes/player/mercenary/low/taunt_bond.vcd") == 0)
			{
				SetTauntEffect(TF_TAUNT_BOND, 1.6f);
			}
		}
	}

	pExpresser->DisallowMultipleScenes();
}

void CTFPlayer::SetTauntEffect( int nTaunt, float flThinkTime, int nTauntLayer )
{
	m_iTaunt = nTaunt;
 	m_fTauntEffectTick = gpGlobals->curtime + flThinkTime;
	m_iTauntLayer = nTauntLayer;
}

void CTFPlayer::TauntEffectThink()
{
	if ( gpGlobals->curtime < m_fTauntEffectTick || m_iTaunt == -1 )
		return;
	
	switch ( m_iTaunt )
	{
	case TF_TAUNT_POW:	
		{
			// Fire a bullet in the direction player was looking at.
			Vector vecSrc, vecShotDir, vecEnd;
			QAngle angShot = EyeAngles();
			AngleVectors( angShot, &vecShotDir );
			vecSrc = Weapon_ShootPosition();
			vecEnd = vecSrc + vecShotDir * 500;

			trace_t tr;
			UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr );

			if ( tr.fraction < 1.0f )
			{
				CBaseEntity *pEntity = tr.m_pEnt;
				if ( pEntity && (
						( pEntity->IsPlayer() || pEntity->IsNPC() )
						&& (
							!InSameTeam( pEntity )
							|| pEntity->GetTeamNumber() == TF_TEAM_MERCENARY
							|| pEntity->GetTeamNumber() == TEAM_UNASSIGNED 
						)
					)
				){
					Vector vecForce, vecDamagePos;
					QAngle angForce( -45, angShot[YAW], 0 );
					AngleVectors( angForce, &vecForce );
					vecForce *= 25000;

					vecDamagePos = tr.endpos;

					CTakeDamageInfo info( this, this, GetActiveTFWeapon(), vecForce, vecDamagePos, 500, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON );
					pEntity->TakeDamage( info );
				}
			}
			m_iTaunt = -1;
			break;
		}
	case TF_TAUNT_BOND:
	{
		// Fire a bullet in the direction player was looking at.
		Vector vecSrc, vecShotDir, vecEnd;
		QAngle angShot = EyeAngles();
		AngleVectors(angShot, &vecShotDir);
		vecSrc = Weapon_ShootPosition();
		vecEnd = vecSrc + vecShotDir * 500;

		trace_t tr;
		UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID | CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr);

		if (tr.fraction < 1.0f)
		{
			CBaseEntity *pEntity = tr.m_pEnt;
			if (pEntity && (
				(pEntity->IsPlayer() || pEntity->IsNPC())
				&& (
				!InSameTeam(pEntity)
				|| pEntity->GetTeamNumber() == TF_TEAM_MERCENARY
				|| pEntity->GetTeamNumber() == TEAM_UNASSIGNED
				)
				)
				){
				Vector vecForce, vecDamagePos;
				QAngle angForce(-45, angShot[YAW], 0);
				AngleVectors(angForce, &vecForce);
				vecForce *= 25000;

				vecDamagePos = tr.endpos;

				CTakeDamageInfo info(this, this, GetActiveTFWeapon(), vecForce, vecDamagePos, 500, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_BOND);
				pEntity->TakeDamage(info);
			}
		}
		m_iTaunt = -1;
		break;
	}
	case TF_TAUNT_HADUKEN:
		{
			Vector vecAttackDir = BodyDirection2D();
			Vector vecOrigin = WorldSpaceCenter() + vecAttackDir * 64;
			Vector mins = vecOrigin - Vector( 24, 24, 24 );
			Vector maxs = vecOrigin + Vector( 24, 24, 24 );

			QAngle angForce( -45, EyeAngles()[YAW], 0 );
			Vector vecForce;
			AngleVectors( angForce, &vecForce );
			vecForce *= 25000;
			float flDamage = 500;
			CBaseEntity *pEnts[256];

			int numEnts = UTIL_EntitiesInBox( pEnts, 256, mins, maxs, FL_CLIENT|FL_OBJECT );

			for ( int i = 0; i < numEnts; i++ )
			{
				CBaseEntity *pEntity = pEnts[i];

				if ( pEntity == this || !pEntity->IsAlive() || ( InSameTeam( pEntity ) && pEntity->GetTeamNumber() != TF_TEAM_MERCENARY && pEntity->GetTeamNumber() != TEAM_UNASSIGNED ) || !FVisible( pEntity, MASK_SOLID ) )
					continue;

				Vector vecDamagePos = WorldSpaceCenter();
				vecDamagePos += ( pEntity->WorldSpaceCenter() - vecDamagePos ) * 0.75f;

				CTakeDamageInfo info( this, this, GetActiveTFWeapon(), vecForce, vecDamagePos, flDamage, DMG_IGNITE, TF_DMG_CUSTOM_TAUNTATK_HADOUKEN) ;
				pEntity->TakeDamage( info );
			}
			m_iTaunt = -1;
			break;
		}
		case TF_TAUNT_FENCING:
		{
			Vector vecAttackDir = BodyDirection2D();
			Vector vecOrigin = WorldSpaceCenter() + vecAttackDir * 64;
			Vector mins = vecOrigin - Vector( 24, 24, 24 );
			Vector maxs = vecOrigin + Vector( 24, 24, 24 );

			QAngle angForce( -45, EyeAngles()[YAW], 0 );
			Vector vecForce;
			AngleVectors( angForce, &vecForce );
			vecForce *= 25000;
			float flDamage = 0;
			switch ( m_iTauntLayer )
			{
				case 0:
				case 1:
					flDamage = 25;
					break;
				case 2:
					flDamage = 500;
					break;
			}

			CBaseEntity *pEnts[256];

			int numEnts = UTIL_EntitiesInBox( pEnts, 256, mins, maxs, FL_CLIENT|FL_OBJECT );

			for ( int i = 0; i < numEnts; i++ )
			{
				CBaseEntity *pEntity = pEnts[i];

				if ( pEntity == this || !pEntity->IsAlive() || ( InSameTeam( pEntity ) && pEntity->GetTeamNumber() != TF_TEAM_MERCENARY && pEntity->GetTeamNumber() != TEAM_UNASSIGNED ) || !FVisible( pEntity, MASK_SOLID ) )
					continue;

				Vector vecDamagePos = WorldSpaceCenter();
				vecDamagePos += ( pEntity->WorldSpaceCenter() - vecDamagePos ) * 0.75f;

				CTakeDamageInfo info( this, this, GetActiveTFWeapon(), vecForce, vecDamagePos, flDamage, DMG_SLASH, TF_DMG_CUSTOM_TAUNTATK_FENCING );
				pEntity->TakeDamage( info );
			}
			switch ( m_iTauntLayer )
			{
				case 0:
					SetTauntEffect( TF_TAUNT_FENCING, 0.33f, 1 );
					break;
				case 1:
					SetTauntEffect( TF_TAUNT_FENCING, 1.8f, 2 );
					break;
				case 2:
					m_iTaunt = -1;
					break;
			}
			break;
		}
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CTFPlayer::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	// This is a lame way to detect a taunt!
	if ( m_bInitTaunt )
	{
		m_bInitTaunt = false;
		return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
	}
	else
	{
		return InstancedScriptedScene( this, pszScene, NULL, flDelay, false, response, true, filter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	// If we have 'disguiseclass' criteria, pretend that we are actually our
	// disguise class. That way we just look up the scene we would play as if 
	// we were that class.
	int iMutator = TFGameRules()->GetMutator();

	int disguiseIndex = criteriaSet.FindCriterionIndex( "disguiseclass" );

	if ( disguiseIndex != -1 )
	{
		criteriaSet.AppendCriteria( "playerclass", criteriaSet.GetValue(disguiseIndex) );
		iMutator = m_Shared.GetDisguiseClassMod();
	}
	else
	{
		if ( GetPlayerClass() )
		{
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[ GetPlayerClass()->GetClassIndex() ] );
		}
	}
	
	for( int i = 0; i < m_hCustomCriteria.Count(); i++ )
	{
		criteriaSet.AppendCriteria( m_hCustomCriteria[i].szName, m_hCustomCriteria[i].szValue );
	}

	if( iMutator == TF_CLASSMOD_TFC )
		criteriaSet.AppendCriteria( "playermutator", "TFC" );

	criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs("%d", m_Shared.GetNumKillsInTime(30.0)) );

	int iTotalKills = 0;
	PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( this );
	if ( pStats )
	{
		iTotalKills = pStats->statsCurrentLife.m_iStat[TFSTAT_KILLS] + pStats->statsCurrentLife.m_iStat[TFSTAT_KILLASSISTS]+ 
			pStats->statsCurrentLife.m_iStat[TFSTAT_BUILDINGSDESTROYED];
	}
	criteriaSet.AppendCriteria( "killsthislife", UTIL_VarArgs( "%d", iTotalKills ) );
	criteriaSet.AppendCriteria( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "invulnerable", m_Shared.InCondUber() ? "1" : "0" );
	criteriaSet.AppendCriteria( "critboosted", m_Shared.InCondCrit() ? "1" : "0" );
	criteriaSet.AppendCriteria( "shielded", m_Shared.InCondShield() ? "1" : "0" );
	criteriaSet.AppendCriteria( "beinghealed", m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "waitingforplayers", (TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->IsInPreMatch()) ? "1" : "0" );

	// this is for paylaod
	criteriaSet.AppendCriteria( "teamrole", GetTFTeam()->GetRole() ? "defense" : "offense" );

	if ( GetTFTeam() )
	{
		int iTeamRole = GetTFTeam()->GetRole();

		if ( iTeamRole == 1 )
		{
			criteriaSet.AppendCriteria( "teamrole", "defense" );
		}
		else
		{
			criteriaSet.AppendCriteria( "teamrole", "offense" );
		}
	}

	// Current weapon role
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		int iWeaponRole = pActiveWeapon->GetTFWpnData().m_iWeaponType;
		int	iClass = GetPlayerClass()->GetClassIndex();
	
		if (pActiveWeapon->GetTFWpnData().m_iClassWeaponType[iClass] >= 0)
			iWeaponRole = pActiveWeapon->GetTFWpnData().m_iClassWeaponType[iClass];
		switch( iWeaponRole )
		{
		case TF_WPN_TYPE_PRIMARY:
		default:
			criteriaSet.AppendCriteria( "weaponmode", "primary" );
			break;
		case TF_WPN_TYPE_SECONDARY:
			criteriaSet.AppendCriteria( "weaponmode", "secondary" );
			break;
		case TF_WPN_TYPE_MELEE:
			criteriaSet.AppendCriteria( "weaponmode", "melee" );
			break;
		case TF_WPN_TYPE_BUILDING:
			criteriaSet.AppendCriteria( "weaponmode", "building" );
			break;
		case TF_WPN_TYPE_PDA:
			criteriaSet.AppendCriteria( "weaponmode", "pda" );
			break;
		}

		if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE || pActiveWeapon->GetWeaponID() == TF_WEAPON_RAILGUN )
		{
			CTFSniperRifle *pRifle = dynamic_cast<CTFSniperRifle*>(pActiveWeapon);
			if ( pRifle && pRifle->IsZoomed() )
			{
				criteriaSet.AppendCriteria( "sniperzoomed", "1" );
			}
		}
		else if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_MINIGUN || pActiveWeapon->GetWeaponID() == TF_WEAPON_GATLINGGUN || pActiveWeapon->GetWeaponID() == TFC_WEAPON_ASSAULTCANNON )
		{
			CTFMinigun *pMinigun = dynamic_cast<CTFMinigun*>(pActiveWeapon);
			if ( pMinigun )
			{
				criteriaSet.AppendCriteria( "minigunfiretime", UTIL_VarArgs("%.1f", pMinigun->GetFiringTime() ) );
			}
		}
	}

	// Player under crosshair
	trace_t tr;
	Vector forward;
	EyeVectors( &forward );
	UTIL_TraceLine( EyePosition(), EyePosition() + (forward * MAX_TRACE_LENGTH), MASK_BLOCKLOS_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);
			if ( pTFPlayer )
			{
				int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
				if ( !InSameTeam(pTFPlayer) )
				{
					// Prevent spotting stealthed enemies who haven't been exposed recently
					if ( pTFPlayer->m_Shared.InCondInvis() )
					{
						if ( pTFPlayer->m_Shared.GetLastStealthExposedTime() < (gpGlobals->curtime - 3.0) )
						{
							iClass = TF_CLASS_UNDEFINED;
						}
						else
						{
							iClass = TF_CLASS_SPY;
						}
					}
					else if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iClass = pTFPlayer->m_Shared.GetDisguiseClass();
					}
				}

				if ( iClass > TF_CLASS_UNDEFINED && iClass <= TF_CLASS_COUNT_ALL )
				{
					criteriaSet.AppendCriteria( "crosshair_on", g_aPlayerClassNames_NonLocalized[iClass] );				

					bool bEnemy = ( pTFPlayer->GetTeamNumber() != GetTeamNumber() || pTFPlayer->GetTeamNumber() == TF_TEAM_MERCENARY );
					
					criteriaSet.AppendCriteria( "crosshair_enemy", bEnemy ? "Yes" : "No" );		
				}
			}
		}
	}

	// end of round voicelines
	if ( TFGameRules() )
	{
		if ( TFGameRules()->GetWinningTeam() == this->GetTeamNumber() )
		{
			criteriaSet.AppendCriteria( "OnWinningTeam", "1" );
			criteriaSet.AppendCriteria( "IsCompWinner", "1" );
		}
		else
		{
			criteriaSet.AppendCriteria( "OnWinningTeam", "0" );
		}
	}

	// Previous round win
	bool bLoser = ( TFGameRules()->GetPreviousRoundWinners() != TEAM_UNASSIGNED && TFGameRules()->GetPreviousRoundWinners() != GetTeamNumber() );
	criteriaSet.AppendCriteria( "LostRound", UTIL_VarArgs("%d", bLoser) );

	// Control points
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CTriggerAreaCapture *pAreaTrigger = dynamic_cast<CTriggerAreaCapture*>(pTouch);
				if ( pAreaTrigger )
				{
					CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
					if ( pCP )
					{
						if ( pCP->GetOwner() == GetTeamNumber() )
						{
							criteriaSet.AppendCriteria( "OnFriendlyControlPoint", "1" );
						}
						else 
						{
							if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
								 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
							{
								criteriaSet.AppendCriteria( "OnCappableControlPoint", "1" );
							}
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE )
		return false;

	if ( !pPlayer->IsAlive() && IsAlive() )
	{
		// Everyone can chat like normal when the round/game ends
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN || TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
			return true;

		// Separate rule for spectators.
		if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			return tf_spectalk.GetBool();

		// Living players can't hear dead ones unless gravetalk is enabled.
		return tf_gravetalk.GetBool();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CTFPlayer::GetResponseSystem()
{
	int iClass = GetPlayerClass()->GetClassIndex();

	if ( m_bSpeakingConceptAsDisguisedSpy && m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iClass = m_Shared.GetDisguiseClass();
	}

	bool bValidClass = ( iClass >= TF_CLASS_SCOUT && iClass < TF_CLASS_COUNT_ALL );
	bool bValidConcept = ( m_iCurrentConcept >= 0 && m_iCurrentConcept < MP_TF_CONCEPT_COUNT );
	Assert( bValidClass );
	Assert( bValidConcept );

	if ( !bValidClass || !bValidConcept || !TFGameRules() )
	{
		return BaseClass::GetResponseSystem();
	}
	else
	{
		return TFGameRules()->m_ResponseRules[iClass].m_ResponseSystems[m_iCurrentConcept];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	if ( !IsAlive() )
		return false;

	bool bReturn = false;

	if ( IsSpeaking() )
	{
		if ( iConcept != MP_CONCEPT_DIED )
			return false;
	}

	// Save the current concept.
	m_iCurrentConcept = iConcept;

	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !filter )
	{
		CSingleUserRecipientFilter filter(this);

		int iEnemyTeam = ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;

		// test, enemies and myself
		CTeamRecipientFilter disguisedFilter( iEnemyTeam );
		disguisedFilter.AddRecipient( this );

		CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
		Assert( pExpresser );

		pExpresser->AllowMultipleScenes();

		// play disguised concept to enemies and myself
		char buf[128];
		Q_snprintf( buf, sizeof(buf), "disguiseclass:%s", g_aPlayerClassNames_NonLocalized[ m_Shared.GetDisguiseClass() ] );

		if ( modifiers )
		{
			Q_strncat( buf, ",", sizeof(buf), 1 );
			Q_strncat( buf, modifiers, sizeof(buf), COPY_ALL_CHARACTERS );
		}

		m_bSpeakingConceptAsDisguisedSpy = true;

		bool bPlayedDisguised = SpeakIfAllowed( g_pszMPConcepts[iConcept], buf, pszOutResponseChosen, bufsize, &disguisedFilter );

		m_bSpeakingConceptAsDisguisedSpy = false;

		// test, everyone except enemies and myself
		CBroadcastRecipientFilter undisguisedFilter;
		undisguisedFilter.RemoveRecipientsByTeam( GetGlobalTFTeam(iEnemyTeam) );
		undisguisedFilter.RemoveRecipient( this );

		// play normal concept to teammates
		bool bPlayedNormally = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &undisguisedFilter );

		pExpresser->DisallowMultipleScenes();

		bReturn = ( bPlayedDisguised && bPlayedNormally );
	}
	else
	{
		// play normally
		bReturn = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );	
	}

	//Add bubble on top of a player calling for medic.
	if ( bReturn )
	{
		if ( iConcept == MP_CONCEPT_PLAYER_MEDIC )
		{
			SaveMe();
		}
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateExpression( void )
{
	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( MP_CONCEPT_PLAYER_EXPRESSION, szScene, sizeof( szScene ) ) )
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat(30,40);
		return;
	}
	
	// Ignore updates that choose the same scene
	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), szScene ) == 0 )
		return;

	if ( m_hExpressionSceneEnt )
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString( szScene );
	float flDuration = InstancedScriptedScene( this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearExpression( void )
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Only show subtitle to enemy if we're disguised as the enemy
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return ( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() != GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow rapid-fire voice commands
//-----------------------------------------------------------------------------
bool CTFPlayer::CanSpeakVoiceCommand( void )
{
	return ( gpGlobals->curtime > m_flNextVoiceCommandTime );
}

//-----------------------------------------------------------------------------
// Purpose: Note the time we're allowed to next speak a voice command
//-----------------------------------------------------------------------------
void CTFPlayer::NoteSpokeVoiceCommand( const char *pszScenePlayed )
{
	Assert( pszScenePlayed );
	m_flNextVoiceCommandTime = gpGlobals->curtime + min( GetSceneDuration( pszScenePlayed ), tf_max_voice_speak_delay.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	bool bIsMedic = false;
	
	//Do Lag comp on medics trying to heal team mates.
	if ( IsPlayerClass( TF_CLASS_MEDIC ) == true )
	{
		bIsMedic = true;

		if ( pEntity->GetTeamNumber() == GetTeamNumber()  )
		{
			CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( GetActiveWeapon() );

			if ( pWeapon && pWeapon->GetHealTarget() )
			{
				if ( pWeapon->GetHealTarget() == pEntity )
					return true;
				else
					return false;
			}
		}
	}
	
	if ( pEntity->GetTeamNumber() == GetTeamNumber() && bIsMedic == false && !friendlyfire.GetBool()
	// Deathmatch Specific Lag Comp, If either the shooter or the Victim is on the merc team, dont return false here
	&& pEntity->GetTeamNumber() != TF_TEAM_MERCENARY && GetTeamNumber() != TF_TEAM_MERCENARY )
		return false;
	
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pEntity->entindex() ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pEntity->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxspeed;
	CBasePlayer *pPlayer = ToBasePlayer((CBaseEntity*)pEntity);
	if ( pPlayer )
		maxspeed = pPlayer->MaxSpeed();
	else
		maxspeed = 600;
	float maxDistance = 1.5 * maxspeed * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );

	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SpeakWeaponFire( int iCustomConcept )
{
	if ( iCustomConcept == MP_CONCEPT_NONE )
	{
		if ( m_flNextSpeakWeaponFire > gpGlobals->curtime )
			return;

		iCustomConcept = MP_CONCEPT_FIREWEAPON;
	}

	m_flNextSpeakWeaponFire = gpGlobals->curtime + 5;

	// Don't play a weapon fire scene if we already have one
	if ( m_hWeaponFireSceneEnt )
		return;

	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( iCustomConcept, szScene, sizeof( szScene ) ) )
		return;

	float flDuration = InstancedScriptedScene( this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextSpeakWeaponFire = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearWeaponFireScene( void )
{
	if ( m_hWeaponFireSceneEnt )
	{
		StopScriptedScene( this, m_hWeaponFireSceneEnt );
		m_hWeaponFireSceneEnt = NULL;
	}
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
}

int CTFPlayer::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ),"Health: %d / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CTFPlayer::GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes )
{
	AI_Response response;
	bool bResult = SpeakConcept( response, iConcept );

	if ( bResult )
	{
		if ( response.IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( response.GetContext() );
			}
		}
		else
		{
			AddContext( response.GetContext() );
		}

		V_strncpy( chSceneBuffer, response.GetResponsePtr(), numSceneBufferBytes );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: Mapmaker input to force this NPC to speak a response rules concept
//-----------------------------------------------------------------------------
void CTFPlayer::InputSpeakResponseConcept( inputdata_t &inputdata )
{
//	SpeakMapmakerInterruptConcept( inputdata.value.StringID() );
	int iConcept = GetMPConceptIndexFromString( inputdata.value.String() );

	if ( iConcept != MP_CONCEPT_NONE )
	{
		SpeakConceptIfAllowed( iConcept );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets a player on fire
//-----------------------------------------------------------------------------
void CTFPlayer::InputIgnitePlayer( inputdata_t &inputdata )
{
	m_Shared.Burn( ToTFPlayer( inputdata.pActivator ), inputdata.value.Float() );
}

void CTFPlayer::InputExtinguishPlayer(inputdata_t &inputdata)
{
	if ( m_Shared.InCond( TF_COND_BURNING ) )
	{
		EmitSound( "TFPlayer.FlameOut" );
		m_Shared.RemoveCond( TF_COND_BURNING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Poison a player
//-----------------------------------------------------------------------------
void CTFPlayer::InputPoisonPlayer(inputdata_t &inputdata)
{
	m_Shared.Poison(ToTFPlayer(inputdata.pActivator), inputdata.value.Float());
}

void CTFPlayer::InputDePoisonPlayer(inputdata_t &inputdata)
{
	if (m_Shared.InCond(TF_COND_POISON))
		m_Shared.RemoveCond(TF_COND_POISON);
}

//-----------------------------------------------------------------------------
// Purpose: Set a player as a zombie
//-----------------------------------------------------------------------------
void CTFPlayer::InputSetZombie( inputdata_t &inputdata )
{
	CommitSuicide( true, true );
	m_Shared.SetZombie( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: Set a player's team WITHOUT killing them
//-----------------------------------------------------------------------------
void CTFPlayer::InputSetTeamNoKill( inputdata_t &inputdata )
{
	ChangeTeam( inputdata.value.Int(), true );
}

//-----------------------------------------------------------------------------
// Purpose: Add an attribute to a weapon
//-----------------------------------------------------------------------------
void CTFPlayer::InputSetAttributeOfWeapon( inputdata_t &inputdata )
{
	char szValue[256];
	Q_strncpy( szValue, inputdata.value.String(), sizeof(szValue) );

	CUtlVector<char*> hArgs;
	Q_SplitString( szValue,":", hArgs );

	FOR_EACH_VEC( hArgs, i )
	{
		Q_StripPrecedingAndTrailingWhitespace( hArgs[i] );
	}

	int iSlot = -1, iPos = -1;
	TFGameRules()->GetWeaponSlot( hArgs[0], iSlot, iPos, this );

	CTFWeaponBase *pWeapon = GetWeaponInSlot(iSlot, iPos);

	if( !pWeapon )
		return;

	pWeapon->PurgeAttributes();

	for( int i = 1; i+1 < hArgs.Count(); i =+ 2 )
	{
		pWeapon->AddAttribute( CTFAttribute(GetItemSchema()->GetAttributeID(hArgs[i]), hArgs[i + 1]) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add an attribute to a weapon
//-----------------------------------------------------------------------------
void CTFPlayer::InputAddAttributeToWeapon( inputdata_t &inputdata )
{
	char szValue[256];
	Q_strncpy( szValue, inputdata.value.String(), sizeof(szValue) );

	CUtlVector<char*> hArgs;
	Q_SplitString( szValue,":", hArgs );

	FOR_EACH_VEC( hArgs, i )
	{
		Q_StripPrecedingAndTrailingWhitespace( hArgs[i] );
	}

	WEAPON_FILE_INFO_HANDLE pHandle = LookupWeaponInfoSlot(hArgs[0]);

	if( pHandle == GetInvalidWeaponInfoHandle() )
		return;

	int iSlot = -1, iPos = -1;
	for( int i = 0; i < WeaponCount(); i++ )
	{
		CBaseCombatWeapon *pWeapon = GetWeapon(i);
		if( pWeapon && pWeapon->GetWeaponFileInfoHandle() == pHandle )
		{
			iSlot = pWeapon->GetSlot();
			iPos = pWeapon->GetPosition();
		}
	}

	CTFWeaponBase *pWeapon = GetWeaponInSlot( iSlot, iPos );

	if( !pWeapon )
		return;

	for( int i = 1; i+1 < hArgs.Count(); i =+ 2 )
	{
		pWeapon->AddAttribute( CTFAttribute(GetItemSchema()->GetAttributeID(hArgs[i]), hArgs[i + 1]) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove attribute from weapon
//-----------------------------------------------------------------------------
void CTFPlayer::InputRemoveAttributeFromWeapon( inputdata_t &inputdata )
{
	char szValue[256];
	Q_strncpy( szValue, inputdata.value.String(), sizeof(szValue) );

	CUtlVector<char*> hArgs;
	Q_SplitString( szValue, ":", hArgs );

	FOR_EACH_VEC( hArgs, i )
	{
		Q_StripPrecedingAndTrailingWhitespace( hArgs[i] );
	}

	WEAPON_FILE_INFO_HANDLE pHandle = LookupWeaponInfoSlot(hArgs[0]);

	if( pHandle == GetInvalidWeaponInfoHandle() )
		return;

	int iSlot = -1, iPos = -1;
	for( int i = 0; i < WeaponCount(); i++ )
	{
		CBaseCombatWeapon *pWeapon = GetWeapon(i);
		if( pWeapon && pWeapon->GetWeaponFileInfoHandle() == pHandle )
		{
			iSlot = pWeapon->GetSlot();
			iPos = pWeapon->GetPosition();
		}
	}

	CTFWeaponBase *pWeapon = GetWeaponInSlot( iSlot, iPos );

	if( !pWeapon )
		return;

	for( int i = 1; i < hArgs.Count(); i++ )
	{
		pWeapon->RemoveAttribute( hArgs[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::InputSetArmor(inputdata_t &inputdata)
{
    int newArmor = inputdata.value.Int();
    int max = GetPlayerClass()->GetMaxArmor();
    newArmor = newArmor < 0 ? 0 : newArmor;
    newArmor = newArmor > max ? max : newArmor;
    m_Shared.SetTFCArmor(newArmor);
}

//-----------------------------------------------------------------------------
// Purpose:calculate a score for this player. higher is more likely to be switched
//-----------------------------------------------------------------------------
int	CTFPlayer::CalculateTeamBalanceScore( void )
{
	int iScore = BaseClass::CalculateTeamBalanceScore();

	// switch engineers less often
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		iScore -= 120;
	}

	return iScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// Debugging Stuff
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void DebugParticles( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		// print out their conditions
		pPlayer->m_Shared.DebugPrintConditions();	
	}
}

static ConCommand sv_debug_stuck_particles( "sv_debug_stuck_particles", DebugParticles, "Debugs particles attached to the player under your crosshair." );

//-----------------------------------------------------------------------------
// Purpose: Debug concommand to set the player on fire
//-----------------------------------------------------------------------------
void IgnitePlayer()
{
	CTFPlayer *pPlayer = ToTFPlayer( ToTFPlayer( UTIL_PlayerByIndex( 1 ) ) );
	pPlayer->m_Shared.Burn( pPlayer, 0 );
}
static ConCommand cc_IgnitePlayer( "tf_ignite_player", IgnitePlayer, "Sets you on fire", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestVCD( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			if ( args.ArgC() >= 2 )
			{
				InstancedScriptedScene( pPlayer, args[1], NULL, 0.0f, false, NULL, true );
			}
			else
			{
				InstancedScriptedScene( pPlayer, "scenes/heavy_test.vcd", NULL, 0.0f, false, NULL, true );
			}
		}
	}
}
static ConCommand tf_testvcd( "tf_testvcd", TestVCD, "Run a vcd on the player currently under your crosshair. Optional parameter is the .vcd name (default is 'scenes/heavy_test.vcd')", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestRR( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("No concept specified. Format is tf_testrr <concept>\n");
		return;
	}

	CBaseEntity *pEntity = NULL;
	const char *pszConcept = args[1];

	if ( args.ArgC() == 3 )
	{
		pszConcept = args[2];
		pEntity = UTIL_PlayerByName( args[1] );
	}

	if ( !pEntity || !pEntity->IsPlayer() )
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( !pEntity || !pEntity->IsPlayer() )
		{
			pEntity = ToTFPlayer( UTIL_GetCommandClient() ); 
		}
	}

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			int iConcept = GetMPConceptIndexFromString( pszConcept );
			if ( iConcept != MP_CONCEPT_NONE )
			{
				pPlayer->SpeakConceptIfAllowed( iConcept );
			}
			else
			{
				Msg( "Attempted to speak unknown multiplayer concept: %s\n", pszConcept );
			}
		}
	}
}
static ConCommand tf_testrr( "tf_testrr", TestRR, "Force the player under your crosshair to speak a response rule concept. Format is tf_testrr <concept>, or tf_testrr <player name> <concept>", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*bool CTFPlayer::SetPowerplayEnabled( bool bOn )
{
	if ( bOn )
	{
		m_flPowerPlayTime = gpGlobals->curtime + 99999;
		m_Shared.RecalculateInvuln();
		m_Shared.Burn( this, 0 );

		PowerplayThink();
	}
	else
	{
		m_flPowerPlayTime = 0.0;
		m_Shared.RemoveCond( TF_COND_BURNING );
		m_Shared.RecalculateInvuln();
	}
	return true;
}

uint64 powerplaymask = 0xFAB2423BFFA352AF;
uint64 powerplay_ids[] =
{
	76561197960435530 ^ powerplaymask,
	76561197960265731 ^ powerplaymask,
	76561197960265749 ^ powerplaymask,
	76561197962783665 ^ powerplaymask,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerHasPowerplay( void )
{
	if ( !engine->IsClientFullyAuthenticated( edict() ) )
		return false;

	player_info_t pi;
	if ( engine->GetPlayerInfo( entindex(), &pi ) && ( pi.friendsID ) )
	{
		CSteamID steamIDForPlayer( pi.friendsID, 1, k_EUniversePublic, k_EAccountTypeIndividual );
		for ( int i = 0; i < ARRAYSIZE(powerplay_ids); i++ )
		{
			if ( steamIDForPlayer.ConvertToUint64() == (powerplay_ids[i] ^ powerplaymask) )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PowerplayThink( void )
{
	if ( m_flPowerPlayTime > gpGlobals->curtime )
	{
		float flDuration = 0;
		if ( GetPlayerClass() )
		{
			switch ( GetPlayerClass()->GetClassIndex() )
			{
			case TF_CLASS_SCOUT: flDuration = InstancedScriptedScene( this, "scenes/player/scout/low/laughlong02.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_SNIPER: flDuration = InstancedScriptedScene( this, "scenes/player/sniper/low/laughlong01.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_SOLDIER: flDuration = InstancedScriptedScene( this, "scenes/player/soldier/low/laughevil02.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_MERCENARY: flDuration = InstancedScriptedScene( this, "scenes/player/mercenary/low/laughevil02.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_DEMOMAN: flDuration = InstancedScriptedScene( this, "scenes/player/demoman/low/laughlong02.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_MEDIC: flDuration = InstancedScriptedScene( this, "scenes/player/medic/low/laughlong02.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_HEAVYWEAPONS: flDuration = InstancedScriptedScene( this, "scenes/player/heavy/low/laughlong01.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_PYRO: flDuration = InstancedScriptedScene( this, "scenes/player/pyro/low/laughlong01.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_SPY: flDuration = InstancedScriptedScene( this, "scenes/player/spy/low/laughevil01.vcd", NULL, 0.0f, false, NULL, true ); break;
			case TF_CLASS_ENGINEER: flDuration = InstancedScriptedScene( this, "scenes/player/engineer/low/laughlong01.vcd", NULL, 0.0f, false, NULL, true ); break;
			}
		}

		SetContextThink( &CTFPlayer::PowerplayThink, gpGlobals->curtime + flDuration + RandomFloat( 2, 5 ), "TFPlayerLThink" );
	}
} */

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldAnnouceAchievement( void )
{ 
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.InCondInvis() ||
			 m_Shared.InCond( TF_COND_DISGUISED ) ||
			 m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
	}

	return true; 
}

void CTFPlayer::UpdatePlayerColor ( void )
{
	// bots have their own system so dont do this
	
	Vector vecNewColor;
	switch( GetTeamNumber() )
	{
		default:
		case TF_TEAM_MERCENARY:
			if ( IsFakeClient() )
				return;
			vecNewColor.x = V_atoi( engine->GetClientConVarValue( entindex(), "of_color_r" ) );
			vecNewColor.y = V_atoi( engine->GetClientConVarValue( entindex(), "of_color_g" ) );
			vecNewColor.z = V_atoi( engine->GetClientConVarValue( entindex(), "of_color_b" ) );
			break;
		case TF_TEAM_RED:
			vecNewColor.x = 184.0f;
			vecNewColor.y = 56.0f;
			vecNewColor.z = 59.0f;
			break;
		case TF_TEAM_BLUE:
			vecNewColor.x = 88.0f;
			vecNewColor.y = 133.0f;
			vecNewColor.z = 162.0f;
			break;
	}
	m_vecPlayerColor = vecNewColor / 255;
}

void CTFPlayer::SetCustomModel(inputdata_t &inputdata)
{
	MDLCACHE_CRITICAL_SECTION();
	InvalidateMdlCache();

	if (inputdata.value.String())
	{
		PrecacheModel(inputdata.value.String());
		const char *pszModel = inputdata.value.String();
		if ( pszModel && pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}		
		GetPlayerClass()->SetCustomModel(inputdata.value.String());
	}
	else
	{
		GetPlayerClass()->SetCustomModel( "" );
	}
	UpdateModel();
	DevMsg("CTFPlayer::SetCustomModel - Input successful, data input %s\n", inputdata.value.String());
}


void CTFPlayer::SetCustomArmModel(inputdata_t &inputdata)
{
	MDLCACHE_CRITICAL_SECTION();
	InvalidateMdlCache();
	if (inputdata.value.String())
	{
		PrecacheModel( inputdata.value.String() );
		const char *pszModel = inputdata.value.String();
		if ( pszModel && pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}		
		GetPlayerClass()->SetCustomArmModel( inputdata.value.String() );
	}
	else
	{
		GetPlayerClass()->SetCustomArmModel( "" );
	}
	UpdateArmModel();
	DevMsg("CTFPlayer::SetCustomModel - Input successful, data input %s\n", inputdata.value.String());
}

void CTFPlayer::AddMoney(inputdata_t &inputdata)
{
	int money =inputdata.value.Int();
	DevMsg("Money for everybody, but mostly for me \n %d \n", money);
	AddAccount( money );
}

void CTFPlayer::SetMoney(inputdata_t &inputdata)
{
	int money =inputdata.value.Int();
	m_iAccount = money;
}

void CTFPlayer::InputStripWeapons(inputdata_t &inputdata)
{
	StripWeapons();
}

void CTFPlayer::GiveAllItems()
{
	EquipSuit();
	
	AddAccount( 16000 );

	CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( 0 );
	
	int nWeapons = TF_WEAPON_COUNT; 
	int i;	
	
	for ( i = 0; i < nWeapons; ++i )
	{
		pWeapon = (CTFWeaponBase *)GiveNamedItem( g_aWeaponNames[i] );
		if (pWeapon)
		{
			pWeapon->DefaultTouch(this);
		}
	}

	RestockAmmo(POWERUP_FULL);
	RestockSpecialEffects(POWERUP_FULL);

	GiveAmmo( 1000, TF_AMMO_PRIMARY );
	GiveAmmo( 1000, TF_AMMO_SECONDARY );
	GiveAmmo( 1000, TF_AMMO_METAL );

	TakeHealth(m_iMaxHealth, DMG_GENERIC);
}

void CTFPlayer::RefillHealthAmmo()
{
	EquipSuit();
	
	AddAccount( 16000 );

	RestockAmmo(POWERUP_FULL);
	RestockSpecialEffects(POWERUP_FULL);

	GiveAmmo( 1000, TF_AMMO_PRIMARY );
	GiveAmmo( 1000, TF_AMMO_SECONDARY );
	GiveAmmo( 1000, TF_AMMO_METAL );

	TakeHealth(m_iMaxHealth, DMG_GENERIC);
}

CBaseEntity	*CTFPlayer::GetHeldObject(void)
{
	return PhysCannonGetHeldEntity(GetActiveWeapon());
}

void CTFPlayer::AddAccount( int amount, bool bTrackChange )
{
	m_iAccount += amount;

	if ( m_iAccount < 0 )
		m_iAccount = 0;
	else if ( m_iAccount > 16000 )
		m_iAccount = 16000;
}

// Required for HL2 ents!

//---------------------------------------------------------
//---------------------------------------------------------
Vector CTFPlayer::EyeDirection2D( void )
{
	Vector vecReturn = EyeDirection3D();
	vecReturn.z = 0;
	vecReturn.AsVector2D().NormalizeInPlace();

	return vecReturn;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CTFPlayer::EyeDirection3D( void )
{
	Vector vecForward;

	// Return the vehicle angles if we request them
	if ( GetVehicle() != NULL )
	{
		CacheVehicleView();
		EyeVectors( &vecForward );
		return vecForward;
	}
	
	AngleVectors( EyeAngles(), &vecForward );
	return vecForward;
}

//-----------------------------------------------------------------------------
// Purpose: Used by zombies: drops ammo and health small packs on death
//-----------------------------------------------------------------------------
void CTFPlayer::DropZombieAmmoHealth( void )
{
	if ( !of_zombie_dropitems.GetBool() )
		return;

	QAngle angLaunch = QAngle( 0, 0 ,0 ); // the angle we will launch ourselves from
	Vector vecLaunch;					  // final velocity used to launch the items

	CTFPowerup *pPowerup = dynamic_cast< CTFPowerup * >( CBaseEntity::Create( "item_healthkit_small", WorldSpaceCenter(), angLaunch, this ) );

	if ( pPowerup )
	{
		angLaunch[ PITCH ] += RandomInt( 5 , 15 );
		angLaunch[ YAW ] = RandomInt( 0 , 360 );

		AngleVectors( angLaunch, &vecLaunch );

		// boost it up baby
		vecLaunch *= RandomInt( 200, 500 );

		pPowerup->ChangeTeam( TF_TEAM_RED );
		pPowerup->DropSingleInstance( vecLaunch, this, 10.0f, 0.2f );
	}
	
	QAngle angLaunch2 = QAngle( 0, 0 ,0 ); // the angle we will launch ourselves from
	Vector vecLaunch2;					  // final velocity used to launch the items

	CTFPowerup *pPowerup2 = dynamic_cast< CTFPowerup * >( CBaseEntity::Create( "item_ammopack_small", WorldSpaceCenter(), angLaunch2, this ) );

	if ( pPowerup2 )
	{
		angLaunch2[ PITCH ] += RandomInt( 5 , 15 );
		angLaunch2[ YAW ] = RandomInt( 0 , 720 );

		AngleVectors( angLaunch2, &vecLaunch2 );

		// boost it up baby
		vecLaunch2 *= RandomInt( 225, 550 );

		pPowerup2->ChangeTeam( TF_TEAM_RED );
		pPowerup2->DropSingleInstance( vecLaunch2, this, 10.0f, 0.2f );
	}
}

//================================================================================
//================================================================================
void CTFPlayer::SetBotController( IBot * pBot )
{
    if ( m_pBotController )
    {
        // tf_player.cpp(10855,9): warning C5205: delete of an abstract class 'IBot' that has a non-virtual destructor results in undefined behavior
        // -sappho
		// TODO: Doesn't this cause memory leaks???
		//		 The real solution would be making the destructor virtual, would it not?
		//		 Did so, let me know if there is any issues arise - Kay
		delete m_pBotController;
        //m_pBotController = NULL;
    }

    m_pBotController = pBot;
}

//================================================================================
//================================================================================
void CTFPlayer::SetUpBot()
{
    CreateSenses();
    SetBotController( new CBot( this ) );
}

int CTFPlayer::GetBotType( void ) const
{
	if( !m_pBotController )
		return BOT_TYPE_PUPPET;

	return static_cast<CBot*>(m_pBotController)->RemoveOnDeath() ? BOT_TYPE_LOCKDOWN : BOT_TYPE_INSOURCE;
}

//================================================================================
//================================================================================
void CTFPlayer::CreateSenses()
{
    m_pSenses = new CAI_Senses;
    m_pSenses->SetOuter( this );
}

//================================================================================
//================================================================================
void CTFPlayer::SetDistLook( float flDistLook )
{
    if ( GetSenses() ) {
        GetSenses()->SetDistLook( flDistLook );
    }
}

//================================================================================
//================================================================================
int CTFPlayer::GetSoundInterests()
{
    return SOUND_DANGER | SOUND_COMBAT | SOUND_PLAYER | SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE;
}

//================================================================================
//================================================================================
int CTFPlayer::GetSoundPriority( CSound *pSound )
{
    if ( pSound->IsSoundType( SOUND_COMBAT ) ) {
        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_DANGER ) ) {
        if ( pSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER | SOUND_CONTEXT_EXPLOSION ) ) {
            return SOUND_PRIORITY_HIGHEST;
        }
        else if ( pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE | SOUND_BULLET_IMPACT ) ) {
            return SOUND_PRIORITY_VERY_HIGH;
        }

        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE ) ) {
        return SOUND_PRIORITY_VERY_LOW;
    }

    return SOUND_PRIORITY_NORMAL;
}

//================================================================================
//================================================================================
bool CTFPlayer::QueryHearSound( CSound *pSound )
{
    CBaseEntity *pOwner = pSound->m_hOwner.Get();

    if ( pOwner == this )
        return false;

    if ( pSound->IsSoundType( SOUND_PLAYER ) && !pOwner ) {
        return false;
    }

    if ( pSound->IsSoundType( SOUND_CONTEXT_ALLIES_ONLY ) ) {
        if ( Classify() != CLASS_PLAYER_ALLY ) {
            return false;
        }
    }

    if ( pOwner ) {
        // Solo escuchemos sonidos provocados por nuestros aliados si son de combate.
        if ( TheGameRules->PlayerRelationship( this, pOwner ) == GR_ALLY ) {
            if ( pSound->IsSoundType( SOUND_COMBAT ) && !pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                return true;
            }

            return false;
        }
    }

    if ( ShouldIgnoreSound( pSound ) ) {
        return false;
    }

    return true;
}

//================================================================================
//================================================================================
bool CTFPlayer::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFear )
{
    if ( bOnlyHateOrFear ) {
		int iRelationship = TFGameRules()->PlayerRelationship( this, pEntity ) ;

        if( iRelationship == GR_NOTTEAMMATE || iRelationship == GR_ENEMY )
            return true;

        Disposition_t disposition = IRelationType( pEntity );
        return (disposition == D_HT || disposition == D_FR);
    }

    return true;
}

//================================================================================
//================================================================================
void CTFPlayer::OnLooked( int iDistance )
{
    if ( GetBotController() ) {
        GetBotController()->OnLooked( iDistance );
    }
}

//================================================================================
//================================================================================
void CTFPlayer::OnListened()
{
    if ( GetBotController() ) {
        GetBotController()->OnListened();
    }
}

//================================================================================
//================================================================================
CSound *CTFPlayer::GetLoudestSoundOfType( int iType )
{
    return CSoundEnt::GetLoudestSoundOfType( iType, EarPosition() );
}

//================================================================================
// Devuelve si podemos ver el origen del sonido
//================================================================================
bool CTFPlayer::SoundIsVisible( CSound *pSound )
{
    return (FVisible( pSound->GetSoundReactOrigin() ) && IsInFieldOfView( pSound->GetSoundReactOrigin() ));
}

//================================================================================
//================================================================================
CSound* CTFPlayer::GetBestSound( int validTypes )
{
    CSound *pResult = GetSenses()->GetClosestSound( false, validTypes );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestSound\n" );
    }

    return pResult;
}

//================================================================================
//================================================================================
CSound* CTFPlayer::GetBestScent()
{
    CSound *pResult = GetSenses()->GetClosestSound( true );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestScent\n" );
    }

    return pResult;
}

/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( fromArea == nullptr )
	{
		// first area in path; zero cost
		return 0.0f;
	}

	const CTFNavArea *tfArea = dynamic_cast<const CTFNavArea *>( area );
	if ( tfArea == nullptr )
		return false;

	if ( !m_pPlayer->IsAreaTraversable( area ) )
	{
		// dead end
		return -1.0f;
	}

	// unless the round is over and we are the winning team, we can't enter the other teams spawn
	if ( ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && !TFGameRules()->IsInfGamemode() ) )
	{
		switch ( m_pPlayer->GetTeamNumber() )
		{
			case TF_TEAM_RED:
			{
				if ( tfArea->HasTFAttributes( BLUE_SPAWN_ROOM ) )
					return -1.0f;

				break;
			}
			case TF_TEAM_BLUE:
			{
				if ( tfArea->HasTFAttributes( RED_SPAWN_ROOM ) )
					return -1.0f;

				break;
			}
			default:
				break;
		}
	}

	if ( ladder != nullptr )
		length = ladder->m_length;
	else if ( length <= 0.0f )
		length = ( area->GetCenter() - fromArea->GetCenter() ).Length();

	const float dz = fromArea->ComputeAdjacentConnectionHeightChange( area );
	if ( dz >= m_flStepHeight )
	{
		// too high!
		if ( dz >= m_flMaxJumpHeight )
			return -1.0f;

		// jumping is slow
		length *= 2;
	}
	else
	{
		// yikes, this drop will hurt too much!
		if ( dz < -m_flDeathDropHeight )
			return -1.0f;
	}

	return fromArea->GetCostSoFar() + length;
}
*/