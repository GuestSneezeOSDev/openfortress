//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "tf_gamerules.h"
#include "in_buttons.h"

#ifdef GAME_DLL
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "tf_fx.h"
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMelee, DT_TFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTFWeaponBaseMelee, DT_TFWeaponBaseMelee )
#ifdef GAME_DLL
	SendPropFloat( SENDINFO(m_flChargeMeter), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargeMeter) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseMelee )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_melee, CTFWeaponBaseMelee );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponBaseMelee )
DEFINE_FUNCTION( Smack )
END_DATADESC()
#endif

#ifdef GAME_DLL
ConVar tf_meleeattackforcescale( "tf_meleeattackforcescale", "80.0", FCVAR_CHEAT | FCVAR_GAMEDLL );
ConVar of_grav_debug("of_grav_debug", "0", FCVAR_REPLICATED, "Shows the debug spheres and boxes for reflecting.");
#endif

// Gravity Gaunlets / Melee Airblasts Cvars Start
ConVar of_grav_reflect_type("of_grav_reflect_type", "0", FCVAR_REPLICATED, "If set to 1, Melee Airblasts will use a HITBOX instead of a HITSPHERE.");

ConVar of_grav_reflect_foward_dist("of_grav_reflect_foward_dist", "32", FCVAR_NOTIFY | FCVAR_REPLICATED, "The foward distance from the player's eyes the Airblast's AOE starts at.");
ConVar of_grav_reflect_angel_limit("of_grav_reflect_angel_limit", "0.5", FCVAR_NOTIFY | FCVAR_REPLICATED, "The size of the cone from the player's eyes which allows a valid reflect.");

ConVar of_grav_reflect_box_height("of_grav_reflect_box_height", "64", FCVAR_NOTIFY | FCVAR_REPLICATED, "The height of the HITBOX for a reflect. Only used if Melee Airblasts are using a HITBOX and not a HITSPHERE.");
ConVar of_grav_reflect_box_size("of_grav_reflect_box_size", "128", FCVAR_NOTIFY | FCVAR_REPLICATED,"The size of the HITBOX for a reflect. Only used if Melee Airblasts are using a HITBOX and not a HITSPHERE.");

ConVar of_grav_reflect_sphere_size("of_grav_reflect_sphere_size", "128", FCVAR_NOTIFY | FCVAR_REPLICATED, "The size of the HITSPHERE for a reflect. Only used if Melee Airblasts are using a HITSPHERE and not a HITBOX.");
// Gravity Gaunlets / Melee Airblasts Cvars End

