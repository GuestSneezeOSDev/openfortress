//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots/bot_manager.h"

#include "bots/bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots/in_utils.h"
#endif

#include "team.h"
#include "tf_player_resource.h"
#include <tf_team.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bot_quota("tf_bot_quota", "0", FCVAR_NOTIFY, "Determines the total number of tf bots in the game.");
ConVar tf_bot_join_after_player( "tf_bot_join_after_player", "1", FCVAR_NOTIFY, "If nonzero, bots wait until a player joins before entering the game." );
ConVar tf_bot_auto_vacate( "tf_bot_auto_vacate", "1", FCVAR_NOTIFY, "If nonzero, bots will automatically leave to make room for human players." );
extern ConVar tf_bot_quota_mode;
extern ConVar tf_bot_auto_balance;
extern ConVar mp_teams_unbalance_limit;

extern Vector GetRandomPlayerColor();

CBotManager g_BotManager;
CBotManager *TheBots = &g_BotManager;

bool KickBotFromTeam( int iTeam )
{
	CBasePlayer* pPlayer = NULL;

	CTeam *pTeam = GetGlobalTeam(iTeam);
	for( int i = 0; i < pTeam->GetNumPlayers(); i++ )
	{
		CBasePlayer *pCandidate = pTeam->GetPlayer( i );

		if ( !pCandidate )
			continue;

		if ( !pCandidate->IsBot() )
			continue;

		if( !pCandidate->IsBotOfType( BOT_TYPE_INSOURCE ) || pCandidate->IsBotOfType( BOT_TYPE_LOCKDOWN ))
			continue;

		if( !pPlayer ||
			GetTFPlayerResource()->GetTotalScore( pCandidate->entindex() ) 
			< GetTFPlayerResource()->GetTotalScore( pPlayer->entindex() )
			)
			pPlayer = pCandidate;
	}

	if( pPlayer )
	{
		pPlayer->GetBotController()->Kick();

		return true;
	}

	return false;
}

bool BotChangeTeams( int iCurrTeam, int iNewTeam )
{
	CPlayer* pPlayer = NULL;

	CTFTeam *pTeam = GetGlobalTFTeam(iCurrTeam);
	for( int i = 0; i < pTeam->GetNumPlayers(); i++ )
	{
		CPlayer *pCandidate = (CPlayer*) pTeam->GetPlayer( i );

		if ( !pCandidate )
			continue;

		if ( !pCandidate->IsBot() )
			continue;

		if( !pCandidate->IsBotOfType( BOT_TYPE_INSOURCE ) || pCandidate->IsBotOfType( BOT_TYPE_LOCKDOWN ))
			continue;

		if( !pPlayer ||
			GetTFPlayerResource()->GetTotalScore( pCandidate->entindex() ) 
			< GetTFPlayerResource()->GetTotalScore( pPlayer->entindex() )
			)
			pPlayer = pCandidate;
	}

	if( pPlayer )
	{
		pPlayer->HandleCommand_JoinTeam( GetGlobalTFTeam( iNewTeam )->GetName() );

		return true;
	}

	return false;
}

void AutoBalanceBots()
{
	if( !TeamplayRoundBasedRules() )
		return;

	if( !TeamplayRoundBasedRules()->ShouldBalanceTeams( true ) )
		return;
	
	int iTeamPlayerNumbers[TF_TEAM_COUNT];
	memset( iTeamPlayerNumbers, -1, sizeof(int) * TF_TEAM_COUNT );

	int iHeaviestTeam = TEAM_UNASSIGNED;
	int iLightestTeam = TEAM_UNASSIGNED;

	CBasePlayer *pPlayer = NULL;

	int iTeam = TF_TEAM_RED;
	for( CTFTeam *pTeam = GetGlobalTFTeam(iTeam); pTeam != GetGlobalTFTeam(TF_TEAM_MERCENARY) ; pTeam = GetGlobalTFTeam(++iTeam) )
	{
		iTeamPlayerNumbers[iTeam] = pTeam->GetNumPlayers() - pTeam->GetNumLockdownBots();

		if( iTeamPlayerNumbers[iTeam] > iTeamPlayerNumbers[iHeaviestTeam] )
			iHeaviestTeam = iTeam;

		if( iLightestTeam == TEAM_UNASSIGNED || iTeamPlayerNumbers[iTeam] < iTeamPlayerNumbers[iLightestTeam] )
			iLightestTeam = iTeam;
	}

	if( iTeamPlayerNumbers[iLightestTeam] == iTeamPlayerNumbers[iHeaviestTeam] )
		return;

	if( iTeamPlayerNumbers[iHeaviestTeam] - iTeamPlayerNumbers[iLightestTeam] <= mp_teams_unbalance_limit.GetInt() )
		return;

	if( GetGlobalTFTeam(iHeaviestTeam)->GetNumQuotaBots() > 0 )
	{
		// Instead of kicking and adding bots, simply make them switch teams
		BotChangeTeams( iHeaviestTeam, iLightestTeam );
		/*
		// Kick from heaviest team
		if( !KickBotFromTeam( iHeaviestTeam ) )
			return;
		
		CPlayer *pPlayer = CreateBot( NULL, NULL, NULL );
		Assert( pPlayer );

		if ( pPlayer )
		{
			// Should probably be its own function somewhere
			pPlayer->m_vecPlayerColor = GetRandomPlayerColor();

			// Add to lightest
			pPlayer->HandleCommand_JoinTeam( GetGlobalTFTeam( iLightestTeam )->GetName() );
			pPlayer->HandleCommand_JoinClass( "random" );
		}
		*/
	}
}

