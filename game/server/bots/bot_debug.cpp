//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_gamerules.h"
#else
#include "bots/in_utils.h"
#endif

#include "entity_weapon_spawner.h"
#include "entity_condpowerup.h"
#include "entity_healthkit.h"
#include "entity_ammopack.h"

#include "nav.h"
#include "nav_mesh.h"

#include "ai_hint.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_bot_debug;
extern ConVar tf_bot_debug_locomotion_extra;
extern ConVar tf_bot_debug_locomotion;
extern ConVar tf_bot_debug_memory;
extern ConVar tf_bot_debug_memory_extra;

extern ConVar tf_bot_debug_desires;
extern ConVar tf_bot_debug_conditions;
extern ConVar tf_bot_debug_cmd;
extern ConVar tf_bot_debug_max_msgs;

extern ConVar think_limit;

//================================================================================
// Returns whether to display debugging information for this bot.
//================================================================================
bool CBot::ShouldShowDebug()
{
    if ( !tf_bot_debug.GetBool() )
        return false;

    if ( GetHost() != UTIL_GetListenServerHost() && tf_bot_debug.GetInt() != GetHost()->entindex() ) {
        if ( !IsLocalPlayerWatchingMe() )
            return false;
    }

    return true;
}

//================================================================================
// Displays debug information on the host screen
//================================================================================
void CBot::DebugDisplay()
{
    VPROF_BUDGET( "DebugDisplay", VPROF_BUDGETGROUP_BOTS );

    if ( !ShouldShowDebug() )
        return;

    m_flDebugYPosition = 0.06f;
    CFmtStr msg;

    const Color green = Color( 0, 255, 0, 200 );
    const Color red = Color( 255, 0, 0, 200 );
    const Color yellow = Color( 255, 255, 0, 200 );
    const Color blue = Color( 0, 255, 255, 200 );
    const Color pink = Color( 255, 150, 150, 200 );
    const Color white = Color( 255, 255, 255, 200 );
	const Color gray = Color( 150, 150, 150, 200 );

    float thinkTime = m_RunTimer.GetDuration().GetMillisecondsF();
    float scheduleTime = m_ScheduleTimer.GetDuration().GetMicrosecondsF();

    // General
    if ( thinkTime >= think_limit.GetFloat() ) {
        DebugScreenText( msg.sprintf( "%s (%.3f ms)", GetName(), thinkTime ), red );
        DebugAddMessage( "RunAI: %.3f ms !!", thinkTime );
    }
    else {
        DebugScreenText( msg.sprintf( "%s (%.3f ms)", GetName(), thinkTime ) );
    }

    DebugScreenText( msg.sprintf( "%s - %s", GetProfile()->GeSkillName(), g_TacticalModes[GetTacticalMode()] ) );
    DebugScreenText( msg.sprintf( "Health: %i", GetHealth() ) );
    DebugScreenText( "" );

    // State
    switch ( GetState() ) {
        case STATE_IDLE:
        default:
            DebugScreenText( "IDLE", green );
            break;

        case STATE_ALERT:
            DebugScreenText( "ALERT", yellow );
            DebugScreenText( msg.sprintf( "    Time Left: %.2fs", GetStateDuration() ), yellow );
            break;

        case STATE_COMBAT:
            DebugScreenText( "COMBAT", red );
            break;

        case STATE_PANIC:
            DebugScreenText( "PANIC!!", red );
            DebugScreenText( msg.sprintf( "    Time Left: %.2fs", GetStateDuration() ), red );
            break;
    }

    DebugScreenText( "" );
    DebugScreenText( "A.I." );

    // Schedule
    if ( GetActiveSchedule() ) {
        IBotSchedule *pSchedule = GetActiveSchedule();
        DebugScreenText( msg.sprintf( "    Schedule: %s (%.3f ms)", g_BotSchedules[pSchedule->GetID()], scheduleTime ) );
        DebugScreenText( msg.sprintf( "    Task: %s", pSchedule->GetActiveTaskName() ) );

        /*if ( scheduleTime >= 0.5f ) {
            DebugAddMessage( "%s:%s %.3f ms !!", g_BotSchedules[pSchedule->GetID()], pSchedule->GetActiveTaskName(), scheduleTime );
        }*/
    }
    else {
        DebugScreenText( msg.sprintf( "    Schedule: -" ) );
        DebugScreenText( msg.sprintf( "    Task: -" ) );
    }

    {
        IBotSchedule *pIdealSchedule = GetSchedule( SelectIdealSchedule() );

        if ( pIdealSchedule )
            DebugScreenText( msg.sprintf( "    Ideal Schedule: %s (%.2f)", g_BotSchedules[pIdealSchedule->GetID()], pIdealSchedule->GetDesire() ) );
        else
            DebugScreenText( msg.sprintf( "    Ideal Schedule: -" ) );
    }

    // Schedule Desires
    if ( tf_bot_debug_desires.GetBool() ) 
	{
		DebugScreenText("");
		DebugScreenText( "Desires:" );

        FOR_EACH_MAP( m_nSchedules, it )
        {
            IBotSchedule *pItem = m_nSchedules[it];

            float realDesire = pItem->GetInternalDesire();
            Color desireColor = white;

			if (realDesire >= BOT_DESIRE_VERYHIGH)
			{
				desireColor = green;
			}
			else if (realDesire == BOT_DESIRE_HIGH)
			{
				desireColor = yellow;
			}
			else if (realDesire == BOT_DESIRE_MODERATE)
			{
				desireColor = pink;
			}
			else if (realDesire > BOT_DESIRE_NONE)
			{
				desireColor = red;
			}

            if ( pItem == GetActiveSchedule() )
			{
				DebugScreenText(msg.sprintf("    %s = %.2f (%.2f)", g_BotSchedules[pItem->GetID()], pItem->GetInternalDesire(), pItem->GetDesire()), green);
			}
			else
			{
				DebugScreenText(msg.sprintf("    %s = %.2f (%.2f)", g_BotSchedules[pItem->GetID()], pItem->GetInternalDesire(), pItem->GetDesire()), desireColor);
			}
        }
    }

    // Memory
	if (tf_bot_debug_memory.GetBool())
	{
		if (GetMemory()) 
		{
			DebugScreenText("");
			DebugScreenText(msg.sprintf("Memory (Ents: %i) (%.3f ms):", GetMemory()->GetKnownCount(), GetMemory()->GetUpdateCost()), white);
			DebugScreenText(msg.sprintf("    Threats: %i (Nearby: %i - Dangerous: %i)", GetMemory()->GetThreatCount(), GetDataMemoryInt("NearbyThreats"), GetDataMemoryInt("NearbyDangerousThreats")), yellow);
			DebugScreenText(msg.sprintf("    Friends: %i (Nearby: %i)", GetMemory()->GetFriendCount(), GetDataMemoryInt("NearbyFriends")), green);
			
			if (GetEnemy()) 
			{
				CEntityMemory *pThreat = GetMemory()->GetPrimaryThreat();
				CEntityMemory *pIdeal = GetMemory()->GetIdealThreat();
				if (pIdeal) 
				{
					DebugScreenText(msg.sprintf("    Ideal Threat: %s (%s)", pIdeal->GetEntity()->GetClassname(), STRING(pIdeal->GetEntity()->GetEntityName())), blue);
					if (pIdeal != pThreat) 
					{
						NDebugOverlay::EntityBounds(pIdeal->GetEntity(), blue.r(), blue.g(), blue.b(), 15.0f, 0.1f);
					}
				}

				DebugScreenText(msg.sprintf("    Primary Threat: %s (%s)", pThreat->GetEntity()->GetClassname(), STRING(pThreat->GetEntity()->GetEntityName())), red);
				DebugScreenText(msg.sprintf("        Time Left: %.2fs", pThreat->GetTimeLeft()), white);
				DebugScreenText(msg.sprintf("        Visible: %i (%.2f since visible)", pThreat->IsVisible(), pThreat->GetElapsedTimeSinceVisible()), white);
				DebugScreenText(msg.sprintf("        Distance: %.2f", pThreat->GetDistance()), white);
				DebugScreenText(msg.sprintf("        Hitbox: (H: %i) (C: %i) (LL: %i) (RL: %i)",
					pThreat->GetVisibleHitbox().head.IsValid(),
					pThreat->GetVisibleHitbox().chest.IsValid(),
					pThreat->GetVisibleHitbox().leftLeg.IsValid(),
					pThreat->GetVisibleHitbox().rightLeg.IsValid()), white);

				if (tf_bot_debug_memory_extra.GetBool()) 
				{
					NDebugOverlay::EntityBounds(pThreat->GetEntity(), red.r(), red.g(), red.b(), 15.0f, 0.1f);
				}
				else 
				{
					NDebugOverlay::EntityBounds(pThreat->GetEntity(), red.r(), red.g(), red.b(), 5.0f, 0.1f);
				}

				if (pThreat) 
				{
					if (pThreat->GetVisibleHitbox().head.IsValid())
					{
						NDebugOverlay::Box(pThreat->GetVisibleHitbox().head, Vector(-2, -2, -2), Vector(2, 2, 2), 255, 0, 0, 10.0f, 0.1f); // Rojo
					}
					if (pThreat->GetVisibleHitbox().chest.IsValid())
					{
						NDebugOverlay::Box(pThreat->GetVisibleHitbox().chest, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 255, 10.0f, 0.1f); // Azul
					}
					if (pThreat->GetVisibleHitbox().leftLeg.IsValid())
					{
						NDebugOverlay::Box(pThreat->GetVisibleHitbox().leftLeg, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 255, 0, 10.0f, 0.1f); // Verde
					}
					if (pThreat->GetVisibleHitbox().rightLeg.IsValid())
					{
						NDebugOverlay::Box(pThreat->GetVisibleHitbox().rightLeg, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 255, 0, 10.0f, 0.1f); // Verde
					}
				}
			}
			else 
			{
				DebugScreenText(msg.sprintf("    Primary Threat: None"), red);
			}
		}
	}

    // Squad
    if ( GetSquad() ) 
	{
		 DebugScreenText( "" );
		DebugScreenText( "Squad:", pink );

		if ( GetSquad()->GetLeader() == GetHost() )
		{
			DebugScreenText( msg.sprintf( "    Name: %s (LEADER)", GetSquad()->GetName() ), pink );
		}
		else
		{
			DebugScreenText(msg.sprintf("    Name: %s", GetSquad()->GetName()), pink);
			DebugScreenText(msg.sprintf("    Members: %i", GetSquad()->GetCount()), pink);
		}
    }

    // Vision
	if ( GetVision() ) 
	{
		DebugScreenText( "" );
		DebugScreenText( msg.sprintf("Vision (%.3f ms):", GetVision()->GetUpdateCost()), yellow );

		if ( GetVision()->HasAimGoal() ) 
		{
			int priority = GetVision()->GetPriority();
		
			if ( GetVision()->IsAimReady() ) 
			{
				DebugScreenText( msg.sprintf( "    Looking At: %s (READY)", GetVision()->GetDescription() ), yellow );
				DebugScreenText( msg.sprintf( "    Time Left: %.2fs", GetVision()->GetTimer().GetRemainingTime() ), yellow );
			}
			else 
			{
			DebugScreenText( msg.sprintf( "    Looking At: %s (AIMING)", GetVision()->GetDescription() ), yellow );
			}

			DebugScreenText( msg.sprintf( "    Priority: %s", g_PriorityNames[priority] ), yellow );

			if ( GetVision()->GetAimTarget() ) 
			{
				DebugScreenText( msg.sprintf( "    Entity: %s", GetVision()->GetAimTarget()->GetClassname() ), yellow );
			}

				NDebugOverlay::Line( GetHost()->EyePosition(), GetVision()->GetAimGoal(), yellow.r(), yellow.g(), yellow.b(), true, 0.1f );
		}
		else 
		{
			DebugScreenText( msg.sprintf( "    Looking At: None" ), yellow );
		}
    }

    // Locomotion
	if (tf_bot_debug_locomotion.GetBool())
	{
		if (GetLocomotion())
		{
			DebugScreenText("");
			DebugScreenText(msg.sprintf("Locomotion (%.3f ms):", GetLocomotion()->GetUpdateCost()), blue);

			if (GetLocomotion()->HasDestination())
			{
				Vector vecDestination = GetLocomotion()->GetDestination();
				int priority = GetLocomotion()->GetPriority();

				DebugScreenText(msg.sprintf("    Destination: %s (%.2f since build)", GetLocomotion()->GetDescription(), GetLocomotion()->GetPath()->GetElapsedTimeSinceBuild()), blue);
				DebugScreenText(msg.sprintf("    Distance Left: %.2f", GetLocomotion()->GetDistanceToDestination()), blue);
				DebugScreenText(msg.sprintf("    Priority: %s", g_PriorityNames[priority]), blue);
				DebugScreenText(msg.sprintf("    Using Ladder: %i", GetLocomotion()->IsUsingLadder()), blue);
				//DebugScreenText( msg.sprintf( "    Commands: forward: %.2f, side: %.2f, up: %.2f", GetUserCommand()->forwardmove, GetUserCommand()->sidemove, GetUserCommand()->upmove ), blue );

				if (GetFollow() && GetFollow()->IsFollowing()) {
					DebugScreenText(msg.sprintf("    Following: %s (Enabled: %i)", GetFollow()->GetEntity()->GetClassname(), GetFollow()->IsEnabled()), blue);
				}

				if (GetLocomotion()->IsStuck()) {
					DebugScreenText(msg.sprintf("    STUCK (%.2f)", GetLocomotion()->GetStuckDuration()), red);
				}

				NDebugOverlay::Line(GetAbsOrigin(), vecDestination, blue.r(), blue.g(), blue.b(), true, 0.1f);
			}
			else
			{
				DebugScreenText(msg.sprintf("    Destination: None"), blue);
			}
		}
	}

	// Conditions
	if (tf_bot_debug_conditions.GetBool())
	{
		DebugScreenText("");
		DebugScreenText("Conditions:");

		for (int i = 0; i < LAST_BCONDITION; ++i)
		{
			if (HasCondition((BCOND)i))
			{
				const char *pName = g_Conditions[i];

				if (pName)
				{
					DebugScreenText(msg.sprintf("    %s", pName));
				}
			}
		}
	}

    // Deep Memory
	if (tf_bot_debug_memory_extra.GetBool() && GetMemory())
	{
		FOR_EACH_MAP_FAST(GetMemory()->m_Memory, it)
		{
			CEntityMemory *memory = GetMemory()->m_Memory[it];
			Assert(memory);

			CBaseEntity *pEntity = memory->GetEntity();
			Assert(pEntity);

			if (memory == GetMemory()->GetPrimaryThreat() || memory == GetMemory()->GetIdealThreat())
			{
				continue;
			}

			if (pEntity)
			{
				Vector lastposition = memory->GetLastKnownPosition();
				CCollisionProperty *pCollide = pEntity->CollisionProp();

				if (pEntity->MyCombatCharacterPointer())
				{
					lastposition.z -= HalfHumanHeight;
				}

				if (memory->GetInformer())
				{
					NDebugOverlay::Line(lastposition, memory->GetInformer()->WorldSpaceCenter(), pink.r(), pink.g(), pink.b(), true, 0.1f);
					NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), pink.r(), pink.g(), pink.b(), 5.0f, 0.1f);
				}
				else
				{
					if (memory->IsFriend())
					{
						NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), green.r(), green.g(), green.b(), 5.0f, 0.1f);
					}
					else if (memory->IsEnemy())
					{
						NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), red.r(), red.g(), red.b(), 5.0f, 0.1f);
					}
					else
					{
						if (memory->GetEntity()->ClassMatches("dm_weapon_spawner"))
						{
							NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), yellow.r(), yellow.g(), yellow.b(), 5.0f, 0.1f);
						}

						if (memory->GetEntity()->ClassMatches("dm_powerup_spawner"))
						{
							NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), blue.r(), blue.g(), blue.b(), 5.0f, 0.1f);
						}

						if (
							memory->GetEntity()->ClassMatches("item_healthkit_full") ||
							memory->GetEntity()->ClassMatches("item_healthkit_medium") ||
							memory->GetEntity()->ClassMatches("item_healthkit_small")
							)
						{
							NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), green.r(), green.g(), green.b(), 5.0f, 0.1f);
						}

						if (
							memory->GetEntity()->ClassMatches("item_ammopack_full") ||
							memory->GetEntity()->ClassMatches("item_ammopack_medium") ||
							memory->GetEntity()->ClassMatches("item_ammopack_small")
							)
						{
							NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), gray.r(), gray.g(), gray.b(), 5.0f, 0.1f);
						}

						NDebugOverlay::Box(lastposition, pCollide->OBBMins(), pCollide->OBBMaxs(), white.r(), white.g(), white.b(), 5.0f, 0.1f);
					}
				}
			}
		}
	}

    // Command
	if (tf_bot_debug_cmd.GetBool())
	{
		DebugScreenText("");
		DebugScreenText(msg.sprintf("command_number: %i", GetUserCommand()->command_number));
		DebugScreenText(msg.sprintf("tick_count: %i", GetUserCommand()->tick_count));
		DebugScreenText(msg.sprintf("viewangles: %.2f, %.2f", GetUserCommand()->viewangles.x, GetUserCommand()->viewangles.y));
		DebugScreenText(msg.sprintf("forwardmove: %.2f", GetUserCommand()->forwardmove));
		DebugScreenText(msg.sprintf("sidemove: %.2f", GetUserCommand()->sidemove));
		DebugScreenText(msg.sprintf("upmove: %.2f", GetUserCommand()->upmove));
		DebugScreenText(msg.sprintf("buttons:%i", (int)GetUserCommand()->buttons));
		DebugScreenText(msg.sprintf("impulse: %i", (int)GetUserCommand()->impulse));
		DebugScreenText(msg.sprintf("weaponselect: %i", GetUserCommand()->weaponselect));
		DebugScreenText(msg.sprintf("weaponsubtype: %i", GetUserCommand()->weaponsubtype));
		DebugScreenText(msg.sprintf("random_seed: %i", GetUserCommand()->random_seed));
		DebugScreenText(msg.sprintf("mousedx: %i", (int)GetUserCommand()->mousedx));
		DebugScreenText(msg.sprintf("mousedy: %i", (int)GetUserCommand()->mousedy));
		DebugScreenText(msg.sprintf("hasbeenpredicted: %i", (int)GetUserCommand()->hasbeenpredicted));
	}

    // Debug Messages

    const float fadeAge = 7.0f;
    const float maxAge = 10.0f;

    DebugScreenText( "" );
    DebugScreenText( "" );

	FOR_EACH_VEC(m_debugMessages, it)
	{
		DebugMessage *message = &m_debugMessages.Element(it);

		if (!message)
		{
			continue;
		}

		if (message->m_age.GetElapsedTime() < maxAge)
		{
			int alpha = 255;

			if (message->m_age.GetElapsedTime() > fadeAge)
			{
				alpha *= (1.0f - (message->m_age.GetElapsedTime() - fadeAge) / (maxAge - fadeAge));
			}

			DebugScreenText(UTIL_VarArgs("%2.f - %s", message->m_age.GetStartTime(), message->m_string), Color(255, 255, 255, alpha));
		}
	}

    // Interesting Spots

	if (!GetHost()->IsBot())
	{
		return;
	}

	if( !GetHost()->IsBotOfType( BOT_TYPE_INSOURCE ) )
		return;

    Vector vecDummy;

    // CSpotCriteria
    CSpotCriteria criteria;
    criteria.SetMaxRange( 1000.0f );

    // Cover Spots
    SpotVector spotList;
    Utils::FindNavCoverSpot( &vecDummy, GetAbsOrigin(), criteria, GetHost(), &spotList );

    SpotVector hintList;
    CHintCriteria hintCriteria;
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_MED );
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_LOW );
    hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING );
    Utils::FindHintSpot( GetAbsOrigin(), hintCriteria, criteria, GetHost(), &spotList );

	FOR_EACH_VEC(spotList, it)
	{
		Vector vecSpot = spotList.Element(it);
		if (GetHost()->FVisible(vecSpot) && GetHost()->IsInFieldOfView(vecSpot))
		{
			NDebugOverlay::VertArrow(vecSpot + Vector(0, 0, 15.0f), vecSpot, 15.0f / 4.0f, 255, 255, 255, 100.0f, true, 0.15f);
		}
		else
		{
			NDebugOverlay::VertArrow(vecSpot + Vector(0, 0, 15.0f), vecSpot, 15.0f / 4.0f, 0, 0, 0, 100.0f, true, 0.15f);
		}
	}

	FOR_EACH_VEC(hintList, it)
	{
		Vector vecSpot = hintList.Element(it);

		if (GetHost()->FVisible(vecSpot) && GetHost()->IsInFieldOfView(vecSpot))
		{
			NDebugOverlay::VertArrow(vecSpot + Vector(0, 0, 15.0f), vecSpot, 15.0f / 4.0f, 255, 255, 255, 0, true, 0.15f);
		}
		else
		{
			NDebugOverlay::VertArrow(vecSpot + Vector(0, 0, 15.0f), vecSpot, 15.0f / 4.0f, 0, 0, 0, 0, true, 0.15f);
		}
	}
}