ConVar of_melee_ignore_teammates( "of_melee_ignore_teammates", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Sets if melee can attack through teammates or not." );

ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Controls random crits for melee weapons.\n0 - Melee weapons do not randomly crit. \n1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. \n2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting.", true, 0, true, 2 );
extern ConVar tf_weapon_criticals;
extern ConVar friendlyfire;
extern ConVar of_haste_fire_multiplier;
extern ConVar of_zombie_lunge_crit;

class CTraceFilterMeleeIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterMeleeIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterMeleeIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, CBaseEntity *pOwner )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ), m_hOwner( pOwner )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsPlayer() )
		{
			if ( pEntity == m_hOwner )
				return false;

			if ( !of_melee_ignore_teammates.GetBool() )
				return true;

			if ( ( pEntity->GetTeamNumber() == m_iIgnoreTeam && !friendlyfire.GetBool() ) )
				return false;
		}

		return true;
	}

	CBaseEntity *m_hOwner;
	int m_iIgnoreTeam;
};

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBaseMelee::CTFWeaponBaseMelee()
{
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	m_bConnected = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Precache()
{
	BaseClass::Precache();

	CBaseEntity::PrecacheScriptSound("Chainsaw.Charge");
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Spawn()
{
	Precache();

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetSchemaName() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast< CTFWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in melee weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;
	Assert( m_pWeaponInfo );

	// No ammo.
	m_iClip1 = -1;
	m_flChargeMeter = 1.0f;

	BaseClass::Spawn();
}

bool CTFWeaponBaseMelee::Deploy()
{
	return BaseClass::Deploy();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	}
	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

	// Swing the weapon.
	Swing( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;
	
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	bool bStartedCharge = false;
	float flNextAttack = gpGlobals->curtime + 0.5;
	
	if ( CanShieldCharge() )
	{
		if ( m_flChargeMeter >= 1.0f )
		{
			pPlayer->m_Shared.AddCond( TF_COND_SHIELD_CHARGE );
			bStartedCharge = true;
			if (m_iChargeSound == 0)
			{
				pPlayer->EmitSound("Chainsaw.Charge");
				m_iChargeSound = 1;
				//DevMsg("This should play the charge sound\n");
			}
		}
		else
		{
			flNextAttack = gpGlobals->curtime;
		}
	}
	
	if ( !bStartedCharge )
		pPlayer->DoClassSpecialSkill();
	
	m_bInAttack2 = true;

	m_flNextSecondaryAttack = flNextAttack;
}

bool CTFWeaponBaseMelee::CanShieldCharge()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;
	
	if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return false;
	
	if ( !m_pWeaponInfo->m_bCanShieldCharge )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	pPlayer->m_Shared.RemoveCond( TF_COND_SPAWNPROTECT );
#endif
	CalcIsAttackCritical();

	// Play the melee swing and miss (r/whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();
	
	if ( !FiresInBursts() )
		// Set next attack times.
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	
//	DevMsg("Sequence duration: %f\n", SequenceDuration());
	
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}

	m_flSmackTime = gpGlobals->curtime + GetSmackDelay();
	if( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_SHIELD_CHARGE );
		pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
		m_flChargeMeter = 0.0f;
		m_iNumBeepsToBeep = 1;
		m_iChargeSound = 0;
	}
}

float CTFWeaponBaseMelee::GetSmackDelay()
{
	float flSmackDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_HASTE ) )
			flSmackDelay *= of_haste_fire_multiplier.GetFloat();
	}
	if( flSmackDelay >= GetFireRate() )
	{
		flSmackDelay = max( 0.0f, GetFireRate() - 0.001f );
	}
	return flSmackDelay;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::DoViewModelAnimation( void )
{
	Activity act;
	if ( IsCurrentAttackACrit() && GetTFWpnData().m_bUsesCritAnimation )
	{
		 act = ACT_VM_SWINGHARD;
	}
	else if (PrimaryAttackSwapsActivities())
	{
		act = ( m_bSwapFire ) ? ACT_VM_HITLEFT : ACT_VM_HITRIGHT;
		m_bSwapFire = !m_bSwapFire;
	}
	else
	{
		act = ACT_VM_HITCENTER;
	}
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

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;
	
	if ( FiresInBursts() )
	{
		if ( InBurst() && m_flNextShotTime < gpGlobals->curtime )
			BurstFire();

		if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK ) && m_flNextPrimaryAttack < gpGlobals->curtime  )
		{
			BeginBurstFire();
		}
	}
	// Check for smack.	
	if ( (m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime)  )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}
	
	ShieldChargeThink();
	
	BaseClass::ItemPostFrame();
}

void CTFWeaponBaseMelee::ItemHolsterFrame()
{
	ShieldChargeThink();
	BaseClass::ItemHolsterFrame();
}

void CTFWeaponBaseMelee::ItemBusyFrame()
{
	ShieldChargeThink();
	BaseClass::ItemBusyFrame();
}

