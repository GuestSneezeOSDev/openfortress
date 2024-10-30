//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_fx_shared.h"
#include "tf_projectile_nail.h"
#include "in_buttons.h"
#include "of_projectile_bfg.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_rocketlauncher.h"
#include "of_weapon_ripper.h"

#ifdef GAME_DLL
	#include "tf_gamestats.h"
	#include "of_projectile_tripmine.h"
	#include "tf_projectile_bomblet.h"
#endif

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGun, DT_TFWeaponBaseGun )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGun, DT_TFWeaponBaseGun )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flAccurateAtTick ) ),
// Server specific.
#else
	SendPropTime( SENDINFO( m_flAccurateAtTick ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGun )
#if defined( CLIENT_DLL )
	DEFINE_FIELD( m_flChargeBeginTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAccurateAtTick, FIELD_FLOAT ),
#endif
END_PREDICTION_DATA()

// Server specific.

BEGIN_DATADESC( CTFWeaponBaseGun )
#if !defined( CLIENT_DLL ) 
DEFINE_THINKFUNC( ZoomOutIn ),
DEFINE_THINKFUNC( ZoomOut ),
DEFINE_THINKFUNC( ZoomIn ),
#endif
END_DATADESC()


ConVar of_noreload( "of_noreload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles weapon reloading." );
extern ConVar of_infiniteammo;
extern ConVar of_haste_fire_multiplier;

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGun::CTFWeaponBaseGun()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iShotsDue = 0;
	m_flNextShotTime = 0.0f;
	m_flAccurateAtTick = 0.0f;
	m_iDamageIncrease = 0;
	m_flBlastRadiusIncrease = 0;
}

