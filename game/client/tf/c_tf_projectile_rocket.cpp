//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_rocket.h"
#include "tf_gamerules.h"
#include "iefx.h"
#include "dlight.h"
#include "c_te_legacytempents.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

extern ConVar of_muzzlelight;

BEGIN_NETWORK_TABLE( C_TFProjectile_Rocket, DT_TFProjectile_Rocket )
	RecvPropInt( RECVINFO( m_bCritical ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::C_TFProjectile_Rocket( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::~C_TFProjectile_Rocket( void )
{
	ParticleProp()->StopEmission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateRocketTrails();	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::CreateRocketTrails(void)
{
	if (IsDormant())
		return;

	if (enginetrace->GetPointContents(GetAbsOrigin()) & MASK_WATER)
		ParticleProp()->Create("rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail");
	else
		ParticleProp()->Create(GetTrailParticleName(), PATTACH_POINT_FOLLOW, "trail");

	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return;

	if (m_bCritical)
	{
		switch (GetTeamNumber())
		{
		case TF_TEAM_BLUE:
			ParticleProp()->Create("critical_rocket_blue", PATTACH_ABSORIGIN_FOLLOW);
			break;
		case TF_TEAM_RED:
			ParticleProp()->Create("critical_rocket_red", PATTACH_ABSORIGIN_FOLLOW);
			break;
		case TF_TEAM_MERCENARY:
			pPlayer->m_Shared.UpdateParticleColor (ParticleProp()->Create("critical_rocket_dm", PATTACH_ABSORIGIN_FOLLOW) );
			break;
		case TF_TEAM_NPC:
			ParticleProp()->Create("eyeboss_projectile", PATTACH_ABSORIGIN_FOLLOW);
			break;
		default:
			break;
		}
	}
}

void C_TFProjectile_Rocket::CreateLightEffects(void)
{
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if( !pPlayer )
		return;
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

//-----------------------------------------------------------------------------
// Purpose: Bonucy Rocket
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_BouncyRocket, DT_TFProjectile_BouncyRocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_BouncyRocket, DT_TFProjectile_BouncyRocket )
	RecvPropTime( RECVINFO( m_flDetTime ) ),
END_NETWORK_TABLE()

C_TFProjectile_BouncyRocket::C_TFProjectile_BouncyRocket()
{
	m_hTimerParticle = NULL;
}

C_TFProjectile_BouncyRocket::~C_TFProjectile_BouncyRocket()
{
	ParticleProp()->StopEmission();
	m_hTimerParticle = NULL;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

int C_TFProjectile_BouncyRocket::DrawModel(int flags)
{
	float flTimer = (m_flDetTime - m_flCreationTime) ? (m_flDetTime - m_flCreationTime) : 1.0f;
	if( m_hTimerParticle )
		m_hTimerParticle->SetControlPoint( RADIUS_CP1, Vector( 1.0f - (( m_flDetTime - gpGlobals->curtime ) / flTimer) , 0, 0 ) );

	return C_BaseAnimating::DrawModel(flags);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

const char *C_TFProjectile_BouncyRocket::GetTrailParticleName(void)
{
	if (GetTeamNumber() == TF_TEAM_BLUE)
		return "pipebombtrail_blue";
	else if (GetTeamNumber() == TF_TEAM_RED)
		return "pipebombtrail_red";
	else
		return "pipebombtrail_dm";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_BouncyRocket::CreateRocketTrails(void)
{
	CreateLightEffects();

	CNewParticleEffect *pParticle = ParticleProp()->Create(GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW);
	C_TFPlayer *pPlayer = ToTFPlayer(GetOwnerEntity());

	if (pPlayer && pParticle)
	{
		m_hTimerParticle = pParticle;
		pPlayer->m_Shared.UpdateParticleColor(pParticle);
	}
	if (m_bCritical)
	{
		switch (GetTeamNumber())
		{
		case TF_TEAM_BLUE:
			m_hTimerParticle = ParticleProp()->Create("critical_pipe_blue", PATTACH_ABSORIGIN_FOLLOW);
			break;
		case TF_TEAM_RED:
			m_hTimerParticle = ParticleProp()->Create("critical_pipe_red", PATTACH_ABSORIGIN_FOLLOW);
			break;
		case TF_TEAM_MERCENARY:
			m_hTimerParticle = ParticleProp()->Create("critical_pipe_dm", PATTACH_ABSORIGIN_FOLLOW);
			if (pPlayer)
				pPlayer->m_Shared.UpdateParticleColor(m_hTimerParticle);
			break;
		default:
			break;
		}
	}
	
	m_flCreationTime = gpGlobals->curtime;
}