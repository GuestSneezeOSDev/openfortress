//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//  
//
//=============================================================================
#include "cbase.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase.h"
#include "takedamageinfo.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
	#include "fx_impact.h"
#else
	#include "tf_fx.h"
	#include "ilagcompensationmanager.h"
#endif

ConVar tf_use_fixed_weaponspreads( "tf_use_fixed_weaponspreads", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "If set to 1, weapons that fire multiple pellets per shot will use a non-random pellet distribution." );
// Client specific.
#ifdef CLIENT_DLL

class CGroupedSound
{
public:
	string_t	m_SoundName;
	Vector		m_vecPos;
};

CUtlVector<CGroupedSound> g_aGroupedSounds;

//-----------------------------------------------------------------------------
// Purpose: Called by the ImpactSound function.
//-----------------------------------------------------------------------------
void ImpactSoundGroup( const char *pSoundName, const Vector &vecEndPos )
{
	int iSound = 0;

	// Don't play the sound if it's too close to another impact sound.
	for ( iSound = 0; iSound < g_aGroupedSounds.Count(); ++iSound )
	{
		CGroupedSound *pSound = &g_aGroupedSounds[iSound];
		if ( pSound )
		{
			if ( vecEndPos.DistToSqr( pSound->m_vecPos ) < ( 300.0f * 300.0f ) )
			{
				if ( Q_stricmp( pSound->m_SoundName, pSoundName ) == 0 )
					return;
			}
		}
	}

	// Ok, play the sound and add it to the list.
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, NULL, pSoundName, &vecEndPos );

	iSound = g_aGroupedSounds.AddToTail();
	g_aGroupedSounds[iSound].m_SoundName = pSoundName;
	g_aGroupedSounds[iSound].m_vecPos = vecEndPos;
}