void CTFWeaponBaseMelee::ShieldChargeThink()
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;	
	if ( pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		if ( m_pWeaponInfo->m_flChargeDuration > 0.0f )
		{
			float flChargeAmount = gpGlobals->frametime / m_pWeaponInfo->m_flChargeDuration;
			float flNewLevel = max( m_flChargeMeter - flChargeAmount, 0.0 );
			m_flChargeMeter = flNewLevel;
			if ( m_pWeaponInfo->m_flCritOnChargeLevel != -1 && m_flChargeMeter <= m_pWeaponInfo->m_flCritOnChargeLevel && !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
				pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
			if ( m_flChargeMeter <= 0.0f )
			{
				pOwner->m_Shared.RemoveCond( TF_COND_SHIELD_CHARGE );
				pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
				m_iNumBeepsToBeep = 1;
				m_iChargeSound = 0;
			}
		}
	}
	else if ( m_pWeaponInfo->m_flChargeRechargeRate > 0.0f && m_flChargeMeter < 1.0f )
	{
		float flChargeAmount = gpGlobals->frametime / m_pWeaponInfo->m_flChargeRechargeRate;
		float flNewLevel = min( m_flChargeMeter + flChargeAmount, 1 );
		m_flChargeMeter = flNewLevel;
	}
#ifdef CLIENT_DLL
	if (m_flChargeMeter == 1.0f && m_iNumBeepsToBeep > 0)
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if (!pLocalPlayer)
			return;

		pLocalPlayer->EmitSound("ChargeFilled.Ding");
		m_iNumBeepsToBeep = 0;
	}
#endif
}

void CTFWeaponBaseMelee::BurstFire( void )
{
	BaseClass::BurstFire();
}

bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &trace )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );
	float range = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flMeleeRange;
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * range;

	int team = pPlayer->GetTeamNumber();
	if ( team == TF_TEAM_MERCENARY ) 
		team = 0;

	bool bRetry = false;
	do
	{
		if( bRetry )
			team = 0;

		CTraceFilterMeleeIgnoreTeammates meleefilter( pPlayer, COLLISION_GROUP_NONE, team, pPlayer );

		// See if we hit anything.
		UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &meleefilter, &trace );
		if ( trace.fraction >= 1.0 )
		{
			UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &meleefilter, &trace );
			if ( trace.fraction < 1.0 )
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if ( !pHit || pHit->IsBSPModel() )
				{
					// Why duck hull min/max?
					FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
				}

				// This is the point on the actual surface (the hull could have hit space)
				// vecSwingEnd = trace.endpos;	
			}
		}
		
		bRetry = true;
	}
	while( !trace.m_pEnt && team != 0 );

	return ( trace.fraction < 1.0f );
}

