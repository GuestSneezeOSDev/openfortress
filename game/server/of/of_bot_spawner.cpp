// ========= Copyright Open Fortress Developers, CC-BY-NC-SA ============
// Purpose: Spawns bots via templates and assigns them different tasks
// Author(s): KaydemonLP


// Note: Bots have currently been removed from Open Fortress
//		 However this entity's code remains here
//		 due to the fact that it can be reused
//		 with any future bot system


#include "cbase.h"

#include "tf_player.h"
#include "bot.h"
#include "bot_manager.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "entitylist.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "of_bot_spawner.h"
#include "of_map_data.h"
#include "props_shared.h"
#include "entity_tools_server.h"

// memdbgon must be the last include file in a .cpp file!!!
// memdbgon is the worst show EVER it is the wo
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CTFBotSpawner)

DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

DEFINE_KEYFIELD( m_iszBotTemplate[0], FIELD_STRING, "BotTemplate1" ),
DEFINE_KEYFIELD( m_iszBotTemplate[1], FIELD_STRING, "BotTemplate2" ),
DEFINE_KEYFIELD( m_iszBotTemplate[2], FIELD_STRING, "BotTemplate3" ),
DEFINE_KEYFIELD( m_iszBotTemplate[3], FIELD_STRING, "BotTemplate4" ),
DEFINE_KEYFIELD( m_iszBotTemplate[4], FIELD_STRING, "BotTemplate5" ),

DEFINE_KEYFIELD( m_iTemplate,			FIELD_INTEGER,	"TemplateNum" 		 ),
DEFINE_KEYFIELD( m_bRandomizeTemplates, FIELD_BOOLEAN, 	"RandomizeTemplates" ),
DEFINE_KEYFIELD( m_iWaveSize, 			FIELD_INTEGER, 	"WaveSize"			 ),
DEFINE_KEYFIELD( m_flWaveIntervals, 	FIELD_FLOAT, 	"WaveIntervals"		 ),
DEFINE_KEYFIELD( m_bSpawnWhenCleared,	FIELD_BOOLEAN,	"SpawnWhenCleared"	 ),
DEFINE_KEYFIELD( m_iszWavePreset, 		FIELD_STRING, 	"WavePreset"		 ),

DEFINE_KEYFIELD( m_iszTarget, 		FIELD_STRING, "Target" ),
DEFINE_KEYFIELD( m_iszSpawnPoint, 	FIELD_STRING, "SpawnPoint" ),

DEFINE_KEYFIELD( m_flSpawnRadius,	FIELD_FLOAT, "SpawnRadius" ),
// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, 	 "SpawnWave",		SpawnWave		),
DEFINE_INPUTFUNC( FIELD_VOID, 	 "ResetInterval",	ResetInterval	),
DEFINE_INPUTFUNC( FIELD_FLOAT, 	 "SetInterval",		SetInterval		),
DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTemplate",		SetTemplate		),
DEFINE_INPUTFUNC( FIELD_STRING,  "SetTarget",		SetTarget		),
DEFINE_INPUTFUNC( FIELD_VOID,	 "KillAllBots",		KillAllBots		),

DEFINE_INPUTFUNC( FIELD_VOID, "Enable", Enable	 ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", Disable ),

// Outputs

DEFINE_OUTPUT( m_OnAllBotsKilled, "OnAllBotsKilled" ),
DEFINE_OUTPUT( m_OnBotKilled, "OnBotKilled" ),
DEFINE_OUTPUT( m_OnBotsSpawned, "OnBotsSpawned" ),

DEFINE_THINKFUNC( GeneratorThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFBotSpawner, DT_BotSpawner )
	SendPropInt( SENDINFO( m_iTemplate ) ),
	SendPropInt( SENDINFO( m_iMaxTemplates ) ),
	SendPropInt( SENDINFO( m_iWaveSize ) ),
	SendPropStringT( SENDINFO( m_iszWavePreset ) ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszBotTemplate ) ), m_iszBotTemplate ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( of_bot_spawner, CTFBotSpawner );

extern void UTIL_PrecacheSchemaWeapon( const char *szName );

