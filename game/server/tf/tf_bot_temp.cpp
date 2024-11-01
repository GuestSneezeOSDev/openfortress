//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "of_loadout.h"
#include "of_items_game.h"
#include "datacache/imdlcache.h"

void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CTFPlayer *pBot );

ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "1", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_defend( "bot_defend", "0", 0, "Set to a team number, and that team will all keep their combat shields raised." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
ConVar bot_dontmove( "bot_dontmove", "0", 0, "Force bots to not move." );
ConVar bot_saveme( "bot_saveme", "0", FCVAR_CHEAT );
static ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );
ConVar bot_selectweaponslot( "bot_selectweaponslot", "-1", FCVAR_CHEAT, "Set to weapon slot that bot should switch to." );
ConVar bot_randomnames( "bot_randomnames", "0", FCVAR_CHEAT );
ConVar bot_jump( "bot_jump", "0", FCVAR_CHEAT, "Force all bots to repeatedly jump." );

extern ConVar of_allow_special_classes;

static int BotNumber = 1;
static int g_iNextBotTeam = -1;
static int g_iNextBotClass = -1;

typedef struct
{
	bool			backwards;

	float			nextturntime;
	bool			lastturntoright;

	float			nextstrafetime;
	float			sidemove;

	QAngle			forwardAngle;
	QAngle			lastAngles;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	bool m_bWasDead;
	float m_flDeadTime;

	bool m_bCosmeticsAreSet;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];


//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam, int iClass, const char *pszCustomName, const Vector &vecColor = vec3_origin )
{
	g_iNextBotTeam = iTeam;
	g_iNextBotClass = iClass;

	char botname[ 64 ];
	if ( pszCustomName && pszCustomName[0] )
	{
		Q_strncpy( botname, pszCustomName, sizeof( botname ) );
	}
	else if ( bot_randomnames.GetBool() )
	{
		static const char *szBotNames[] =
		{
			"Bot",
			"This is a medium Bot",
			"This is a super long bot name that is too long for the game to allow",
			"Another bot",
			"Yet more Bot names, medium sized",
			"B",
		};

		Q_strncpy( botname, szBotNames[RandomInt( 0, ARRAYSIZE( szBotNames ) - 1)], sizeof( botname ) );
	}
	else
	{
		Q_snprintf( botname, sizeof( botname ), "Bot%02i", BotNumber );
	}

	edict_t *pEdict = engine->CreateFakeClient( botname );
	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a CBasePlayer for the bot, and call spawn
	//ClientPutInServer( pEdict, botname );
	CTFPlayer *pPlayer = ((CTFPlayer *)CBaseEntity::Instance( pEdict ));
	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
	
	pPlayer->m_bPuppet = true;

	// random color
	pPlayer->m_vecPlayerColor = vecColor;

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	BotNumber++;

	botdata_t *pBot = &g_BotData[ pPlayer->entindex() - 1 ];
	pBot->m_bWasDead = false;
	pBot->m_WantedTeam = iTeam;
	pBot->m_WantedClass = iClass;
	pBot->m_bCosmeticsAreSet = false;
	pBot->m_flJoinTeamTime = gpGlobals->curtime + 0.3;

	LoadoutManager()->RegisterPlayer(pPlayer);

	return pPlayer;
}


