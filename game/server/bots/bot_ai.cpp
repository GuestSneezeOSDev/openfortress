//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_gamerules.h"
#include "in_utils.h"
#else
#include "bots/in_utils.h"
#endif

#include "tf_gamerules.h"
#include "bots/of/schedules/of_bot_schedules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_bot_primary_attack;
extern ConVar of_bots_range_short;
extern ConVar of_bots_range_medium;
extern ConVar of_bots_range_long;

//================================================================================
// Sets a condition
//================================================================================
void CBot::SetCondition( BCOND condition )
{
    m_nConditions.Set( condition );
}

//================================================================================
// Forgets a condition
//================================================================================
void CBot::ClearCondition( BCOND condition )
{
    m_nConditions.Clear( condition );
}

//================================================================================
// Returns if the bot is in a condition
//================================================================================
bool CBot::HasCondition( BCOND condition ) const
{
    if ( IsConditionsAllowed() ) {
        Assert( !"Attempt to verify a condition before gathering!" );
        return false;
    }

    return m_nConditions.IsBitSet( condition );
}

//================================================================================
// Add a component to the list
//================================================================================
void CBot::AddComponent( IBotComponent *pComponent )
{
    if ( !pComponent )
        return;

    pComponent->Reset();

    if ( pComponent->IsSchedule() ) {
        IBotSchedule *pSchedule = dynamic_cast<IBotSchedule *>(pComponent);
        Assert( pSchedule );

        if ( pSchedule ) {
            m_nSchedules.InsertOrReplace( pSchedule->GetID(), pSchedule );
        }
    }
    else {
        m_nComponents.InsertOrReplace( pComponent->GetID(), pComponent );
    }
}

//================================================================================
// Create the components that the bot will have
//================================================================================
void CBot::SetUpComponents()
{
    // Basic components
    // Each custom A.I. decide which ones to remove and add.
    ADD_COMPONENT( CBotVision );
    ADD_COMPONENT( CBotFollow );
    ADD_COMPONENT( CBotLocomotion );
    ADD_COMPONENT( CBotMemory );
    ADD_COMPONENT( CBotAttack );
    ADD_COMPONENT( CBotDecision ); // This component is mandatory!
}

//================================================================================
// Create the schedules that the bot will have
//================================================================================
void CBot::SetUpSchedules()
{
    // Basic schedules
    // Each custom A.I. decide which ones to remove and add.

   // ADD_COMPONENT( CHuntEnemySchedule ); We need to redo this so they don't explode their minds

    ADD_COMPONENT( CReloadSchedule );
    ADD_COMPONENT( CCoverSchedule );
    ADD_COMPONENT( CHideSchedule );

	ADD_COMPONENT( CPickupHealthSpawnerSchedule );
	ADD_COMPONENT( CPickupPowerupSpawnerSchedule );
	ADD_COMPONENT( CPickupWeaponSpawnerSchedule );
	ADD_COMPONENT( CPickupAmmoSpawnerSchedule );

	//ADD_COMPONENT(CGotoFlagSchedule);
	//ADD_COMPONENT(CCaptureFlagSchedule);
	//ADD_COMPONENT(CDefendFlagSchedule);

	if (TFGameRules())
	{
		if (TFGameRules()->InGametype(TF_GAMETYPE_CTF))
		{
			
			int iCoinFlip = RandomInt(1, 10);

			if (iCoinFlip == 1)
			{
				//DevMsg("Coin flip INT landed on... %i.\nFreedom Mode\n", iCoinFlip);
				//FREEROAM ONLY
				ADD_COMPONENT(CFreeRoamSchedule);
			}
			else if (iCoinFlip > 1 && iCoinFlip <= 4)
			{
				//DevMsg("Coin flip INT landed on... %i.\nCapture and Defence Mode.\n", iCoinFlip);
				ADD_COMPONENT(CGotoFlagSchedule);
				ADD_COMPONENT(CCaptureFlagSchedule);
				ADD_COMPONENT(CDefendFlagSchedule);
			}
			else if (iCoinFlip > 4 && iCoinFlip <= 7)
			{
				//DevMsg("Coin flip INT landed on... %i.\nDefence Mode\n", iCoinFlip);
				ADD_COMPONENT(CDefendFlagSchedule);
				ADD_COMPONENT(CFreeRoamSchedule);
			}
			else if (iCoinFlip > 7 && iCoinFlip <= 10)
			{
				//DevMsg("Coin flip INT landed on... %i.\nCapture Mode\n", iCoinFlip);
				ADD_COMPONENT(CGotoFlagSchedule);
				ADD_COMPONENT(CCaptureFlagSchedule);
			}
            else
            {
                DevMsg("Coin flip INT landed on... %i.\nSomething has gone wrong!\n", iCoinFlip);
            }
		}
		else
		{
			ADD_COMPONENT(CFreeRoamSchedule);
		}
	}

    //ADD_COMPONENT( CChangeWeaponSchedule ); Replaced by OF specific
    //ADD_COMPONENT( CHideAndHealSchedule );
    //ADD_COMPONENT( CHideAndReloadSchedule );
    //ADD_COMPONENT( CMoveAsideSchedule ); Replaced by strafing in the combat component
    //ADD_COMPONENT( CCallBackupSchedule );
    //ADD_COMPONENT( CDefendSpawnSchedule );
    //ADD_COMPONENT( CInvestigateLocationSchedule ); // TODO: Finish

}
//================================================================================
// Simplized version of the AI for Lethal Lockdown
//================================================================================
void COFBot::SetUpSchedules()
{
	// ADD_COMPONENT( CHuntPlayerSchedule ); Hunt down the player.
	// ADD_COMPONENT( CHuntEveryoneSchedule ); Hunt down ALL players.
	// ADD_COMPONENT( CHuntObjectiveSchedule ); Keep moving toward what they are told to go to.

	ADD_COMPONENT(CReloadSchedule);
	ADD_COMPONENT(CCoverSchedule);
	ADD_COMPONENT(CHideSchedule);

	//ADD_COMPONENT(CWanderSchedule); Just make the bot wander around the map.
}