//================================================================================
//================================================================================
void CBot::DebugScreenText(const char *pText, Color color, float yPosition, float duration)
{
	if (yPosition < 0)
	{
		yPosition = m_flDebugYPosition;
	}
	else
	{
		m_flDebugYPosition = yPosition;
	}

	NDebugOverlay::ScreenText(0.6f, yPosition, pText, color.r(), color.g(), color.b(), color.a(), duration);
	m_flDebugYPosition += 0.02f;
}

//================================================================================
//================================================================================
void CBot::DebugAddMessage(const char *format, ...)
{
	va_list varg;
	char buffer[1024];

	va_start(varg, format);
	vsprintf(buffer, format, varg);
	va_end(varg);

	DebugMessage message;
	message.m_age.Start();
	Q_strncpy(message.m_string, buffer, 1024);

	m_debugMessages.AddToHead(message);

	if (m_debugMessages.Count() >= tf_bot_debug_max_msgs.GetInt())
	{
		m_debugMessages.RemoveMultipleFromTail(1);
	}
}

#ifdef DEBUG
Vector g_DebugSpot1;
Vector g_DebugSpot2;

CNavPath g_DebugPath;
CNavPathFollower g_DebugNavigation;

CON_COMMAND_F( tf_bot_debug_mark_spot1, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugSpot1 = pOwner->GetAbsOrigin();
}

CON_COMMAND_F( tf_bot_debug_mark_spot2, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugSpot2 = pOwner->GetAbsOrigin();
}

CON_COMMAND_F( tf_bot_debug_process_navigation, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugPath.Invalidate();

    CBot *pBot = new CBot( pOwner );
    pBot->Spawn();

    CSimpleBotPathCost pathCost( pBot );
	bool result = g_DebugPath.Compute( g_DebugSpot1, g_DebugSpot2, pathCost );
	g_DebugPath.Draw();

    if ( result ) {
        DevMsg( "Path Computed Correctly\n" );
    }
    else {
        DevWarning( "Path Computed FAIL!\n" );
    }

    delete pBot;
    pBot = NULL;
}
#endif