// Handler for the "bot" command.
CON_COMMAND_F( bot, "Add a bot.", FCVAR_CHEAT )
{
	//CDODPlayer *pPlayer = CDODPlayer::Instance( UTIL_GetCommandClientIndex() );

	// The bot command uses switches like command-line switches.
	// -count <count> tells how many bots to spawn.
	// -team <index> selects the bot's team. Default is -1 which chooses randomly.
	//	Note: if you do -team !, then it 
	// -class <index> selects the bot's class. Default is -1 which chooses randomly.
	// -frozen prevents the bots from running around when they spawn in.
	// -name sets the bot's name

	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 128 );

	if (args.FindArg( "-all" ))
		count = TF_CLASS_COUNT_ALL - 1;

	// Look at -frozen.
	bool bFrozen = !(!!args.FindArg( "-frozen" ));
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		// What team do they want?
		int iTeam = TEAM_SPECTATOR;

		char const* pVal = args.FindArg("-team");
		if (pVal)
		{
			if (stricmp(pVal, "red") == 0)
				iTeam = TF_TEAM_RED;
			else if (stricmp(pVal, "blue") == 0)
				iTeam = TF_TEAM_BLUE;
			else if (stricmp(pVal, "spectator") == 0)
				iTeam = TEAM_SPECTATOR;
			else if (stricmp(pVal, "mercenary") == 0)
				iTeam = TF_TEAM_MERCENARY;
			else
				iTeam = RandomInt(1, 1000);
			if (iTeam <= 500)
				iTeam = TF_TEAM_RED;
			else if (iTeam > 500)
				iTeam = TF_TEAM_BLUE;
		}
		else
		{
			iTeam = RandomInt(1, 1000);
			if (iTeam <= 500)
				iTeam = TF_TEAM_RED;
			else if (iTeam > 500)
				iTeam = TF_TEAM_BLUE;
		}

		int iClass = TF_CLASS_UNDEFINED;

		// If the -all arg is specified, we just spawn a bot for every class
		if (args.FindArg("-all"))
		{
			iClass = TF_CLASS_COUNT_ALL - 1 - count;
		}
		else
		{
			pVal = args.FindArg("-class");

			// If a class was specified in the command, use it and ignore class limits
			if (pVal)
			{
				for (int i = 1; i < TF_CLASS_COUNT_ALL; i++)
				{
					if (stricmp(GetPlayerClassData(i, 0)->m_szClassName, pVal) == 0)
					{
						iClass = i;
						break;
					}
				}
			}
		}

		// If a valid class was not specified, choose one randomly from the allowed set of classes
		if (iClass == TF_CLASS_UNDEFINED)
		{
			iClass = TFGameRules()->GetForcedClassIndex(iTeam);

			// If we're not forcing a class, select one randomly
			if (iClass == -1)
			{
				int iRandomIterationCount = 0;

				// We do this check so that the bots dont spawn as classes they shouldn't
				do {
					// Don't let them be the same class twice in a row
					iClass = random->RandomInt(TF_FIRST_NORMAL_CLASS, TF_CLASS_COUNT_ALL - 1);

					// Don't spend too much time on randomising
					// If it takes too long, just go through all classes and select the first
					if (iRandomIterationCount > 12)
					{
						for (int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++)
						{
							if (TFGameRules()->IsClassAllowed(iTeam, iClass))
							{
								iClass = i;
								break;
							}
						}

						// If the only allowed class is random (meaning none are allowed), just randomly pick a normal class
						if (iClass == TF_CLASS_RANDOM) {
							iClass = random->RandomInt(TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS);
						}

						break;
					}
				} while (!TFGameRules()->IsClassAllowed(iTeam, iClass)); // Conform to configured class limits
			}
		}

		char const *pName = args.FindArg( "-name" );

		// pick a random color!
		Vector m_vecPlayerColor = vec3_origin;

		if ( TFGameRules() && TFGameRules()->IsDMGamemode() )
		{
			float flColors[3];

			// allow a color to be picked manually
			pVal = args.FindArg( "-color" );

			if ( pVal )
			{
				UTIL_StringToVector( flColors, pVal );
			}
			else
			{
				for ( int i = 0; i < ARRAYSIZE( flColors ); i++ )
					flColors[i] = RandomFloat( 1, 255 );
			}

			m_vecPlayerColor.Init( flColors[0], flColors[1], flColors[2] );

			m_vecPlayerColor /= 255.0f;
		}

		// BotPutInServer( bFrozen, iTeam, iClass, pName );
		CTFPlayer *pPlayer = static_cast<CTFPlayer*>( BotPutInServer( bFrozen, iTeam, iClass, pName, m_vecPlayerColor ) );

		if( pPlayer )
		{
			pVal = args.FindArg( "-cosmetics" );
			
			botdata_t *pBot = &g_BotData[ pPlayer->entindex() - 1 ];

			if( pVal )
			{
				pPlayer->ForceEquipCosmetics( pVal, iClass );
				pBot->m_bCosmeticsAreSet = true;
			}	
		}
	}
}

