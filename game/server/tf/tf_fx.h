//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//  
//
//=============================================================================

#ifndef TF_FX_H
#define TF_FX_H
#ifdef _WIN32
#pragma once
#endif

#include "particle_parse.h"

struct TFExplosion_Visuals_t
{
	TFExplosion_Visuals_t()
	{
		m_iDamage = 0;
		m_iTeam = TEAM_UNASSIGNED;
		m_bTeamColored = false;

		Q_strcpy( m_szExplosionEffect, "ExplosionCore_wall" );
		Q_strcpy( m_szExplosionPlayerEffect, "ExplosionCore_MidAir" );
		Q_strcpy( m_szExplosionWaterEffect, "ExplosionCore_MidAir_underwater" );
		Q_strcpy( m_szExplosionSound, "Weapon_QuakeRPG.Explode" );

		m_vecPlayerColor.Init( 128,0, 128 );
	}

	int m_iDamage;
	int m_iTeam;
	bool m_bTeamColored;
	char m_szExplosionEffect[128];
	char m_szExplosionPlayerEffect[128];
	char m_szExplosionWaterEffect[128];
	char m_szExplosionSound[128];
	Vector m_vecPlayerColor;
};

void TE_FireBullets( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, int iWeaponID, int iMode, int iSeed, float flSpread, bool bFixedSpread, int iBullets, float flRange, int iAmmoType, float flDamage, int bCritical, bool bFirstShot, int iTracerFrequency );
void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int nEntIndex, TFExplosion_Visuals_t hVisuals );

void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity = false );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity = NULL, int iAttachmentPoint = -1, bool bResetAllParticlesOnEntity = false );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity = NULL, int iAttachType = PATTACH_CUSTOMORIGIN );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity = NULL );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity = NULL );

#endif	// TF_FX_H