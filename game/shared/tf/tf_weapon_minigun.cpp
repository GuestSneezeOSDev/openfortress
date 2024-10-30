//====== Copyright � 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_minigun.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "soundenvelope.h"
#else
	#include "tf_player.h"
#endif

#define MAX_BARREL_SPIN_VELOCITY	20
#define TF_MINIGUN_WINDUP_TIME 1.0f
#define OF_GATLINGGUN_WINDUP_TIME 0.5f

extern ConVar of_haste_reload_multiplier;

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigun, DT_WeaponMinigun )

BEGIN_NETWORK_TABLE( CTFMinigun, DT_WeaponMinigun )
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

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFMinigun )
	DEFINE_FIELD(  m_iWeaponState, FIELD_INTEGER ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_minigun, CTFMinigun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_minigun );

IMPLEMENT_NETWORKCLASS_ALIASED( TFGatlingGun, DT_WeaponGatlingGun )

BEGIN_NETWORK_TABLE( CTFGatlingGun, DT_WeaponGatlingGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGatlingGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_gatlinggun, CTFGatlingGun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_gatlinggun );


IMPLEMENT_NETWORKCLASS_ALIASED( TFCAssaultCannon, DT_TFCAssaultCannon )

BEGIN_NETWORK_TABLE( CTFCAssaultCannon, DT_TFCAssaultCannon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCAssaultCannon)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_assaultcannon, CTFCAssaultCannon );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_assaultcannon );

#ifdef CLIENT_DLL

extern ConVar of_beta_muzzleflash;

#endif

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFMinigun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Minigun functions.
//
//=============================================================================

acttable_t CTFGatlingGun::m_acttablbChaingun_Mercenary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_CHAINGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_CHAINGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_CHAINGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_CHAINGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_CHAINGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_CHAINGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_CHAINGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_CHAINGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_CHAINGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_CHAINGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_CHAINGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_CHAINGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_CHAINGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_CHAINGUN, false },

	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_MERC_CHAINGUN_ATTACK_STAND_PREFIRE, false },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_MERC_CHAINGUN_ATTACK_STAND_POSTFIRE, false },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_MERC_CHAINGUN_ATTACK_STAND_STARTFIRE, false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_MERC_CHAINGUN_ATTACK_CROUCH_PREFIRE, false },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_MERC_CHAINGUN_ATTACK_CROUCH_POSTFIRE, false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_MERC_CHAINGUN_ATTACK_SWIM_PREFIRE, false },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_MERC_CHAINGUN_ATTACK_SWIM_POSTFIRE, false },
};

acttable_t *CTFGatlingGun::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttablbChaingun_Mercenary);
		return m_acttablbChaingun_Mercenary;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMinigun::CTFMinigun()
{

#ifdef CLIENT_DLL
	m_pSoundCur = NULL;
#endif


#ifdef CLIENT_DLL

	m_pMuzzleEffect = NULL;
	m_iMuzzleAttachment = -1;
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMinigun::~CTFMinigun()
{
	WeaponReset();
}

void CTFMinigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = AC_STATE_IDLE;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1;
	m_flNextFiringSpeech = 0;

	m_flBarrelAngle = 0;

	m_flBarrelCurrentVelocity = 0;
	m_flBarrelTargetVelocity = 0;

#ifdef CLIENT_DLL
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;

	StopMuzzleEffect();
#endif
}

void CTFMinigun::WeaponSound(WeaponSound_t sound_type, float soundtime)
{
	// Dont let WeaponSound play crit anims here, as it cant stop the loop
	if( sound_type == BURST )
		return;

	BaseClass::WeaponSound(sound_type, soundtime);
}

