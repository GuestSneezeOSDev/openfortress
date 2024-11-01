//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
	class C_TFPlayer;
#else
	class CTFPlayer;
#endif

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif


//=============================================================================

#define PERMANENT_CONDITION		-1

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
	DECLARE_EMBEDDED_NETWORKVAR()

public:
	float flDamage;
	float flTime;
	bool bKill;
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );

	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	// Condition (TF_COND_*).
	int		GetCond() const						{ return m_nPlayerCond; }
	void	SetCond( int nCond )				{ m_nPlayerCond = nCond; }
	
    SERVERONLY_EXPORT void	AddCond( int nCond, float flDuration = PERMANENT_CONDITION );
    SERVERONLY_EXPORT void	RemoveCond( int nCond );
	
	bool	InCond( int nCond );
	void	RemoveAllCond(CTFPlayer *pPlayer);
	void	OnConditionAdded( int nCond );
	void	OnConditionRemoved( int nCond );
	void	ConditionThink( void );
	float	GetConditionDuration( int nCond );

	void	ConditionGameRulesThink( void );

	void	ConditionHealthBuff(void);

	void	InvisibilityThink( void );

	int		GetMaxBuffedHealth( void );
	int		GetMaxBuffedHealthDM( void );
	int		GetDefaultHealth( void );
	
	bool InCondUber( void );
	void RemoveCondUber( void );
	
	bool InCondShield( void );
	void RemoveCondShield( void );
	
	bool InCondCrit( void );
	void RemoveCondCrit( void );
	
	bool InCondInvis( void );
	void RemoveCondInvis( void );

	bool InPowerupCond();
	
	void SetSpawnEffect( int iEffect ){ m_iRespawnEffect = iEffect; }
	int GetSpawnEffects( void ){ return m_iRespawnEffect; };

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	UpdateConditions( int nCond, int nCondOld, int nCondOffset );
#endif

    SERVERONLY_EXPORT void	Disguise( int nTeam, int nClass );
	void	CompleteDisguise( void );
    SERVERONLY_EXPORT void	RemoveDisguise( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void )				{ return m_nDisguiseTeam; }
	int		GetDisguiseClass( void ) 			{ return m_nDisguiseClass; }
	int		GetDisguiseClassMod( void ) 		{ return m_nDisguiseClassMod; }
	int		GetDesiredDisguiseClass( void )		{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void )		{ return m_nDesiredDisguiseTeam; }
	EHANDLE GetDisguiseTarget( void ) 	
	{
#ifdef CLIENT_DLL
		if ( m_iDisguiseTargetIndex == TF_DISGUISE_TARGET_INDEX_NONE )
			return NULL;
		return cl_entitylist->GetNetworkableHandle( m_iDisguiseTargetIndex );
#else
		return m_hDisguiseTarget.Get();
#endif
	}
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );

#ifdef CLIENT_DLL
	void	OnDisguiseChanged( void );
	void 	UpdateCritParticle( void );
	void	RecalcDisguiseWeapon( void );
	int		GetDisguiseWeaponModelIndex( void ) { return m_iDisguiseWeaponModelIndex; }
	CTFWeaponInfo *GetDisguiseWeaponInfo( void );
	bool	UpdateParticleColor( CNewParticleEffect *pParticle );
#endif

#ifdef GAME_DLL
	void	Heal( CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal = false );
	void	StopHealing( CTFPlayer *pPlayer );
	void	RecalculateInvuln( bool bInstantRemove = false );
	int		FindHealerIndex( CTFPlayer *pPlayer );
	EHANDLE GetHealerByIndex( int index );
	EHANDLE	GetFirstHealer();
	bool	HealerIsDispenser( int index ) const;
	void	HealthKitPickupEffects( int iAmount );