bool CTFWeaponBaseGun::Reload( void )
{
	if ( InBurst() )
		return false;

	return BaseClass::Reload( );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::BurstFire( void )
{
	if ( m_iClip1 <= 0 )
	{
		if ( FiresInBursts() )
			m_iShotsDue = 0;
		return;
	}
	BaseClass::BurstFire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::BeginBurstFire(void)
{
	//Ivory: Otherwise animations and sounds play even if gun cannot shoot
	if (!CanAttack())
		return;

	BaseClass::BeginBurstFire();
}

void CTFWeaponBaseGun::HandleFireOnEmpty()
{
	if( LoadsManualy() && m_flNextPrimaryAttack <= gpGlobals->curtime )
		PrimaryAttack();

	BaseClass::HandleFireOnEmpty();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;
	
	if ( !CanAttack() )
		return;	
	
	if( LoadsManualy() && !InBarrage() )
	{
		Reload();
		return;
	}
	
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
	{
		if ( FiresInBursts() )
			m_iShotsDue = 0;
		
		m_bInBarrage = false;
		return;
	}

	if ( GetWindupTime() > 0.0f )
	{
		if ( m_bWindingUp )
		{
			if ( gpGlobals->curtime < m_flWindTick )
				return;
		}
		else
		{
			m_bWindingUp = true;
			m_flWindTick = gpGlobals->curtime + GetWindupTime();
			SendWeaponAnim( ACT_VM_CHARGEUP );
			pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRE);
			return;
		}
	}

	if ( !FiresInBursts() )
	{
		// Are we capable of firing again?
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;
	}
	else if ((FiresInBursts() && m_iShotsDue == 0))
	{
		return;
	}

	CalcIsAttackCritical();

#ifdef GAME_DLL
	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN && GetWeaponID() != TF_WEAPON_GATLINGGUN && GetWeaponID() != TFC_WEAPON_ASSAULTCANNON )
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if (PrimaryAttackSwapsActivities())
	{
		if (!m_bSwapFire)
			DoViewModelAnimation();
		else
		{
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			float flSpeedMultiplier = 1.0f;
			if ( pPlayer->m_Shared.InCond( TF_COND_HASTE ) )
				flSpeedMultiplier = of_haste_fire_multiplier.GetFloat();
			
			CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
			if ( vm )
			{
				vm->SetPlaybackRate( 1.0f / flSpeedMultiplier );
			}
		}

		m_bSwapFire = !m_bSwapFire;
	}
	else
	{
		DoViewModelAnimation();
	}

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	//fire line tracer to determine if projectile will hit a player, needed for Impressive medal
#ifdef GAME_DLL
	if (WeaponID_IsSniperRifle(GetWeaponID()))
	{
		Vector vecSrc = pPlayer->Weapon_ShootPosition();

		trace_t tr;
		UTIL_TraceLine(vecSrc, vecSrc + pPlayer->GetAutoaimVector(0) * GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.m_pEnt)
		{
			if (tr.m_pEnt->IsPlayer())
				pPlayer->m_iImpressiveCount++;
			else
				pPlayer->m_iImpressiveCount = 0;
		}
	}
#endif

	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	int i = 1;
	bool bProjectile = !(GetProjectileType() == TF_PROJECTILE_BULLET || GetProjectileType() == TF_PROJECTILE_NONE);
	CBaseEntity *pProjectile = FireProjectile(pPlayer);
	while( bProjectile && i < GetBulletsPerShot() )
	{
		RandomSeed( iSeed );
		pProjectile = FireProjectile(pPlayer);
		i++;
		iSeed++;
	}
  
	OnShotFired();

	m_iDamageIncrease += m_pWeaponInfo->m_iContinuousFireDamageIncrease;
	m_flBlastRadiusIncrease += m_pWeaponInfo->m_flContinuousFireBlastRadiusIncrease;

	// Set next attack times.
	if ( GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay == 0 )
	{
		if( Clip1() <= 0 && ReserveAmmo() <= 0 ) // Stikynote
			m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->m_flLastShotDelay;
		else
			m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}
	
	m_flAccurateAtTick = gpGlobals->curtime + 1.25;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	if ( Clip1() > 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	
	if ( !InBurst() )
		m_bWindingUp = false;	

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}	

void CTFWeaponBaseGun::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;
 
	if( LoadsManualy() && !( pOwner->m_nButtons & (IN_ATTACK | IN_RELOAD) ) && m_iClip1 > 0 )
	{
		AbortReload();
		m_bInBarrage = true;
	}
	
	if( LoadsManualy() && InBarrage() )
		pOwner->m_nButtons |= IN_ATTACK;	
 
	if ( InBurst() && m_flNextShotTime < gpGlobals->curtime )
		BurstFire();

	if ( FiresInBursts() && pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK ) && m_flNextPrimaryAttack < gpGlobals->curtime && m_iClip1 > 0 )
	{
		BeginBurstFire();
	}

	BaseClass::ItemPostFrame();	
	
}