void AutoKickBot()
{
	int iTeamPlayerNumbers[TF_TEAM_COUNT];
	memset( iTeamPlayerNumbers, -1, sizeof(int) * TF_TEAM_COUNT );

	int iHeaviestTeam = TEAM_UNASSIGNED;
	int iLightestTeam = TEAM_UNASSIGNED;

	int iTeam = TF_TEAM_RED;
	for( CTeam *pTeam = GetGlobalTeam(iTeam); pTeam != GetGlobalTeam(TF_TEAM_MERCENARY) ; pTeam = GetGlobalTeam(++iTeam) )
	{
		iTeamPlayerNumbers[iTeam] = pTeam->GetNumPlayers();

		if( iTeamPlayerNumbers[iTeam] > iTeamPlayerNumbers[iHeaviestTeam] )
			iHeaviestTeam = iTeam;

		if( iLightestTeam == TEAM_UNASSIGNED || iTeamPlayerNumbers[iTeam] < iTeamPlayerNumbers[iLightestTeam] )
			iLightestTeam = iTeam;
	}

	// If teams are unbalanced, assume a bot is on the team with more players
	if( iTeamPlayerNumbers[iLightestTeam] != iTeamPlayerNumbers[iHeaviestTeam] )
	{
		// Kick that bot
		if( KickBotFromTeam(iHeaviestTeam) )
			return;
	}

	// Otherwise just kick any bot
	CBasePlayer* pPlayer = NULL;
	for( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CBasePlayer *pCandidate = UTIL_PlayerByIndex( i );

		if ( !pCandidate )
			continue;

		if ( !pCandidate->IsBot() )
			continue;

		if( !pCandidate->IsBotOfType( BOT_TYPE_INSOURCE ) )
			continue;

		if( !pPlayer ||
			GetTFPlayerResource()->GetTotalScore( pCandidate->entindex() ) 
			< GetTFPlayerResource()->GetTotalScore( pPlayer->entindex() )
			)
			pPlayer = pCandidate;
	}

	if( pPlayer )
	{
		pPlayer->GetBotController()->Kick();
	}
}

