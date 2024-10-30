
//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_rocket.h"

//=============================================================================
//
// TF Rocket functions (Server specific).
//
#define ROCKET_MODEL "models/weapons/w_models/w_rocket.mdl"

LINK_ENTITY_TO_CLASS( tf_projectile_rocket, CTFProjectile_Rocket );
PRECACHE_REGISTER( tf_projectile_rocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( CTFProjectile_Rocket, DT_TFProjectile_Rocket )
	SendPropInt( SENDINFO( m_bCritical ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Rocket *CTFProjectile_Rocket::Create ( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flSpeed )
{
	CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CTFBaseRocket::Create( pWeapon, "tf_projectile_rocket", vecOrigin, vecAngles, pOwner, flSpeed ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Spawn()
{
	SetModel( ROCKET_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Precache()
{
	PrecacheModel( ROCKET_MODEL );
	PrecacheParticleSystem( "critical_rocket_blue" );
	PrecacheParticleSystem( "critical_rocket_red" );
	PrecacheParticleSystem( "rockettrail" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Rocket::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Rocket::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Rocket::GetCustomDamageType() 
{ 
	if ( m_bCritical >= 2)
	{
		return TF_DMG_CUSTOM_CRIT_POWERUP;
	}
	else
	{
		if (WasDefelected())
		{
			return TF_DMG_CUSTOM_REFLECT;
		}
		
		return TF_DMG_CUSTOM_NONE;
	}
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFCProjectile_IncendRocket *CTFCProjectile_IncendRocket::Create ( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flSpeed )
{
	CTFCProjectile_IncendRocket *pRocket = static_cast<CTFCProjectile_IncendRocket*>( CTFBaseRocket::Create( pWeapon, "tfc_projectile_incendrocket", vecOrigin, vecAngles, pOwner, flSpeed ) );

	if ( pRocket )
		pRocket->SetScorer( pScorer );

	return pRocket;
}

LINK_ENTITY_TO_CLASS( tfc_projectile_incendrocket, CTFCProjectile_IncendRocket );
PRECACHE_REGISTER( tfc_projectile_incendrocket );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(tf_projectile_bouncyrocket, CTFProjectile_BouncyRocket);
PRECACHE_REGISTER(tf_projectile_bouncyrocket);

IMPLEMENT_NETWORKCLASS_ALIASED(TFProjectile_BouncyRocket, DT_TFProjectile_BouncyRocket)

BEGIN_NETWORK_TABLE(CTFProjectile_BouncyRocket, DT_TFProjectile_BouncyRocket)
	SendPropTime( SENDINFO( m_flDetTime ) ),
END_NETWORK_TABLE()

#define BOUNCYROCKET_SPEED flSpeed //750.0f
#define BOUNCYROCKET_MODEL "models/weapons/w_models/w_grenade_bouncer.mdl"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_BouncyRocket::CTFProjectile_BouncyRocket()
{
	szImpactHard[0] = '\0';
	szImpactSoft[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_BouncyRocket::GetDamageType() 
{ 
	return BaseClass::GetDamageType()  &~ DMG_USEDISTANCEMOD;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_BouncyRocket *CTFProjectile_BouncyRocket::Create(CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flSpeed, float creationTime, Vector velocity)
{
	CTFProjectile_BouncyRocket *pRocket = static_cast<CTFProjectile_BouncyRocket*>(CTFBaseRocket::Create(pWeapon, "tf_projectile_bouncyrocket", vecOrigin, vecAngles, pOwner, flSpeed));
	if (!pRocket)
		return NULL;

	//set movement parameters
	pRocket->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM);
	pRocket->SetElasticity(0.9f);

	if (creationTime)
	{
		pRocket->creationTime = creationTime;
		pRocket->SetAbsVelocity(velocity);
	}
	else
	{
		//adjust angle velocity slightly upward
		Vector vel = pRocket->GetAbsVelocity();
		VectorNormalize(vel);
		QAngle angle;
		VectorAngles(vel, angle);
		angle.x -= 15.f;
		if (angle.x < 270.f && angle.x > 180.f)
			angle.x = 270.f;
		AngleVectors(angle, &vel);
		pRocket->SetAbsVelocity(vel * BOUNCYROCKET_SPEED);
	}

	pRocket->SetThink(&CTFProjectile_BouncyRocket::FlyThink);

	return pRocket;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::Precache(void)
{
	PrecacheModel(BOUNCYROCKET_MODEL);
	PrecacheParticleSystem("stickybombtrail_blue");
	PrecacheParticleSystem("stickybombtrail_red");
	PrecacheParticleSystem("stickybombtrail_dm");
	PrecacheScriptSound("BouncerGrenade.ImpactHard");
	PrecacheScriptSound("BouncerGrenade.ImpactSoft");
	CTFBaseRocket::Precache();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::Spawn(void)
{
	SetModel(BOUNCYROCKET_MODEL);
	Vector angle = Vector(RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f));
	VectorNormalize(angle);
	m_rotationVector = QAngle(angle.x, angle.y, angle.z) * 20.f;
	CTFBaseRocket::Spawn();
	
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );

	surfacedata_t *psurf = physprops->GetSurfaceData(physprops->GetSurfaceIndex(GetModelPtr()->pszSurfaceProp()));
	if( psurf )
	{
		Q_strncpy(szImpactHard, physprops->GetString(psurf->sounds.impactHard), sizeof(szImpactHard));
		Q_strncpy(szImpactSoft, physprops->GetString(psurf->sounds.impactSoft), sizeof(szImpactSoft));
	}	
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::FlyThink(void)
{
	if (creationTime)
	{
		m_flCreationTime = creationTime;
		creationTime = 0.f;
	}

	if (gpGlobals->curtime > m_flCollideWithTeammatesTime && m_bCollideWithTeammates == false)
		m_bCollideWithTeammates = true;

	//expiration time
	if( gpGlobals->curtime > m_flDetTime )
		Detonate();

	//rotation
	QAngle angles = GetAbsAngles() + m_rotationVector;
	SetAbsAngles(angles);

	//Water behavior
	int waterLevel = GetWaterLevel();
	Vector Vel = GetAbsVelocity();
	if (waterLevel > WL_Waist)
	{
		SetGravity(0.5f);

		if ( waterLevel != m_iOldWaterLevel )
			SetAbsVelocity( Vel / 2.f );
		else if ( Vector2D(Vel.x, Vel.y).Length() > 10.f )
			SetAbsVelocity( Vector( Vel.x * 0.8f, Vel.y * 0.8f, Vel.z ) );
	}
	else
		SetGravity(1.5f);

	m_iOldWaterLevel = waterLevel;
	SetNextThink(gpGlobals->curtime + 0.05f);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::BounceSound(const short data)
{
	if( szImpactHard[0] == '\0' || szImpactSoft[0] == '\0' )
		return;

	if (data == 14) //wooden surface
		EmitSound(szImpactSoft);
	else
		EmitSound(szImpactHard);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BouncyRocket::RocketTouch(CBaseEntity *pOther)
{
	if (pOther && pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS))
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if (pTrace->surface.flags & SURF_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	if (pTrace->m_pEnt->IsWorld())
	{
		if (GetAbsVelocity().Length() > 200.f)
		{
			Vector vecAbsVelocity = GetAbsVelocity();

			//projectile becomes uncapable of moving vertically under this odd circumstances
			//spawn a new bouncer to take its place if they occur
			Vector vec = vecAbsVelocity;
			VectorNormalize(vec);
			float dot3D = DotProduct(vec, pTrace->plane.normal);
			vec = Vector(vec.x, vec.y, 0.f);
			float dot2D = DotProduct(vec, pTrace->plane.normal);

			//do the bounce
			float backoff = DotProduct(vecAbsVelocity, pTrace->plane.normal) * 2.0f;
			Vector change = pTrace->plane.normal * backoff;
			vecAbsVelocity -= change;
			vecAbsVelocity *= GetWaterLevel() == WL_Eyes ? 0.6f : GetElasticity();

			if (gpGlobals->curtime > m_flLastBounce + 0.1f)
			{
				BounceSound(pTrace->surface.surfaceProps);
				m_flLastBounce = gpGlobals->curtime;
			}

			if (dot3D < 0.f && dot2D > 0.f)
			{
				QAngle angle;
				VectorAngles(vecAbsVelocity, angle);
				CTFProjectile_BouncyRocket *pProjectile = CTFProjectile_BouncyRocket::Create((CTFWeaponBase *)GetOriginalLauncher(), GetAbsOrigin(), angle,
																							 GetOwnerEntity(), GetOwnerEntity(), -1 ,m_flCreationTime, vecAbsVelocity);
				if (pProjectile)
				{
					pProjectile->SetFuseTime( m_flDetTime );
					pProjectile->SetCritical(GetCritical());
					pProjectile->SetDamage(GetDamage());
					pProjectile->SetLauncher(GetOriginalLauncher());
				}

				UTIL_Remove(this);
			}
			else
			{
				SetAbsVelocity(vecAbsVelocity);
			}
		}

		return;
	}

	trace_t trace;
	memcpy(&trace, pTrace, sizeof(trace_t));
	Explode(&trace, pOther);
}