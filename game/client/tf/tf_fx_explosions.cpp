//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "ragdollexplosionenumerator.h"
#include "c_tf_player.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "dlight.h"
#include "iefx.h"
#include "tempentity.h"
#include "fx_explosion.h"

extern ConVar of_muzzlelight;

//--------------------------------------------------------------------------------------------------------------
CTFWeaponInfo *GetTFWeaponInfo( int iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback( const Vector &vecOrigin, const Vector &vecNormal, ClientEntityHandle_t hEntity, 
	int iDamage, int iTeam, 
	bool bTeamColored, char *szExplosionEffect, char *szExplosionPlayerEffect,
	char *szExplosionWaterEffect, char *szExplosionSound,
	Vector vecPlayerColor )
{
	bool bIsPlayer = false;
	
	if( hEntity.Get() )
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance(hEntity);
		if (pEntity && pEntity->IsPlayer())
		{
			bIsPlayer = true;
		}
	}
	
	// Calculate the angles, given the normal.
	bool bIsWater = (UTIL_PointContents(vecOrigin) & CONTENTS_WATER);
	bool bInAir = false;
	QAngle angExplosion(0.0f, 0.0f, 0.0f);

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if (fabs(vecNormal.x) < 0.05f && fabs(vecNormal.y) < 0.05f && fabs(vecNormal.z) < 0.05f)
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles(vecNormal, angExplosion);
		bInAir = false;
	}
	// Base explosion effect and sound.
	const char *pszEffect = "ExplosionCore_wall";
	const char *pszSound = "BaseExplosionEffect.Sound";

	// Explosions.
	if ( bIsWater )
	{
		if (Q_strlen(szExplosionWaterEffect) > 0)
		{
			pszEffect = szExplosionWaterEffect;
		}

		WaterExplosionEffect().Create(vecOrigin, 100.0f, 10.0f, TE_EXPLFLAG_NONE);
	}
	else
	{
		if( bIsPlayer || bInAir )
		{
			if( Q_strlen(szExplosionPlayerEffect) > 0 )
			{
				pszEffect = szExplosionPlayerEffect;
			}
		}
		else
		{
			if( Q_strlen(szExplosionEffect) > 0 )
			{
				pszEffect = szExplosionEffect;
			}
		}
	}

	if( bTeamColored )
	{
		char buf[MAX_PATH];
		Q_strncpy( buf, pszEffect, sizeof(buf) );
		switch( iTeam )
		{
			case TF_TEAM_RED:
				Q_strncat( buf, "_red", sizeof(buf) );
				break;
			case TF_TEAM_BLUE:
				Q_strncat( buf, "_blue", sizeof(buf) );
				break;
			case TF_TEAM_MERCENARY:
				Q_strncat( buf, "_dm", sizeof(buf) );
				break;
			default:
				break;
		}
		pszEffect = buf;
	}

	// Sound.
	if( Q_strlen(szExplosionSound) > 0 )
	{
		pszSound = szExplosionSound;
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound(filter, SOUND_FROM_WORLD, pszSound, &vecOrigin);

	if( of_muzzlelight.GetBool() )
	{
		dlight_t *dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC);
		dl->origin = vecOrigin;
		dl->color.r = 255;
		dl->color.g = 250;
		dl->color.b = 140;
		dl->decay = 200;
		dl->radius = 512.f;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		dl->die = gpGlobals->curtime + 0.1f;
	}

	if( iTeam == TF_TEAM_MERCENARY )
	{
		Vector Color = vecPlayerColor;
		DispatchParticleEffect(pszEffect, vecOrigin, angExplosion, Color, Color);
	}
	else
		DispatchParticleEffect(pszEffect, vecOrigin, angExplosion);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	void			AffectRagdolls( void );
	Vector		m_vecOrigin;
	Vector		m_vecNormal;
	ClientEntityHandle_t m_hEntity;

	int m_iDamage;
	int m_iTeam;
	bool m_bTeamColored;
	Vector m_vecPlayerColor;
	char m_szExplosionEffect[128];
	char m_szExplosionPlayerEffect[128];
	char m_szExplosionWaterEffect[128];
	char m_szExplosionSound[128];

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_hEntity = INVALID_EHANDLE_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::AffectRagdolls( void )
{
#ifdef OF_CLIENT_DLL
	CRagdollExplosionEnumerator	ragdollEnum( m_vecOrigin, 125, 300, m_iDamage );
#else
	CRagdollExplosionEnumerator	ragdollEnum( m_vecOrigin, 125, 300 );
#endif
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_RESPONSIVE_EDICTS, m_vecOrigin, 125, false, &ragdollEnum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	AffectRagdolls();

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_hEntity, m_iDamage, m_iTeam, 
		m_bTeamColored, m_szExplosionEffect, m_szExplosionPlayerEffect, m_szExplosionWaterEffect, m_szExplosionSound, 
		m_vecPlayerColor );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
	RecvPropInt( RECVINFO( m_iDamage ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropBool( RECVINFO( m_bTeamColored ) ),
	RecvPropVector( RECVINFO( m_vecPlayerColor ) ),
	RecvPropString( RECVINFO( m_szExplosionEffect) ),
	RecvPropString( RECVINFO( m_szExplosionPlayerEffect ) ),
	RecvPropString( RECVINFO( m_szExplosionWaterEffect ) ),
	RecvPropString( RECVINFO( m_szExplosionSound ) )
END_RECV_TABLE()

