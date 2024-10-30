//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bot.h"

#ifdef INSOURCE_DLL
#include "in_gamerules.h"
#include "in_utils.h"
#include "players_system.h"
#else
#include "ai_senses.h"
#include "gamerules.h"
#include "bots/in_utils.h"
#include "tf_player.h"
#endif

#ifdef OF_DLL
#include "entity_weapon_spawner.h"
#include "entity_condpowerup.h"
#include "entity_healthkit.h"
#include "entity_ammopack.h"
#include "tf_weaponbase_melee.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_bot_locomotion_allow_teleport;
extern ConVar tf_bot_locomotion_hiddden_teleport;
extern ConVar tf_bot_locomotion_allow_wiggle;
extern ConVar tf_bot_debug_jump;
extern ConVar tf_bot_primary_attack;
extern ConVar tf_bot_dont_attack;

ConVar of_bots_ideal_range_melee	("of_bots_ideal_range_melee",	"100.0",	FCVAR_REPLICATED | FCVAR_NOTIFY, "Max of the ideal range for using MELEE RANGE weapons for BOTS.\n");
ConVar of_bots_ideal_range_short	("of_bots_ideal_range_short",	"400.0",	FCVAR_REPLICATED | FCVAR_NOTIFY, "Max of the ideal range for using SHORT RANGE weapons for BOTS.\n");
ConVar of_bots_ideal_range_medium	("of_bots_ideal_range_medium",	"700.0",	FCVAR_REPLICATED | FCVAR_NOTIFY, "Max of the ideal range for using MEDIUM RANGE weapons for BOTS.\n");

//ConVar of_bots_range_over_trinity	("of_bots_range_over_trinity", "0", FCVAR_REPLICATED, "Show overhead player voice icons when players are speaking.\n");

bool WeaponID_Avoid(int iWeaponID)
{
	return iWeaponID == TF_WEAPON_DYNAMITE_BUNDLE;
}

bool WeaponID_GoodLongRange(int iWeaponID)
{
	return
		iWeaponID == TF_WEAPON_PISTOL_MERCENARY
		|| iWeaponID == TF_WEAPON_REVOLVER_MERCENARY
		|| iWeaponID == TF_WEAPON_TOMMYGUN
		|| iWeaponID == TF_WEAPON_ASSAULTRIFLE
		|| iWeaponID == TF_WEAPON_SUPERSHOTGUN
		|| iWeaponID == TF_WEAPON_GIB 
		|| iWeaponID == TF_WEAPON_SUPER_ROCKETLAUNCHER
		|| iWeaponID == TF_WEAPON_RAILGUN;
}

bool WeaponID_GoodMidRange(int iWeaponID)
{
	return
		iWeaponID == TF_WEAPON_PISTOL_MERCENARY
		|| iWeaponID == TF_WEAPON_PISTOL_AKIMBO
		|| iWeaponID == TF_WEAPON_REVOLVER_MERCENARY
		|| iWeaponID == TF_WEAPON_TOMMYGUN
		|| iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DM
		|| iWeaponID == TF_WEAPON_GRENADELAUNCHER_MERCENARY
		|| iWeaponID == TF_WEAPON_LIGHTNING_GUN;
}

