//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_FLAMETHROWER_H
#define TF_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
	#include "c_tf_projectile_rocket.h"

	#define CTFFlameThrower C_TFFlameThrower
	#define CTFFlameRocket C_TFFlameRocket
	#define CTFFlameThrower_Retro C_TFFlameThrower_Retro
	#define CTFFlameThrowerDM C_TFFlameThrowerDM
#else
	#include "tf_projectile_rocket.h"
#endif

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING
};

//=========================================================
// Flamethrower Weapon
//=========================================================
class CTFFlameThrower : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CTFFlameThrower, CTFWeaponBaseGun );
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlameThrower();
	~CTFFlameThrower();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_FLAMETHROWER; }
	virtual bool	IsFlamethrowerPyro() const { return true; }
	virtual void	WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f ) override;

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	Lower( void );
	virtual void	WeaponReset( void );

	virtual void	DestroySounds( void );
	virtual bool CanSoftZoom( void ) { return false; }

	Vector GetVisualMuzzlePos();
	Vector GetFlameOriginPos();

#ifdef CLIENT_DLL
	virtual bool	Deploy( void );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove( void );
	virtual void	SetDormant( bool bDormant );

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	void			RestartParticleEffect();	

	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();
#else
	void			OnAirblast( CBaseEntity *pEntity );
#endif
private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( int, m_bCritFire );

	float m_flStartFiringTime;
	float m_flNextPrimaryAttackAnim;

	int			m_iParticleWaterLevel;
	float		m_flAmmoUseRemainder;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pFiringStartSound;
	CSoundPatch	*m_pFiringLoop;
	bool		m_bFiringLoopCritical;
	bool		m_bFlameEffects;
	CSoundPatch *m_pPilotLightSound;
	CNewParticleEffect	*m_pFireParticle;
#endif

	CTFFlameThrower( const CTFFlameThrower & );
};

//=============================================================================
#define	TF_FLAMETHROWER_ROCKET_DAMAGE				15
#define TF_FLAMETHROWER_ROCKET_BURN_RADIUS			198
//=============================================================================

// Generic flame rocket.
class CTFFlameRocket : public CTFProjectile_Rocket
{
	DECLARE_CLASS( CTFFlameRocket, CTFProjectile_Rocket );
public:
	DECLARE_NETWORKCLASS(); 

	virtual int		GetWeaponID( void ) const		{ return TF_WEAPON_FLAMETHROWER_ROCKET; }
	virtual float	GetRadius() { return TF_FLAMETHROWER_ROCKET_BURN_RADIUS; }

#ifdef GAME_DLL
	// Creation.
	static CTFFlameRocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );	
	virtual void	Spawn();
	virtual void	Precache();
#else
	virtual const char *GetTrailParticleName( void ) { return "incendiaryrockettrail"; }
#endif
};

#ifdef GAME_DLL

class CTFFlameEntity : public CBaseEntity
{
	DECLARE_CLASS( CTFFlameEntity, CBaseEntity );
public:

	virtual void Spawn( void );

public:
	static CTFFlameEntity *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float m_flDmgAmount, int m_iCustomDmgType );

	void FlameThink( void );
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );
private:
	void OnCollide( CBaseEntity *pOther );

	Vector					m_vecInitialPos;		// position the flame was fired from
	Vector					m_vecPrevPos;			// position from previous frame
	Vector					m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector					m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	float					m_flTimeRemove;			// time at which the flame should be removed
	int						m_iDmgType;				// damage type
	int						m_iCustomDmgType;		// custom damage type
	float					m_flDmgAmount;			// amount of base damage
	CUtlVector<EHANDLE>		m_hEntitiesBurnt;		// list of entities this flame has burnt
	EHANDLE					m_hAttacker;			// attacking player
	int						m_iAttackerTeam;		// team of attacking player
};

#endif // GAME_DLL

class CTFFlameThrower_Retro : public CTFFlameThrower
{
	DECLARE_CLASS(CTFFlameThrower_Retro, CTFFlameThrower);
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int	 GetWeaponID(void) const { return TFC_WEAPON_FLAMETHROWER; }
	virtual bool CanSoftZoom(void) { return true; }

};

class CTFFlameThrowerDM : public CTFFlameThrower
{
	DECLARE_CLASS(CTFFlameThrowerDM, CTFFlameThrower);
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableFlameThrower[];

	virtual int		GetWeaponID(void) const { return TF_WEAPON_FLAMETHROWER_MERCEANARY; }
	virtual bool	CanSoftZoom(void) { return true; }

};

#endif // TF_WEAPON_FLAMETHROWER_H