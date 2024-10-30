//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "CreateMultiplayerGameDialog.h"
#include "CreateMultiplayerGameServerPage.h"
#include "CreateMultiplayerGameGameplayPage.h"
#include "CreateMultiplayerGameMovementPage.h"
#include "CreateMultiplayerGameWeaponsPage.h"
#include "CreateMultiplayerGameMiscPage.h"

#include "EngineInterface.h"
#include "ModInfo.h"
#include "GameUI_Interface.h"

#include <stdio.h>

using namespace vgui;

#include "vgui/ILocalize.h"
#include "filesystem.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar ui_scaling;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui::Panel *parent) : PropertyDialog(parent, "CreateMultiplayerGameDialog")
{
	SetProportional( ui_scaling.GetBool() );
	SetDeleteSelfOnClose( true );
	SetSize(420, 516);
	SetSizeable( false );
	
	SetTitle("#GameUI_CreateServer", true);
	SetOKButtonText("#GameUI_Start");

	m_pServerPage = new CCreateMultiplayerGameServerPage(this, "ServerPage");
	m_pGameplayPage = new CCreateMultiplayerGameGameplayPage(this, "GameplayPage");
	m_pMovementPage = new CCreateMultiplayerGameMovementPage(this, "MovementPage");
	m_pWeaponsPage = new CCreateMultiplayerGameWeaponsPage(this, "WeaponsPage");
	m_pMiscPage = new CCreateMultiplayerGameMiscPage(this, "MiscPage");


	AddPage(m_pServerPage, "#GameUI_Server");
	AddPage(m_pGameplayPage, "#GameUI_Game");
	AddPage(m_pMovementPage, "#GameUI_Movement");
	AddPage(m_pWeaponsPage, "#GameUI_Weapons");
	AddPage(m_pMiscPage, "#GameUI_Misc");

	// create KeyValues object to load/save config options

	// TODO fix up
#if defined( WIN32 ) && !defined( _X360 )
	m_pSavedData = new KeyValues( "ServerConfig" );

	// load the config data
	if (m_pSavedData)
	{
		m_pSavedData->LoadFromFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" ); // this is game-specific data, so it should live in GAME, not CONFIG

		const char *startMap = m_pSavedData->GetString("map", "");
		if (startMap[0])
		{
			m_pServerPage->SetMap(startMap);
		}
	}
#else
	m_pSavedData = NULL;
#endif
	m_pServerPage->EnableBots( m_pSavedData );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameDialog::~CCreateMultiplayerGameDialog()
{
	if (m_pSavedData)
	{
		m_pSavedData->deleteThis();
		m_pSavedData = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: runs the server when the OK button is pressed
//-----------------------------------------------------------------------------
bool CCreateMultiplayerGameDialog::OnOK(bool applyOnly)
{
	// reset server enforced cvars
	g_pCVar->RevertFlaggedConVars( FCVAR_REPLICATED );	

	// Cheats were disabled; revert all cheat cvars to their default values.
	// This must be done heading into multiplayer games because people can play
	// demos etc and set cheat cvars with sv_cheats 0.
	g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );

	DevMsg( "FCVAR_CHEAT cvars reverted to defaults.\n" );

	BaseClass::OnOK(applyOnly);

	// get these values from m_pServerPage and store them temporarily
	char szMapName[64], szHostName[64], szPassword[64];
	Q_strncpy(szMapName, m_pServerPage->GetMapName(), sizeof( szMapName ));
	Q_strncpy(szHostName, m_pGameplayPage->GetHostName(), sizeof( szHostName ));
	Q_strncpy(szPassword, m_pGameplayPage->GetPassword(), sizeof( szPassword ));

#if defined( WIN32 ) && !defined( _X360 )
	// save the config data
	if (m_pSavedData)
	{
		if (m_pServerPage->IsRandomMapSelected())
		{
			// it's set to random map, just save an
			m_pSavedData->SetString("map", "");
		}
		else
		{
			m_pSavedData->SetString("map", szMapName);
		}

		// save config to a file
		m_pSavedData->SaveToFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" );
	}
#endif

	char szMapCommand[1024];

	// create the command to execute
	Q_snprintf(szMapCommand, sizeof( szMapCommand ), "disconnect\nwait\nwait\nsv_lan 1\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nprogress_enable\nmap %s\n",
		m_pGameplayPage->GetMaxPlayers(),
		szPassword,
		szHostName,
		szMapName
	);

	// exec
	engine->ClientCmd_Unrestricted(szMapCommand);

	return true;
}