// Handler for the bot_kick command
CON_COMMAND_F( bot_kick, "Kick the specified bot(s).", FCVAR_CHEAT )
{
	CBasePlayer *pPlayer = NULL;

	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			// Dont kick insource bots
			if( pPlayer->IsBotOfType( BOT_TYPE_INSOURCE ) )
				continue;

			if( pPlayer->IsBotOfType( BOT_TYPE_LOCKDOWN ) )
				continue;

			if ( pPlayer->GetFlags() & FL_FAKECLIENT )
			{				
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pTFPlayer->GetUserID() ) );
				pTFPlayer->m_flLastAction = gpGlobals->curtime;
					
			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}

		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		if ( pTFPlayer && ( pTFPlayer->GetFlags() & FL_FAKECLIENT ) )
		{
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pTFPlayer->GetUserID() ) );
			pTFPlayer->m_flLastAction = gpGlobals->curtime;
		}
	}
}

// Handler for the bot_taunt command
CON_COMMAND_F( bot_taunt, "Force specified bot(s) to taunt", FCVAR_CHEAT )
{
	CBasePlayer *pPlayer = NULL;

	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
			{				
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

				if ( pTFPlayer )
					pTFPlayer->Taunt( 0 );
					
			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}

		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		if ( pTFPlayer && ( pTFPlayer->GetFlags() & FL_FAKECLIENT ) )
			pTFPlayer->Taunt( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		
		if ( !pPlayer || !pPlayer->IsConnected() )
			continue;

		if ( !pPlayer->m_bPuppet || pPlayer->MyNextBotPointer() != NULL )
			continue;

		Bot_Think( pPlayer );
	}
}