bool WeaponID_GoodShortRange(int iWeaponID)
{
	return
		iWeaponID == TF_WEAPON_PISTOL_MERCENARY
		|| iWeaponID == TF_WEAPON_PISTOL_AKIMBO
		|| iWeaponID == TF_WEAPON_NAILGUN
		|| iWeaponID == TF_WEAPON_SHOTGUN
		|| iWeaponID == TF_WEAPON_TOMMYGUN
		|| iWeaponID == TF_WEAPON_SUPERSHOTGUN
		|| iWeaponID == TF_WEAPON_FLAMETHROWER_MERCEANARY
		|| iWeaponID == TF_WEAPON_GRENADELAUNCHER_MERCENARY
		|| iWeaponID == TF_WEAPON_GATLINGGUN;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookDangerSpot() const
{
    if ( IsCombating() )
        return false;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    // Not while we have vision of our enemy
    if ( memory && memory->IsVisibleRecently(2.0f) )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookInterestingSpot() const
{
    return m_IntestingAimTimer.IsElapsed();
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookRandomSpot() const
{
    return m_RandomAimTimer.IsElapsed();
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookSquadMember() const
{
    if ( !GetBot()->GetSquad() )
        return false;

    if ( !GetVision()->IsVisionTimeExpired() )
        return false;

    if ( !GetBot()->GetSquad() )
        return false;

    if ( GetBot()->GetSquad()->GetCount() == 1 )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookThreat() const
{
    if ( HasCondition( BCOND_WITHOUT_ENEMY ) )
        return false;

    // Let's not bother looking at unimportant enemies unless they are close to me.
    if ( !IsImportantEnemy() && !HasCondition( BCOND_ENEMY_NEAR ) ) 
	{
        return false;
    }

    // We have no vision of the enemy
    if ( HasCondition( BCOND_ENEMY_LOST ) ) 
	{
        // But we have a vision of his last position and we are close
        // Let's start looking at other sides
        if ( HasCondition( BCOND_ENEMY_LAST_POSITION_VISIBLE ) && (HasCondition( BCOND_ENEMY_TOO_NEAR ) || HasCondition( BCOND_ENEMY_NEAR )) )
            return false;

        if ( HasCondition( BCOND_ENEMY_TOO_FAR ) )
            return false;
    }    

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldFollow() const
{
    if ( !GetLocomotion() )
        return false;

    if ( !GetFollow()->IsFollowing() )
        return false;

    if ( !GetFollow()->IsEnabled() )
        return false;

    return true;
}


//================================================================================
//================================================================================
bool CBotDecision::ShouldUpdateNavigation() const
{
    if ( !CanMove() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldTeleport( const Vector &vecGoal ) const
{
    // Only if nobody is watching
    if ( tf_bot_locomotion_hiddden_teleport.GetBool() ) {
#ifdef INSOURCE_DLL
        if ( ThePlayersSystem->IsVisible( GetHost() ) )
            return false;

        if ( ThePlayersSystem->IsVisible( vecGoal ) )
            return false;
#else
        for ( int i = 0; i <= gpGlobals->maxClients; ++i ) {
            CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( i ) );

            if ( !pPlayer )
                continue;

            if ( !pPlayer->IsAlive() )
                continue;

            if ( pPlayer->IsBot() )
                continue;

            if ( pPlayer == GetHost() )
                continue;

            if ( pPlayer->FVisible( vecGoal ) && pPlayer->IsInFieldOfView( vecGoal ) )
                return false;
        }
#endif
    }

    Vector vUpBit = GetHost()->GetAbsOrigin();
    vUpBit.z += 1;

    trace_t tr;
    UTIL_TraceHull( vecGoal, vUpBit, GetHost()->WorldAlignMins(), GetHost()->WorldAlignMaxs(),
                    MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &tr );

    if ( tr.startsolid || tr.fraction < 1.0 )
        return false;

    return tf_bot_locomotion_allow_teleport.GetBool();
}

//================================================================================
// It returns if we should move randomly to try to untie us.
//================================================================================
bool CBotDecision::ShouldWiggle() const
{
    if ( !GetLocomotion() )
        return false;

    if ( !GetLocomotion()->IsStuck() )
        return false;

    if ( GetLocomotion()->GetStuckDuration() < 1.0f )
        return false;

	return tf_bot_locomotion_allow_wiggle.GetBool();
}

//================================================================================
// Returns if we should run
// +speed
//================================================================================
bool CBotDecision::ShouldRun() const
{
    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsJumping() )
        return false;

#ifdef INSOURCE_DLL
    if ( !GetHost()->IsOnGround() )
        return false;
#else
    if ( !(GetHost()->GetFlags() & FL_ONGROUND) )
        return false;
#endif

    if ( GetLocomotion()->IsUsingLadder() )
        return false;

    if ( GetLocomotion()->IsRunning() )
        return true;

    if ( GetBot()->GetActiveScheduleID() == SCHEDULE_NONE ) {
        if ( GetLocomotion()->HasDestination() ) 
		{
            const float range = 45.0f;
            float flDistance = GetLocomotion()->GetDistanceToDestination();

            if ( flDistance > range )
                return true;
        }
    }

    return false;
}

//================================================================================
// Returns if we should walk (slow walking)
// +walk
//================================================================================
bool CBotDecision::ShouldSneak() const
{
    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsSneaking() )
        return true;

    return false;
}

//================================================================================
// Returns if we should crouch
// +duck
//================================================================================
bool CBotDecision::ShouldCrouch() const
{
    if ( !GetLocomotion() )
        return false;

    // Always crouch jump
    // TODO: Scenarios where crouch jump should not be done?
    if ( GetLocomotion()->IsJumping() )
        return true;

    if ( GetLocomotion()->IsCrouching() )
        return true;

    return false;
}

//================================================================================
// Returns if we should jump
// +jump
//================================================================================
bool CBotDecision::ShouldJump() const
{
    VPROF_BUDGET( "ShouldJump", VPROF_BUDGETGROUP_BOTS );

    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsJumping() )
        return false;

#ifdef INSOURCE_DLL
    if ( !GetHost()->IsOnGround() )
        return false;
#else
    if ( !(GetHost()->GetFlags() & FL_ONGROUND) )
        return false;
#endif

    if ( GetLocomotion()->IsUsingLadder() )
        return false;

    if ( GetLocomotion()->HasDestination() ) {
        // TODO: All the code below creates traces in each frame to check if there is something blocking us and we need to jump.
        // Obviously I think this form is very bizarre and there are better ways to do it.

        // Height to check if something is blocking us
        Vector vecFeetBlocked = GetLocomotion()->GetFeet();
        vecFeetBlocked.z += StepHeight;

        // Height to see if we can jump
        Vector vecFeetClear = GetLocomotion()->GetFeet();
        vecFeetClear.z += JumpCrouchHeight + 5.0f;

        // We create a new vector with the value of the next position but at the height of the player's feet
		Vector vecNextSpot = GetLocomotion()->HasNextSpot() ? GetLocomotion()->GetNextSpot() : GetLocomotion()->GetDestination();
        vecNextSpot.z = GetLocomotion()->GetFeet().z;

        // Sacamos el angulo hacia el destino
        Vector vec2Angles = vecNextSpot - GetLocomotion()->GetFeet();
        QAngle angles;
        VectorAngles( vec2Angles, angles );

        // Sacamos el vector que indica "enfrente"
        Vector vecForward;
        AngleVectors( angles, &vecForward );

        // Trazamos dos l�neas para verificar que podemos hacer el salto
        trace_t blocked;
        UTIL_TraceLine( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &blocked );

        trace_t clear;
        UTIL_TraceLine( vecFeetClear, vecFeetClear + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &clear );

        if ( tf_bot_debug_jump.GetBool() && GetBot()->ShouldShowDebug() ) {
            NDebugOverlay::Line( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
            NDebugOverlay::Line( vecFeetClear, vecFeetClear + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
        }

        // Algo nos esta bloqueando
        if ( blocked.fraction < 1.0 && clear.fraction == 1.0 ) {
            if ( blocked.m_pEnt ) {
                if ( blocked.m_pEnt->IsPlayer() ) {
#ifdef INSOURCE_DLL
                    CPlayer *pPlayer = ToInPlayer( blocked.m_pEnt );
                    Assert( pPlayer );

                    // Si esta incapacitada pasamos por encima de el (like a boss)
                    if ( !pPlayer->IsDejected() )
                        return false;
#else
                    return false;
#endif
                        
                }
                else if ( blocked.m_pEnt->MyCombatCharacterPointer() ) {
                    return false;
                }
            }

            if ( tf_bot_debug_jump.GetBool() && GetBot()->ShouldShowDebug() ) {
                NDebugOverlay::EntityBounds( GetHost(), 0, 255, 0, 5.0f, 0.1f );
            }

            return true;
        }
    }

    return false;
}


//================================================================================
// Returns if we can hunt our enemy
//================================================================================
bool CBotDecision::CanHuntThreat() const
{
    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_WITHOUT_ENEMY ) )
        return false;

    if ( HasCondition( BCOND_HELPLESS ) )
        return false;

    if ( !GetProfile()->IsEasiest() ) 
	{
        if ( HasCondition( BCOND_LOW_HEALTH ) )
            return false;

        // There are several more dangerous enemies, we should not go
        if ( GetDataMemoryInt( "NearbyDangerousThreats" ) >= 3 )
            return false;
    }

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();
    Assert( memory );

    if ( memory == NULL )
        return false;

    float distance = memory->GetDistance();
    const float tolerance = 700.0f;

    if ( distance >= tolerance ) 
	{
        if ( GetFollow() ) 
		{
            if ( GetFollow()->IsFollowingActive() && IsDangerousEnemy() ) 
			{
                return false;
            }
        }
    }

    return true;
}

//================================================================================
// Returns if we can investigate the source of a danger sound
//================================================================================
bool CBotDecision::ShouldInvestigateSound() const
{
    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_HELPLESS ) )
        return false;

    if ( HasCondition( BCOND_HEAR_MOVE_AWAY ) )
        return false;

    if ( !HasCondition( BCOND_HEAR_COMBAT ) && !HasCondition( BCOND_HEAR_ENEMY ) && !HasCondition( BCOND_HEAR_DANGER ) )
        return false;

    if ( IsCombating() || !HasCondition( BCOND_WITHOUT_ENEMY ) )
        return false;

    if ( GetBot()->GetSquad() && GetBot()->GetSquad()->GetStrategie() == COWARDS )
        return false;

    if ( GetFollow() && GetFollow()->IsFollowingActive() )
        return false;

    CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER );

    if ( !pSound )
        return false;

    // We are in defensive mode, if the sound
    // it's far better we stayed here
    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE ) {
        float distance = GetHost()->GetAbsOrigin().DistTo( pSound->GetSoundOrigin() );
        const float tolerance = 600.0f;

        if ( distance >= tolerance )
            return false;
    }

    return true;
}