#ifdef GAME_DLL
class CPathTrack;
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
#endif

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	trace_t trace;

	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	//CBaseEntity *pTargetEnt = NULL;

	// We hit, setup the smack.
	if ( DoSwingTrace( trace ) )
	{
		// Hit sound - immediate.
		// added check for npc
		if( trace.m_pEnt->IsPlayer() || trace.m_pEnt->IsNPC() )
		{
			WeaponSound( MELEE_HIT );
		}
		else
		{
			WeaponSound( MELEE_HIT_WORLD );
		}

		// Get the current player.
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( !pPlayer )
			return;

		if( GetTFWpnData().m_bExplosionOnHit )
		{
#ifdef GAME_DLL
			CPVSFilter filter( trace.endpos );
			
			if ( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
			{
				TE_TFExplosion( filter, 0.0f, trace.endpos, trace.plane.normal, trace.m_pEnt->entindex(), GetExplosionVisualInfo() );
			}
			else
			{
				TE_TFExplosion( filter, 0.0f,  trace.endpos, trace.plane.normal, -1, GetExplosionVisualInfo() );
			}
			
			// Use the thrower's position as the reported position
			Vector vecReported = trace.endpos;
			
			CPathTrack *pSource = static_cast<CPathTrack*>( CBaseEntity::Create( "path_track", trace.endpos, QAngle(0,0,0), pPlayer ) );
			
			int iCustomDamage = TF_DMG_CUSTOM_NONE;
			float flDamage = GetMeleeDamage( trace.m_pEnt, iCustomDamage );
			CTakeDamageInfo info( pSource, pPlayer, this, trace.endpos, trace.endpos, flDamage, GetDamageType(), 0, &vecReported );
			info.SetWeapon( this );
			info.SetDamagePosition( trace.endpos );
			info.SetDamageCustom( GetDamageType() );
	
			float flRadius = GetDamageRadius();

			RadiusDamage( info, trace.endpos, flRadius, CLASS_NONE, NULL );

			// Don't decal players with scorch.
			if ( trace.m_pEnt && !trace.m_pEnt->IsPlayer() )
			{
				UTIL_DecalTrace( &trace, "Scorch" );
			}
			
			UTIL_Remove( pSource );
#endif
			/*
			if( GetTFWpnData().m_bAirblastOnSwing )
			{
				// Where are we aiming?
				Vector vForward;
				QAngle vAngles = pPlayer->EyeAngles();
				AngleVectors( vAngles, &vForward);
				
				// First airblast the target entity, since its farther away from the other ranges and a successfull target shot should be rewarded
				if( trace.m_pEnt )
				{
					if ( trace.m_pEnt->IsCombatCharacter() )
					{
						// Convert angles to vector
						Vector vForwardDir;
						AngleVectors( vAngles, &vForwardDir );

						CBaseCombatCharacter *pCharacter = dynamic_cast<CBaseCombatCharacter *>( trace.m_pEnt );

						if ( pCharacter )
						{
							pTargetEnt = pCharacter;
							AirBlastCharacter( pCharacter, pPlayer, vForwardDir );
						}
					}
					else
					{
						// TODO: vphysics specific tracing!

						Vector vecPos = trace.m_pEnt->GetAbsOrigin();
						Vector vecAirBlast;

						// TODO: handle trails here i guess?
						GetProjectileAirblastSetup( pPlayer, vecPos, &vecAirBlast, false );
						
						pTargetEnt = trace.m_pEnt;
						AirBlastProjectile( trace.m_pEnt, pPlayer, this, vecAirBlast );
					}
				}
			}
			*/
		}

		Vector vecForward; 
		AngleVectors( pPlayer->EyeAngles(), &vecForward );
		Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
		Vector vecSwingEnd = vecSwingStart + vecForward * 48;


		// Do Damage.
		int iCustomDamage = TF_DMG_CUSTOM_NONE;
		float flDamage = GetMeleeDamage( trace.m_pEnt, iCustomDamage );
#ifdef GAME_DLL
		int iDmgType = GetDamageType();
		int iCritValue;

		if (of_zombie_lunge_crit.GetBool())
		{
			iCritValue = pPlayer->m_Shared.IsLunging() ? 2 : IsCurrentAttackACrit();
		}
		else
		{
			iCritValue = IsCurrentAttackACrit();
		}
		
		if ( iCritValue )
		{
			// TODO: Not removing the old critical path yet, but the new custom damage is marking criticals as well for melee now.
			iDmgType |= DMG_CRITICAL;
			if (iCritValue >= 2)
			{
				iCustomDamage = TF_DMG_CUSTOM_CRIT_POWERUP;
			}
		}
		CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );
		CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * tf_meleeattackforcescale.GetFloat() );
		trace.m_pEnt->DispatchTraceAttack( info, vecForward, &trace ); 
		ApplyMultiDamage();

		OnEntityHit( trace.m_pEnt );
