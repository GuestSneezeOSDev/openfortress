//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_MINIGUN_H
#define TF_WEAPON_MINIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFMinigun C_TFMinigun
#define CTFGatlingGun C_TFGatlingGun
#define CTFCAssaultCannon C_TFCAssaultCannon
#endif

enum MinigunState_t
{
	// Firing states.
	AC_STATE_IDLE = 0,
	AC_STATE_STARTFIRING,
	AC_STATE_FIRING,
	AC_STATE_SPINNING,
	AC_STATE_DRYFIRE
};

//=============================================================================
//
// TF Weapon Minigun
//
class CTFMinigun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFMinigun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFMinigun();
	~CTFMinigun();

	virtual void	Precache( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MINIGUN; }
	MinigunState_t  GetWeaponState( void ) const		{ return m_iWeaponState; }
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	void			SharedAttack();
	virtual void	WeaponIdle();
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Lower( void );
	virtual void	HandleFireOnEmpty( void );
	virtual void	WeaponReset( void );
	virtual void	WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f ) override;
	
	virtual void	ItemPostFrame( void );
	virtual bool	IsChainGun() const { return false; }
	virtual bool 	CanSoftZoom( void ) { return false; }
	virtual MinigunState_t GetMinigunState(){ return m_iWeaponState; }
#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void );
#endif


	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_MINIGUN; }

	float			GetFiringTime( void ) { return (m_flStartedFiringAt >= 0) ? (gpGlobals->curtime - m_flStartedFiringAt) : 0; }


#ifdef CLIENT_DLL
	float GetBarrelRotation();
#endif

private:
	
	CTFMinigun( const CTFMinigun & ) {}

	void WindUp( void );
	void WindDown( void );


#ifdef CLIENT_DLL
	// Barrel spinning
	virtual CStudioHdr *OnNewModel( void );
	virtual void		StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	
	virtual void		UpdateOnRemove( void );

	void				CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );

	void				OnDataChanged( DataUpdateType_t type );
		
	virtual void	ItemPreFrame( void );
	virtual void	ItemBusyFrame( void );
	
	// Firing sound
	void				WeaponSoundUpdate( void );

	void				UpdateBarrelMovement( void );
	virtual void		SetDormant( bool bDormant );


#endif

private:
	virtual void PlayWeaponShootSound( void ) {}	// override base class call to play shoot sound; we handle that ourselves separately

	CNetworkVar( MinigunState_t, m_iWeaponState );
	CNetworkVar( int, m_bCritShot );

	float			m_flNextFiringSpeech;
	float			m_flStartedFiringAt;
	float	m_flBarrelCurrentVelocity;
	float	m_flBarrelTargetVelocity;
	int		m_iBarrelBone;
	float	m_flBarrelAngle;
	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iMinigunSoundCur;			// the enum value of the weapon sound currently being played

#ifdef CLIENT_DLL
	void StartBrassEffect();
	void StopBrassEffect();
	void HandleBrassEffect();

	CNewParticleEffect *m_pEjectBrassEffect;
	int					m_iEjectBrassAttachment;

	void StartMuzzleEffect();
	void StopMuzzleEffect();
	void HandleMuzzleEffect();

	CNewParticleEffect *m_pMuzzleEffect;
	int					m_iMuzzleAttachment;
#endif
};

class CTFGatlingGun : public CTFMinigun
{
public:
	DECLARE_CLASS( CTFGatlingGun, CTFMinigun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttablbChaingun_Mercenary[];

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GATLINGGUN; }
	
	virtual bool	IsChainGun() const { return true; }
	virtual bool 	CanSoftZoom(void) { return true; }
};

class CTFCAssaultCannon : public CTFMinigun
{
public:
	DECLARE_CLASS( CTFCAssaultCannon, CTFMinigun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_ASSAULTCANNON; }
};

#endif // TF_WEAPON_MINIGUN_H