bool CTFWeaponBaseGun::FiresInBursts( void )
{
	return BaseClass::FiresInBursts();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();	
	
	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer )
{
	int iProjectile = GetProjectileType();
	CBaseEntity *pProjectile = NULL;

	switch( iProjectile )
	{
	case TF_PROJECTILE_BULLET:
		FireBullet( pPlayer );
		break;

	case TF_PROJECTILE_ROCKET:
		pProjectile = FireRocket( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_SYRINGE:
		pProjectile = FireNail( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_NAIL:
		pProjectile = FireNail( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB:
		pProjectile = FirePipeBomb( pPlayer, false );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB_REMOTE:
		pProjectile = FirePipeBomb( pPlayer, true );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_TRIPMINE:
		pProjectile = FireTripmine( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_INCENDROCKET:
		pProjectile = FireIncendRocket( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB_DM:
		pProjectile = FirePipeBombDM( pPlayer, false );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_TRANQ:
		pProjectile = FireNail( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;
	case TF_PROJECTILE_COOM:
		pProjectile = FireCoom( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_BOUNCYROCKET:
		pProjectile = FireBouncer(pPlayer);
		pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
		break;

	case TF_PROJECTILE_RIPPER:
		pProjectile = FireSawblade( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_NONE:
	default:
		// do nothing!
		DevMsg( "Weapon does not have a projectile type set\n" );
		break;
	}

	return pProjectile;
}

void CTFWeaponBaseGun::OnShotFired( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	
#ifdef GAME_DLL
	if( !pPlayer )
		return;

	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();
	pPlayer->m_Shared.RemoveCond( TF_COND_SPAWNPROTECT );
#endif

	if ( m_iClip1 != -1 )
	{
		if ( !of_noreload.GetBool() || ReserveAmmo() <= 0 || ( GetWeaponID() == TF_WEAPON_SUPERSHOTGUN || GetWeaponID() == TF_WEAPON_ETERNALSHOTGUN ) )
		{
			m_iClip1 -= m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
		}
		else if ( !of_infiniteammo.GetBool() ) 
		{
			m_iReserveAmmo -= 1;
		}	
	}
	else
	{
		if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
		{
			if ( !of_infiniteammo.GetBool() ) 
				m_iReserveAmmo -= m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
		}
		else if ( !of_infiniteammo.GetBool() ) 
		{
			pPlayer->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iSecondaryAmmoType );
		}
	}

	DoFireEffects();

	UpdatePunchAngles( pPlayer );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;

	if ( flPunchAngle > 0 )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		pPlayer->SetPunchAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bullet!
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
{
	if ( GetWeaponID() != TF_WEAPON_LIGHTNING_GUN )
		PlayWeaponShootSound();

	bool bFirstShot =  m_flAccurateAtTick < gpGlobals->curtime;
	
	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		GetWeaponID(),
		m_iWeaponMode,
		CBaseEntity::GetPredictionRandomSeed() & 255,
		GetWeaponSpread(),
		!GetTFWpnData().m_bNoFixedSpread,
		GetBulletsPerShot(),
		GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flRange,
		GetTFWpnData().iAmmoType,
		GetProjectileDamage(),
		IsCurrentAttackACrit(),
		bFirstShot,
		GetTFWpnData().m_iTracerFrequency );
}

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		{
			return false;
		}

		return true;
	}

	int m_iIgnoreTeam;
};

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /* = true */)
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}
	else
	{
		int team = pPlayer->GetTeamNumber();
		if ( team == TF_TEAM_MERCENARY ) team = 0;		
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, team );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}

	// Offset actual start point
	*vecSrc = vecShootPos + (vecForward * vecOffset.x) + (vecRight * vecOffset.y) + (vecUp * vecOffset.z);

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 )
	{
		VectorAngles( tr.endpos - *vecSrc, *angForward );
	}
	else
	{
		VectorAngles( endPos - *vecSrc, *angForward );
	}
	
	float flSpread = GetWeaponSpread();
	angForward->x += RandomFloat( -flSpread, flSpread );
	angForward->y += RandomFloat( -flSpread, flSpread );
}

//-----------------------------------------------------------------------------
// Purpose: Fire a BFG Projectile
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireCoom( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	
	bool bCenter = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bCenterfireProjectile;

	int iQuakeCvar = 0;
	
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered") );

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );	

	if ( bCenter || iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFBFGProjectile *pProjectile = CTFBFGProjectile::Create( this, vecSrc, angForward, pPlayer, pPlayer, GetProjectileSpeed() );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetLauncher( this );
		pProjectile->SetKillIcon( GetClassname() );
		if( GetTFWpnData().m_bDropBomblets )
		{
			pProjectile->m_pBombletInfo = new bomblet_t();
			SetupBombletInfo(pProjectile->m_pBombletInfo, this, IsCurrentAttackACrit());
		}
	}
	return pProjectile;

#endif

	return NULL;
}

Vector vecQuadOffset[] =
{
	Vector( 0, 0,-8 ),
	Vector( 0,-8,-8 ),
	Vector( 0,-8, 0 ),
	Vector( 0, 0, 0 ),
};

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	
	bool bCenter = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bCenterfireProjectile;

	int iQuakeCvar = 0;
	
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_centered") );

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );	

	if ( bCenter || iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	
	if( GetWeaponID() == TF_WEAPON_SUPER_ROCKETLAUNCHER )
		vecOffset += vecQuadOffset[Clip1() - (Clip1() / 5) - 1];
	
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create(this, vecSrc, angForward, pPlayer, pPlayer, GetProjectileSpeed());
	if ( pProjectile )
	{
		CTFSuperRocketLauncher *pQuad = dynamic_cast<CTFSuperRocketLauncher*>(this);
		if( pQuad && pQuad->m_bHoming )
		{
			if( !pQuad->m_hTargetDot )
				pQuad->CreateTargetDot();
			pProjectile->SetHomingTarget( pQuad->m_hTargetDot );		
		}
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetLauncher( this );
		pProjectile->SetKillIcon( GetClassname() );
		if( GetTFWpnData().m_bDropBomblets )
		{
			pProjectile->m_pBombletInfo = new bomblet_t();
			SetupBombletInfo(pProjectile->m_pBombletInfo, this, IsCurrentAttackACrit());
		}
	}
	
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a projectile nail
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, int iSpecificNail )
{
	PlayWeaponShootSound();

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 16,6,-8 );
	
	int iQuakeCvar = 0;
	
