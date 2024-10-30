//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that shoots TF projectiles
//
// $NoKeywords: $
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CBaseEntity - tf_point_weapon_mimic
- m_nWeaponType (Offset 856) (Save|Key)(4 Bytes) - WeaponType
- m_pzsFireSound (Offset 864) (Save|Key)(4 Bytes) - FireSound
- m_pzsFireParticles (Offset 868) (Save|Key)(4 Bytes) - ParticleEffect
- m_pzsModelOverride (Offset 872) (Save|Key)(4 Bytes) - ModelOverride
- m_flModelScale (Offset 876) (Save|Key)(4 Bytes) - ModelScale
- m_flSpeedMin (Offset 880) (Save|Key)(4 Bytes) - SpeedMin
- m_flSpeedMax (Offset 884) (Save|Key)(4 Bytes) - SpeedMax
- m_flDamage (Offset 888) (Save|Key)(4 Bytes) - Damage
- m_flSplashRadius (Offset 892) (Save|Key)(4 Bytes) - SplashRadius
- m_flSpreadAngle (Offset 896) (Save|Key)(4 Bytes) - SpreadAngle
- m_bCrits (Offset 900) (Save|Key)(1 Bytes) - Crits
- InputFireOnce (Offset 0) (Input)(0 Bytes) - FireOnce
- InputFireMultiple (Offset 0) (Input)(0 Bytes) - FireMultiple
- DetonateStickies (Offset 0) (Input)(0 Bytes) - DetonateStickies
*/

#ifndef CTFPointWeaponMimic_H
#define CTFPointWeaponMimic_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class CTFPointWeaponMimic : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFPointWeaponMimic, CBaseEntity );
	DECLARE_DATADESC();

	CTFPointWeaponMimic();
	~CTFPointWeaponMimic();
	
	virtual void Spawn();

	QAngle GetFiringAngles();
	Vector GetFiringOrigin();

	void Fire();

	void FireGrenade();
	void FireRocket();

	float GetSpeed();

	void FireStickyGrenade();

	void	DeathNotice( CBaseEntity *pVictim );

	void	InputFireOnce( inputdata_t &inputdata );
	void	InputFireMultiple( inputdata_t &inputdata );
	void	DetonateStickies( inputdata_t &inputdata );
	
private:
	int m_iStickyBombCount;

	int m_nWeaponType;        
	string_t m_pzsFireSound;
	string_t m_pzsFireParticles;
	string_t m_pzsModelOverride;                                 
	float m_flModelScale;                                            
	float m_flSpeedMin;                                             
	float m_flSpeedMax;                                             
	float m_flDamage;                                                
	float m_flSplashRadius;                                         
	float m_flSpreadAngle;                                           
	int m_bCrits;              

	bool m_bHasOverridenModel;

	CUtlVector< CHandle<CTFGrenadePipebombProjectile> > m_StickyBombs; 

	CTFWeaponInfo *pRocketInfo = NULL;
	CTFWeaponInfo *pGrenadeInfo = NULL;
	CTFWeaponInfo *pStickyBombInfo = NULL;
};

#endif //CTFPointWeaponMimic_H