//================================================================================
// Returns the [IBotSchedule] registered for this ID
//================================================================================
IBotSchedule *CBot::GetSchedule( int schedule )
{
    int index = m_nSchedules.Find( schedule );

    if ( m_nSchedules.IsValidIndex( index ) ) {
        return m_nSchedules.Element( index );
    }

    return NULL;
}

//================================================================================
// Returns the active schedule ID.
//================================================================================
int CBot::GetActiveScheduleID()
{
    if ( !GetActiveSchedule() ) {
        return SCHEDULE_NONE;
    }

    return GetActiveSchedule()->GetID();
}

//================================================================================
// Returns the ideal schedule for the bot (With more desire)
//================================================================================
int CBot::SelectIdealSchedule()
{
    if ( !GetHost()->IsAlive() )
        return SCHEDULE_NONE;

    float desire = BOT_DESIRE_NONE;
    IBotSchedule *pIdeal = GetActiveSchedule();

    // We have an active schedule
    // We take her initial desire level
    if ( pIdeal ) {
        desire = pIdeal->GetInternalDesire();
    }

    FOR_EACH_MAP( m_nSchedules, it )
    {
        IBotSchedule *pSchedule = m_nSchedules[it];

        // This schedule has a greater desire!
        if ( pSchedule->GetInternalDesire() > desire ) {
            pIdeal = pSchedule;
            desire = pSchedule->GetDesire();
        }
    }

    if ( pIdeal && desire > BOT_DESIRE_NONE ) {
        return pIdeal->GetID();
    }

    return SCHEDULE_NONE;
}

//================================================================================
// Updates the current schedule
//================================================================================
void CBot::UpdateSchedule()
{
    VPROF_BUDGET( "UpdateSchedule", VPROF_BUDGETGROUP_BOTS );

    // Maybe an custom A.I. want to change a schedule.
    int idealSchedule = TranslateSchedule( SelectIdealSchedule() );

    if ( idealSchedule == SCHEDULE_NONE ) {
        if ( GetActiveSchedule() ) {
            GetActiveSchedule()->Finish();
            m_nActiveSchedule = NULL;
        }
        
        return;
    }

    IBotSchedule *pSchedule = GetSchedule( idealSchedule );
    AssertMsg( pSchedule, "GetSchedule == NULL" );

    if ( pSchedule == NULL )
        return;

    if ( GetActiveSchedule() ) {
        if ( GetActiveSchedule() == pSchedule ) {
            m_ScheduleTimer.Start();
            GetActiveSchedule()->Update();
            m_ScheduleTimer.End();
            return;
        }
        else {
            GetActiveSchedule()->Finish();
        }
    }

    m_ScheduleTimer.Start();
    m_nActiveSchedule = pSchedule;
    m_nActiveSchedule->Start();
    m_nActiveSchedule->Update();
    m_ScheduleTimer.End();
}