#endif
	bool	IsTopThree();
	virtual void SetTopThree( bool bTop3 );

	bool	IsZombie();
	bool	IsLastSurvivor();
	virtual void SetZombie( bool bZombie );
	virtual void SetSurvivor(bool bSurvivor);

	int		GetNumHealers( void ) { return m_nNumHealers; }

	bool	IsControlStunned( void );

	CNetworkVar(int, m_iArmor);
	int		GetTFCArmor(void){ return m_iArmor; };
	void	SetTFCArmor(int armortfcvalue){ m_iArmor = armortfcvalue; }
  CNetworkVar( int, m_iLives);

	int		GetTFLives(void){ return m_iLives; };
	void	SetTFLives(int lives){ m_iLives = lives; }

	void	SetSwapWeaponSpawner( CBaseEntity *pEnt );
	CBaseEntity *GetSwapWeaponSpawner( void );

    SERVERONLY_EXPORT void	Burn( CTFPlayer *pPlayer, float flTime );
	void	Poison(CTFPlayer *pPlayer, float flTime);
	void	Tranq(CTFPlayer *pPlayer, float flTime, float flMovementSpeed, float flWeaponSpeed);
	void	FuckUpLegs(CTFPlayer *pPlayer, float flTime, float flSpeed);

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;

	// Utility.

	bool	IsAlly( CBaseEntity *pEntity );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float flInvisFadeTime );
	float	GetPercentInvisible( void );
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }

	int		GetDesiredPlayerClassIndex( void );

	float	GetSpyCloakMeter() const		{ return m_flCloakMeter; }
	void	SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }

	//Jumping
	bool	IsJumping( void ) { return m_bJumping; }
	void	SetJumping( bool bJumping );
	void	SetJumpBuffer(bool buffer);
	bool	GetJumpBuffer() { return m_bBlockJump; }

	//Air dash
	bool    IsAirDashing( void ) { return m_bAirDash; }
	void    SetAirDash( bool bAirDash );
	int     GetAirDashCount( void ) { return m_iAirDashCount; }
	void    AddAirDashCount();
	void    SetAirDashCount( int iAirDashCount );
	
	// Gravity Gauntlets
	bool    IsHovering( void ) { return m_bHovering; }
	void	SetHovering( bool bHovering ){ m_bHovering = bHovering; };

	// Jetpack
	bool    IsJetpackEngaged(void) { return m_bJetpackEngaged; }
	void	SetJetpackEngaged(bool bJetpackEngaged) { m_bJetpackEngaged = bJetpackEngaged; };

	//Grappling hook
	CBaseEntity *GetHook( void ) { return m_Hook; }
	void    SetHook(CBaseEntity *hook);
	void    SetHookProperty(float pull);
	float	GetHookProperty() { return m_flGHookProp; }
	//CSlide
	void	SetCSlideDuration(float duration);
	float	GetCSlideDuration() { return m_flCSlideDuration; }
	//Lunge
	bool	DoLungeCheck();
	float	GetNextLungeTime(void) { return m_flNextLungeTime; }
	void	SetNextLungeTime(float flNextLungeTime) { m_flNextLungeTime = flNextLungeTime; }
	bool	IsLunging(void);
	//Air control disabled (lunge and jumppads)
	void	SetNoAirControl(bool control) { m_bNoAirControl = control; }
	bool	IsNoAirControl() { return m_bNoAirControl; }

	// loser state
	bool	IsLoser( void );
	// hauling state
	bool	IsHauling( void );

	void	DebugPrintConditions( void );

	int		PlayDeathAnimation( CBaseAnimating *pAnim, int iDamageCustom, bool bDissolve );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	float	m_flStepSoundDelay;
	float	m_flJumpSoundDelay;
	
private:

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddCritBoosted( int iCond );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddBerserk( void );
	void OnAddShield( void );
	void OnAddShieldCharge( void );
	void OnAddHaste( void );
	void OnAddJauggernaught( void );
	void OnAddPoison(void);
	void OnAddTranq(void);
	void OnAddPiercedLegs(void);
	void OnAddJetpackPowerup(void);
	void OnAddJetpackEngaged(void);

	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveCritBoosted( int iCond );
	void OnRemoveTaunting( void );
	void OnRemoveTeleported( void );
	void OnRemoveBerserk( void );
	void OnRemoveShield( void );
	void OnRemoveShieldCharge( void );
	void OnRemoveHaste( void );
	void OnRemoveJauggernaught( void );
	void OnRemovePoison(void);
	void OnRemoveTranq(void);
	void OnRemovePiercedLegs(void);
	void OnRemoveJetpackPowerup(void);
	void OnRemoveJetpackEngaged(void);

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	bool  IsProvidingInvuln( CTFPlayer *pPlayer );
	void  SetInvulnerable( bool bState, bool bInstant = false );
#endif
public:
	CNetworkVar( bool, bWatchReady );
	CNetworkVar( float, m_flNextZoomTime );
private:

	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.

	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	CNetworkVar( int, m_nPlayerCondEx );        
	CNetworkVar( int, m_nPlayerCondEx2 );       // https://csrd.science/misc/datadump/current/netprops.txt
	CNetworkVar( int, m_nPlayerCondEx3 );		// Disgusting, don't blame me -ficool2
	CNetworkVar( int, m_nPlayerCondEx4 );

	CNetworkArray( float, m_flCondExpireTimeLeft, TF_COND_LAST );	// Time until each condition expires

