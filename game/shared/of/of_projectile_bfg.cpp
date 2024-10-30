//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_gamerules.h"
#include "tf_weaponbase.h"
#include "of_projectile_bfg.h"
#include "tf_weapon_flamethrower.h"

#define ROCKET_MODEL "models/weapons/w_models/w_bfg_hack.mdl"

// Server specific.
#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "iscorer.h"
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#include "baseanimating.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "ai_basenpc.h"
#include "collisionutils.h"
#else
#include "c_baseanimating.h"
#include "c_te_legacytempents.h"
#include "particles_new.h"
#include "dlight.h"
#include "iefx.h"
#endif

#ifdef CLIENT_DLL
extern ConVar of_muzzlelight;
#else
ConVar  of_bfg_boxsize("of_bfg_boxsize", "60", FCVAR_CHEAT, "Size of the AOE damage entity." );
ConVar  of_bfg_debug("of_bfg_debug", "0", FCVAR_CHEAT, "Visualize the BFG projectiles as boxes." );
ConVar  of_bfg_damage("of_bfg_damage", "25", FCVAR_CHEAT, "Damage the BFG does per tick." );
ConVar  of_bfg_tickrate("of_bfg_tickrate", "0.03", FCVAR_CHEAT, "Tickrate for AOE damage." );
#endif
//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBFGProjectile, DT_TFBFGProjectile )

LINK_ENTITY_TO_CLASS( tf_projectile_bfg, CTFBFGProjectile );
PRECACHE_REGISTER( tf_projectile_bfg );

BEGIN_NETWORK_TABLE( CTFBFGProjectile, DT_TFBFGProjectile )
// Client specific.
#ifdef CLIENT_DLL
RecvPropInt( RECVINFO( m_bSpawnTrails ) ),
RecvPropInt( RECVINFO( m_bCritical ) ),
RecvPropVector( RECVINFO( m_vInitialVelocity ) ),

RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