CTFBotSpawner::CTFBotSpawner()
{
	m_flBotSpawnTick = 0.0f;
	m_bBotsSpawned = true;
	m_bDisabled = false;
	m_iTemplate = 0;
	m_iMaxTemplates = 0;
	pSpawnPoint = NULL;

	SetThink( &CTFBotSpawner::GeneratorThink );
	
	SetNextThink( gpGlobals->curtime );
	
	ListenForGameEvent("player_death");
	
	for( int i = 0; i < MAX_BOT_TEMPLATES; i++ )
		inBotTemplate[i] = NULL;
}

void CTFBotSpawner::PrecacheTemplate( KeyValues *kvTemplate )
{
	int iModel = PrecacheModel( kvTemplate->GetString("Model", "") );
	PrecacheGibsForModel(iModel);
	if( kvTemplate->GetString("Weapons") )
	{
		CCommand args;
		args.Tokenize(kvTemplate->GetString("Weapons"));
		for( int i = 0; i < args.ArgC(); i++ )
		{
			UTIL_PrecacheSchemaWeapon( args[i] );
		}
	}

	if( kvTemplate->GetString("Class", NULL) )
	{
		int iIndex = GetClassIndexFromString( kvTemplate->GetString("Class", NULL), TF_CLASS_COUNT );
		if( iIndex == TF_CLASS_UNDEFINED || iIndex == TF_CLASS_RANDOM )
			return;

		if( !TFGameRules() )
			return;

		TFGameRules()->PrecacheClass( iIndex );
	}
}

void CTFBotSpawner::LoadAndPrecacheTemplates( void )
{
	// If we have a wave preset, load that
	if( STRING(m_iszWavePreset.Get())[0] != '\0' )
	{
		inWavePreset = GetMapData()->GetWavePreset(STRING(m_iszWavePreset.Get()));
		if( !inWavePreset )
		{
			Warning("Preset %s not found, aborting\n", STRING(m_iszWavePreset.Get()));
			return;
		}

		m_iWaveSize = 0;
		m_iTemplate = 0;

		int iCurrentTemplate = 0;
		FOR_EACH_VALUE( inWavePreset, kvValue )
		{
			inBotTemplate[iCurrentTemplate] = GetMapData()->GetBotTemlpate( kvValue->GetName() );

			if( !inBotTemplate[iCurrentTemplate] )
			{
				Warning("Bot template %s not found\n", kvValue->GetName());
				continue;
			}

			PrecacheTemplate( inBotTemplate[iCurrentTemplate] );
			iCurrentTemplate++;
			m_iWaveSize += kvValue->GetInt();
		}

		return;
	}

	// Otherwise, load it through templates
	int iCurrentTemplate = 0;
	for( int i = 0; i < MAX_BOT_TEMPLATES; i++ )
	{
		if( STRING(m_iszBotTemplate[i])[0] == '\0' )
		{
			Warning("BotTemplate%d not specified, skipping\n", i+1);
			continue;
		}

		inBotTemplate[iCurrentTemplate] = GetMapData()->GetBotTemlpate( STRING(m_iszBotTemplate[i]) );

		if( inBotTemplate[iCurrentTemplate] ) // unless its the first template, copy the last template if we dont find the targeted one
		{
			PrecacheTemplate(inBotTemplate[iCurrentTemplate]);
			iCurrentTemplate++;
		}
	}
	m_iMaxTemplates = iCurrentTemplate;
}

void CTFBotSpawner::Spawn( void )
{
	LoadAndPrecacheTemplates();

	// if we dont manage to find a single template, ABORT ABORT ABORT
	if( !inBotTemplate[0] )
	{
		Warning("Bot generator failed to find template, aborting\n");
		UTIL_Remove( this );
		return;
	}

	if( m_flWaveIntervals > -1 )
		m_flBotSpawnTick = gpGlobals->curtime + m_flWaveIntervals;
	else
		m_flBotSpawnTick = gpGlobals->curtime;
	
	m_bBotsSpawned = false;
	
	pSpawnPoint = gEntList.FindEntityByName( NULL, STRING(m_iszSpawnPoint) );
	if( !pSpawnPoint || !STRING(m_iszSpawnPoint) )
		pSpawnPoint = this;
	
	pTarget = gEntList.FindEntityByName( NULL, STRING(m_iszTarget) );
	if( !pTarget || STRING(m_iszTarget)[0] == '\0' )
		pTarget = NULL;
}