//TFTODO: What if the player we're disguised as leaves the server?
//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	CNetworkVar( int, m_nDisguiseClassMod );	// Class spy is disguised as.
	EHANDLE m_hDisguiseTarget;					// Playing the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseTargetIndex );
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player seperate from teammates

	float m_flInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;

	CNetworkVar( int, m_nNumHealers );
	CNetworkVar( int, m_iRespawnEffect );
	
	CNetworkVar( bool, m_bIsTopThree );
	CNetworkVar( bool, m_bIsZombie );
	CNetworkVar( bool, m_bIsLastSurvivor);
	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL
	// Healer handling
	struct healers_t
	{
		EHANDLE	pPlayer;
		float	flAmount;
		bool	bDispenserHeal;
	};
	CUtlVector< healers_t >	m_aHealers;	
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing

	float m_flInvulnerableOffTime;
#else
	HPARTICLEFFECT m_pCritBoostEffect;
	HPARTICLEFFECT m_pOverhealEffect;
#endif

	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CNetworkVar( int,		m_nNumFlames );
	float					m_flFlameBurnTime;
	float					m_flFlameRemoveTime;
	float					m_flTauntRemoveTime;

	//Poison Stuff
	CHandle<CTFPlayer>		m_hPoisonAttacker;
	float					m_flPoisonTime;
	float					m_flPoisonRemoveTime;
	float					m_flPoisonDamageTaken;

	//Tranq Stuff
	float					m_flTranqRemoveTime;
	float					m_flTranqMovementSlowness;
	float					m_flTranqExtraSlowness;

	//Fucked Up Legs Stuff
	float					m_flPiercedLegsRemoveTime;
	float					m_flPiercedLegsSlowness;

	float m_flDisguiseCompleteTime;

	int	m_nOldConditions;
	int m_nOldConditionsEx;
	int m_nOldConditionsEx2;
	int m_nOldConditionsEx3;
	int m_nOldConditionsEx4;

	int	m_nOldDisguiseClass;

	bool m_bOldIsLoser;

	CNetworkVar( int, m_iDesiredPlayerClass );

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( bool, m_bAirDash );
	CNetworkVar( int,  m_iAirDashCount );
	
	CNetworkVar( bool, m_bHovering );

	CNetworkVar( bool, m_bJetpackEngaged );
#ifdef GAME_DLL
	CNetworkHandle( CBaseEntity, m_Hook );
	CNetworkHandle( CBaseEntity, m_pWeaponSpawner );
#else
	EHANDLE		 m_Hook;
	EHANDLE		 m_pWeaponSpawner;
#endif
	CNetworkVar( float, m_flGHookProp );
	CNetworkVar( bool, m_bBlockJump);
	CNetworkVar( float, m_flCSlideDuration );
	CNetworkVar(float, m_flRampJumpVel);

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( float, m_flNextLungeTime );
	CNetworkVar( bool, m_bNoAirControl );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS + 1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS + 1 );	// array of state per other player whether other players are dominating this player
	
private:
    CNetworkVar(bool, m_bLaunchedFromJumppad);
public:
    void    SetLaunchedFromJumppad(bool b){m_bLaunchedFromJumppad = b;}
    bool    IsLaunchedFromJumppad(){return m_bLaunchedFromJumppad;}

#ifdef GAME_DLL
	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;
#else
	int m_iDisguiseWeaponModelIndex;
	int m_iOldDisguiseWeaponModelIndex;
	CTFWeaponInfo *m_pDisguiseWeaponInfo;

	WEAPON_FILE_INFO_HANDLE	m_hDisguiseWeaponInfo;
#endif

	int m_iJauggernaughtOldClass;
};			   

#define TF_DEATH_DOMINATION				0x0001	// killer is dominating victim
#define TF_DEATH_ASSISTER_DOMINATION	0x0002	// assister is dominating victim
#define TF_DEATH_REVENGE				0x0004	// killer got revenge on victim
#define TF_DEATH_ASSISTER_REVENGE		0x0008	// assister got revenge on victim

#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"
#define DM_SHIELD_MODEL 	"models/player/attachments/mercenary_shield.mdl"
#define JETPACK_MODEL		"models/player/items/mercenary/jetpack/jetpack.mdl"

extern const char *g_pszBDayGibs[22];

#endif // TF_PLAYER_SHARED_H
