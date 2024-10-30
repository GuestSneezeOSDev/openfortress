//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "of_weapon_chainsaw.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"

// Server specific.
#else
#include "tf_player.h"
#endif

extern ConVar of_melee_ignore_teammates;
extern ConVar friendlyfire;
extern ConVar of_haste_reload_multiplier;

#define OF_CHAINSAW_WINDUP_TIME 1.0f

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

acttable_t CTFChainsaw::m_acttableChainsaw_Mercenary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_CHAINSAW, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_CHAINSAW, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_CHAINSAW, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_CHAINSAW, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_CHAINSAW, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_CHAINSAW, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_CHAINSAW, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_CHAINSAW, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_CHAINSAW, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_CHAINSAW, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_CHAINSAW, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_CHAINSAW, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_CHAINSAW, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_CHAINSAW, false },

	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_MERC_CHAINSAW_ATTACK_STAND_PREFIRE, false },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_MERC_CHAINSAW_ATTACK_STAND_POSTFIRE, false },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_MERC_CHAINSAW_ATTACK_STAND_STARTFIRE, false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_MERC_CHAINSAW_ATTACK_CROUCH_PREFIRE, false },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_MERC_CHAINSAW_ATTACK_CROUCH_POSTFIRE, false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_MERC_CHAINSAW_ATTACK_SWIM_PREFIRE, false },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_MERC_CHAINSAW_ATTACK_SWIM_POSTFIRE, false },
};

acttable_t *CTFChainsaw::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableChainsaw_Mercenary);
		return m_acttableChainsaw_Mercenary;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

//=============================================================================
//
// Weapon Chainsaw tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFChainsaw, DT_TFWeaponChainsaw )

BEGIN_NETWORK_TABLE( CTFChainsaw, DT_TFWeaponChainsaw )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropInt( RECVINFO( m_bCritShot ) )
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bCritShot ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFChainsaw )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_chainsaw, CTFChainsaw );
//PRECACHE_WEAPON_REGISTER( tf_weapon_chainsaw );

CTFChainsaw::CTFChainsaw()
{
	m_iWeaponState = CS_IDLE;
	m_iMinigunSoundCur = SPECIAL2;
}

bool CTFChainsaw::DoSwingTrace( trace_t &trace )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -0, -0, -0 );
	static Vector vecSwingMaxs( 0, 0, 0 );
	float range = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flMeleeRange;
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
	{
		return false;
	}

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * range;

	int team = pPlayer->GetTeamNumber();
	if ( team == TF_TEAM_MERCENARY ) 
	{
		team = 0;		
	}

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
			vecSwingEnd = trace.endpos;	
		}
	}

	return ( trace.fraction < 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::PrimaryAttack()
{
	SharedAttack();
}

void CTFChainsaw::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
	{
		return;
	}
	
	switch ( m_iWeaponState )
	{
	default:
	case CS_IDLE:
		{
			// Removed the need for cells to powerup the AC
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				WindUp();
			}
			break;
		}
	case CS_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				m_iWeaponState = CS_FIRING;
#ifdef GAME_DLL
				pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );

				pPlayer->RemoveInvisibility();
				pPlayer->RemoveDisguise();
				pPlayer->m_Shared.RemoveCond( TF_COND_SPAWNPROTECT );
