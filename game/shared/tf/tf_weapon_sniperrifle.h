//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_SNIPERRIFLE_H
#define TF_WEAPON_SNIPERRIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CTFSniperRifle C_TFSniperRifle
#define CTFRailgun C_TFRailgun
#define CSniperDot C_SniperDot
#define CTFCSniperRifle C_TFCSniperRifle
#endif

//=============================================================================
//
// Sniper Rifle Laser Dot class.
//
class CSniperDot : public CBaseEntity
{
public:

	DECLARE_CLASS( CSniperDot, CBaseEntity );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	// Creation/Destruction.
	CSniperDot( void );
	~CSniperDot( void );

	static CSniperDot *Create( const Vector &origin , int damage, CBaseEntity *pOwner = NULL, bool bVisibleDot = true);
	void		ResetChargeTime( void ) { m_flChargeStartTime = gpGlobals->curtime; }

	// Attributes.
	int			ObjectCaps()							{ return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	// Targeting.
	void        Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal );
	CBaseEntity	*GetTargetEntity( void )				{ return m_hTargetEnt; }

	int m_nDamage;
	
// Client specific.
#ifdef CLIENT_DLL

	// Rendering.
	virtual bool			IsTransparent( void )		{ return true; }
	virtual RenderGroup_t	GetRenderGroup( void )		{ return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	virtual int				DrawModel( int flags );
	virtual bool			ShouldDraw( void );

	//
	virtual void			OnDataChanged( DataUpdateType_t updateType );

	CMaterialReference		m_hSpriteMaterial;

#endif

protected:

	Vector					m_vecSurfaceNormal;
	EHANDLE					m_hTargetEnt;

	CNetworkVar( float, m_flChargeStartTime );
};

//=============================================================================
//
// Sniper Rifle class.
//
class CTFSniperRifle : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSniperRifle, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSniperRifle();
	~CTFSniperRifle();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_SNIPERRIFLE; }

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableSniperRifle[];

	virtual void Spawn();
	virtual void Precache();
	void		 ResetTimers( void );
	virtual void SetReloadTimer(float flReloadTime);

	virtual bool Reload( void );
	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	void		 HandleZooms( void );
	virtual void ItemPostFrame( void );
	virtual bool Lower( void );
	virtual float GetProjectileDamage( void );
	virtual int	GetDamageType() const;
	virtual void DoFireEffects( void );

	virtual void WeaponReset( void );

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false );
	virtual bool CanSoftZoom( void ) { return false; }

	void CreateSniperDot(void);
	void DestroySniperDot(void);
	void UpdateSniperDot(void);

	// Auto-rezooming handling
	void SetRezoom(bool bRezoom, float flDelay);

	CNetworkVar(float, m_flChargedDamage);

#ifdef CLIENT_DLL
	float GetHUDDamagePerc( void );
#endif

#ifdef GAME_DLL
	CHandle<CSniperDot>		m_hSniperDot;
#endif

	bool IsZoomed( void );

private:

	void Zoom( void );
	void ZoomIn(void);
	void ZoomOut(void);
	void ZoomOutIn( void );

	void Fire( CTFPlayer *pPlayer );

	// Handles rezooming after the post-fire unzoom
	float m_flUnzoomTime;
	float m_flRezoomTime;
	bool m_bRezoomAfterShot;

	CTFSniperRifle( const CTFSniperRifle & );
};

class CTFRailgun: public CTFSniperRifle
{
public:

	DECLARE_CLASS( CTFRailgun, CTFSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_RAILGUN; }

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableRailgun[];
};

class CTFCSniperRifle : public CTFSniperRifle
{
public:

	DECLARE_CLASS(CTFCSniperRifle, CTFSniperRifle);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
    CNetworkVar(float, m_flChargedDamage);

    virtual float GetProjectileDamage( void );

#ifdef CLIENT_DLL
	float GetHUDDamagePerc(void);
#endif

	virtual int	GetWeaponID(void) const { return TFC_WEAPON_SNIPER_RIFLE; }

private:

	void Fire(CTFPlayer *pPlayer);
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo);

	virtual void SetRezoom(bool bRezoom, float flDelay);

	void Zoom(void);
	void ZoomOutIn(void);
	void ZoomIn( void );
	void ZoomOut( void );

	virtual void ItemPostFrame(void);

	// Handles rezooming after the post-fire unzoom
	float m_flUnzoomTime;
	float m_flRezoomTime;
	bool m_bRezoomAfterShot;

};

#endif // TF_WEAPON_SNIPERRIFLE_H