void CTFBotSpawner::GeneratorThink()
{
	if( m_bDisabled )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;	
	}
	
	bool bShouldSpawn = !m_bBotsSpawned && 
						(m_iBots.Count() <= 0 || !m_bSpawnWhenCleared);
	
	if( bShouldSpawn && m_flBotSpawnTick < gpGlobals->curtime )
	{
		inputdata_t iTemp;
		SpawnWave( iTemp );

		if( m_flWaveIntervals > -1 )
			m_flBotSpawnTick = gpGlobals->curtime + m_flWaveIntervals;
		else
			m_bBotsSpawned = true;
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CTFBotSpawner::FireGameEvent( IGameEvent *event )
{
	if( FStrEq(event->GetName(), "player_death" ) )
	{
		int iPlayer = event->GetInt("userid");
		int iBot = m_iBots.Find(iPlayer);
		if( iBot < 0 )
			return;

		CPlayer *pBot = dynamic_cast<CPlayer*>(UTIL_PlayerByUserId( m_iBots[iBot] ));
		if( !pBot )
			return;

		QAngle angLaunch = QAngle( 0, 0 ,0 ); // the angle we will launch ourselves from
		Vector vecLaunch;	// final velocity used to launch the items
				
		KeyValues *kvItems = m_hBotTemplate[iBot]->FindKey("DropItems");
		if( kvItems )
		{
			FOR_EACH_VALUE( kvItems, kvSubKey )
			{
				CTFPowerup *pPowerup = dynamic_cast< CTFPowerup* >( CBaseEntity::CreateNoSpawn( kvSubKey->GetName(), pBot->WorldSpaceCenter(), angLaunch, pBot ) );

				if ( pPowerup )
				{
					CCommand args;
					args.Tokenize(kvSubKey->GetString());
					for( int i = 0; i < args.ArgC(); i += 2 )
					{
						g_ServerTools.SetKeyValue( pPowerup, args[i], args[i+1] );
					}
					// We trigger activate again because we need the team to properly change
					// Yes this causes asserts but its better than checking if the string is TeamNum for every argument
					pPowerup->Activate();
					pPowerup->Spawn();
					angLaunch[ PITCH ] += RandomInt( 5 , 15 );
					angLaunch[ YAW ] = RandomInt( 0 , 360 );
			
					AngleVectors( angLaunch, &vecLaunch );
			
					// boost it up baby
					vecLaunch *= RandomInt( 200, 500 );
					pPowerup->DropSingleInstance( vecLaunch, pBot, 30.0f, 0.2f );
				}
						
				QAngle angLaunch2 = QAngle( 0, 0 ,0 ); // the angle we will launch ourselves from
				Vector vecLaunch2;					  // final velocity used to launch the items
			}
		}

		CBaseEntity *pKiller = UTIL_PlayerByUserId( event->GetInt("attacker") );
			
		m_OnBotKilled.FireOutput( pKiller ? pKiller : this, this );
		m_iBots.Remove(iBot);
		m_hBotTemplate.Remove(iBot);

		if( m_iBots.Count() <= 0 )
			m_OnAllBotsKilled.FireOutput( this, this );
	}
}

void CTFBotSpawner::SpawnWave( inputdata_t &inputdata )
{
	m_OnBotsSpawned.FireOutput( this, this );
	m_iSpawnedBots = 0;

	if( inWavePreset )
	{
		int y = 0;
		FOR_EACH_VALUE( inWavePreset, kvValue )
		{
			for( int i = 0; i < kvValue->GetInt(); i++ )
			{
				SpawnBot( y );
			}
			y++;
		}
	}
	else
	{
		for ( int i = 0; i < m_iWaveSize; ++i )
		{
			SpawnBot(m_iTemplate);
		}
	}
}

void CTFBotSpawner::SpawnBot( int iTemplate )
{
	if( !inBotTemplate[iTemplate] )
	{
		Warning("TEMPLATE %d NOT FOUND\n", iTemplate);
		return;
	}
	char szBotName[32];
	Q_strncpy( szBotName, inBotTemplate[iTemplate]->GetString( "Name", GetRandomPlayerName()), sizeof(szBotName) );

	CPlayer *bot = CreateBot( szBotName, NULL, NULL );
	if( bot == nullptr )
		return;

	// We set this twice so the cosmetics get set properly
	CBot* pBot = static_cast<CBot*>(bot->GetBotController());
	pBot->SetRemoveOnDeath( true );

	char szTeam[10];
	Q_strncpy( szTeam, inBotTemplate[iTemplate]->GetString("Team", "Mercenary"), sizeof(szTeam) );

	int iTeam = TF_TEAM_MERCENARY;

	for ( int i = TEAM_SPECTATOR; i < TF_TEAM_COUNT; ++i )
	{
		if ( stricmp( szTeam, g_aTeamNames[i] ) == 0 )
		{
			iTeam = i;
			break;
		}
	}
	bot->ChangeTeam(iTeam);

	// pick a random color!
	Vector m_vecPlayerColor = vec3_origin;

	if ( TFGameRules() && TFGameRules()->IsDMGamemode() )
	{
		float flColors[ 3 ];

		for ( int i = 0; i < ARRAYSIZE( flColors ); i++ )
			flColors[ i ] = RandomFloat( 1, 255 );

		m_vecPlayerColor.Init( flColors[0], flColors[1], flColors[2] );

		m_vecPlayerColor /= 255.0f;

		bot->m_vecPlayerColor = m_vecPlayerColor;
	}

	char szClassName[32];
	Q_strncpy( szClassName, inBotTemplate[iTemplate]->GetString("Class", "random"), sizeof(szClassName) );

	// Join class, so init everything class related before this
	bot->SetClass( szClassName );
	
	if( inBotTemplate[iTemplate]->GetInt("Health", -1) > -1 )
		bot->GetPlayerClass()->SetCustomHealth( inBotTemplate[iTemplate]->GetInt("Health") );
	
	if( inBotTemplate[iTemplate]->GetFloat("Speed", -1) > -1 )
		bot->GetPlayerClass()->SetCustomSpeed( inBotTemplate[iTemplate]->GetFloat("Speed") );
	
	KeyValues *inResponseRules = inBotTemplate[iTemplate]->FindKey("ResponseCriteria");
	if( inResponseRules )
	{
		FOR_EACH_SUBKEY( inResponseRules, kvSubKey )
		{
			bot->AddResponseCriteria( kvSubKey->GetName(), kvSubKey->GetString() );
		}
	}

	char szCosmetics[128];
	Q_strncpy( szCosmetics, inBotTemplate[iTemplate]->GetString("cosmetics", ""), sizeof(szCosmetics) );
	if( szCosmetics )
		bot->ForceEquipCosmetics( szCosmetics, bot->GetDesiredPlayerClassIndex() );
	
	// PRE-SPAWN
	// SPAWN THE BOT
	bot->ForceRespawn();
	// The remove on death attribute gets removed here :(
	// POST-SPAWN

	// Post-Respawn modifiers
	char szModelName[128];
	Q_strncpy( szModelName, inBotTemplate[iTemplate]->GetString("Model", ""), sizeof(szModelName) );	
	
	if( szModelName[0] != '\0' )
	{
		bot->InvalidateMdlCache();
		int iModel = PrecacheModel( szModelName );
		PrecacheGibsForModel( iModel );	
		
		bot->GetPlayerClass()->SetCustomModel(szModelName);
		bot->UpdateModel();
	}
	
	bot->SetModelScale( inBotTemplate[iTemplate]->GetFloat("ModelScale", 1.0) );
	
	if( inBotTemplate[iTemplate]->GetString("Weapons", NULL) )
	{
		bot->StripWeapons();
		CCommand args;
		args.Tokenize(inBotTemplate[iTemplate]->GetString("Weapons"));
		for( int i = 0; i < args.ArgC(); i++ )
		{
			CTFWeaponBase *pGivenWeapon = static_cast<CTFWeaponBase *>(bot->GiveNamedItem( args[i] ));
			if( pGivenWeapon )
			{
				pGivenWeapon->GiveTo(bot);
				bot->Weapon_Switch( pGivenWeapon );
			}
		}
	}

	pBot->SetRemoveOnDeath( true );
	
	QAngle angSpawnAngle( 0, 360.0f * ( (float)m_iSpawnedBots / (float)m_iWaveSize ), 0 );
	Vector vDir;
	AngleVectors( angSpawnAngle, &vDir, NULL, NULL );
	bot->SetAbsOrigin( pSpawnPoint->GetAbsOrigin() + ( vDir * m_flSpawnRadius ) );

	// TODO: Reimplement
    if( pBot->GetFollow() )
	{
		if( pTarget )
			pBot->GetFollow()->Start( pTarget, true );
		else
			pBot->GetFollow()->Stop();
    }
	
	m_iBots.AddToTail(bot->GetUserID());
	m_hBotTemplate.AddToTail(inBotTemplate[iTemplate]);
	
	m_iSpawnedBots++;
}

void CTFBotSpawner::ResetInterval( inputdata_t &inputdata )
{
	if( m_flWaveIntervals > -1 )
		m_flBotSpawnTick = gpGlobals->curtime + m_flWaveIntervals;
	else
	{
		m_flBotSpawnTick = gpGlobals->curtime;
		m_bBotsSpawned = true;
	}
}

void CTFBotSpawner::SetInterval( inputdata_t &inputdata )
{
	m_flWaveIntervals = inputdata.value.Float();
	ResetInterval(inputdata);
}

void CTFBotSpawner::SetTemplate( inputdata_t &inputdata )
{
	m_iTemplate = min( inputdata.value.Int(), m_iMaxTemplates );
}

void CTFBotSpawner::SetTarget( inputdata_t &inputdata )
{
	char szTemp[16];
	Q_strncpy( szTemp, inputdata.value.String(), sizeof(szTemp) );
	strlwr(szTemp);
	if( FStrEq( szTemp, "!activator" ) )
	{
		pTarget = inputdata.pActivator;
		m_iszTarget = pTarget->GetEntityName();
	}
	else if( FStrEq( szTemp, "!caller" ) )
	{
		pTarget = inputdata.pCaller;
		m_iszTarget = pTarget->GetEntityName();
	}
	else
	{
		m_iszTarget = MAKE_STRING(inputdata.value.String());
		pTarget = gEntList.FindEntityByName( NULL, STRING(m_iszTarget) );
		if( !pTarget || inputdata.value.String()[0] == '\0' )
			pTarget = NULL;
	}
	
	DevMsg("Settgin all bot targets to %s\n", inputdata.value.String());
	
	if( pTarget )
		DevMsg("Target class %s\n", STRING( pTarget->GetEntityName() ) );
	
	for( int i = 0; i < m_iBots.Count(); i++ )
	{
		CPlayer *pPlayer = dynamic_cast<CPlayer*>(UTIL_PlayerByUserId( m_iBots[i] ));
		if( pPlayer )
		{
			// TODO: Reenable
			if( pPlayer->GetBotController()->GetFollow() )
			{
				pPlayer->GetBotController()->GetFollow()->Start( pTarget, true );
			}
		}
		else
		{
			m_iBots.Remove(i);
			m_hBotTemplate.Remove(i);
			i--;
		}
	}
}

void CTFBotSpawner::KillAllBots( inputdata_t &inputdata )
{
	CUtlVector<CPlayer*> m_hBotsToKill;
	for( int i = 0; i < m_iBots.Count(); i++ )
	{
		CPlayer *pPlayer = dynamic_cast<CPlayer*>(UTIL_PlayerByUserId( m_iBots[i] ));
		if( pPlayer )
		{
			m_hBotsToKill.AddToTail(pPlayer);
		}
	}

	for( int i = 0; i < m_hBotsToKill.Count(); i++ )
	{
		m_hBotsToKill[i]->CommitSuicide( false, true );
	}
}


void CTFBotSpawner::Enable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	ResetInterval(inputdata);
}

void CTFBotSpawner::Disable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}
//pentLandmark = gEntList.FindEntityByName(pentLandmark, m_iLandmark, NULL, pOther, pOther );	
