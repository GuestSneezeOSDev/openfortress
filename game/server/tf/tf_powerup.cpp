//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"

//=============================================================================
float PackRatios[POWERUP_SIZES] =
{
	0.1,    // TINY 
	0.2,	// SMALL
	0.5,	// MEDIUM
	1.0,	// FULL
};

//=============================================================================
//
// CTF Powerup tables.
//

BEGIN_DATADESC( CTFPowerup )

// Keyfields.
DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
DEFINE_KEYFIELD( m_bHide, FIELD_BOOLEAN, "HiddenWhenRespawning" ),
DEFINE_KEYFIELD( fl_RespawnTime, FIELD_FLOAT, "respawntime" ),
DEFINE_KEYFIELD( m_iszSpawnSound, FIELD_STRING, "spawn_sound" ),
DEFINE_KEYFIELD( fl_RespawnDelay, FIELD_FLOAT, "respawndelay" ),

// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

// Outputs.

DEFINE_OUTPUT( m_OnRespawn, "OnRespawn" ),

END_DATADESC();

//=============================================================================
//
// CTF Powerup functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPowerup::CTFPowerup()
{
	m_bDisabled = false;
	m_bRespawning = false;
	fl_RespawnTime = 10;
	fl_RespawnDelay = -1.0f;
	m_iszSpawnSound = MAKE_STRING( "Item.Materialize" );
	m_bHide = 1;
#ifdef OF_DLL
	m_flCooldown = -1;
#endif

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Spawn( void )
{
	m_bInitialDelay = true;
	BaseClass::Precache();
	BaseClass::Spawn();

	BaseClass::SetOriginalSpawnOrigin( GetAbsOrigin() );
	BaseClass::SetOriginalSpawnAngles( GetAbsAngles() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	PrecacheScriptSound( STRING( m_iszSpawnSound ) );
	if ( m_bDisabled )
	{
		SetDisabled( true );
	}
	
	if ( GetRespawnDelay() > 0.0f )
	{
		Respawn();
	}
	else
	{
		m_bInitialDelay = false;
		m_bRespawning = false;
	}
	ResetSequence( LookupSequence("idle") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* CTFPowerup::Respawn( void )
{
	m_bRespawning = true;
	CBaseEntity *pReturn = BaseClass::Respawn();
	
	// Override the respawn time
	if ( m_bInitialDelay )
		SetNextThink( gpGlobals->curtime + GetRespawnDelay() );
	else
		SetNextThink( gpGlobals->curtime + GetRespawnTime() );
	
	return pReturn;
}

float CTFPowerup::GetRespawnDelay( void )
{
	return fl_RespawnDelay;
}

float CTFPowerup::GetRespawnTime( void )
{
	return fl_RespawnTime;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Materialize( void )
{
	if ( !m_bDisabled )
	{
		// changing from invisible state to visible.
		m_nRenderFX = kRenderFxNone;
		RemoveEffects( EF_NODRAW );
	}
	m_OnRespawn.FireOutput( this, this );
	EmitSound( STRING( m_iszSpawnSound ) );
	m_bRespawning = false;
	m_bInitialDelay = false;
	SetTouch( &CItem::ItemTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::ValidTouch( CBasePlayer *pPlayer )
{
	// Is the item enabled?
	if ( IsDisabled() )
	{
		return false;
	}

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Team number and does it match?
	int iTeam = GetTeamNumber();
	if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
	{
		return false;
	}

#ifdef OF_DLL
	if( m_flCooldown != -1 && gpGlobals->curtime < m_flCooldown && pPlayer == GetOwnerEntity() )
		return false;
#endif

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );  // And the tf player,       Guess we could just use the tf player, no?

	if ( pTFPlayer && pTFPlayer->m_Shared.IsZombie() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::MyTouch( CBasePlayer *pPlayer )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerup::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
	if ( bDisabled )
	{
		AddEffects( EF_NODRAW );
	}
	else
	{
		// only turn it back on if we're not in the middle of respawning
		if ( !m_bRespawning )
		{
            RemoveEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used by TF2 to drop stuff like the sandvich or candy cane packs
//-----------------------------------------------------------------------------
#ifdef OF_DLL
void CTFPowerup::DropSingleInstance( const Vector &Velocity, CBaseCombatCharacter *pBCC, float flLifetime, float flTime, bool bUsePhysics /*= false*/ )
#else
void CTFPowerup::DropSingleInstance( const Vector &Velocity, CBaseCombatCharacter *pBCC, float flLifetime, float flTime )
#endif
{
#ifdef OF_DLL
	if( bUsePhysics )
	{
		CreateVPhysics();
		VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
	}
	else
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
#else
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
#endif
	AddSpawnFlags( SF_NORESPAWN );
	SetOwnerEntity( pBCC );
	SetAbsVelocity( Velocity );

	if ( flTime != 0.0f )
		m_flCooldown = gpGlobals->curtime + flTime;

	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + flLifetime, "DieContext" ); // Set its death time to whenever the powerup would have ran out
}