//================================================================================
// Returns whether the bot should be hidden for certain actions (reload, heal)
//================================================================================
bool CBotDecision::ShouldCover() const
{
    if ( !CanMove() )
        return false;

    if ( IsDangerousEnemy() )
        return true;

    if ( HasCondition( BCOND_HELPLESS ) )
        return true;

    return false;
}

//================================================================================
// Returns if we can take the specified weapon
//================================================================================
bool CBotDecision::ShouldGrabWeapon( CBaseWeapon *pWeapon ) const
{
    if ( pWeapon == NULL )
        return false;

    if ( pWeapon->GetOwner() )
        return false;

    const float nearDistance = 500.0f;

    if ( GetBot()->GetEnemy() ) {
        // There is an enemy nearby
        if ( Utils::IsSpotOccupied( pWeapon->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber() ) )
            return false;

        // The enemy can attack me easily if I go
        if ( Utils::IsCrossingLineOfFire( pWeapon->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber() ) )
            return false;
    }

    if ( !pWeapon->IsMeleeWeapon() ) {
        // We have little ammunition
        // The weapon may not be better, but at least it has ammunition.
        if ( HasCondition( BCOND_EMPTY_PRIMARY_AMMO ) || HasCondition( BCOND_LOW_PRIMARY_AMMO ) ) {
            if ( pWeapon->HasAnyAmmo() )
                return true;
        }
    }

    if ( TheGameRules->FShouldSwitchWeapon( GetHost(), pWeapon ) )
        return true;

    // We are an player ally, we try not to take the weapons close to a human player
    // TODO: Apply this even if we are on the enemy team with enemy human players. (Teamplay)
    if ( GetHost()->Classify() == CLASS_PLAYER_ALLY ) {
        if ( Utils::IsSpotOccupiedByClass( pWeapon->GetAbsOrigin(), CLASS_PLAYER, NULL, nearDistance ) )
            return false;
    }

    return false;
}
#ifdef OF_DLL
//================================================================================
// Returns if we can take the specified weapon
//================================================================================
bool CBotDecision::ShouldGrabSpawner( CWeaponSpawner *pSpawner ) const
{
	if (pSpawner == NULL)
	{
        return false;
	}

	if (pSpawner->m_bRespawning)
	{
        return false;
	}

	if (pSpawner->m_bDisabled)
	{
		return false;
	}

	CSpotCriteria criteria;
	criteria.SetMaxRange(1000.0f);

	if (!(Utils::IsValidSpot(pSpawner->GetAbsOrigin(), GetHost()->GetAbsOrigin(), criteria, GetHost())))
	{
		return false;
		DevMsg("In theory this should only show up if we can't reach it. [WEAPON SPAWNER]\n");
	}

	CTFWeaponBase *pSameWeapon = (CTFWeaponBase*)(GetHost()->Weapon_OwnsThisType(pSpawner->GetWeaponName()));

	if ( pSameWeapon )
	{
		if ( !(pSameWeapon->IsMeleeWeapon()) && pSameWeapon->GetMaxReserveAmmo() <= pSameWeapon->ReserveAmmo())
		{
			return false;
		}
		else if (!(pSameWeapon->IsMeleeWeapon()))
		{
			if ( Q_strcmp(pSameWeapon->GetName(), pSpawner->GetWeaponName()) == 0)
			{
				//DevMsg("Should, pSameWeapon = %s and pSpawner's Weapon = %s.\n", pSameWeapon->GetName(), pSpawner->GetWeaponName());
			}
			else
			{
				//DevMsg("Shouldn't, pSameWeapon = %s and pSpawner's Weapon = %s.\n", pSameWeapon->GetName(), pSpawner->GetWeaponName());
				return false;
			}
		}
		else
			return false;
	}

    const float nearDistance = 500.0f;

    if ( GetBot()->GetEnemy() ) 
	{
        // There is an enemy nearby
        if ( Utils::IsSpotOccupied( pSpawner->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber() ) )
            return false;

        // The enemy can attack me easily if I go
        if ( Utils::IsCrossingLineOfFire( pSpawner->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber() ) )
            return false;
    }

    return true;
}

