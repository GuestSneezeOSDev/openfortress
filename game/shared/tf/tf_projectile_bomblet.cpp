//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Mirv Bomb functions
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_bomblet.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "iefx.h"
	#include "dlight.h"
	#include "c_te_legacytempents.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gravity;

#ifdef CLIENT_DLL
extern ConVar of_muzzlelight;
#else
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );	
#endif

#define GRENADE_MODEL_BOMBLET "models/weapons/w_models/w_grenade_bomblet.mdl"

#define TF_WEAPON_GRENADE_MIRV_BOMB_GRAVITY		0.5f
#define TF_WEAPON_GRENADE_MIRV_BOMB_FRICTION	0.8f
#define TF_WEAPON_GRENADE_MIRV_BOMB_ELASTICITY	0.45f

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeMirvBomb, DT_TFGrenadeMirvBomb )

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_mirv_bomb, CTFGrenadeMirvBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_mirv_bomb );

BEGIN_NETWORK_TABLE( CTFGrenadeMirvBomb, DT_TFGrenadeMirvBomb )
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropInt( RECVINFO( m_bCritical ) ),
	RecvPropString( RECVINFO( m_szBombletTrailParticle ) ),
	
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 20 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),
	SendPropInt( SENDINFO( m_bCritical ) ),
	SendPropStringT( SENDINFO( m_szBombletTrailParticle ) ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
void SetupBombletInfo( bomblet_t *m_pBombletInfo,  CTFWeaponBase *pWeapon, int bCritical )
{
	m_pBombletInfo->m_pLauncher = pWeapon;
	m_pBombletInfo->m_pOwner = pWeapon->GetOwner();
	m_pBombletInfo->m_flBombletDamage = pWeapon->GetTFWpnData().m_flBombletDamage;
	m_pBombletInfo->m_flBombletTimer = pWeapon->GetTFWpnData().m_flBombletTimer;
	m_pBombletInfo->m_flBombletDamageRadius = pWeapon->GetTFWpnData().m_flBombletDamageRadius;
	m_pBombletInfo->m_iAmmount = pWeapon->GetTFWpnData().m_iBombletAmount;
	m_pBombletInfo->m_bCritical = bCritical;
	m_pBombletInfo->m_bExplodeOnImpact = !pWeapon->GetTFWpnData().m_bExplodeOnImpact;
	m_pBombletInfo->m_bBombletEffectTeamColored = pWeapon->GetTFWpnData().m_bBombletEffectTeamColored;
	Q_strncpy(m_pBombletInfo->m_szKillIcon, UTIL_VarArgs("%s_bomblet", pWeapon->GetKillIcon()[0] != '\0' ? pWeapon->GetKillIcon() : pWeapon->GetClassname()), sizeof(m_pBombletInfo->m_szKillIcon));

	Q_strncpy(m_pBombletInfo->m_szBombletTrailParticle, pWeapon->GetTFWpnData().m_szBombletTrailParticle, sizeof(m_pBombletInfo->m_szBombletTrailParticle));
	Q_strncpy(m_pBombletInfo->m_szBombletModel, pWeapon->GetTFWpnData().m_szBombletModel, sizeof(m_pBombletInfo->m_szBombletModel));
}

void SetupBombletInfo( bomblet_t *m_pBombletInfo, bomblet_t *pBombletInfo )
{
	m_pBombletInfo->m_pLauncher = pBombletInfo->m_pLauncher;
	m_pBombletInfo->m_pOwner = pBombletInfo->m_pOwner;

	m_pBombletInfo->m_flBombletDamage = pBombletInfo->m_flBombletDamage;
	m_pBombletInfo->m_flBombletTimer = pBombletInfo->m_flBombletTimer;
	m_pBombletInfo->m_flBombletDamageRadius = pBombletInfo->m_flBombletDamageRadius;

	m_pBombletInfo->m_iAmmount = pBombletInfo->m_iAmmount;
	m_pBombletInfo->m_bCritical = pBombletInfo->m_bCritical;
	m_pBombletInfo->m_bExplodeOnImpact = pBombletInfo->m_bExplodeOnImpact;

	m_pBombletInfo->m_bBombletEffectTeamColored = pBombletInfo->m_bBombletEffectTeamColored;
	Q_strncpy(m_pBombletInfo->m_szKillIcon, pBombletInfo->m_szKillIcon, sizeof(m_pBombletInfo->m_szKillIcon));

	Q_strncpy(m_pBombletInfo->m_szBombletTrailParticle, pBombletInfo->m_szBombletTrailParticle, sizeof(m_pBombletInfo->m_szBombletTrailParticle));
	Q_strncpy(m_pBombletInfo->m_szBombletModel, pBombletInfo->m_szBombletModel, sizeof(m_pBombletInfo->m_szBombletModel));
}

void SpawnBomblets( bomblet_t *pInfo, TFExplosion_Visuals_t *pExplosionVisuals , trace_t *pTrace, int iTeam )
{
	float flBaseOffset = RandomFloat(40, 60);
	for ( int iBomb = 0; iBomb < pInfo->m_iAmmount; ++iBomb )
	{
		Vector vecSrc = pTrace->endpos + ( pTrace->plane.normal * 4 );

		Vector vecVelocity;
		QAngle angBase;
		VectorAngles(pTrace->plane.normal, angBase);
		angBase.x += flBaseOffset;
		float flRot = DEG2RAD( (360 / pInfo->m_iAmmount) * iBomb );
		
		
		AngleVectors(angBase, &vecVelocity);

		vecVelocity *= RandomFloat(275, 300);
		
		vecVelocity = (vecVelocity * cos(flRot)) +
			(CrossProduct(pTrace->plane.normal, vecVelocity) * sin(flRot)) +
			(pTrace->plane.normal * DotProduct(pTrace->plane.normal, vecVelocity) * (1 - cos(flRot)));

		Vector vecZero( 0,0,0 );

		CBaseCombatCharacter *pOwner = pInfo->m_pOwner ? (CBaseCombatCharacter*)pInfo->m_pOwner.Get() : NULL;

		CTFGrenadeMirvBomb *pBomb = CTFGrenadeMirvBomb::Create( vecSrc, vec3_angle, vecVelocity, vecZero, pOwner, pInfo, iTeam );
		
		// Kay forgot to check this on a dedicated penguin machine :^)

			// While i did not check on linux, despite all my rage, 
			// i still did put a check every time before this function is called
			// So pInfo *should* have always been non NULL
			// I would try removing this check and see if it still works - Kay

				// Yes Kay it works! If check removed. Praise Kay! - Bry

		pBomb->SetLauncher(pInfo->m_pLauncher);
		pBomb->SetDamage(pInfo->m_flBombletDamage);
		pBomb->SetDetonateTimerLength(pInfo->m_flBombletTimer + random->RandomFloat(0.0f, 1.0f));
		pBomb->SetDamageRadius(pInfo->m_flBombletDamageRadius);
		pBomb->SetCritical(pInfo->m_bCritical);
		pBomb->SetKillIcon(pInfo->m_szKillIcon);


		pBomb->SetExplosionVisualInfo( pExplosionVisuals );
	}		
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeMirvBomb *CTFGrenadeMirvBomb::Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
							                    const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, bomblet_t *pInfo, int teamnumber )
{
	CTFGrenadeMirvBomb *pBomb = static_cast<CTFGrenadeMirvBomb*>( CBaseEntity::CreateNoSpawn( "tf_weapon_grenade_mirv_bomb", position, angles, pOwner ) );
	if ( pBomb )
	{
		PrecacheModel ( pInfo->m_szBombletModel );
		pBomb->SetModel( pInfo->m_szBombletModel );
		pBomb->m_bExplodeOnImpact = false;
		if ( pInfo->m_bExplodeOnImpact )
		{
			pBomb->SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );
			pBomb->m_bExplodeOnImpact = true;
		}
		char szTrailEffect[MAX_WEAPON_STRING];
		if ( pInfo->m_bBombletEffectTeamColored )
		{
			switch ( teamnumber )
			{
				case TF_TEAM_BLUE:
				Q_snprintf( szTrailEffect, MAX_WEAPON_STRING, "%s_blue", pInfo->m_szBombletTrailParticle );
				break;
				case TF_TEAM_RED:
				Q_snprintf( szTrailEffect, MAX_WEAPON_STRING, "%s_red", pInfo->m_szBombletTrailParticle );
				break;
				case TF_TEAM_MERCENARY:
				Q_snprintf( szTrailEffect, MAX_WEAPON_STRING, "%s_dm", pInfo->m_szBombletTrailParticle );
				break;
			}
		}
		else
			Q_snprintf( szTrailEffect, MAX_WEAPON_STRING, "%s", pInfo->m_szBombletTrailParticle );
		pBomb->m_szBombletTrailParticle.Set( AllocPooledString (szTrailEffect) );
		pBomb->Spawn();
		pBomb->SetupInitialTransmittedGrenadeVelocity( velocity );
		pBomb->SetThrower( pOwner ); 
		pBomb->SetOwnerEntity( pOwner );

		pBomb->SetGravity( TF_WEAPON_GRENADE_MIRV_BOMB_GRAVITY );
		pBomb->SetFriction( TF_WEAPON_GRENADE_MIRV_BOMB_GRAVITY );
		pBomb->SetElasticity( TF_WEAPON_GRENADE_MIRV_BOMB_ELASTICITY );

		pBomb->ChangeTeam( teamnumber );

		pBomb->SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );

		IPhysicsObject *pPhysicsObject = pBomb->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &velocity, &angVelocity );
		}
	}

	return pBomb;
}

