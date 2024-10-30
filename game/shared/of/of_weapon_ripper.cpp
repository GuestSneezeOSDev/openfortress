#include "cbase.h"
#include "of_weapon_ripper.h"
#include "tf_gamerules.h"
#include "collisionutils.h"

#define SAWBLADE_MODEL	"models/weapons/w_models/w_saw_ballista_projectile.mdl"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Ripper, DT_TFProjectile_Ripper )

BEGIN_NETWORK_TABLE( CTFProjectile_Ripper, DT_TFProjectile_Ripper )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_sawblade, CTFProjectile_Ripper );

ConVar of_ripper_prop_lifetime( "of_ripper_prop_lifetime", "5.0", FCVAR_REPLICATED, "How long the blade stuck in the wall lives for before fading", true, 0.0f, false, 0.0f );
ConVar of_ripper_projectile_lifetime("of_ripper_projectile_lifetime", "1.0", FCVAR_REPLICATED, "How long the blade exist without caring about number of bounces", true, 0.0f, false, 0.0f);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFProjectile_Ripper::~CTFProjectile_Ripper()
{
#if defined( CLIENT_DLL )
	ParticleProp()->StopEmission();
#endif
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( IsDormant() )
			return;
		
		ParticleProp()->Create( "warp_version", PATTACH_ABSORIGIN_FOLLOW );

		C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( !pPlayer )
			return;

		const char *pszTeamName;
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				pszTeamName = "red";
				break;
			case TF_TEAM_BLUE:
				pszTeamName = "blue";
				break;
			default:
				pszTeamName = "dm";
				break;
		}

		char pszParticleEffect[32];
		Q_snprintf( pszParticleEffect, sizeof( pszParticleEffect ), "demo_charge_root_%s", pszTeamName );
		pPlayer->m_Shared.UpdateParticleColor( ParticleProp()->Create( pszParticleEffect, PATTACH_ABSORIGIN_FOLLOW ) );

		if ( m_bCritical )
		{
			switch ( GetTeamNumber() )
			{
				case TF_TEAM_BLUE:
					ParticleProp()->Create( "critical_rocket_blue", PATTACH_ABSORIGIN_FOLLOW );
					break;
				case TF_TEAM_RED:
					ParticleProp()->Create( "critical_rocket_red", PATTACH_ABSORIGIN_FOLLOW );
					break;
				case TF_TEAM_MERCENARY:
					pPlayer->m_Shared.UpdateParticleColor( ParticleProp()->Create( "critical_rocket_dm", PATTACH_ABSORIGIN_FOLLOW ) );
					break;
				case TF_TEAM_NPC:
					ParticleProp()->Create( "eyeboss_projectile", PATTACH_ABSORIGIN_FOLLOW );
					break;
				default:
					break;
			}
		}
	}
}
#endif