//================================================================================
// Returns if we can take the powerup
//================================================================================
bool CBotDecision::ShouldGrabPowerup(CCondPowerup *pPowerup) const
{
	if (pPowerup == NULL)
	{
		return false;
	}

	if (pPowerup->m_bRespawning)
	{
		return false;
	}

	if (pPowerup->m_bDisabled)
	{
		return false;
	}

	CSpotCriteria criteria;
	criteria.SetMaxRange(1000.0f);

	if (!(Utils::IsValidSpot(pPowerup->GetAbsOrigin(), GetHost()->GetAbsOrigin(), criteria, GetHost())))
	{
		return false;
		DevMsg("In theory this should only show up if we can't reach it. [POWEUP SPAWNER]\n");
	}

	if ( GetHost()->m_Shared.InCond(pPowerup->m_iCondition) )
	{
		return false;
	}

	const float nearDistance = 500.0f;

	if (GetBot()->GetEnemy())
	{
		// There is an enemy nearby
		if (Utils::IsSpotOccupied(pPowerup->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber()))
			return false;

		// The enemy can attack me easily if I go
		if (Utils::IsCrossingLineOfFire(pPowerup->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber()))
			return false;
	}



	return true;
}
//================================================================================
// Returns if we can take the HealthKit
//================================================================================
bool CBotDecision::ShouldGrabHealthKit( CHealthKit *pHealthKit ) const
{

	if ( pHealthKit == NULL)
	{
		return false;
	}

	if ( pHealthKit->m_bRespawning )
	{
		return false;
	}

	if ( pHealthKit->m_bDisabled )
	{
		return false;
	}

	if (pHealthKit->GetPowerupSize() == POWERUP_TINY)
	{
		return false;
	}

	CSpotCriteria criteria;
	criteria.SetMaxRange(1000.0f);

	if (!(Utils::IsValidSpot(pHealthKit->GetAbsOrigin(), GetHost()->GetAbsOrigin(), criteria, GetHost())))
	{
		return false;
		DevMsg("In theory this should only show up if we can't reach it. [HEALTHKIT]\n");
	}
	
	int iCurrentHealth = GetHost()->GetHealth();
	int iFullHealth = (GetHost()->GetPlayerClass()->GetMaxHealth());

	if (iCurrentHealth >= iFullHealth)
	{
		//DevMsg("We either have more health than our max or is equal to it.\n");
		return false;
	}

	const float nearDistance = 500.0f;

	if (GetBot()->GetEnemy())
	{
		// There is an enemy nearby
		if (Utils::IsSpotOccupied(pHealthKit->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber()))
			return false;

		// The enemy can attack me easily if I go
		if (Utils::IsCrossingLineOfFire(pHealthKit->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber()))
			return false;
	}

	return true;
}
//================================================================================
// Returns if we can take the AmmoPack
//================================================================================
bool CBotDecision::ShouldGrabAmmoPack(CAmmoPack *pAmmoPack) const
{

	if (pAmmoPack == NULL)
	{
		return false;
	}

	if (pAmmoPack->m_bRespawning)
	{
		return false;
	}

	if (pAmmoPack->m_bDisabled)
	{
		return false;
	}

	CSpotCriteria criteria;
	criteria.SetMaxRange(1000.0f);

	if (!(Utils::IsValidSpot(pAmmoPack->GetAbsOrigin(), GetHost()->GetAbsOrigin(), criteria, GetHost())))
	{
		return false;
		DevMsg("In theory this should only show up if we can't reach it. [AMMOPACK]\n");
	}

	int iMidAmmo = (GetHost()->GetActiveBaseWeapon()->GetMaxReserveAmmo() * 0.50);

	if ( 
		GetHost()->GetActiveBaseWeapon()->ReserveAmmo() >= iMidAmmo
		)
	{
		return false;
	}


	const float nearDistance = 500.0f;

	if (GetBot()->GetEnemy())
	{
		// There is an enemy nearby
		if (Utils::IsSpotOccupied(pAmmoPack->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber()))
			return false;

		// The enemy can attack me easily if I go
		if (Utils::IsCrossingLineOfFire(pAmmoPack->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber()))
			return false;
	}

	return true;
}
#endif
//================================================================================
// Returns if we can change the weapon if necessary
//================================================================================
bool CBotDecision::ShouldSwitchToWeapon( CBaseWeapon *pWeapon ) const
{
    if ( pWeapon == NULL )
        return false;

    return GetHost()->Weapon_CanSwitchTo( pWeapon );
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldHelpFriends() const
{
    if ( GetProfile()->IsEasiest() )
        return false;

    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_HELPLESS ) )
        return false;

    if ( IsCombating() )
        return false;

    if ( GetBot()->GetSquad() ) {
        if ( GetBot()->GetSquad()->GetStrategie() == COWARDS )
            return false;

        if ( GetBot()->GetSquad()->GetActiveCount() <= 2 && GetBot()->GetSquad()->GetStrategie() == LAST_CALL_FOR_BACKUP )
            return false;
    }

    return true;
}

//================================================================================
// Returns if we can help a dejected friend
//================================================================================
bool CBotDecision::ShouldHelpDejectedFriend( CPlayer *pDejected ) const
{
#ifdef INSOURCE_DLL
    if ( !pDejected->IsDejected() )
        return false;

    CPlayer *pHelping = ToInPlayer( GetDataMemoryEntity( "DejectedFriend" ) );

    if ( pHelping ) {
        // We must help him!
        if ( pHelping == pDejected )
            return true;

        // I am trying to help someone else
        if ( pHelping->IsDejected() )
            return false;
    }

    if ( pDejected->IsBeingHelped() )
        return false;

    // TODO: Check other bots that are already trying to help
    return true;
#else
    AssertOnce( !"Implement in your mod" );
    return false;
#endif
}