bool CTFGrenadeMirvBomb::ExplodeOnImpact( void )
{
	return m_bExplodeOnImpact;
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CTFGrenadeMirvBomb::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CNewParticleEffect *pParticle = ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, "wick" );
		
		C_TFPlayer *pPlayer = ToTFPlayer( GetThrower() );

		CreateLightEffects();
		if ( pPlayer && pParticle )
		{
			pPlayer->m_Shared.UpdateParticleColor( pParticle );
		}

		if ( m_bCritical )
		{
			switch( GetTeamNumber() )
			{
			case TF_TEAM_BLUE:
				ParticleProp()->Create( "critical_pipe_blue", PATTACH_POINT_FOLLOW );
				break;
			case TF_TEAM_RED:
				ParticleProp()->Create( "critical_pipe_red", PATTACH_POINT_FOLLOW );
				break;
			case TF_TEAM_MERCENARY:
					if ( pPlayer )
						pPlayer->m_Shared.UpdateParticleColor( ParticleProp()->Create( "critical_pipe_dm", PATTACH_POINT_FOLLOW ) );
					else
						ParticleProp()->Create( "critical_pipe_dm", PATTACH_POINT_FOLLOW );
				break;
			default:
				break;
			}
		}

	}
}	


void CTFGrenadeMirvBomb::CreateLightEffects(void)
{
	// Handle the dynamic light
	if ( of_muzzlelight.GetBool() )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( GetThrower() );
		dlight_t *dl;
		AddEffects(EF_DIMLIGHT);
		if ( IsEffectActive(EF_DIMLIGHT) )
		{
			dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + index);
			dl->origin = GetAbsOrigin();
			dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			switch ( GetTeamNumber() )
			{
				case TF_TEAM_RED:
					if (!m_bCritical) 
					{
						dl->color.r = 255; dl->color.g = 30; dl->color.b = 10; dl->style = 0;
					}
					else 
					{
						dl->color.r = 255; dl->color.g = 10; dl->color.b = 10; dl->style = 1;
					}
					break;
				case TF_TEAM_BLUE:
					if (!m_bCritical) 
					{
						dl->color.r = 10; dl->color.g = 30; dl->color.b = 255; dl->style = 0;
					}
					else 
					{
						dl->color.r = 10; dl->color.g = 10; dl->color.b = 255; dl->style = 1;
					}
					break;
				case TF_TEAM_MERCENARY:
					if (!pPlayer)
						break;
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
					if (!m_bCritical) 
					{
						dl->color.r = r; dl->color.g = g ; dl->color.b = b ; dl->style = 0;
					}
					else 
					{
						dl->color.r = r; dl->color.g = g; dl->color.b = b; dl->style = 1;
					}
					break;
			}
			dl->die = gpGlobals->curtime + 0.01f;
			dl->radius = 256.0f;
			dl->decay = 512.0f;
			dl->die = gpGlobals->curtime + 0.001;
			tempents->RocketFlare(GetAbsOrigin() );
		}
	}
}
const char *CTFGrenadeMirvBomb::GetTrailParticleName( void )
{
	return m_szBombletTrailParticle;

}
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeMirvBomb::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeMirvBomb::Precache()
{
	BaseClass::Precache();
}
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeMirvBomb::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_MirvBomb.Bounce" );
}
#endif
