#pragma once

#include "tf_weapon_rocketlauncher.h"
#include "tf_weaponbase_rocket.h"

#if defined( GAME_DLL )
#include "iscorer.h"
#else
#define CTFProjectile_Ripper C_TFProjectile_Ripper
#define CTFWeaponSawbladeLauncher C_TFWeaponSawbladeLauncher
#endif

class CTFProjectile_Ripper : public CTFBaseRocket
#if defined( GAME_DLL )
	, public IScorer
#endif
{
	DECLARE_CLASS( CTFProjectile_Ripper, CTFBaseRocket );
public:
	DECLARE_NETWORKCLASS();

	virtual ~CTFProjectile_Ripper();

#if defined( CLIENT_DLL )
	virtual void	OnDataChanged( DataUpdateType_t updateType );
#endif

#if defined( GAME_DLL )
	static CTFProjectile_Ripper *Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flSpeed = 1100.0f );

	virtual void	Spawn();
	virtual void	Precache();
	virtual void	RocketTouch( CBaseEntity *pOther );

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void			SetScorer( CBaseEntity *pScorer );

	int				GetCritical() { return m_bCritical; }
	void			SetCritical( int bCritical ) { m_bCritical = bCritical; }
	virtual int		GetDamageType();
	virtual int		GetCustomDamageType();

private:
	void			FlyThink( void );
	//int				CheckHitbox( CBaseEntity *pother, int nHitbox ); // Returns the hitgroup of the closest hitbox
	//bool			CheckIfInside(CBaseEntity* pList, int iCount);
	bool			CheckIfInside();
	void			Bounce( void );

	float m_flRipperSpawnTime;
	Vector vecVelocityNormal;
	Vector vecVelocityHalved;
	CBaseHandle m_Scorer;
	int m_nNumBounces;
	float m_flNextBounce;

	bool m_bInside;
	float m_flNormalSpeed;
	float m_flHalvedSpeed;

#endif

	CNetworkVar( int, m_bCritical );
};

class CTFWeaponSawbladeLauncher : public CTFRocketLauncher
{
	DECLARE_CLASS( CTFWeaponSawbladeLauncher, CTFRocketLauncher );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponSawbladeLauncher();
	virtual ~CTFWeaponSawbladeLauncher() {}

	virtual void	Precache();
	virtual int		GetWeaponID() const { return TF_WEAPON_RIPPER; }

private:
	CTFWeaponSawbladeLauncher( CTFWeaponSawbladeLauncher const & );
};