#ifdef GAME_DLL
int CTFMinigun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::PrimaryAttack()
{
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( !CanAttack() )
	{
		WeaponIdle();
		return;
	}


	if ( pPlayer->m_nButtons & IN_ATTACK )
		//|| ( pPlayer->m_nButtons & IN_ATTACK2 && IsChainGun() ) )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if ( !IsChainGun() )
	{
		if (pPlayer->m_nButtons & IN_ATTACK2)
			m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}
	
	switch ( m_iWeaponState )
	{
	default:
	case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();
			break;
		}
	case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
						m_iWeaponState = AC_STATE_SPINNING;
#ifdef GAME_DLL
						pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				}
				else
				{
					m_iWeaponState = AC_STATE_FIRING;
#ifdef GAME_DLL
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
				}

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			break;
		}
	case AC_STATE_FIRING:
		{
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_iWeaponState = AC_STATE_SPINNING;

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			else if ( ReserveAmmo() <= 0 )
			{
				m_iWeaponState = AC_STATE_DRYFIRE;
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

				// Only fire if we're actually shooting
				BaseClass::PrimaryAttack();		// fire and do timers
				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flTimeWeaponIdle = gpGlobals->curtime + 0.2;
			}
			break;
		}
	case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1;
			if ( ReserveAmmo() > 0 )
			{
				m_iWeaponState = AC_STATE_FIRING;
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				m_iWeaponState = AC_STATE_SPINNING;
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1;
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( ReserveAmmo() > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					m_iWeaponState = AC_STATE_FIRING;
				}
				else
				{
					m_iWeaponState = AC_STATE_DRYFIRE;
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fall through to Primary Attack
//-----------------------------------------------------------------------------
void CTFMinigun::SecondaryAttack( void )
{

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
	{
		return;
	}
	
	if (!IsChainGun())
	{
		SharedAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindUp( void )
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

	float flWindupTime = ( IsChainGun() ? OF_GATLINGGUN_WINDUP_TIME : TF_MINIGUN_WINDUP_TIME ) * flSpeedMultiplier;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	CBaseViewModel *vm = pPlayer->GetViewModel(m_nViewModelIndex);
	if( vm )
	{
		vm->SetPlaybackRate( 1.0f / flSpeedMultiplier );
	}

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_STARTFIRING;

	if ( !IsChainGun() )
	{
		pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	}

#ifdef GAME_DLL
	pPlayer->StopRandomExpressions();
#endif

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif

	pPlayer->TeamFortress_SetSpeed();// Update player's speed
	
	m_flNextPrimaryAttack = gpGlobals->curtime + flWindupTime;
	m_flNextSecondaryAttack = gpGlobals->curtime + flWindupTime;
	m_flTimeWeaponIdle = gpGlobals->curtime + flWindupTime;
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::CanHolster( void ) const
{
	if ( IsChainGun() )
	{
		return BaseClass::CanHolster();
	}
	
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		return false;
	}

	if ( GetActivity() == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		if ( !IsViewModelSequenceFinished() )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
	{
		return;
	}
	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_IDLE;

	if ( !IsChainGun() )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	}

#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_flBarrelTargetVelocity = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPostFrame()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}
	
	if (!IsChainGun())
	{
		if (m_iWeaponState > AC_STATE_IDLE && m_flNextPrimaryAttack < gpGlobals->curtime && !(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2))
		{
			if (pOwner)
			{
				pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_POST);
			}
			WindDown();
		}
	}
	else
	{
		if (m_iWeaponState > AC_STATE_IDLE && m_flNextPrimaryAttack < gpGlobals->curtime && !(pOwner->m_nButtons & IN_ATTACK))
		{
			if (pOwner)
			{
				pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_POST);
			}
			WindDown();
		}
	}
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
	{
		return;
	}

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		}

		WindDown();
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::SendWeaponAnim( int iActivity )
{
#ifdef CLIENT_DLL
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_PRIMARYFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = MAX_BARREL_SPIN_VELOCITY;
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

#endif


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
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFMinigun::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		 m_iWeaponState = AC_STATE_DRYFIRE;

		 SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		 if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		 {
			m_iWeaponState = AC_STATE_SPINNING;
		 }
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFMinigun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );

	// skip resetting this while recording in the tool
	// we change the weapon to the worldmodel and back to the viewmodel when recording
	// which causes the minigun to not spin while recording
	if ( !IsToolRecording() )
	{
		m_flBarrelAngle = 0;

		m_flBarrelCurrentVelocity = 0;
		m_flBarrelTargetVelocity = 0;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if (m_iBarrelBone != -1)
	{
		UpdateBarrelMovement();

		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateBarrelMovement()
{
	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		// update barrel velocity to bring it up to speed or to rest
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, 0.1 );

		if ( 0 == m_flBarrelCurrentVelocity )
		{	
			// if we've stopped rotating, turn off the wind-down sound
			WeaponSoundUpdate();
		}
	}

	// update the barrel rotation based on current velocity
	m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	// Don't do this if using beta muzzleflashes as they handle it by themselves
	if ( !of_beta_muzzleflash.GetBool() )
	{
		HandleMuzzleEffect();
	}

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
		}
	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}
//-----------------------------------------------------------------------------
// Purpose: 
// ditto
//-----------------------------------------------------------------------------
void CTFMinigun::ItemBusyFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartMuzzleEffect()
{
	StopMuzzleEffect();

	if ( !of_beta_muzzleflash.GetBool() )
	{
		C_BaseEntity *pEffectOwner = GetWeaponForEffect();
		if ( !pEffectOwner )
		{
			return;
		}

		// Try and setup the attachment point if it doesn't already exist.
		// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
		// be okay for now.
		if ( m_iMuzzleAttachment == -1 )
		{
			m_iMuzzleAttachment = pEffectOwner->LookupAttachment( "muzzle" );
		}

		// Start the muzzle flash, if a system hasn't already been started.
		if ( m_iMuzzleAttachment != -1 && m_pMuzzleEffect == NULL )
		{
			m_pMuzzleEffect = pEffectOwner->ParticleProp()->Create( "muzzle_minigun_constant", PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopMuzzleEffect()
{
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
	{
		return;
	}

	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		pEffectOwner->ParticleProp()->StopEmission( m_pMuzzleEffect );
		m_pMuzzleEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFMinigun::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	// Prevent jumping while firing
	if ( m_iWeaponState != AC_STATE_IDLE && !IsChainGun() )
	{
		pCmd->buttons &= ~IN_JUMP;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}
//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case AC_STATE_IDLE:
		if ( m_flBarrelCurrentVelocity > 0 )
		{
			iSound = SPECIAL2;	// wind down sound
#ifdef CLIENT_DLL
			if ( m_flBarrelTargetVelocity > 0 )
			{
				m_flBarrelTargetVelocity = 0;
			}
#endif
		}
		else
			iSound = -1;
		break;
	case AC_STATE_STARTFIRING:
		iSound = SPECIAL1;	// wind up sound
		break;
	case AC_STATE_FIRING:
		{
			if ( m_bCritShot ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
	case AC_STATE_SPINNING:
		iSound = SPECIAL3;	// spinning sound
		break;
	case AC_STATE_DRYFIRE:
		iSound = EMPTY;		// out of ammo, still trying to fire
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
	{
		return;
	}

	// play the appropriate sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}


#endif