#ifdef GAME_DLL
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered") );	
#else
	extern ConVar viewmodel_centered;
	iQuakeCvar = V_atoi(viewmodel_centered.GetString());
#endif	

	if ( iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset , &vecSrc, &angForward );

	CTFBaseProjectile *pProjectile = NULL;
	switch( iSpecificNail )
	{
	case TF_PROJECTILE_SYRINGE:
		pProjectile = CTFProjectile_Syringe::Create( vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), GetProjectileSpeed() );
		break;

	case TF_PROJECTILE_TRANQ:
		pProjectile = CTFProjectile_Tranq::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), GetProjectileSpeed());
		break;
		
    case TF_PROJECTILE_NAIL:
		pProjectile = CTFProjectile_Nail::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), GetProjectileSpeed());
		break;

	default:
		Assert(0);
	}

	if ( pProjectile )
	{
		pProjectile->SetWeaponID( GetWeaponID() );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
#ifdef GAME_DLL
		pProjectile->SetDamage( GetProjectileDamage() );
#endif
		pProjectile->SetKillIcon( GetClassname() );
	}
	
	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a  pipe bomb
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FirePipeBomb( CTFPlayer *pPlayer, bool bRemoteDetonate )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	QAngle vecAngles = pPlayer->EyeAngles();

	float flSpread = GetWeaponSpread();
	vecAngles.x += RandomFloat( -flSpread, flSpread );
	vecAngles.y += RandomFloat( -flSpread, flSpread );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	
	bool bCenter = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bCenterfireProjectile;

	int iQuakeCvar = 0;
	
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered") );

	Vector vecOffset( 16.0f, 8.0f, -6.0f );	

	if ( bCenter || iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	
	vecSrc += vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;
	
	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );
		
	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( vecSrc, pPlayer->EyeAngles(), vecVelocity, 
		AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
		pPlayer, GetTFWpnData(), bRemoteDetonate, this );


	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );
		pProjectile->SetKillIcon( GetClassname() );
		if( GetTFWpnData().m_bDropBomblets )
		{
			pProjectile->m_pBombletInfo = new bomblet_t();
			SetupBombletInfo(pProjectile->m_pBombletInfo, this, IsCurrentAttackACrit());
		}
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a DM pipe bomb (no rotation midair)
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FirePipeBombDM( CTFPlayer *pPlayer, bool bRemoteDetonate )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	QAngle vecAngles = pPlayer->EyeAngles();

	float flSpread = GetWeaponSpread();
	vecAngles.x += RandomFloat( -flSpread, flSpread );
	vecAngles.y += RandomFloat( -flSpread, flSpread );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	
	bool bCenter = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bCenterfireProjectile;

	int iQuakeCvar = 0;
	
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered") );

	Vector vecOffset( 16.0f, 8.0f, -6.0f );	

	if ( bCenter || iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	
	vecSrc +=  vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;

	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + vecRight;

	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( vecSrc, pPlayer->EyeAngles(), vecVelocity,
		AngularImpulse( 0, 0, 0 ),
		pPlayer, GetTFWpnData(), bRemoteDetonate, this );

	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );
		pProjectile->SetKillIcon( GetClassname() );
		if( GetTFWpnData().m_bDropBomblets )
		{
			pProjectile->m_pBombletInfo = new bomblet_t();
			SetupBombletInfo(pProjectile->m_pBombletInfo, this, IsCurrentAttackACrit());
		}
	}

	return pProjectile;