#endif
				// Play the melee swing and miss (r/whoosh) always.
				SendPlayerAnimEvent( pPlayer );

				DoViewModelAnimation();
				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			break;
		}
	case CS_FIRING:
		{
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
			// Only fire if we're actually shooting
				BaseClass::Smack();		// fire
#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
#endif
				if ( !FiresInBursts() ) // Since smack doesn't actually set the timers, we do it here instead
				{
					// Set next attack times.
					m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
				}

				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				m_flTimeWeaponIdle = gpGlobals->curtime + 0.2;
			}
			break;
		}
	case CS_DRYFIRE:
		{
			if ( ReserveAmmo() > 0 )
			{
				m_iWeaponState = CS_FIRING;
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				m_iWeaponState = CS_SPINNING;
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case CS_SPINNING:
		{
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( ReserveAmmo() > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					m_iWeaponState = CS_FIRING;
				}
				else
				{
					m_iWeaponState = CS_DRYFIRE;
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WindDown(void)
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
	{
		return;
	}

	SendWeaponAnim(ACT_MP_ATTACK_STAND_POSTFIRE);

	// Set the appropriate firing state.
	m_iWeaponState = CS_IDLE;
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFChainsaw::ItemPostFrame()
{
	CTFPlayer *pOwner = ToTFPlayer(GetOwner());
	if( !pOwner )
	{
		return;
	}

	if( pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE) )
	{
		PrimaryAttack();
	}

	if( 
		m_iWeaponState > CS_IDLE && 
		m_flNextPrimaryAttack < gpGlobals->curtime &&
		(!(pOwner->m_nButtons & IN_ATTACK) && !pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE) ) 
		)
	{
		if( pOwner )
		{
			pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_POST);
		}
		WindDown();
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFChainsaw::SendWeaponAnim( int iActivity )
{
	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
		{
			return true;
		}

		// Otherwise, play the start it
		return BaseClass::SendWeaponAnim( ACT_VM_PRIMARYATTACK );		
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFChainsaw::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > CS_IDLE )
	{
		m_iWeaponState = CS_IDLE;
#ifdef CLIENT_DLL
		WeaponSoundUpdate();
		StopRevSounds();
#endif
		// Time to weapon idle.
		m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
		return;

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != CS_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
		if (pPlayer)
		{
			pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_POST);
		}

		WindDown();

#ifdef CLIENT_DLL 
		WeaponSoundUpdate();
#endif
		m_iMinigunSoundCur = WPN_DOUBLE; // We do this do prevent it spamming the sound again because sauce(tm)
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFChainsaw::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
	{
		return;
	}
	
	float flSpeedMultiplier = 1.0f;
	
	if( pPlayer->m_Shared.InCond(TF_COND_HASTE) )
	{
		flSpeedMultiplier *= of_haste_reload_multiplier.GetFloat();
	}
	
	if( pPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) )
	{
		flSpeedMultiplier *= 0.5f;
	}
	
	float flWindupTime = OF_CHAINSAW_WINDUP_TIME * flSpeedMultiplier;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	CBaseViewModel *vm = pPlayer->GetViewModel(m_nViewModelIndex);
	if( vm )
	{
		vm->SetPlaybackRate( 1.0f / flSpeedMultiplier );
		{}
	}
	// Set the appropriate firing state.
	m_iWeaponState = CS_STARTFIRING;

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + flWindupTime;
	m_flNextSecondaryAttack = gpGlobals->curtime + flWindupTime;
	m_flTimeWeaponIdle = gpGlobals->curtime + flWindupTime;
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
}
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if (!(IsCarrierAlive() && (WeaponState() == WEAPON_IS_ACTIVE)))
	{
		StopRevSounds();
	}

	WeaponSoundUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFChainsaw::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case CS_IDLE:
			iSound = SPECIAL2;	// wind down sound
			if (WeaponState() == WEAPON_IS_ACTIVE)
			{
				StartRevSounds(); //Loops forever when not being held, buggy as all hell
			}
		break;
	case CS_STARTFIRING:
		iSound = SPECIAL1;	// wind up sound
		StopRevSounds();
		break;
	case CS_FIRING:
		{
			if ( m_flNextPrimaryAttack > gpGlobals->curtime )
				return;
			if ( m_bCritShot ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
			StopRevSounds();
		}
		break;
	default:
		Assert( false );
		break;
	}

	// if we're already playing the desired sound, nothing to do
	if ( m_iMinigunSoundCur == iSound )
	{
		return;
	}

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iMinigunSoundCur = iSound;
	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	// play the appropriate sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}

#if defined( CLIENT_DLL )

bool CTFChainsaw::Deploy(void)
{
	StartRevSounds();

	return BaseClass::Deploy();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::StartRevSounds()
{
	if (!m_pRevChainsawSound)
	{

		// Create the looping pilot light sound
		const char *revchainsawsound = GetShootSound(SPECIAL3);
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pRevChainsawSound = controller.SoundCreate(filter, entindex(), revchainsawsound);

		controller.Play(m_pRevChainsawSound, 1.0, 100);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::StopRevSounds()
{
	if (m_pRevChainsawSound)
	{
		CSoundEnvelopeController::GetController().SoundDestroy(m_pRevChainsawSound);
		m_pRevChainsawSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFChainsaw::UpdateOnRemove(void)
{
	StopRevSounds();

	BaseClass::UpdateOnRemove();
}

#endif

#endif