#endif
		// Don't impact trace friendly players or objects
		if ( trace.m_pEnt && trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() )
		{
#ifdef CLIENT_DLL
			UTIL_ImpactTraceDamage( &trace, DMG_CLUB, flDamage );
#endif
			m_bConnected = true;
		}
	}

	if (GetTFWpnData().m_bAirblastOnSwing)
	{
		// Get the current player.
		CTFPlayer* pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			// Where are we aiming?
			Vector vForward;
			QAngle vAngles = pPlayer->EyeAngles();
			AngleVectors(vAngles, &vForward);

			int count = 0;
			CBaseEntity* pList[64];

			if (of_grav_reflect_type.GetBool())
			{
				// "256x256x128 HU box"
				Vector vAirBlastBox = Vector(of_grav_reflect_box_size.GetFloat(), of_grav_reflect_box_size.GetFloat(), of_grav_reflect_box_height.GetFloat());

				// TODO: this isn't an accurate distance
				// offset the box origin from our shoot position
				float flDist = of_grav_reflect_foward_dist.GetFloat();

				// Used as the centre of the box trace
				Vector vOrigin = pPlayer->Weapon_ShootPosition() + vForward * flDist;

				//CBaseEntity *pList[ 32 ];

				count = UTIL_EntitiesInBox(pList, 64, vOrigin - vAirBlastBox, vOrigin + vAirBlastBox, 0);

			#ifdef GAME_DLL
				if (of_grav_debug.GetBool())
				{
					NDebugOverlay::Box(vOrigin, (vAirBlastBox * -1), vAirBlastBox, 0, 0, 255, 25, 0.25);
				}
			#endif
			}
			else
			{
				// TODO: this isn't an accurate distance
				// offset the box origin from our shoot position
				float flDist = of_grav_reflect_foward_dist.GetFloat();

				// Used as the centre of the box trace
				Vector vOrigin = pPlayer->Weapon_ShootPosition() + vForward * flDist;

				//CBaseEntity *pList[ 32 ];

			#ifdef GAME_DLL
				if (of_grav_debug.GetBool())
					NDebugOverlay::Sphere(vOrigin, of_grav_reflect_sphere_size.GetFloat(), 0, 255, 0, false, 0.5f);
			#endif
				count = UTIL_EntitiesInSphere(pList, 64, vOrigin, of_grav_reflect_sphere_size.GetFloat(), 0);
			}

			for (int i = 0; i < count; i++)
			{
				CBaseEntity* pEntity = pList[i];

				if (!pEntity)
					continue;

				if (!pEntity->IsAlive())
					continue;

				if (pEntity->GetTeamNumber() < TF_TEAM_RED)
					continue;

				if (pEntity == pPlayer)
					continue;

				if (!pEntity->IsDeflectable())
					continue;

				trace_t trWorld;

				// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
				// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
				// determined using the flame's bounding box and we're just doing a ray test here, we extend the
				// start point out by the radius of the box.
				// UTIL_TraceLine( GetAbsOrigin() + vDir * WorldAlignMaxs().x, m_vecInitialPos, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );		
				UTIL_TraceLine(pPlayer->Weapon_ShootPosition(), pEntity->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld);

				/*
				if (of_grav_reflect_type.GetBool())
				{
					Vector2D vec_player_to_target = (pEntity->WorldSpaceCenter() - pPlayer->WorldSpaceCenter()).AsVector2D();
					vec_player_to_target.NormalizeInPlace();

					Vector temp1;

					GetTFPlayerOwner()->EyeVectors(&temp1);
					Vector2D eye_vec = temp1.AsVector2D();
					eye_vec.NormalizeInPlace();
					float flAngle = DotProduct2D(vec_player_to_target, eye_vec) / (vec_player_to_target.Length() * eye_vec.Length());

					if (flAngle < of_grav_reflect_angel_limit.GetFloat())
						continue;
				}
				*/

				Vector vec_player_to_target1 = (pEntity->WorldSpaceCenter() - pPlayer->WorldSpaceCenter());
				vec_player_to_target1.NormalizeInPlace();

				Vector temp2;

				GetTFPlayerOwner()->EyeVectors(&temp2);
				Vector eye_vec1 = temp2;
				eye_vec1.NormalizeInPlace();

				float flAngel = DotProduct(vec_player_to_target1, eye_vec1) / (vec_player_to_target1.Length() * eye_vec1.Length());

				if (flAngel < of_grav_reflect_angel_limit.GetFloat())
					continue;

				// can't see it!
				if (trWorld.fraction != 1.0f)
					continue;

				if (pEntity->IsCombatCharacter())
				{
					if (!(GetTFWpnData().m_bNoAirblastOnSwingForPlayers))
					{
						// Convert angles to vector
						Vector vForwardDir;
						AngleVectors(vAngles, &vForwardDir);

						CBaseCombatCharacter* pCharacter = dynamic_cast<CBaseCombatCharacter*>(pEntity);

						if (pCharacter)
						{
							AirBlastCharacter(pCharacter, pPlayer, vForwardDir);

						#ifdef GAME_DLL
							pCharacter->EmitSound("TFPlayer.MeleeReflectImpact");

							if (of_grav_debug.GetBool())
								NDebugOverlay::Box(pCharacter->GetAbsOrigin(), Vector(1, 1, 1), -Vector(1, 1, 1), 0, 255, 0, 125, 1);
						#endif
						}
					}
				}
				else
				{
					// TODO: vphysics specific tracing!

					Vector vecPos = pEntity->GetAbsOrigin();
					Vector vecAirBlast;

					// TODO: handle trails here i guess?
					GetProjectileAirblastSetup(GetTFPlayerOwner(), vecPos, &vecAirBlast, false);

					AirBlastProjectile(pEntity, pPlayer, this, vecAirBlast);

					#ifdef GAME_DLL
					pEntity->EmitSound("Weapon_General.ProjectileReflect");

					if (of_grav_debug.GetBool())
						NDebugOverlay::Box(vecPos, Vector(1, 1, 1) , -Vector(1, 1, 1), 0, 255, 0, 125, 1);
					#endif
				}
			}
		}
	}

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flDamage = 0;

	if( TFGameRules()->IsMutator(NO_MUTATOR) || TFGameRules()->GetMutator() > INSTAGIB_NO_MELEE )
	{
		flDamage = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_nDamage;
		GetAttributeValue_Float("set damage", flDamage);
	}
	else
		flDamage = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_nInstagibDamage;

	float flDamageMultiplier = 1;
	GetAttributeValue_Float("damage multiplier", flDamageMultiplier);

	return flDamage * flDamageMultiplier;
}