//================================================================================
// Mark a task as complete
// TODO: Real life scenario where this is used?
//================================================================================
void CBot::TaskComplete() 
{
	if ( !GetActiveSchedule() )
		return;

	GetActiveSchedule()->TaskComplete();
}

//================================================================================
// Mark as failed a schedule
// TODO: Real life scenario where this is used?
//================================================================================
void CBot::TaskFail( const char *pWhy ) 
{
	if ( !GetActiveSchedule() )
		return;

	GetActiveSchedule()->Fail( pWhy );
}


//================================================================================
// Gets new conditions from environment and statistics
// Conditions that do not require information about components (vision/smell/hearing)
//================================================================================
void CBot::GatherConditions()
{
    VPROF_BUDGET( "GatherConditions", VPROF_BUDGETGROUP_BOTS );

    GatherHealthConditions();
    GatherWeaponConditions();
    GatherEnemyConditions();
    GatherAttackConditions();
    GatherLocomotionConditions();
}

//================================================================================
// Obtains new conditions related to health and damage
//================================================================================
void CBot::GatherHealthConditions()
{
    VPROF_BUDGET( "GatherHealthConditions", VPROF_BUDGETGROUP_BOTS );

	int iFullHealth = (GetHost()->GetPlayerClass()->GetMaxHealth());

	int iHighHealth = (iFullHealth * 0.9);

	int iMidHealth = (iFullHealth * 0.7);

	int iLowHealth = (iFullHealth * 0.4);

	int iCurrentHealth = GetHost()->GetHealth();
	
	if (iCurrentHealth > iFullHealth)
	{
		SetCondition(BCOND_OVERHEAL_HEALTH);

	}
	else if (iCurrentHealth == iFullHealth)
	{
		SetCondition(BCOND_FULL_HEALTH);
	}
	else if (iCurrentHealth < iFullHealth && iCurrentHealth >= iHighHealth)
	{
		SetCondition(BCOND_HIGH_HEALTH);
	}
	else if (iCurrentHealth < iHighHealth && iCurrentHealth >= iMidHealth)
	{
		SetCondition(BCOND_MID_HEALTH);
	}
	else if (iCurrentHealth < iMidHealth && iCurrentHealth >= iLowHealth)
	{
		SetCondition(BCOND_LOW_HEALTH);
	}
	else if (iCurrentHealth < iLowHealth )
	{
		SetCondition(BCOND_CRITICAL_HEALTH);
	}

    if ( m_iRepeatedDamageTimes == 0 )
        return;

    if ( m_iRepeatedDamageTimes <= 2 ) 
	{
        SetCondition( BCOND_LIGHT_DAMAGE );
    }
    else {
        SetCondition( BCOND_REPEATED_DAMAGE );
    }

    if ( m_flDamageAccumulated >= 30.0f ) 
	{
        SetCondition( BCOND_HEAVY_DAMAGE );
    }
}