#ifdef GAME_DLL
ConVar  of_ripper_slowed_speed_perc("of_ripper_slowed_speed_perc", "0.05", FCVAR_NOTIFY | FCVAR_REPLICATED, "Percenge for how much slowdown is applied when the saw ballista's blades are touching someone.");
ConVar  of_ripper_bounces("of_ripper_bounces", "6", FCVAR_NOTIFY | FCVAR_REPLICATED, "How many times the saw can bounce.");
ConVar  of_ripper_debug("of_ripper_debug", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Visualize the ripper projectiles as boxes.");
ConVar  of_ripper_boxsize("of_ripper_boxsize", "50", FCVAR_NOTIFY | FCVAR_REPLICATED, "Withd of the AOE damage from Saw Blades.");
ConVar  of_ripper_boxsize_height("of_ripper_boxsize_height", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "Height of the AOE damage from Saw Blades.");
#endif

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
CTFProjectile_Ripper *CTFProjectile_Ripper::Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flSpeed )
{
	CTFProjectile_Ripper *pRocket = static_cast<CTFProjectile_Ripper *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_sawblade", vecOrigin, vecAngles, pOwner, flSpeed ) );
	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );

		pRocket->SetThink( &CTFProjectile_Ripper::FlyThink );
		pRocket->m_flHalvedSpeed = (flSpeed * of_ripper_slowed_speed_perc.GetFloat());
		pRocket->m_flNormalSpeed = flSpeed;
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::Spawn()
{

	m_flRipperSpawnTime = gpGlobals->curtime;

	m_nNumBounces = 0;
	m_flNextBounce = gpGlobals->curtime;
	BaseClass::Spawn();

	CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase*>(m_hOriginalLauncher.Get());
	if (pWeapon && pWeapon->GetTFWpnData().m_nProjectileModel[0] != 0)
	{
		const char* s_SawBladeModel = pWeapon->GetTFWpnData().m_nProjectileModel;
		if (s_SawBladeModel)
		{
			PrecacheModel(s_SawBladeModel);
			SetModel(s_SawBladeModel);
		}
	}
	else
	{
		PrecacheModel(SAWBLADE_MODEL);
		SetModel(SAWBLADE_MODEL);
	}

	SetModelScale( 2.0 );

	SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );
	SetSolidFlags( FSOLID_TRIGGER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::Precache()
{
	PrecacheScriptSound( "Weapon_SawBallista.Ricochet" );
	PrecacheScriptSound( "Weapon_SawBallista.Hit" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::FlyThink( void )
{
	Vector vecVelocity = GetAbsVelocity();

	if( of_ripper_debug.GetInt() )
	{
		NDebugOverlay::EntityBounds(this, 0, 100, 255, 0, 0);
	}

	trace_t tr;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecVelocity * TICK_INTERVAL, {-8.0f, -8.0f, -1.0f}, {8.0f, 8.0f, 1.0f}, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, this, TFCOLLISION_GROUP_ROCKETS, &tr );
	if ( tr.DidHit() )
	{
		PhysicsMarkEntitiesAsTouching( tr.m_pEnt, tr );
	}
	
	QAngle vecAngles = GetAbsAngles();
	// Spin about the Z axis in flight
	//vecAngles.y += 250.0 * TICK_INTERVAL;
	//SetAbsAngles( vecAngles );

	CTFPlayer* pAttacker = dynamic_cast<CTFPlayer*>((CBaseEntity*)GetOwnerEntity());
	if( !pAttacker )
	{
		SetNextThink( gpGlobals->curtime + 0.03);
		return;
	}
	Vector BaseVector = GetAbsOrigin();
	Vector mins, maxs;
	int iHitbox = of_ripper_boxsize.GetInt();
	int iHitboxHeight = of_ripper_boxsize_height.GetInt();

	Vector expand(iHitbox, iHitbox, iHitboxHeight);

	mins = BaseVector - expand;
	maxs = BaseVector + expand;

	CBaseEntity* pList[64];

	//int iCount = UTIL_EntitiesInSphere(pList, 64, GetAbsOrigin(), of_ripper_boxsize.GetFloat(), NULL);
	int iCount = UTIL_EntitiesInBox(pList, 64, mins, maxs, NULL);

	if (of_ripper_debug.GetInt())
	{
		//NDebugOverlay::Sphere(GetAbsOrigin(), of_ripper_boxsize.GetFloat(), 255, 0, 0, false, 0);
		//Vector BaseVector = GetAbsOrigin();
		//Vector mins1, maxs1;
		//Vector expand(4, 4, 4);

		//mins1 = BaseVector -= expand;
		//maxs1 = BaseVector += expand;
		Vector negative(
			(iHitbox * -1), (iHitbox * -1), (iHitboxHeight * -1)
		);
		NDebugOverlay::Box(GetAbsOrigin(), negative, expand, 255, 0, 0, false, 0);
		//NDebugOverlay::Box(GetAbsOrigin(), , 255, 255, 255, false, 0);
	}

	for (int i = 0; i < iCount; i++)
	{
		CBaseEntity* pEntity = pList[i];

		if( !pEntity )
			continue;

		if( !pEntity->IsPlayer() && !pEntity->IsBaseObject() )
			continue;

		//DevMsg("our entity count is: %i\n", iCount);

		bool bHitEntityIsOurOwner = pEntity == pAttacker || pEntity->GetOwnerEntity() == pAttacker;

		if( pEntity->IsPlayer() || pEntity->IsBaseObject() )
		{
			if( !TFGameRules()->CanEntityHitEntity(this, pEntity) )
				continue;

			if( !pEntity->IsAlive() )
				continue;

			if( bHitEntityIsOurOwner )
			{
				if( m_nNumBounces < 1 )
				{
					continue;
				}
			}

			Ray_t ray;
			ray.Init(GetAbsOrigin(), pEntity->GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs());

			// if bounding box check passes, check player hitboxes
			trace_t trHitbox;
			trace_t trWorld;
			bool bTested = pEntity->GetCollideable()->TestHitboxes( ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox );
			if( !bTested || !trHitbox.DidHit() )
				continue;
		}

		Ray_t ray;
		ray.Init(GetAbsOrigin(), pEntity->GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs());

		// if bounding box check passes, check player hitboxes
		trace_t trHitbox;
		trace_t trWorld;
		bool bTested = pEntity->GetCollideable()->TestHitboxes(ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox);
		if (!bTested || !trHitbox.DidHit())
			continue;

		// now, let's see if the visual could have actually hit this player.  Trace backward from the
		// point of impact to where the ripper was fired, see if we hit anything.  Since the point of impact was
		// determined using the ripper's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine(GetAbsOrigin(), pEntity->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld);


		if (trWorld.fraction == 1.0)
		{
			Vector vecOrigin = GetAbsOrigin();

			int iDamageType = GetDamageType();
			int iDamageCustom = GetCustomDamageType();

			CTakeDamageInfo info(this, pAttacker, GetOriginalLauncher(), vec3_origin, vecOrigin, GetDamage(), iDamageType, iDamageCustom);
			pEntity->TakeDamage(info);
		}

		if( !m_bInside )
		{
			EmitSound("Weapon_SawBallista.SawFlesh");

			if( !bHitEntityIsOurOwner )
			{
				//DevMsg("We are touching something.\n");
				Vector vecForward;
				AngleVectors(vecAngles, &vecForward);

				Vector vecVelocity = vecForward * m_flHalvedSpeed;
				SetAbsVelocity(vecVelocity);
				SetupInitialTransmittedGrenadeVelocity(vecVelocity);

				m_bInside = true;
			}
		}
	}

	/*
	if (iCount <= 1)
	{
		if (m_bInside)
		{
			Vector vecForward;
			AngleVectors(vecAngles, &vecForward);

			Vector vecVelocity = vecForward * m_flNormalSpeed;
			SetAbsVelocity(vecVelocity);
			SetupInitialTransmittedGrenadeVelocity(vecVelocity);

			//DevMsg("We are no longer touching something.\n");
			m_bInside = false;
		}

	}
	*/

	// Why is the inside check done twice?
	if( m_bInside )
	{
		if (iCount == 1)
		{
			Vector vecForward;
			AngleVectors(vecAngles, &vecForward);

			Vector vecVelocity = vecForward * m_flNormalSpeed;
			SetAbsVelocity(vecVelocity);
			SetupInitialTransmittedGrenadeVelocity(vecVelocity);

			//DevMsg("We are only finding ourselves, set to false.\n");
			m_bInside = false;
		}
		else
		{
			//CheckIfInside(pList[i], iCount);
			if (!(CheckIfInside()))
			{
				Vector vecForward;
				AngleVectors(vecAngles, &vecForward);

				Vector vecVelocity = vecForward * m_flNormalSpeed;
				SetAbsVelocity(vecVelocity);
				SetupInitialTransmittedGrenadeVelocity(vecVelocity);

				//DevMsg("We ain't touching anything that is a player.\n");
				m_bInside = false;
			}
		}
	}

	SetNextThink( gpGlobals->curtime + 0.03 );
}

//bool CTFProjectile_Ripper::CheckIfInside( CBaseEntity *pList, int iCount)
bool CTFProjectile_Ripper::CheckIfInside( void )
{
	CBaseEntity* pList[64];

	int iCount = UTIL_EntitiesInSphere(pList, 64, GetAbsOrigin(), 64, NULL);

	for (int i = 0; i < iCount; i++)
	{
		CBaseEntity* pEntity = pList[i];

		if (!pEntity)
			continue;

		//DevMsg("our entity count is: %i\n", iCount);

		if (pEntity->IsPlayer() || pEntity->IsBaseObject())
		{
			if (pEntity->IsAlive())
			{
				return true;
			}
			continue;
		}
	}

	return false;
}
/* This isn't needed. We do not check to see where the weapon touches
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFProjectile_Ripper::CheckHitbox( CBaseEntity *pOther, int nHitbox )
{
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>( pOther );
	if ( !pAnimating )
	{
		return HITGROUP_GENERIC;
	}

	CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
	if ( !pStudioHdr )
	{
		return HITGROUP_GENERIC;
	}

	mstudiohitboxset_t *pSet = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
	if ( !pSet )
	{
		return HITGROUP_GENERIC;
	}

	// Find the closest hitbox we crossed
	float flClosest = 99999.f;
	Vector vecArrowEnd = GetAbsOrigin() + GetAbsVelocity() * TICK_INTERVAL;
	mstudiobbox_t *pBox = NULL, *pCurrent = NULL;
	for ( int i = 0; i < pSet->numhitboxes; i++ )
	{
		pCurrent = pSet->pHitbox( i );

		Vector boxPosition;
		QAngle boxAngles;
		pAnimating->GetBonePosition( pCurrent->bone, boxPosition, boxAngles );

		Ray_t ray;
		ray.Init( vecArrowEnd, boxPosition );

		trace_t trace;
		IntersectRayWithBox( ray, boxPosition + pCurrent->bbmin, boxPosition + pCurrent->bbmax, 0, &trace );

		float flDistance = ( trace.endpos - vecArrowEnd ).Length();
		if ( flDistance < flClosest )
		{
			pBox = pCurrent;
			flClosest = flDistance;
		}
	}

	if ( pBox )
	{
		return pBox->group;
	}

	return HITGROUP_GENERIC;
}
*/
void CTFProjectile_Ripper::Bounce( void )
{
	const trace_t* pTrace = &CBaseEntity::GetTouchTrace();

	if (m_nNumBounces > of_ripper_bounces.GetInt() && ((m_flRipperSpawnTime + of_ripper_projectile_lifetime.GetFloat()) <= gpGlobals->curtime))
	{
		CBaseAnimating* pProp = static_cast<CBaseAnimating*>(CreateEntityByName("prop_dynamic"));
		if (pProp)
		{
			Vector vecOrigin = GetAbsOrigin();
			QAngle vecAngles = GetAbsAngles();

			Vector vecDir;
			AngleVectors(vecAngles, &vecDir);

			vecOrigin += vecDir * (CollisionProp()->OBBMaxs() * 0.667f);

			CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase*>(m_hOriginalLauncher.Get());
			if (pWeapon && pWeapon->GetTFWpnData().m_nProjectileModel[0] != 0)
			{
				const char* s_SawBladeModel = pWeapon->GetTFWpnData().m_nProjectileModel;
				if (s_SawBladeModel)
				{
					pProp->SetModel(s_SawBladeModel);
				}
			}
			else
			{
				pProp->SetModel(SAWBLADE_MODEL);
			}

			pProp->SetModelScale(2.0);
			pProp->SetCollisionGroup(COLLISION_GROUP_DEBRIS); // Collide with world only
			pProp->SetMoveType(MOVETYPE_NONE);

			DispatchSpawn(pProp);
			pProp->Teleport(&vecOrigin, &vecAngles, NULL);

			// Kill ourselves later
			pProp->SetThink(&CBaseEntity::SUB_FadeOut);
			pProp->SetNextThink(gpGlobals->curtime + of_ripper_prop_lifetime.GetFloat());
		}

		UTIL_Remove(this);
		return;	// Poof
	}

	if (m_flNextBounce > gpGlobals->curtime)
		return;

	m_flNextBounce = gpGlobals->curtime + 0.05f; // Dumb
	if ( (m_flRipperSpawnTime + of_ripper_projectile_lifetime.GetFloat()) < gpGlobals->curtime )
	{
		m_nNumBounces++;
	}
	else if (m_nNumBounces == 0)
	{
		m_nNumBounces++;
		//Only here to make so after the bounce u can be hurt by it.
	}

	// Reflect with total elasticity
	Vector vecVelocity = GetAbsVelocity();
	vecVelocity -= 2 * vecVelocity.Dot(pTrace->plane.normal) * pTrace->plane.normal;
	SetAbsVelocity(vecVelocity);

	QAngle angForward;
	VectorAngles( vecVelocity, angForward );
	SetAbsAngles( angForward );

	EmitSound("Weapon_SawBallista.Ricochet");
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::RocketTouch( CBaseEntity *pOther )
{
	if ( pOther == NULL )
		return;
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS ) )
		return;

	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;	// Poof
	}

	if ( pTrace->DidHitWorld() )
	{
		Bounce();
	}
	else
	{
		if (pOther)
		{
			if (!(pOther->IsPlayer()))
			{
				Bounce();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Ripper::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Ripper::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFProjectile_Ripper::GetDamageType()
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
int CTFProjectile_Ripper::GetCustomDamageType()
{
	if ( m_bCritical >= 2 )
	{
		return TF_DMG_CUSTOM_CRIT_POWERUP;
	}
	
	return TF_DMG_CUSTOM_NONE;
}
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponSawbladeLauncher, DT_TFWeaponSawbladeLauncher )

BEGIN_NETWORK_TABLE( CTFWeaponSawbladeLauncher, DT_TFWeaponSawbladeLauncher )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponSawbladeLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_ripper, CTFWeaponSawbladeLauncher );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponSawbladeLauncher::CTFWeaponSawbladeLauncher()
{
	m_bReloadsSingly = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponSawbladeLauncher::Precache()
{
	BaseClass::Precache();
}