void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (pPlayer && TFGameRules()->GetIT() && ToBasePlayer( pEntity ))
	{
		if (TFGameRules()->GetIT() == pPlayer)
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "tagged_player_as_it" );
			if (event)
			{
				event->SetInt( "player", engine->GetPlayerUserId( pPlayer->edict() ) );

				gameeventmanager->FireEvent( event );
			}

			UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_ANNOUNCE_TAG", pPlayer->GetPlayerName(), ToBasePlayer( pEntity )->GetPlayerName() );

			CSingleUserRecipientFilter filter( pPlayer );
			CBaseEntity::EmitSound( filter, pPlayer->entindex(), "Player.TaggedOtherIT" );

			TFGameRules()->SetIT( pEntity );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelper( void )
{
	
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
		return true;
	
	int nCvarValue = tf_weapon_criticals_melee.GetInt();
	if ( nCvarValue == 0 )
		return false;

	if ( nCvarValue == 1 && !tf_weapon_criticals.GetBool() )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	float flPlayerCritMult = pPlayer->GetCritMult();

	return ( RandomInt( 0, WEAPON_RANDOM_RANGE-1 ) <= ( TF_DAMAGE_CRIT_CHANCE_MELEE * flPlayerCritMult ) * WEAPON_RANDOM_RANGE );
}

int CTFWeaponBaseMelee::IsCurrentAttackACritical()
{
	int nCritMod = m_bCurrentAttackIsCrit;
	if ( nCritMod )
	{
		return m_bCurrentAttackIsCrit;
	}
	else
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_CRIT_POWERUP ) )
			return 2;
		else
			return false;
	}
	return m_bCurrentAttackIsCrit;
}