// Server specific.
#else
SendPropInt( SENDINFO( m_bSpawnTrails ) ),
SendPropInt( SENDINFO( m_bCritical ) ),
SendPropVector( SENDINFO( m_vInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),

SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

#endif
END_NETWORK_TABLE()
// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFBFGProjectile )
DEFINE_ENTITYFUNC( RocketTouch ),
DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()
#endif

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBFGProjectile::CTFBFGProjectile()
{
	m_vInitialVelocity.Init();

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = 0.0f;
		
// Server specific.
#else

	m_flDamage = 0.0f;
	m_flDamageRadius = (110.0f * 1.1f);

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBFGProjectile::~CTFBFGProjectile()
{
#ifdef CLIENT_DLL
ParticleProp()->StopEmission();
#else
	if( pOrb )
		pOrb->m_bRemove = true;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBFGProjectile::Spawn()
{
	BaseClass::Spawn();	
	SetModel( ROCKET_MODEL );
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetSolid( SOLID_BBOX );	
	SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );
#ifdef GAME_DLL
	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		pOrb = CTFBFGArea::Create( GetAbsOrigin(),pOwner->EyeAngles(), this, GetDamageType(), of_bfg_damage.GetFloat(), GetCustomDamageType() );
	}
	SetThink( &CTFBFGProjectile::FlyThink );
#endif

}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFBFGProjectile::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType | DMG_ALWAYSGIB;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFBFGProjectile::GetCustomDamageType() 
{ 
	if ( m_bCritical >= 2)
	{
		return TF_DMG_CUSTOM_CRIT_POWERUP;
	}
	else
	{
		return TF_DMG_CUSTOM_NONE;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBFGProjectile *CTFBFGProjectile::Create ( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flSpeed )
{
	CTFBFGProjectile *pRocket = static_cast<CTFBFGProjectile*>( CTFBaseRocket::Create( pWeapon, "tf_projectile_bfg", vecOrigin, vecAngles, pOwner, flSpeed ) );
	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
		UTIL_SetSize( pRocket, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBFGProjectile::Precache()
{
	PrecacheModel( ROCKET_MODEL );
	PrecacheParticleSystem( "critical_rocket_blue" );
	PrecacheParticleSystem( "critical_rocket_red" );
	PrecacheParticleSystem( "mlg_trail_primary_dm" );
	PrecacheParticleSystem( "mlg_trail_primary_blue" );
	PrecacheParticleSystem( "mlg_trail_primary_red" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBFGProjectile::RocketTouch( CBaseEntity *pOther )
{
	BaseClass::RocketTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBFGProjectile::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFBFGProjectile::PhysicsSolidMaskForEntity( void ) const
{ 
	return CBaseAnimating::PhysicsSolidMaskForEntity();
}

void CTFBFGProjectile::FlyThink( void )
{
	// Render debug visualization
	if ( of_bfg_debug.GetInt() )
	{
		NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
	}
	if( m_bSpawnTrails )
		m_bSpawnTrails = false;
	else
		m_bSpawnTrails = true;
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFBFGProjectile::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}
#else
//-----------------------------------------------------------------------------	
//-----------------------------------------------------------------------------
// CLIENTSIDE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CTFBFGProjectile::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateRocketTrails();	
	}
}

const char *CTFBFGProjectile::GetTrailParticleName(void)
{
	switch (GetTeamNumber())
	{
	default:
	case TF_TEAM_RED:
		return "mlg_trail_primary_red";
		break;
	case TF_TEAM_BLUE:
		return "mlg_trail_primary_blue";
		break;
	case TF_TEAM_MERCENARY:
		return "mlg_trail_primary_dm";
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBFGProjectile::CreateRocketTrails(void)
{
	if (IsDormant())
		return;
	
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return;
	
	pPlayer->m_Shared.UpdateParticleColor ( ParticleProp()->Create(GetTrailParticleName(), PATTACH_POINT_FOLLOW, "trail") );
	
/*
	The lightning Ball on its own is horribly optimized, decided that we should merge it with the trail particle
	switch (GetTeamNumber())
	{
		case TF_TEAM_BLUE:
			ParticleProp()->Create("dxhr_lightningball_parent_blue", PATTACH_POINT_FOLLOW, "trail");
			break;
		case TF_TEAM_RED:
			ParticleProp()->Create("dxhr_lightningball_parent_red", PATTACH_POINT_FOLLOW, "trail");
			break;
		case TF_TEAM_MERCENARY:
			pPlayer->m_Shared.UpdateParticleColor (ParticleProp()->Create("dxhr_lightningball_parent_dm", PATTACH_POINT_FOLLOW, "trail") );
			break;
		default:
			break;
	}	
*/
	if (m_bCritical)
	{
		switch (GetTeamNumber())
		{
		case TF_TEAM_BLUE:
			ParticleProp()->Create("critical_rocket_blue", PATTACH_POINT_FOLLOW, "trail");
			break;
		case TF_TEAM_RED:
			ParticleProp()->Create("critical_rocket_red", PATTACH_POINT_FOLLOW, "trail");
			break;
		case TF_TEAM_MERCENARY:
			pPlayer->m_Shared.UpdateParticleColor (ParticleProp()->Create("critical_rocket_dm", PATTACH_POINT_FOLLOW, "trail") );
			break;
		default:
			break;
		}
	}

}

void CTFBFGProjectile::CreateLightEffects(void)
{
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	// Handle the dynamic light
	if (of_muzzlelight.GetBool())
	{
		AddEffects(EF_DIMLIGHT);

		dlight_t *dl;
		if (IsEffectActive(EF_DIMLIGHT))
		{
			dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + index);
			dl->origin = GetAbsOrigin();
			dl->color.r = 255;
			dl->color.g = 100;
			dl->color.b = 10;
			dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			switch (GetTeamNumber())
			{
			case TF_TEAM_RED:
				if (!m_bCritical) {
					dl->color.r = 255; dl->color.g = 100; dl->color.b = 10;
				}
				else {
					dl->color.r = 255; dl->color.g = 10; dl->color.b = 10;
				}
				break;

			case TF_TEAM_BLUE:
				if (!m_bCritical) {
					dl->color.r = 255; dl->color.g = 100; dl->color.b = 10;
				}
				else {
					dl->color.r = 10; dl->color.g = 10; dl->color.b = 255;
				}
				break;

			case TF_TEAM_MERCENARY:
				if (!m_bCritical) {
					dl->color.r = 255; dl->color.g = 100; dl->color.b = 10;
				}
				else {
					float r = pPlayer->m_vecPlayerColor.x * 255;
					float g = pPlayer->m_vecPlayerColor.y * 255;
					float b = pPlayer->m_vecPlayerColor.z * 255;
					if ( r < TF_LIGHT_COLOR_CLAMP && g < TF_LIGHT_COLOR_CLAMP && b < TF_LIGHT_COLOR_CLAMP )
					{
						float maxi = max(max(r, g), b);
						maxi = TF_LIGHT_COLOR_CLAMP - maxi;
						r += maxi;
						g += maxi;
						b += maxi;
					}
					dl->color.r = r; dl->color.g = g ; dl->color.b = b;
				}
				break;
			}
			dl->die = gpGlobals->curtime + 0.01f;
			dl->radius = 340.f;
			dl->decay = 512.0f;

			tempents->RocketFlare(GetAbsOrigin());
		}
	}
}

#endif

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( tf_bfg_radial, CTFBFGArea );

//-----------------------------------------------------------------------------
// Purpose: Spawns this entity
//-----------------------------------------------------------------------------
void CTFBFGArea::Spawn( void )
{
	BaseClass::Spawn();

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	// move noclip: update position from velocity, that's it
	SetMoveType( MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	float iBoxSize = of_bfg_boxsize.GetFloat();
	UTIL_SetSize( this, -Vector( iBoxSize, iBoxSize, iBoxSize ), Vector( iBoxSize, iBoxSize, iBoxSize ) );
	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	m_vecInitialPos = GetAbsOrigin();
	m_vecPrevPos = m_vecInitialPos;
	m_bRemove = false;
	// Setup the think function.
	SetThink( &CTFBFGArea::FlameThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Creates an instance of this entity
//-----------------------------------------------------------------------------
CTFBFGArea *CTFBFGArea::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float flDmgAmount, int iCustomDmgType )
{
	CTFBFGArea *pFlame = static_cast<CTFBFGArea*>( CBaseEntity::Create( "tf_bfg_radial", vecOrigin, vecAngles, pOwner ) );
	if ( !pFlame )
		return NULL;

	// Initialize the owner.
	pFlame->SetOwnerEntity( pOwner );
	pFlame->FollowEntity( pOwner, false );		// to the player that picked up the powerup
	pFlame->m_hAttacker = pOwner->GetOwnerEntity();
	CBaseEntity *pAttacker = (CBaseEntity *) pFlame->m_hAttacker;
	if ( pAttacker )
	{
		pFlame->m_iAttackerTeam =  pAttacker->GetTeamNumber();
	}

	pFlame->ChangeTeam( pOwner->GetTeamNumber()  );
	pFlame->m_iDmgType = ( iDmgType | DMG_PREVENT_PHYSICS_FORCE ) &~ DMG_USEDISTANCEMOD;
	pFlame->m_iCustomDmgType = iCustomDmgType;
	pFlame->m_flDmgAmount = flDmgAmount;
	// Setup the initial angles.
	pFlame->SetAbsAngles( vecAngles );
	
	return pFlame;
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFBFGArea::FlameThink( void )
{
	// if we've expired, remove ourselves
	if ( m_bRemove )
	{
		UTIL_Remove( this );
		return;
	}

	// Do collision detection.  We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server
	if (GetAbsOrigin() != m_vecPrevPos)
	{
		CTFPlayer *pAttacker = dynamic_cast<CTFPlayer *>((CBaseEntity *)m_hAttacker);
		if (!pAttacker)
			return;

		CBaseEntity *pList[64];

		int iCount = UTIL_EntitiesInSphere(pList, 64, GetAbsOrigin(), of_bfg_boxsize.GetFloat(), NULL);

		for (int i = 0; i < iCount; i++)
		{
			CBaseEntity *pEntity = pList[i];
			if (!pEntity)
				continue;

			if( pEntity->IsPlayer() || pEntity->IsBaseObject() )
			{

				if( !TFGameRules()->CanEntityHitEntity( this, pEntity ) )
					continue;

				if( pEntity == pAttacker )
					continue;

				if( pEntity->GetOwnerEntity() == pAttacker )
					continue;

				if( !pEntity->IsAlive() )
					continue;

				Ray_t ray;
				ray.Init(GetAbsOrigin(), pEntity->GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs());

				// if bounding box check passes, check player hitboxes
				trace_t trHitbox;
				trace_t trWorld;
				bool bTested = pEntity->GetCollideable()->TestHitboxes(ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox);
				if (!bTested || !trHitbox.DidHit())
					continue;

				// now, let's see if the visual could have actually hit this player.  Trace backward from the
				// point of impact to where the BFG was fired, see if we hit anything.  Since the point of impact was
				// determined using the BFG's bounding box and we're just doing a ray test here, we extend the
				// start point out by the radius of the box.
				Vector vDir = ray.m_Delta;
				vDir.NormalizeInPlace();
				UTIL_TraceLine(GetAbsOrigin(), pEntity->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld);

				if (of_bfg_debug.GetInt())
				{
					NDebugOverlay::Line(trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f);
				}

				if (trWorld.fraction == 1.0)
				{
					// if there is nothing solid in the way, damage the entity
					OnCollide(pEntity);
				}
			}
		}
	}

	// Render debug visualization if convar on
	if ( of_bfg_debug.GetInt() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int) ( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		}
		else 
		{
			NDebugOverlay::Sphere(GetAbsOrigin(), of_bfg_boxsize.GetFloat(), 0, 255, 0, false, of_bfg_tickrate.GetFloat());
		}
	}

	SetNextThink( gpGlobals->curtime + of_bfg_tickrate.GetFloat() );

	m_vecPrevPos = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities
//-----------------------------------------------------------------------------
void CTFBFGArea::CheckCollision( CBaseEntity *pOther, bool *pbHitWorld )
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFBFGArea::OnCollide( CBaseEntity *pOther )
{

	float flDamage = m_flDmgAmount;
	flDamage = max( flDamage, 1.0 );
	if ( of_bfg_debug.GetInt() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	CBaseEntity *pAttacker = m_hAttacker;
	if ( !pAttacker )
		return;

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, flDamage, m_iDmgType, m_iCustomDmgType );
	info.SetReportedPosition( pAttacker->GetAbsOrigin() );

	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &pTrace );

	pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );
	ApplyMultiDamage();

	// please please work
	CAI_BaseNPC *pNPC = pOther->MyNPCPointer();
	if (pNPC)
	{
		// todo figure out how to find attacker with this
		pNPC->Ignite(TF_BURNING_FLAME_LIFE);
	}
}

#endif // GAME_DLL