bool RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			sidemove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CTFPlayer *fakeclient, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, float frametime )
{
	if ( !fakeclient )
		return;

	CUserCmd cmd;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	Q_memset( &cmd, 0, sizeof( cmd ) );

	if ( !RunMimicCommand( cmd ) )
	{
		VectorCopy( viewangles, cmd.viewangles );
		cmd.forwardmove = forwardmove;
		cmd.sidemove = sidemove;
		cmd.upmove = upmove;
		cmd.buttons = buttons;
		cmd.impulse = impulse;
		cmd.random_seed = random->RandomInt( 0, 0x7fffffff );
	}

	if ( bot_dontmove.GetBool() )
	{
		cmd.forwardmove = 0;
		cmd.sidemove = 0;
		cmd.upmove = 0;
	}

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CTFPlayer *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	QAngle vecViewAngles;
	float forwardmove = 0.0;
	float sidemove = botdata->sidemove;
	float upmove = 0.0;
	unsigned short buttons = 0;
	byte  impulse = 0;
	float frametime = gpGlobals->frametime;

	vecViewAngles = pBot->EyeAngles();

	MDLCACHE_CRITICAL_SECTION();

	// Create some random values
	if ( pBot->GetTeamNumber() == TEAM_UNASSIGNED && gpGlobals->curtime > botdata->m_flJoinTeamTime )
	{
		const char *pszTeam = NULL;
		switch ( botdata->m_WantedTeam )
		{
		case TF_TEAM_RED:
			pszTeam = "red";
			break;
		case TF_TEAM_BLUE:
			pszTeam = "blue";
			break;
		case TF_TEAM_MERCENARY:
			pszTeam = "mercenary";
			break;
		case TEAM_SPECTATOR:
			pszTeam = "spectator";
			break;
		default:
			Assert( false );
			break;
		}
		pBot->HandleCommand_JoinTeam( pszTeam );
	}
	else if ( pBot->GetTeamNumber() != TEAM_UNASSIGNED && pBot->GetPlayerClass()->IsClass( TF_CLASS_UNDEFINED ) )
	{
		// If they're on a team but haven't picked a class, choose a random class..
		pBot->HandleCommand_JoinClass( GetPlayerClassData( botdata->m_WantedClass, 0 )->m_szClassName );
	}
	else if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
	{
		trace_t trace;

		botdata->m_bWasDead = false;

		if ( bot_saveme.GetInt() > 0 )
		{
			pBot->SaveMe();
			bot_saveme.SetValue( bot_saveme.GetInt() - 1 );
		}

		// Stop when shot
		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) )
		{
			if ( !bot_dontmove.GetBool() )
			{
				forwardmove = 600 * ( botdata->backwards ? -1 : 1 );
				if ( botdata->sidemove != 0.0f )
				{
					forwardmove *= random->RandomFloat( 0.1, 1.0f );
				}
			}
			else
			{
				forwardmove = 0;
			}

			if ( bot_jump.GetBool() && pBot->GetFlags() & FL_ONGROUND )
			{
				buttons |= IN_JUMP;
			}
		}

		// Only turn if I haven't been hurt
		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) && !bot_dontmove.GetBool() )
		{
			Vector vecEnd;
			Vector forward;

			QAngle angle;
			float angledelta = 15.0;

			int maxtries = (int)360.0/angledelta;

			if ( botdata->lastturntoright )
			{
				angledelta = -angledelta;
			}

			angle = pBot->GetLocalAngles();

			Vector vecSrc;
			while ( --maxtries >= 0 )
			{
				AngleVectors( angle, &forward );

				vecSrc = pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

				vecEnd = vecSrc + forward * 10;

				UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, 
					MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &trace );

				if ( trace.fraction == 1.0 )
				{
					if ( gpGlobals->curtime < botdata->nextturntime )
					{
						break;
					}
				}

				angle.y += angledelta;

				if ( angle.y > 180 )
					angle.y -= 360;
				else if ( angle.y < -180 )
					angle.y += 360;

				botdata->nextturntime = gpGlobals->curtime + 2.0;
				botdata->lastturntoright = random->RandomInt( 0, 1 ) == 0 ? true : false;

				botdata->forwardAngle = angle;
				botdata->lastAngles = angle;

			}


			if ( gpGlobals->curtime >= botdata->nextstrafetime )
			{
				botdata->nextstrafetime = gpGlobals->curtime + 1.0f;

				if ( random->RandomInt( 0, 5 ) == 0 )
				{
					botdata->sidemove = -600.0f + 1200.0f * random->RandomFloat( 0, 2 );
				}
				else
				{
					botdata->sidemove = 0;
				}
				sidemove = botdata->sidemove;

				if ( random->RandomInt( 0, 20 ) == 0 )
				{
					botdata->backwards = true;
				}
				else
				{
					botdata->backwards = false;
				}
			}

			pBot->SetLocalAngles( angle );
			vecViewAngles = angle;
		}

		if ( bot_selectweaponslot.GetInt() >= 0 )
		{
			int slot = bot_selectweaponslot.GetInt();

			CBaseCombatWeapon *pWpn = pBot->Weapon_GetSlot( slot );

			if ( pWpn )
			{
				pBot->Weapon_Switch( pWpn );
			}

			bot_selectweaponslot.SetValue( -1 );
		}

		// Is my team being forced to defend?
		if ( bot_defend.GetInt() == pBot->GetTeamNumber() )
		{
			buttons |= IN_ATTACK2;
		}
		// If bots are being forced to fire a weapon, see if I have it
		else if ( bot_forcefireweapon.GetString() )
		{
			// Manually look through weapons to ignore subtype			
			CBaseCombatWeapon *pWeapon = NULL;
			const char *pszWeapon = bot_forcefireweapon.GetString();
			for (int i=0;i<MAX_WEAPONS;i++) 
			{
				if ( pBot->GetWeapon(i) && FClassnameIs( pBot->GetWeapon(i), pszWeapon ) )
				{
					pWeapon = pBot->GetWeapon(i);
					break;
				}
			}

			if ( pWeapon )
			{
				// Switch to it if we don't have it out
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

				// Switch?
				if ( pActiveWeapon != pWeapon )
				{
					pBot->Weapon_Switch( pWeapon );
				}
				else
				{
					// Start firing
					// Some weapons require releases, so randomise firing
					if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
					{
						buttons |= IN_ATTACK;
					}

					if ( bot_forceattack2.GetBool() )
					{
						buttons |= IN_ATTACK2;
					}
				}
			}
		}

		if ( bot_flipout.GetInt() )
		{
			if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
			{
				buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
			}
		}
	}
	else
	{
		// Wait for Reinforcement wave
		if ( !pBot->IsAlive() )
		{
			if ( botdata->m_bWasDead )
			{
				// Wait for a few seconds before respawning.
				if ( gpGlobals->curtime - botdata->m_flDeadTime > 3 )
				{
					// Respawn the bot
					buttons |= IN_JUMP;
				}
			}
			else
			{
				// Start a timer to respawn them in a few seconds.
				botdata->m_bWasDead = true;
				botdata->m_flDeadTime = gpGlobals->curtime;
			}
		}
	}

	if( pBot->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED && !botdata->m_bCosmeticsAreSet )
	{
		pBot->EquipRandomCosmetics( pBot->GetPlayerClass()->GetClassIndex() );
		botdata->m_bCosmeticsAreSet = true;
	}

	if ( bot_flipout.GetInt() == 2 )
	{

		QAngle angOffset = RandomAngle( -1, 1 );

		botdata->lastAngles += angOffset;

		for ( int i = 0 ; i < 2; i++ )
		{
			if ( fabs( botdata->lastAngles[ i ] - botdata->forwardAngle[ i ] ) > 15.0f )
			{
				if ( botdata->lastAngles[ i ] > botdata->forwardAngle[ i ] )
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] + 15;
				}
				else
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] - 15;
				}
			}
		}

		botdata->lastAngles[ 2 ] = 0;

		pBot->SetLocalAngles( botdata->lastAngles );
		vecViewAngles = botdata->lastAngles;
	}
	else if ( bot_flipout.GetInt() == 3 )
	{
		botdata->lastAngles.x = sin( gpGlobals->curtime + pBot->entindex() ) * 90;
		botdata->lastAngles.y = AngleNormalize( ( gpGlobals->curtime * 1.7 + pBot->entindex() ) * 45 );
		botdata->lastAngles.z = 0.0;

		float speed = 300; // sin( gpGlobals->curtime / 1.7 + pBot->entindex() ) * 600;
		forwardmove = sin( gpGlobals->curtime + pBot->entindex() ) * speed;
		//sidemove = cos( gpGlobals->curtime * 2.3 + pBot->entindex() ) * speed;
		sidemove = cos( gpGlobals->curtime + pBot->entindex() ) * speed;

		/*
		if (sin(gpGlobals->curtime ) < -0.5)
		{
			buttons |= IN_DUCK;
		}
		else if (sin(gpGlobals->curtime ) < 0.5)
		{
			buttons |= IN_WALK;
		}
		*/

		pBot->SetLocalAngles( botdata->lastAngles );
		vecViewAngles = botdata->lastAngles;

		// no shooting
		buttons &= ~ (IN_ATTACK2 | IN_ATTACK);
	}

	// Fix up the m_fEffects flags
	// pBot->PostClientMessagesSent();

	RunPlayerMove( pBot, vecViewAngles, forwardmove, sidemove, upmove, buttons, impulse, frametime );
}