//-----------------------------------------------------------------------------
// Purpose: This is a cheap ripoff from CBaseCombatWeapon::WeaponSound().
//-----------------------------------------------------------------------------
void FX_WeaponSound( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo )
{
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *pShootSound = pWeaponInfo->aShootSounds[soundType]; 
	if ( !pShootSound || !pShootSound[0] )
		return;

	CBroadcastRecipientFilter filter; 
	if ( !te->CanPredict() )
		return;

	CBaseEntity::EmitSound( filter, iPlayer, pShootSound, &vecOrigin ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void StartGroupingSounds()
{
	Assert( g_aGroupedSounds.Count() == 0 );
	SetImpactSoundRoute( ImpactSoundGroup );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void EndGroupingSounds()
{
	g_aGroupedSounds.Purge();
	SetImpactSoundRoute( NULL );
}

// Server specific.
#else

// Server doesn't play sounds.
void FX_WeaponSound ( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo ) {}
void StartGroupingSounds() {}
void EndGroupingSounds() {}

#endif

//-----------------------------------------------------------------------------
// Purpose: This runs on both the client and the server.  On the server, it 
// only does the damage calculations.  On the client, it does all the effects.
//-----------------------------------------------------------------------------
void FX_FireBullets( int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, bool bFixedSpread, int iBullets, float flRange, 
					 int iAmmoType, float flDamage /* = -1.0f */, int bCritical /* = false*/, bool bFirstShot /* = false*/, int iTracerFrequency /* = 2*/)
{
	bool bDoEffects = false;

#ifdef CLIENT_DLL
	C_TFPlayer *pPlayer = ToTFPlayer( ClientEntityList().GetBaseEntity( iPlayer ) );
#else
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayer ) );
#endif
	if ( !pPlayer )
		return;

// Client specific.
#ifdef CLIENT_DLL
	bDoEffects = true;

	if( iWeapon == TF_WEAPON_LIGHTNING_GUN )
		bDoEffects = false;

	// The minigun has custom sound & animation code to deal with its windup/down.
	if ( pPlayer->ShouldDrawThisPlayer() 
		&& ( iWeapon != TF_WEAPON_MINIGUN && iWeapon != TF_WEAPON_GATLINGGUN && iWeapon != TFC_WEAPON_ASSAULTCANNON && iWeapon != TF_WEAPON_LIGHTNING_GUN ) )
	{
		// Fire the animation event.
		if ( !pPlayer->IsDormant() )
		{
			if ( iMode == TF_WEAPON_PRIMARY_MODE )
			{
				pPlayer->m_PlayerAnimState->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			}
			else
			{
				pPlayer->m_PlayerAnimState->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
			}
		}

		//FX_WeaponSound( pPlayer->entindex(), SINGLE, vecOrigin, pWeaponInfo );
	}

// Server specific.
#else
	// If this is server code, send the effect over to client as temp entity and 
	// dispatch one message for all the bullet impacts and sounds.
	TE_FireBullets( pPlayer->entindex(), vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, bFixedSpread, iBullets, flRange, iAmmoType, flDamage, bCritical, bFirstShot, iTracerFrequency );

	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pPlayer->NoteWeaponFired();

#endif

	// Fire bullets, calculate impacts & effects.
	StartGroupingSounds();

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	// Get the shooting angles.
	Vector vecShootForward, vecShootRight, vecShootUp;
	AngleVectors( vecAngles, &vecShootForward, &vecShootRight, &vecShootUp );

	// Initialize the static firing information.
	FireBulletsInfo_t fireInfo;
	fireInfo.m_vecSrc = vecOrigin;

	fireInfo.m_flDamage = static_cast<int>(flDamage);

	fireInfo.m_flDistance = flRange;
	fireInfo.m_iShots = 1;
	fireInfo.m_vecSpread.Init( flSpread, flSpread, 0.0f );
	fireInfo.m_iAmmoType = iAmmoType;

	// Setup the bullet damage type & roll for crit.
	int	nDamageType	= DMG_GENERIC;
	int nCustomDamageType = TF_DMG_CUSTOM_NONE;
	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	if ( pWeapon )
	{
		nDamageType	= pWeapon->GetDamageType();
		if ( pWeapon->IsCurrentAttackACrit() || bCritical )
		{
			nDamageType |= DMG_CRITICAL;
		}

		nCustomDamageType = pWeapon->GetCustomDamageType();
		
		if ( pWeapon->IsCurrentAttackACrit() >= 2 || bCritical >= 2 )
		{
			nCustomDamageType = TF_DMG_CUSTOM_CRIT_POWERUP;
		}		
	}

	fireInfo.m_iTracerFreq = iTracerFrequency;

	// Reset multi-damage structures.
	ClearMultiDamage();

	int nBulletsPerShot = iBullets;

	bool bNoSpread = false;

	if ( nBulletsPerShot > 1 && bFixedSpread )
	{
		bNoSpread = tf_use_fixed_weaponspreads.GetBool();
	}	
	
	for ( int iBullet = 0; iBullet < nBulletsPerShot; ++iBullet )
	{
		// Initialize random system with this seed.
		RandomSeed( iSeed );

		float x = 0.0f;
		float y = 0.0f;		
		
		if ( bNoSpread )
		{
			int iIndex = iBullet;
			while ( iIndex > 20 )
			{
				iIndex -= 20;
			}

			if (pWeapon && pWeapon->GetTFWpnData().m_bFixedSpreadTFC)
			{
				x = 0.5f * g_vecFixedPatternTFC[iIndex].x;
				y = 0.5f * g_vecFixedPatternTFC[iIndex].y;
			}
			else
			{
				x = 0.5f * g_vecFixedPattern[iIndex].x;
				y = 0.5f * g_vecFixedPattern[iIndex].y;
			}
		}
		else
		{
			// Get circular gaussian spread.
			x = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
			y = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );			
		}
		if ( bFirstShot )
		{
			bFirstShot = false;
			fireInfo.m_vecDirShooting = vecShootForward + ( x *  0.0f * vecShootRight ) + ( y * 0.0f * vecShootUp );
		}
		else
		{
			// Initialize the varialbe firing information.
			fireInfo.m_vecDirShooting = vecShootForward + ( x *  flSpread * vecShootRight ) + ( y * flSpread * vecShootUp );
		}
		fireInfo.m_vecDirShooting.NormalizeInPlace();

		// Fire a bullet.
		pPlayer->FireBullet( fireInfo, bDoEffects, nDamageType, nCustomDamageType );

		// Use new seed for next bullet.
		++iSeed; 
	}

	// Apply damage if any.
	ApplyMultiDamage();

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	EndGroupingSounds();
}