#endif

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: "Fires" a tripmine
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireTripmine( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	CTripmineGrenade *pProjectile = NULL;

	Vector vecAiming	= pPlayer->GetAutoaimVector( 0 );
	Vector vecSrc		= pPlayer->Weapon_ShootPosition( );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

	CBaseEntity *pEntity = tr.m_pEnt;
	if ( pEntity && !( pEntity->GetFlags() & FL_CONVEYOR ) )
	{
		QAngle angles;
		VectorAngles( tr.plane.normal, angles );

		pProjectile = CTripmineGrenade::Create( this, tr.endpos + tr.plane.normal * 2, angles, pPlayer );

		if (pProjectile)
		{
			const CTFWeaponInfo *pInfo = &GetTFWpnData();

			if (pInfo)
			{
				pProjectile->SetDamageRadius(GetDamageRadius());
				pProjectile->SetDamage(GetDamage());
				pProjectile->SetKillIcon( GetClassname() );
			}
		}
	}

	return pProjectile;

#else
	return NULL;
#endif
	
}

//-----------------------------------------------------------------------------
// Purpose: Fire a fire rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireIncendRocket( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	
//	bool bCenter = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bCenterfireProjectile;

	int iQuakeCvar = 0;
	
	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered") );

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );	
	if ( iQuakeCvar )
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFCProjectile_IncendRocket *pProjectile = CTFCProjectile_IncendRocket::Create( this, vecSrc, angForward, pPlayer, pPlayer, GetProjectileSpeed() );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetLauncher( this );
		pProjectile->SetKillIcon( GetClassname() );
		if( GetTFWpnData().m_bDropBomblets )
		{
			pProjectile->m_pBombletInfo = new bomblet_t();
			SetupBombletInfo(pProjectile->m_pBombletInfo, this, IsCurrentAttackACrit());
		}
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bouncing projectile
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireBouncer(CTFPlayer *pPlayer)
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	int iQuakeCvar = 0;

	if (!pPlayer->IsFakeClient())
		iQuakeCvar = V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered"));

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset(24.f, 12.0f, -6.f);
	if (iQuakeCvar)
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	GetProjectileFireSetup(pPlayer, vecOffset, &vecSrc, &angForward, false);

	CTFProjectile_BouncyRocket *pProjectile = CTFProjectile_BouncyRocket::Create(this, vecSrc, angForward, pPlayer, pPlayer, GetProjectileSpeed());
	if (pProjectile)
	{
		pProjectile->SetFuseTime( gpGlobals->curtime + GetTFWpnData().m_flFuseTime );
		pProjectile->SetCritical(IsCurrentAttackACrit());
		pProjectile->SetDamage(GetProjectileDamage());
		pProjectile->SetLauncher(this);
	}
	return pProjectile;

#endif

	return NULL;
}

CBaseEntity *CTFWeaponBaseGun::FireSawblade( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();
	
#ifdef GAME_DLL
	int iQuakeCvar = 0;

	if ( !pPlayer->IsFakeClient() )
		iQuakeCvar = V_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_centered" ) );

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 24.f, 0.0f, 0.0f );
	if ( iQuakeCvar )
	{
		vecOffset.x = 24.f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = 0.0f; //up down
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Ripper *pProjectile = CTFProjectile_Ripper::Create( this, vecSrc, angForward, pPlayer, pPlayer, GetProjectileSpeed() );
	if ( pProjectile )
	{
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );
	}

	return pProjectile;