//------------------------------------------------------------------------------
// Purpose: sends the specified command from a bot
//------------------------------------------------------------------------------
void cc_bot_sendcommand( const CCommand &args )
{
	if ( args.ArgC() < 3 )
	{
		Msg( "Too few parameters.  Usage: bot_command <bot name> <command string...>\n" );
		return;
	}

	const char *commandline = args.GetCommandString();

	// find the rest of the command line past the bot index
	commandline = strstr( commandline, args[2] );
	Assert( commandline );

	int iSize = Q_strlen(commandline) + 1;
	char *pBuf = (char *)malloc(iSize);
	Q_snprintf( pBuf, iSize, "%s", commandline );

	if ( pBuf[iSize-2] == '"' )
	{
		pBuf[iSize-2] = '\0';
	}

	// make a command object with the intended command line
	CCommand command;
	command.Tokenize( pBuf );

	CBasePlayer *pPlayer = NULL;

	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
			{
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				// send the command
				TFGameRules()->ClientCommand( pPlayer, command );

			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}	

		if ( pPlayer->GetFlags() & FL_FAKECLIENT )
			TFGameRules()->ClientCommand( pPlayer, command );
	}
}
static ConCommand bot_sendcommand( "bot_command", cc_bot_sendcommand, "<bot id> <command string...>.  Sends specified command on behalf of specified bot", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: sends the specified command from a bot
//------------------------------------------------------------------------------
void cc_bot_kill( const CCommand &args )
{
	CBasePlayer *pPlayer = NULL;

	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
			{				
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				pPlayer->CommitSuicide();
					
			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}

		if ( pPlayer->GetFlags() & FL_FAKECLIENT )
			pPlayer->CommitSuicide();
	}
}