//================================================================================
// Returns if bot is low health and must be hidden
//================================================================================
bool CBotDecision::IsLowHealth() const
{
	int lowHealth = ( GetHost()->GetPlayerClass()->GetMaxHealth() * 0.5 );

    if ( GetHost()->GetHealth() <= lowHealth ) 
	{
        return true;
    }

    return false;
}
//================================================================================
// Returns if the bot can move
//================================================================================
bool CBotDecision::CanMove() const
{
    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsDisabled() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::IsUsingFiregun() const
{
    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    if ( !pWeapon )
        return false;

    return !pWeapon->IsMeleeWeapon();
}

//================================================================================
//================================================================================
bool CBotDecision::CanAttack() const
{
    // Returns if Bot has the ability to attack
    if ( tf_bot_dont_attack.GetBool() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::CanCrouchAttack() const
{
    if ( GetProfile()->IsEasiest() )
        return false;

    if ( !GetLocomotion() )
        return false;

    if ( GetBot()->GetActiveSchedule() )
        return false;

    if ( !GetBot()->GetPrimaryThreat() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldCrouchAttack() const
{
    CEntityMemory *memory = GetBot()->GetPrimaryThreat();
    Assert( memory );

    if ( memory->GetDistance() <= 150.0f )
        return false;

    Vector vecEyePosition = GetHost()->GetAbsOrigin();
    vecEyePosition.z += VEC_DUCK_VIEW.z;

    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.SetPassEntity( GetHost() );

    Vector vecForward;
    GetHost()->GetVectors( &vecForward, NULL, NULL );

    trace_t tr;
    UTIL_TraceLine( vecEyePosition, vecEyePosition + vecForward * 3000.0f, MASK_SHOT, &traceFilter, &tr );

    if ( tr.m_pEnt == memory->GetEntity() ) {
        //NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, true, 0.5f );
        return true;
    }

    //NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.5f );
    return false;
}

//================================================================================
//================================================================================
bool CBotDecision::IsEnemy( CBaseEntity * pEntity ) const
{
    int relationship = TheGameRules->PlayerRelationship( GetHost(), pEntity );
	// Only check enemy status of players and buildings
    return ( pEntity->IsPlayer() || pEntity->IsBaseObject() ) &&(relationship == GR_ENEMY || relationship == GR_NOTTEAMMATE);
}

//================================================================================
//================================================================================
bool CBotDecision::IsFriend( CBaseEntity * pEntity ) const
{
    int relationship = TheGameRules->PlayerRelationship( GetHost(), pEntity );
    return (relationship == GR_ALLY || relationship == GR_TEAMMATE);
}
//================================================================================
//================================================================================
bool CBotDecision::IsSelf( CBaseEntity * pEntity ) const
{
    return (GetHost() == pEntity);
}
//================================================================================
//================================================================================
bool CBotDecision::IsBetterEnemy( CBaseEntity * pIdeal, CBaseEntity * pPrevious ) const
{
    if ( pIdeal == NULL )
        return false;

    if ( pPrevious == NULL )
        return true;

	CTFPlayer *pCurrent = dynamic_cast<CTFPlayer *>(pIdeal);
	CTFPlayer *pOlder = dynamic_cast<CTFPlayer *>(pPrevious);

    if ( !GetProfile()->IsEasiest() ) 
	{
        if ( GetDecision()->IsAbleToSee( pIdeal ) && !GetDecision()->IsAbleToSee( pPrevious ) )
            return true;
    }

    int previousPriority = GetHost()->IRelationPriority( pPrevious );
    int idealPriority = GetHost()->IRelationPriority( pIdeal );

    float flPreviousDistance = pPrevious->GetAbsOrigin().DistTo( GetHost()->GetAbsOrigin() );
    float flIdealDistance = pIdeal->GetAbsOrigin().DistTo( GetHost()->GetAbsOrigin() );

    // The enemy to compare is further away
    if ( flIdealDistance > flPreviousDistance ) 
	{
        // But has more priority
        if ( idealPriority > previousPriority )
            return true;

        return false;
    }

    // Has more priority
    if ( idealPriority > previousPriority )
        return true;

    // We certainly give priority to enemies very close
    if ( flIdealDistance <= 200.0f )
        return true;

    // Is more dangerous!
    if ( IsDangerousEnemy( pIdeal ) && !IsDangerousEnemy( pPrevious ) )
        return true;

	if ( pCurrent->m_Shared.InCondCrit() && !( pCurrent->m_Shared.InCondCrit() ) )
		return true;

    return false;
}

//================================================================================
// Returns if the specified entity is dangerous to us
//================================================================================
bool CBotDecision::IsDangerousEnemy( CBaseEntity *pEnemy ) const
{
    if ( pEnemy == NULL ) 
	{
        pEnemy = GetBot()->GetEnemy();
    }

    if ( pEnemy == NULL )
        return false;

    if ( pEnemy->IsPlayer() ) 
	{
        return true;
    }

    // Each bot must implement its own criteria.
    return false;
}

//================================================================================
// Returns if the specified entity is important to kill
//================================================================================
bool CBotDecision::IsImportantEnemy( CBaseEntity * pEnemy ) const
{
    if ( pEnemy == NULL )
        pEnemy = GetBot()->GetEnemy();

    if ( pEnemy == NULL )
        return false;

    if ( IsDangerousEnemy( pEnemy ) )
        return true;

    if ( pEnemy->IsPlayer() )
        return true;

    return false;
}

//================================================================================
// Returns if we have lost sight of our current enemy
//================================================================================
bool CBotDecision::IsPrimaryThreatLost() const
{
    return (HasCondition( BCOND_ENEMY_LOST ));
}

//================================================================================
// It returns if we should perform our tasks more carefully.
//================================================================================
bool CBotDecision::ShouldMustBeCareful() const
{
    if ( GetProfile()->IsEasiest() )
        return false;

    if ( GetDataMemoryInt("NearbyDangerousThreats") >= 2 )
        return true;

    if ( IsDangerousEnemy() )
        return true;

    return false;
}
//================================================================================
// Scan all our weapons
//================================================================================
void CBotDecision::ScanAllWeaponsRange()
{
	CBaseWeapon *pCurrent = GetHost()->GetActiveBaseWeapon();

	if (pCurrent == NULL)
		return;

	for (int iWeapon = 0; iWeapon < GetHost()->WeaponCount(); ++iWeapon)
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(GetHost()->GetWeapon(iWeapon));

		if( pWeapon )
		{
			if ( !( WeaponID_Avoid( pWeapon->GetWeaponID() ) ) )
			{
				if ( pWeapon->IsMeleeWeapon() )
				{
					CTFWeaponBase *pMeleeWeapon = static_cast<CTFWeaponBase *>(m_pMeleeWeapon.Get());
					if (
						!( pMeleeWeapon )
						||
						( pMeleeWeapon && ( pWeapon->GetWeight() > pMeleeWeapon->GetWeight() ) )
						)
					{
						m_pMeleeWeapon = pWeapon;
						//DevMsg("Our Melee is now: %s\n", pMeleeWeapon->GetPrintName());
					}
				}

				if ( WeaponID_GoodLongRange (pWeapon->GetWeaponID()))
				{
					CTFWeaponBase *pLongRangeWeapon = static_cast<CTFWeaponBase *>(m_pLongRangeWeapon.Get());
					if (
						!( pLongRangeWeapon )
						||
						( pLongRangeWeapon && ( pWeapon->GetWeight() >  pLongRangeWeapon->GetWeight() ) )
						)
					{
						m_pLongRangeWeapon = pWeapon;
						//DevMsg("Our Long Range Weapon is now: %s\n", pLongRangeWeapon->GetPrintName());
					}
				}

				if ( WeaponID_GoodMidRange (pWeapon->GetWeaponID()))
				{
					CTFWeaponBase *pMidRangeWeapon = static_cast<CTFWeaponBase *>(m_pMidRangeWeapon.Get());
					if (
						!( pMidRangeWeapon )
						||
						( pMidRangeWeapon && ( pWeapon->GetWeight() >  pMidRangeWeapon->GetWeight() ) )
						)
					{
						m_pMidRangeWeapon = pWeapon;
						//DevMsg("Our Medium Range Weapon is now: %s\n", pMidRangeWeapon->GetPrintName());
					}
				}

				if ( WeaponID_GoodShortRange (pWeapon->GetWeaponID()))
				{
					CTFWeaponBase *pShortRangeWeapon = static_cast<CTFWeaponBase *>(m_pShortRangeWeapon.Get());
					if (
						!( pShortRangeWeapon )
						||
						( pShortRangeWeapon && ( pWeapon->GetWeight() >  pShortRangeWeapon->GetWeight() ) )
						)
					{
						m_pShortRangeWeapon = pWeapon;
						//DevMsg("Our Short Range Weapon is now: %s\n", pShortRangeWeapon->GetPrintName());
					}
				}
			}
		}
	}
}
//================================================================================
// We change our current weapon by the best we have according to the situation.
//================================================================================
void CBotDecision::SwitchToBestWeapon()
{
    CBaseWeapon *pCurrent = GetHost()->GetActiveBaseWeapon();

    if ( pCurrent == NULL )
        return;

	float meleeRange = of_bots_ideal_range_melee.GetFloat();
	float shortRange = of_bots_ideal_range_short.GetFloat();
	float mediumRange = of_bots_ideal_range_medium.GetFloat();

	CEntityMemory *memory = GetBot()->GetPrimaryThreat();

	if ( GetHost()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		ScanAllWeaponsRange();

		if (IsCombating())
		{
			if (memory)
			{
				if (memory->GetDistance() < meleeRange)
				{
					CTFWeaponBase *pMeleeWeapon = static_cast<CTFWeaponBase *>(m_pMeleeWeapon.Get());
					if (ShouldUseThisWeaponForThis(pMeleeWeapon, false) && pCurrent != pMeleeWeapon)
					{
						//DevMsg("they're too close! Get out the melee!\n");
						GetHost()->Weapon_Switch(pMeleeWeapon);
					}
				}
				else if (memory->GetDistance() >= meleeRange && memory->GetDistance() < shortRange)
				{
                    CTFWeaponBase* pShortRangeWeapon = static_cast<CTFWeaponBase*>(m_pShortRangeWeapon.Get());
					if (ShouldUseThisWeaponForThis(pShortRangeWeapon, false) && pCurrent != pShortRangeWeapon)
					{
						//DevMsg("We should switch to our short range weapon.\n");
						GetHost()->Weapon_Switch(pShortRangeWeapon);
					}
				}
				else if (memory->GetDistance() >= shortRange && memory->GetDistance() < mediumRange)
				{
					CTFWeaponBase *pMidRangeWeapon = static_cast<CTFWeaponBase *>(m_pMidRangeWeapon.Get());
					if (ShouldUseThisWeaponForThis(pMidRangeWeapon, false) && pCurrent != pMidRangeWeapon)
					{
                        CTFWeaponBase* pMidRangeWeapon = static_cast<CTFWeaponBase*>(m_pMidRangeWeapon.Get());
						//DevMsg("We should switch to our medium range weapon.\n");
						GetHost()->Weapon_Switch(pMidRangeWeapon);
					}
				}
				else if (memory->GetDistance() >= mediumRange)
				{
                    CTFWeaponBase* pLongRangeWeapon = static_cast<CTFWeaponBase*>(m_pLongRangeWeapon.Get());
					if (ShouldUseThisWeaponForThis(pLongRangeWeapon, false) && pCurrent != pLongRangeWeapon)
					{
						//DevMsg("We should switch to our long range weapon.\n");
						GetHost()->Weapon_Switch(pLongRangeWeapon);
					}
				}
			}
		}
		else
		{
			if (IsIdle()) 
			{
				CBaseWeapon *pBest = dynamic_cast<CBaseWeapon *>(TheGameRules->GetNextBestWeapon(GetHost(), NULL));

				if (pBest != pCurrent)
				{
					GetHost()->Weapon_Switch(pBest);
				}
			}
		}
	}
}
//================================================================================
// See if we should actually use this weapon.
//================================================================================
bool CBotDecision::ShouldUseThisWeaponForThis( CBaseWeapon *pGivenWeapon, bool bIsReloadCheck ) const
{
	CBaseWeapon *pCurrent = GetHost()->GetActiveBaseWeapon();

	if (pCurrent == NULL)
		return false;

	if (pGivenWeapon)
	{
		if ( !(bIsReloadCheck) )
		{
			if ( pCurrent != pGivenWeapon &&
				pGivenWeapon->HasAnyAmmo() )
			{
				//DevMsg( "We are switching to %s.\n", pGivenWeapon->GetPrintName() );
				return true;
			}
			else
				return false;
		}
		else
		{
			if ( (pCurrent != pGivenWeapon) &&
				( pGivenWeapon->Clip1() >= 0 && pGivenWeapon->Clip1() != pGivenWeapon->GetMaxClip1() ) && pGivenWeapon->HasPrimaryAmmo() )
			{
				//DevMsg("We are switching to %s to reload it.\n", pGivenWeapon->GetPrintName());
				return true;
			}
			else 
				return false;
		}
	}
	else
		return false;
}
//================================================================================
//================================================================================
float CBotDecision::GetWeaponIdealRange(CBaseWeapon *pWeapon) const
{
	if (pWeapon == NULL)
		pWeapon = GetHost()->GetActiveBaseWeapon();

	if (pWeapon == NULL)
		return -1.0f;

	if (pWeapon->IsMeleeWeapon())
		return pWeapon->GetTFWpnData().GetWeaponData(TF_WEAPON_PRIMARY_MODE).m_flMeleeRange;

	/*
	float shortRange =	( of_bots_ideal_range_short.GetFloat()	* 1.25 );
	float mediumRange = ( of_bots_ideal_range_medium.GetFloat() * 1.25 );

	if ( WeaponID_GoodLongRange( pWeapon->GetWeaponID() ) )
		return longRange;

	if ( WeaponID_GoodMidRange( pWeapon->GetWeaponID() ) )
		return mediumRange;

	if ( WeaponID_GoodShortRange( pWeapon->GetWeaponID() ) )
		return shortRange;
	*/

	return 9000.0f;

}

//================================================================================
// Returns if there is a cover spot close to the bot position
//================================================================================
bool CBotDecision::GetNearestCover( float radius, Vector *vecCoverSpot ) const
{
    CSpotCriteria criteria;
    criteria.SetMaxRange( radius );
    criteria.UseNearest( true );
    criteria.OutOfVisibility( true );
    criteria.AvoidTeam( GetBot()->GetEnemy() );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );

    if ( GetHost()->GetActiveBaseWeapon() && (GetHost()->GetActiveBaseWeapon()->GetWeaponID() == TF_WEAPON_RAILGUN )) 
	{
        criteria.SniperSpots( true );
    }

    return Utils::FindCoverPosition( vecCoverSpot, GetHost(), criteria );
}

//================================================================================
// Returns if the bot is in a coverage position
//================================================================================
bool CBotDecision::IsInCoverPosition() const
{
    Vector vecCoverSpot;
    const float tolerance = 75.0f;

    if ( !GetNearestCover( GET_COVER_RADIUS, &vecCoverSpot ) )
        return false;

    if ( GetHost()->GetAbsOrigin().DistTo( vecCoverSpot ) > tolerance )
        return false;

    return true;
}
//================================================================================
//================================================================================
BCOND CBotDecision::ShouldRangeAttack1()
{
    if ( !CanAttack() )
        return BCOND_NONE;

    if ( tf_bot_primary_attack.GetBool() )
        return BCOND_CAN_RANGE_ATTACK1;

    if ( !CanShoot() )
        return BCOND_NONE;

    // TODO: A way to support attacks without an active enemy
    if ( HasCondition( BCOND_WITHOUT_ENEMY ) )
        return BCOND_NONE;

    // Without vision of the enemy
    if ( HasCondition( BCOND_ENEMY_LOST ) || HasCondition( BCOND_ENEMY_OCCLUDED ) )
        return BCOND_NONE;

    if ( GetVision() ) 
	{
        if ( GetVision()->GetAimTarget() != GetBot()->GetEnemy() )
            return BCOND_NOT_FACING_ATTACK;

        if ( !GetVision()->IsAimReady() ) 
		{
            // We still do not have our aim on the enemy, but humans usually shoot regardless.
            if ( RandomInt(0, 60) <= 50 )
                return BCOND_NOT_FACING_ATTACK;
        }
    }

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

	float flDistance = GetMemory()->GetPrimaryThreatDistance();

    // TODO: The code commented below was an attempt to make the Bot continue firing 
    // for a few seconds at the enemy's last known position, but a more intelligent way is needed.

    /*if ( IsPrimaryThreatLost() ) {
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
    return BCOND_NONE;

    if ( GetSkill()->GetSkill() <= SKILL_MEDIUM )
    return BCOND_NONE;

    if ( GetBot()->GetActiveSchedule() != NULL )
    return BCOND_NONE;

    if ( GetVision() ) {
    if ( GetVision()->GetAimTarget() != memory->GetEntity() || !GetVision()->IsAimReady() )
    return BCOND_NOT_FACING_ATTACK;
    }

    if ( HasCondition( BCOND_ENEMY_LOST ) )
    return BCOND_NONE;
    }*/

    if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) )
        return BCOND_NONE;

    if ( !(pWeapon->CanAttack()) )
        return BCOND_NONE;

    if ( flDistance > GetWeaponIdealRange() )
        return BCOND_TOO_FAR_TO_ATTACK;    

    // A better way to do this and move it to a better place.
    float fireRate = pWeapon->GetFireRate();
	float delay = fireRate + GetProfile()->GetAttackDelay();
    m_ShotRateTimer.Start( delay );

    return BCOND_CAN_RANGE_ATTACK1;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldRangeAttack2()
{
    if ( tf_bot_dont_attack.GetBool() )
        return BCOND_NONE;

    // TODO: A way to support attacks without an active enemy
    if ( HasCondition(BCOND_WITHOUT_ENEMY) )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

    // Snipa!
    if ( pWeapon->GetWeaponID() == TF_WEAPON_RAILGUN ) 
	{
        if ( !GetProfile()->IsVeryHard() || !GetProfile()->IsHard() ) 
		{
            // Zoom!
            if ( IsCombating() && !(GetHost()->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY)) ) 
			{
                return BCOND_CAN_RANGE_ATTACK2;
            }

            // We removed the zoom
            else if ( IsIdle() && GetHost()->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY) )
			{
                return BCOND_CAN_RANGE_ATTACK2;
            }
        }
    }

    // TODO: Each weapon can have its own behavior in the secondary attack
    return BCOND_NONE;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldMeleeAttack1()
{
    if ( tf_bot_dont_attack.GetBool() )
        return BCOND_NONE;

    if ( tf_bot_primary_attack.GetBool() )
        return BCOND_CAN_MELEE_ATTACK1;

    // TODO: A way to support attacks without an active enemy
    if ( HasCondition(BCOND_WITHOUT_ENEMY) )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || !pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

    float flDistance = GetMemory()->GetPrimaryThreatDistance();

    // TODO: Criteria to attack with a melee weapon
    if ( flDistance > pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flMeleeRange * 2 )
        return BCOND_TOO_FAR_TO_ATTACK;

    // TODO
    return BCOND_CAN_MELEE_ATTACK1;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldMeleeAttack2()
{
    // TODO
    return BCOND_NONE;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldStrafe()
{
    if( HasCondition( BCOND_WITHOUT_ENEMY ) )
        return BCOND_NONE;

    if( HasCondition( BCOND_ENEMY_LOST ) )
		return BCOND_NONE;

    if( HasCondition( BCOND_ENEMY_OCCLUDED ) )
		return BCOND_NONE;

    if( HasCondition( BCOND_ENEMY_DEAD ) )
		return BCOND_NONE;

	if( GetHost()->m_Shared.InCondUber() )
		return BCOND_NONE;

	if( GetHost()->m_Shared.InCond(TF_COND_ZOOMED) )
		return BCOND_NONE;

	if( GetHost()->m_Shared.InCond(TF_COND_DISGUISED) )
		return BCOND_NONE;

	if( GetHost()->m_Shared.InCond(TF_COND_DISGUISING) )
		return BCOND_NONE;

	if( GetHost()->m_Shared.InCondInvis() )
		return BCOND_NONE;
	
	if( GetHost()->IsTaunting() )
		return BCOND_NONE;

	if( GetProfile()->GetSkill() == SKILL_EASY )
		return BCOND_NONE;

	return BCOND_SHOULD_STRAFE;
}

//================================================================================
//================================================================================
bool CBotDecision::IsAbleToSee( CBaseEntity * entity, FieldOfViewCheckType checkFOV ) const
{
    if ( entity->MyCombatCharacterPointer() ) {
        return GetHost()->IsAbleToSee( entity->MyCombatCharacterPointer(), (CBaseCombatCharacter::FieldOfViewCheckType)checkFOV );
    }

    return GetHost()->IsAbleToSee( entity, (CBaseCombatCharacter::FieldOfViewCheckType)checkFOV );
}

//================================================================================
//================================================================================
bool CBotDecision::IsAbleToSee( const Vector & pos, FieldOfViewCheckType checkFOV ) const
{
#ifdef INSOURCE_DLL
    return GetHost()->IsAbleToSee( pos, (CBaseCombatCharacter::FieldOfViewCheckType)checkFOV );
#else
    return (GetHost()->FVisible( pos ) && GetHost()->IsInFieldOfView( pos ));
#endif
}

//================================================================================
//================================================================================
bool CBotDecision::IsInFieldOfView( CBaseEntity * entity ) const
{
    return GetHost()->IsInFieldOfView( entity );
}

//================================================================================
//================================================================================
bool CBotDecision::IsInFieldOfView( const Vector & pos ) const
{
    return GetHost()->IsInFieldOfView( pos );
}

//================================================================================
//================================================================================
bool CBotDecision::IsLineOfSightClear( CBaseEntity *entity, CBaseEntity **hit ) const
{
    if ( GetMemory() ) {
        CEntityMemory *memory = GetMemory()->GetEntityMemory( entity );

        if ( memory ) {
            return IsLineOfSightClear( memory->GetIdealPosition(), entity, hit );
        }
    }

    return IsLineOfSightClear( entity->WorldSpaceCenter(), entity, hit );
}

//================================================================================
//================================================================================
bool CBotDecision::IsLineOfSightClear( const Vector & pos, CBaseEntity * entityToIgnore, CBaseEntity **hit ) const
{
    if ( !IsAbleToSee( pos ) )
        return false;

    // We draw a line pretending to be the bullets
    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.AddEntityToIgnore( GetHost() );
    traceFilter.AddEntityToIgnore( entityToIgnore );

    trace_t tr;
    UTIL_TraceLine( GetHost()->EyePosition(), pos, MASK_SHOT, &traceFilter, &tr );

    *hit = tr.m_pEnt;
    return (tr.fraction == 1.0f);
}