//================================================================================
// Gets new conditions related to current weapon
//================================================================================
void CBot::GatherWeaponConditions()
{
    VPROF_BUDGET( "GatherWeaponConditions", VPROF_BUDGETGROUP_BOTS );

    // We change to the best weapon for this situation
    // TODO: A better place to put this.
    GetDecision()->SwitchToBestWeapon();

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    if ( pWeapon == NULL ) 
	{
        SetCondition( BCOND_HELPLESS );
        return;
    }

	int ammo = 0;
	int totalAmmo = pWeapon->IsMeleeWeapon() ? 1 : pWeapon->ReserveAmmo();
	int totalRange = 1;

	if ( pWeapon->UsesClipsForAmmo1() ) 
	{
		ammo = pWeapon->Clip1();
		int maxAmmo = pWeapon->GetMaxClip1();
		totalRange = (maxAmmo * 0.5);

		if ( ammo == 0 )
		{
			SetCondition( BCOND_EMPTY_CLIP1_AMMO );
		}
		else if ( ammo < totalRange )
		{
			SetCondition( BCOND_LOW_CLIP1_AMMO );
        }

		if (totalAmmo == 0)
		{
			SetCondition(BCOND_EMPTY_CURRENT_AMMO);
		}
		else if (totalAmmo < totalRange)
		{
			SetCondition(BCOND_LOW_CURRENT_AMMO);
		}
    }
    // Secondary ammunition
#ifndef OF_DLL
    {
        int ammo = 0;
        int totalAmmo = GetHost()->GetAmmoCount( pWeapon->GetSecondaryAmmoType() );
        int totalRange = 15;

        if ( pWeapon->UsesClipsForAmmo2() ) {
            ammo = pWeapon->Clip2();
            int maxAmmo = pWeapon->GetMaxClip2();
            totalRange = (maxAmmo * 0.5);

            if ( ammo == 0 )
                SetCondition( BCOND_EMPTY_CLIP2_AMMO );
            else if ( ammo < totalRange )
                SetCondition( BCOND_LOW_CLIP2_AMMO );
        }

        if ( totalAmmo == 0 )
            SetCondition( BCOND_EMPTY_SECONDARY_AMMO );
        else if ( totalAmmo < totalRange )
            SetCondition( BCOND_LOW_SECONDARY_AMMO );
    }
#endif
    // You have no ammunition of any kind, you are defenseless
    if ( HasCondition( BCOND_EMPTY_PRIMARY_AMMO ) &&
         HasCondition( BCOND_EMPTY_CLIP1_AMMO ) &&
         HasCondition( BCOND_EMPTY_SECONDARY_AMMO ) &&
         HasCondition( BCOND_EMPTY_CLIP2_AMMO ) )
        SetCondition( BCOND_HELPLESS );
}

//================================================================================
// Gets new conditions related to the current enemy
//================================================================================
void CBot::GatherEnemyConditions()
{
    VPROF_BUDGET( "GatherEnemyConditions", VPROF_BUDGETGROUP_BOTS );

    CEntityMemory *memory = GetPrimaryThreat();

    if ( memory == NULL ) 
	{
        SetCondition( BCOND_WITHOUT_ENEMY );
        return;
    }

    if ( !IsCombating() ) 
	{
        Alert();
    }

    // List of distances to establish useful conditions.
	float flIdealDistance = 0.0f;

	CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();
	flIdealDistance = GetDecision()->GetWeaponIdealRange( pWeapon );

	GetMemory()->UpdateDataMemory( "IdealDistance", flIdealDistance );

    float enemyDistance = memory->GetDistance();


    if ( enemyDistance <= flIdealDistance / 4 )
        SetCondition( BCOND_ENEMY_TOO_NEAR );
    if ( enemyDistance <= flIdealDistance )
        SetCondition( BCOND_ENEMY_NEAR );
    if ( enemyDistance >= flIdealDistance * 2 )
        SetCondition( BCOND_ENEMY_FAR );
    if ( enemyDistance >= flIdealDistance * 4 )
        SetCondition( BCOND_ENEMY_TOO_FAR );

    if ( !memory->IsVisible() ) 
	{
        // We have no vision of the enemy
        SetCondition( BCOND_ENEMY_LOST );         
    }
    else 
	{
        SetCondition( BCOND_SEE_ENEMY );

        CBaseEntity *pBlockedBy = NULL;

        // No direct line of sight with our enemy
        if ( !GetDecision()->IsLineOfSightClear( memory->GetEntity(), &pBlockedBy ) ) 
		{
            SetCondition( BCOND_ENEMY_OCCLUDED );

            // A friend is in front of us!
            if ( GetDecision()->IsFriend( pBlockedBy ) ) 
			{
                SetCondition( BCOND_ENEMY_OCCLUDED_BY_FRIEND );
            }
        }
    }

    if ( GetDecision()->IsAbleToSee( memory->GetLastKnownPosition() ) ) 
	{
        SetCondition( BCOND_ENEMY_LAST_POSITION_VISIBLE );
    }

    if ( !GetEnemy()->IsAlive() ) 
	{
        if ( GetProfile()->IsEasy() ) 
		{
            if ( GetEnemy()->m_lifeState == LIFE_DEAD ) 
			{
                SetCondition( BCOND_ENEMY_DEAD );
            }
        }
        else 
		{
            SetCondition( BCOND_ENEMY_DEAD );
        }
    }
}