static ConCommand bot_kill( "bot_kill", cc_bot_kill, "<bot id>.  Kills bot.", FCVAR_CHEAT );

CON_COMMAND_F( bot_changeteams, "Make all bots change teams", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
		{
			int iTeam = pPlayer->GetTeamNumber();			
			if ( TF_TEAM_BLUE == iTeam || TF_TEAM_RED == iTeam )
			{
				// toggle team between red & blue
				pPlayer->ChangeTeam( TF_TEAM_BLUE + TF_TEAM_RED - iTeam, false );
			}	
			else if ( iTeam == TEAM_SPECTATOR || iTeam == TEAM_UNASSIGNED )
			{
				pPlayer->ChangeTeam( RandomInt( TF_TEAM_RED, TF_TEAM_BLUE ), false );
			}
		}
	}
}

CON_COMMAND_F( bot_refill, "Refill all bot ammo counts", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsFakeClient() )
		{
			pPlayer->GiveAmmo( 1000, TF_AMMO_PRIMARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_SECONDARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_METAL );
			pPlayer->TakeHealth( 999, DMG_GENERIC );
		}
	}
}

CON_COMMAND_F( bot_whack, "Deliver lethal damage from player to specified bot", FCVAR_CHEAT )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Too few parameters.  Usage: bot_whack <bot name>\n" );
		return;
	}
	
	CBasePlayer *pPlayer = NULL;

	CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_GetCommandClient() );


	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
			{				
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				CTakeDamageInfo info( pPlayer, pTFPlayer, 1000, DMG_BULLET );
				info.SetInflictor( pTFPlayer->GetActiveTFWeapon() );
				pPlayer->TakeDamage( info );	
					
			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}

		if ( pPlayer->GetFlags() & FL_FAKECLIENT )
		{ 
			CTakeDamageInfo info( pPlayer, pTFPlayer, 1000, DMG_BULLET );
			info.SetInflictor( pTFPlayer->GetActiveTFWeapon() );
			pPlayer->TakeDamage( info );	
		}	
	}

}

CON_COMMAND_F( bot_teleport, "Teleport the specified bot to the specified position & angles.\n\tFormat: bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>", FCVAR_CHEAT )
{
	if ( args.ArgC() < 8 )
	{
		Msg( "Too few parameters.  bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>\n" );
		return;
	}

	CBasePlayer *pPlayer = NULL;

	// get all bots if these parameters are specified or team specific ones
	if ( args.FindArg( "all" ) || args.FindArg( "red" ) || args.FindArg( "blue" ) || args.FindArg( "mercenary" ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
			{				
				if ( args.FindArg( "red" ) && pPlayer->GetTeamNumber() != TF_TEAM_RED )
					continue;

				if ( args.FindArg( "blue" ) && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
					continue;

				if ( args.FindArg( "mercenary" ) && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY )
					continue;

				Vector vecPos( atof(args[2]), atof(args[3]), atof(args[4]) );
				QAngle vecAng( atof(args[5]), atof(args[6]), atof(args[7]) );
				pPlayer->Teleport( &vecPos, &vecAng, NULL );
					
			}
		}
	}
	else
	{
		// get the bot's player object
		pPlayer = UTIL_PlayerByName( args[1] );

		if ( !pPlayer )
		{
			Msg( "No bot with name %s\n", args[1] );
			return;
		}

		if ( pPlayer->GetFlags() & FL_FAKECLIENT )
		{ 
			Vector vecPos( atof(args[2]), atof(args[3]), atof(args[4]) );
			QAngle vecAng( atof(args[5]), atof(args[6]), atof(args[7]) );
			pPlayer->Teleport( &vecPos, &vecAng, NULL );
		}
	}
}