#endif

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PlayWeaponShootSound( void )
{
	if (IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileSpeed( void )
{
	// placeholder for now
		// Not anymore :) - Kay
	// grenade launcher and pipebomb launcher hook this to set variable pipebomb speed
	
	float flBaseSpeed = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flProjectileSpeed;
	// If the weapon has this attribute, set the input variable to its value
	// In this case, if we have the set spread attribute, we set our base spread to it
	// Otherwise do nothing, aka it stays at the WeaponData value
	GetAttributeValue_Float("set projectile speed", flBaseSpeed);

	// Base multiplier is 1, since anything multiplied by 1 gives the same value
	float flSpeedMultiplier = 1;

	// If we do however have the attribute, set it to that, so we multiply our base value by it
	GetAttributeValue_Float("projectile speed multiplier", flSpeedMultiplier);
	return flBaseSpeed * flSpeedMultiplier;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetWeaponSpread( void )
{
	float flBaseSpread = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flSpread;
	// If the weapon has this attribute, set the input variable to its value
	// In this case, if we have the set spread attribute, we set our base spread to it
	// Otherwise do nothing, aka it stays at the WeaponData value
	GetAttributeValue_Float("set spread", flBaseSpread);

	// Base multiplier is 1, since anything multiplied by 1 gives the same value
	float flSpreadMultiplier = 1;

	// If we do however have the attribute, set it to that, so we multiply our base value by it
	GetAttributeValue_Float("spread multiplier", flSpreadMultiplier);
	return flBaseSpread * flSpreadMultiplier;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileDamage( void )
{
	float flBaseDamage = (float)GetDamage();

	float flDamageMultiplier = 1;
	GetAttributeValue_Float("damage multiplier", flDamageMultiplier);

	return flBaseDamage * flDamageMultiplier;
}

int CTFWeaponBaseGun::GetBulletsPerShot( void ) const
{
	int iBaseBullets = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_nBulletsPerShot;
	GetAttributeValue_Int("set bullets per shot", iBaseBullets);

	float flBulletsMultiplier = 1;
	GetAttributeValue_Float("bullets per shot multiplier", flBulletsMultiplier);
	return iBaseBullets * flBulletsMultiplier;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
	
	m_bWindingUp = false;
	
#if !defined( CLIENT_DLL )

	// Make sure to zoom out before we holster the weapon.
	ZoomOut();
	SetContextThink( NULL, 0, ZOOM_CONTEXT );

#endif

	return BaseClass::Holster( pSwitchingTo );
}
//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoFireEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Muzzle flash on weapon.
	
	bool bMuzzleFlash = true;

	/*
	if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
		if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN || pWeapon->GetWeaponID() == TF_WEAPON_GATLINGGUN || pWeapon->GetWeaponID() == TFC_WEAPON_ASSAULTCANNON ) )
		{
			bMuzzleFlash = false;
		}
	}
	*/

	if ( bMuzzleFlash )
	{
		pPlayer->DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ToggleZoom( void )
{
	// Toggle the zoom.
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer )
	{
		if( pPlayer->GetFOV() >= 65 )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

	// Get the zoom animation time.
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomIn( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Set the weapon zoom.
	// TODO: The weapon fov should be gotten from the script file.
	pPlayer->SetFOV( pPlayer, TF_WEAPON_ZOOM_FOV, 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOut( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TF_COND_ZOOMED );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOutIn( void )
{
	//Zoom out, set think to zoom back in.
	ZoomOut();
	SetContextThink( &CTFWeaponBaseGun::ZoomIn, gpGlobals->curtime + ZOOM_REZOOM_TIME, ZOOM_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoViewModelAnimation( void )
{
	Activity act;
	if ( m_pWeaponInfo->m_bLastShotAnim && Clip1() <= 0 && ReserveAmmo() <= 0 )
		act = ( IsCurrentAttackACrit() && GetTFWpnData().m_bUsesCritAnimation ) ? ACT_VM_LAST_PRIMARYATTACK_CRIT : ACT_VM_LAST_PRIMARYATTACK;
	else
		act = ( IsCurrentAttackACrit() && GetTFWpnData().m_bUsesCritAnimation ) ? ACT_VM_PRIMARYATTACK_CRIT : ACT_VM_PRIMARYATTACK;
	SendWeaponAnim( act );
	
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if( !pPlayer )
		return;
	float flSpeedMultiplier = 1.0f;
	if ( pPlayer->m_Shared.InCond( TF_COND_HASTE ) )
		flSpeedMultiplier = of_haste_fire_multiplier.GetFloat();
	
	CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( vm )
	{
		vm->SetPlaybackRate( 1.0f / flSpeedMultiplier );
	}
}