//================================================================================
// Obtiene nuevas condiciones relacionadas al ataque
//================================================================================
void CBot::GatherAttackConditions()
{
    VPROF_BUDGET("SelectAttackConditions", VPROF_BUDGETGROUP_BOTS);

    BCOND condition = GetDecision()->ShouldRangeAttack1();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = GetDecision()->ShouldRangeAttack2();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = GetDecision()->ShouldMeleeAttack1();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = GetDecision()->ShouldMeleeAttack2();

    if ( condition != BCOND_NONE )
        SetCondition( condition );


	condition = GetDecision()->ShouldStrafe();

	if( condition != BCOND_NONE )
		SetCondition( condition );
}

//================================================================================
//================================================================================
void CBot::GatherLocomotionConditions()
{
    if ( !GetLocomotion() )
        return;

    if ( GetLocomotion()->HasDestination() ) 
	{
        if ( GetLocomotion()->IsUnreachable() ) 
		{
            SetCondition( BCOND_GOAL_UNREACHABLE );
        }
    }
}

//================================================================================
//================================================================================
CSquad *CBot::GetSquad() 
{
    return GetHost()->GetSquad();
}

//================================================================================
//================================================================================
void CBot::SetSquad( CSquad *pSquad ) 
{
    GetHost()->SetSquad( pSquad );
}

//================================================================================
//================================================================================
void CBot::SetSquad( const char *name ) 
{
    GetHost()->SetSquad( name );
}

//================================================================================
//================================================================================
bool CBot::IsSquadLeader()
{
    if ( !GetSquad() )
        return false;

    return (GetSquad()->GetLeader() == GetHost());
}

//================================================================================
//================================================================================
CPlayer * CBot::GetSquadLeader()
{
    if ( !GetSquad() )
        return NULL;

    return GetSquad()->GetLeader();
}

//================================================================================
//================================================================================
IBot * CBot::GetSquadLeaderAI()
{
    if ( !GetSquad() )
        return NULL;

    if ( !GetSquad()->GetLeader() )
        return NULL;

    return GetSquad()->GetLeader()->GetBotController();
}

//================================================================================
// Un miembro de nuestro escuadron ha recibido da�o
//================================================================================
void CBot::OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo & info ) 
{
    // Estoy a la defensiva, pero mi escuadron necesita ayuda
   // if ( ShouldHelpFriend() )
    {
        // TODO
    }

    // Han atacado a un jugador normal, reportemos a sus amigos Bots
    if ( !pMember->IsBot() && info.GetAttacker() ) {
        OnMemberReportEnemy( pMember, info.GetAttacker() );
    }
}

//================================================================================
//================================================================================
void CBot::OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo & info ) 
{
    if ( !GetMemory() || !GetVision() )
        return;

    CEntityMemory *memory = GetMemory()->GetEntityMemory( pMember );

    if ( !memory )
        return;

    // �Amigo! �Noo! :'(
    if ( !GetProfile()->IsHard() && memory->IsVisible() ) {
        GetVision()->LookAt( "Squad Member Death", pMember->GetAbsOrigin(), PRIORITY_VERY_HIGH, RandomFloat( 0.3f, 1.5f ) );
    }
}

//================================================================================
//================================================================================
void CBot::OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy ) 
{
    if ( !GetMemory() )
        return;

    // Posici�n estimada del enemigo
    Vector vecEstimatedPosition = pEnemy->WorldSpaceCenter();
    const float errorDistance = 100.0f;

    // Cuando un amigo nos reporta la posici�n de un enemigo siempre debe haber un margen de error,
    // un humano no puede saber la posici�n exacta hasta verlo con sus propios ojos.
    vecEstimatedPosition.x += RandomFloat( -errorDistance, errorDistance );
    vecEstimatedPosition.y += RandomFloat( -errorDistance, errorDistance );

    // Actualizamos nuestra memoria
    GetMemory()->UpdateEntityMemory( pEnemy, vecEstimatedPosition, pMember );
}
