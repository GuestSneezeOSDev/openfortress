//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exploding bomb
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
-m_flDamage(Offset 1188) (Save | Key)(4 Bytes) - damage
- m_flRadius(Offset 1196) (Save | Key)(4 Bytes) - radius
- m_nHealth(Offset 1192) (Save | Key)(4 Bytes) - health
- m_strExplodeParticleName(Offset 1200) (Save | Key)(4 Bytes) - explode_particle
- m_strExplodeSoundName(Offset 1208) (Save | Key)(4 Bytes) - sound
- m_eWhoToDamage(Offset 1212) (Save | Key)(4 Bytes) - friendlyfire
- m_OnDetonate(Offset 1156) (Save | Key | Output)(0 Bytes) - OnDetonate
- Detonate(Offset 0) (Input)(0 Bytes) - Detonate
*/

#include "cbase.h"
#include "baseanimating.h"
#include "world.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_fx.h"
#include "particle_parse.h"
#include "te_particlesystem.h"

#include "tf_generic_bomb.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTFGenericBomb )	
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	// m_nHealth in TF2
	DEFINE_KEYFIELD( m_iHealth, FIELD_INTEGER, "health" ),
	DEFINE_KEYFIELD( m_strExplodeParticleName, FIELD_STRING, "explode_particle" ),
	DEFINE_KEYFIELD( m_strExplodeSoundName,	FIELD_SOUNDNAME, "sound" ),
	// fixme: this isn't a bool in TF2 - m_eWhoToDamage
	// probably a handle (?)
	DEFINE_KEYFIELD( m_eWhoToDamage, FIELD_BOOLEAN, "friendlyfire" ),
	DEFINE_OUTPUT( Detonate, "OnDetonate" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Detonate", InputDetonate ),
END_DATADESC()

//PRECACHE_REGISTER( tf_generic_bomb );

// CTFGenericBomb - tf_generic_bomb
LINK_ENTITY_TO_CLASS( tf_generic_bomb, CTFGenericBomb );

void CTFGenericBomb::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( STRING( m_strExplodeSoundName ) );
	PrecacheParticleSystem( STRING( m_strExplodeParticleName ) );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericBomb::CTFGenericBomb()
{
	SetMaxHealth( 1 );
	SetHealth( 1 );
	m_flDamage = 150.0f;
	m_flRadius = 100.0f;
	m_eWhoToDamage = false;
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericBomb::Spawn()
{
	char *szModel = (char *)STRING( GetModelName() );

	if (!szModel || !*szModel )
	{
		Warning( "tf_generic_bomb at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	Precache();

	int iModelIndex = PrecacheModel( szModel );
	PrecacheGibsForModel( iModelIndex );
	SetModel( szModel );

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();

	// force it here, because our base classes could have set it to DAMAGE_EVENTS_ONLY
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity /*= NULL*/, int iAttachType /*= PATTACH_CUSTOMORIGIN*/ )
void CTFGenericBomb::Event_Killed( const CTakeDamageInfo &info )
{
	SetSolid( SOLID_NONE ); 

	// grenade_ar2.cpp
	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( WorldSpaceCenter(), WorldSpaceCenter() + 60*vecForward , MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

	if ( STRING( m_strExplodeParticleName ) )
	{
		CPVSFilter filter( GetAbsOrigin() );
		TE_TFParticleEffect( filter, 0.0f, STRING( m_strExplodeParticleName ), GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );
	}

	if ( STRING( m_strExplodeSoundName ) )
		EmitSound( STRING( m_strExplodeSoundName ) );

	CBaseEntity *pAttacker = this;

	if ( info.GetAttacker() )
		pAttacker = info.GetAttacker();

	CTakeDamageInfo info2( this, pAttacker, m_flDamage, DMG_BLAST );

	if ( m_eWhoToDamage )
		// takedamageinfo.h
		info2.SetForceFriendlyFire( true );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	Detonate.FireOutput( this, this, 0.0f );

	BaseClass::Event_Killed( info2 );

	m_takedamage = DAMAGE_NO;

	TFGameRules()->RadiusDamage( info2, GetAbsOrigin(), m_flRadius, CLASS_NONE, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity /*= NULL*/, int iAttachType /*= PATTACH_CUSTOMORIGIN*/ )

// same as above but traces the activator as the damage owner
void CTFGenericBomb::InputDetonate( inputdata_t &inputdata )
{
	SetSolid( SOLID_NONE ); 

	// grenade_ar2.cpp
	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( WorldSpaceCenter(), WorldSpaceCenter() + 60*vecForward , MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

	if ( STRING( m_strExplodeParticleName ) )
	{
		CPVSFilter filter( GetAbsOrigin() );
		TE_TFParticleEffect( filter, 0.0f, STRING( m_strExplodeParticleName ), GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );
	}

	if ( STRING( m_strExplodeSoundName ) )
		EmitSound( STRING( m_strExplodeSoundName ) );

	CBaseEntity *pAttacker = this;

	if ( inputdata.pActivator )
		pAttacker = inputdata.pActivator;

	CTakeDamageInfo info( this, this, m_flDamage, DMG_BLAST );

	if ( m_eWhoToDamage )
		// takedamageinfo.h
		info.SetForceFriendlyFire( true );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	Detonate.FireOutput( this, this, 0.0f );

	BaseClass::Event_Killed( info );

	m_takedamage = DAMAGE_NO;

	TFGameRules()->RadiusDamage( info, GetAbsOrigin(), m_flRadius, CLASS_NONE, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// CTFPumpkinBomb - tf_pumpkin_bomb


BEGIN_DATADESC( CTFPumpkinBomb )	
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	// m_nHealth in TF2
	DEFINE_KEYFIELD( m_iHealth, FIELD_INTEGER, "health" ),
END_DATADESC()

//PRECACHE_REGISTER( tf_pumpkin_bomb );

// CTFPumpkinBomb - tf_generic_bomb
LINK_ENTITY_TO_CLASS( tf_pumpkin_bomb, CTFPumpkinBomb );

// ugly!
void CTFPumpkinBomb::Precache()
{
	const char *szModel = (char *)STRING( GetModelName() );

	szModel = "models/props_halloween/pumpkin_explode.mdl";
	SetModelName( AllocPooledString( szModel ) );

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( "Halloween.PumpkinExplode" );
	PrecacheParticleSystem( "pumpkin_explode" );

	BaseClass::Precache();

	int iModelIndex = PrecacheModel( szModel );
	PrecacheGibsForModel( iModelIndex );
	SetModel( szModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPumpkinBomb::CTFPumpkinBomb()
{
	SetMaxHealth( 1 );
	SetHealth( 1 );
	m_flDamage = 190.0f;
	m_flRadius = 300.0f;
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPumpkinBomb::Spawn()
{
	m_takedamage = DAMAGE_YES;

	Precache();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity /*= NULL*/, int iAttachType /*= PATTACH_CUSTOMORIGIN*/ )
void CTFPumpkinBomb::Event_Killed( const CTakeDamageInfo &info )
{
	SetSolid( SOLID_NONE ); 

	// grenade_ar2.cpp
	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( WorldSpaceCenter(), WorldSpaceCenter() + 60*vecForward , MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

	CPVSFilter filter( GetAbsOrigin() );

	TE_TFParticleEffect( filter, 0.0f, "pumpkin_explode", GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );

	EmitSound( "Halloween.PumpkinExplode" );

	CBaseEntity *pAttacker = this;

	if ( info.GetAttacker() )
		pAttacker = info.GetAttacker();

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	CTakeDamageInfo info2( this, pAttacker, m_flDamage, DMG_BLAST );

	BaseClass::Event_Killed( info2 );

	m_takedamage = DAMAGE_NO;

	TFGameRules()->RadiusDamage( info2, GetAbsOrigin(), m_flRadius, CLASS_NONE, NULL );
}