void BotQuotaThink( int iPlayers, int iActivePlayers, int iBots )
{
    int iMode = UTIL_StringFieldToInt( tf_bot_quota_mode.GetString(), g_QuotaModes, TF_BOT_QUOTA_COUNT );
	iMode = max( 0, iMode );
	
	int iDesiredBots = 0;
	int iRealPlayers = iPlayers - iBots;
	
	switch( iMode )
	{
		case TF_BOT_QUOTA_FILL:
		{
			// We just check the total amount of players here
			iDesiredBots = max( tf_bot_quota.GetInt() - iActivePlayers, 0 );
			break;
		}
		case TF_BOT_QUOTA_MATCH:
		{
			iDesiredBots = (int)((float)iActivePlayers * tf_bot_quota.GetFloat());
			break;
		}
		case TF_BOT_QUOTA_BALANCE:
		{
			int iTeamPlayerNumbers[TF_TEAM_COUNT];
			memset( iTeamPlayerNumbers, -1, sizeof(int) * TF_TEAM_COUNT );

			int iHeavyTeam = TEAM_UNASSIGNED;
			int iLightTeam = TEAM_UNASSIGNED;

			int iTeam = TF_TEAM_RED;
			for( CTFTeam *pTeam = GetGlobalTFTeam(iTeam); pTeam != GetGlobalTFTeam(TF_TEAM_MERCENARY); pTeam = GetGlobalTFTeam(++iTeam) )
			{
				// Heaviest team should not have any bots
				iTeamPlayerNumbers[iTeam] = pTeam->GetNumPlayers() - pTeam->GetNumQuotaBots();

				if( iTeamPlayerNumbers[iTeam] > iTeamPlayerNumbers[iHeavyTeam] )
					iHeavyTeam = iTeam;

				if( iLightTeam == TEAM_UNASSIGNED || iTeamPlayerNumbers[iTeam] < iTeamPlayerNumbers[iLightTeam] )
					iLightTeam = iTeam;
			}

			// Add as many bots needed to make every team have the same player count
			iTeam = TF_TEAM_RED;
			for( CTFTeam *pTeam = GetGlobalTFTeam(iTeam); pTeam != GetGlobalTFTeam(TF_TEAM_MERCENARY); pTeam = GetGlobalTFTeam(++iTeam) )
			{
				iDesiredBots += iTeamPlayerNumbers[iHeavyTeam] - (pTeam->GetNumPlayers() - pTeam->GetNumQuotaBots());
			}

			break;
		}
		case TF_BOT_QUOTA_NORMAL:
		default:
		{
			iDesiredBots = tf_bot_quota.GetFloat();
			break;
		}
	}

	if( tf_bot_auto_vacate.GetBool() )
		iDesiredBots = min( iDesiredBots, gpGlobals->maxClients - iRealPlayers - 1 );
	
	if( tf_bot_join_after_player.GetBool() && iRealPlayers < 1 )
		iDesiredBots = 0;

	// We dont have our desired bots
	if( iBots != iDesiredBots )
	{
		// If we have more bots, kick them
		if( iDesiredBots < iBots )
		{
			AutoKickBot();
		}
		else
		{
		// Not above, so below, add a bot
		// JoinTeam auto handles balancing
		
			CPlayer *pPlayer = CreateBot( NULL, NULL, NULL );
			Assert( pPlayer );

			if ( pPlayer )
			{
				// Should probably be its own function somewhere
				pPlayer->m_vecPlayerColor = GetRandomPlayerColor();
			
				pPlayer->HandleCommand_JoinTeam( "auto" );
				pPlayer->HandleCommand_JoinClass( "random" );

                pPlayer->EquipRandomCosmetics( pPlayer->GetDesiredPlayerClassIndex() );
			}
		}
	}

	AutoBalanceBots();
}

void SourceBot_RunAll()
{
	int iPlayers = 0;
	int iActivePlayers = 0;
	int iBots = 0;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) 
	{
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(it) );

        if ( !pPlayer )
            continue;

		iPlayers++;
		if( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			iActivePlayers++;

		if( !pPlayer->IsBot() )
			continue;
		
		if( pPlayer->IsBotOfType(BOT_TYPE_INSOURCE) || pPlayer->IsBotOfType(BOT_TYPE_LOCKDOWN) )
			iActivePlayers--;

		if( !pPlayer->IsBotOfType(BOT_TYPE_INSOURCE) && !pPlayer->IsBotOfType(BOT_TYPE_LOCKDOWN) )
			continue;

		iBots++;

        pPlayer->GetBotController()->Update();
    }
	
	BotQuotaThink( iPlayers, iActivePlayers, iBots );
}

//================================================================================
//================================================================================
CBotManager::CBotManager() : CAutoGameSystemPerFrame("BotManager")
{
}

//================================================================================
//================================================================================
bool CBotManager::Init()
{
    Utils::InitBotTrig();
    return true;
}

//================================================================================
//================================================================================
void CBotManager::LevelInitPostEntity()
{
    
}

//================================================================================
//================================================================================
void CBotManager::LevelShutdownPreEntity()
{
#ifdef INSOURCE_DLL
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsBot() )
            continue;
		
		if( !pPlayer->IsBotOfType( BOT_TYPE_INSOURCE ) )
			continue;

        pPlayer->Kick();
    }

    engine->ServerExecute();
#endif
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePreEntityThink()
{
	// This is literally the entire bot system  think, why is it disabled by default? - Kay
#if defined( INSOURCE_DLL ) || defined( OF_DLL )
    SourceBot_RunAll();
#endif
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePostEntityThink()
{

}
