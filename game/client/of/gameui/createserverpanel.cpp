//=============================================================================
//
// Purpose: Fancy create server panel
//
//=============================================================================

#include "cbase.h"
#include "createserverpanel.h"

#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include "filesystem.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

#include "tier0/dbg.h"

using namespace vgui;
using namespace BaseModUI;

#define RANDOM_MAP "#GameUI_RandomMap"

CreateServerPanel::CreateServerPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pMapList = NULL;
	m_pCheckButton = NULL;
	m_pSavedData = new KeyValues("ServerConfig");

	// load the config data
	if (m_pSavedData)
	{
		m_pSavedData->LoadFromFile(g_pFullFileSystem, "ServerConfig.vdf", "GAME"); // this is game-specific data, so it should live in GAME, not CONFIG
	}
	//m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CreateServerPanel::~CreateServerPanel()
{
}

void CreateServerPanel::Activate()
{
	BaseClass::Activate();

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "CreateServerOpen");
}

void CreateServerPanel::OnClose()
{
	BaseClass::OnClose();
}

void CreateServerPanel::OnCommand(const char *command)
{
	if (!Q_strcmp(command, "Exit"))
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "CreateServerClose");
	}

	if (!Q_strcmp(command, "Start"))
	{
		StartServer();
	}

	if (!Q_strcmp(command, "OldMenu"))
	{
		engine->ClientCmd_Unrestricted("gamemenucommand OpenCreateMultiplayerGameDialog");
		Hide();
	}
}

void CreateServerPanel::OnHudAnimationEnd( KeyValues *pKV )
{
	const char *szName = pKV->GetString("name");
	if( FStrEq( szName, "CreateServerClose" ) )
		Hide();
}

void CreateServerPanel::StartServer()
{
	// reset server enforced cvars
	g_pCVar->RevertFlaggedConVars(FCVAR_REPLICATED);

	// Cheats were disabled; revert all cheat cvars to their default values.
	// This must be done heading into multiplayer games because people can play
	// demos etc and set cheat cvars with sv_cheats 0.
	g_pCVar->RevertFlaggedConVars(FCVAR_CHEAT);

	DevMsg("FCVAR_CHEAT cvars reverted to defaults.\n");

	// get these values from m_pServerPage and store them temporarily
	char szMapName[64], szHostName[64], szPassword[64];
	Q_strncpy(szMapName, GetMapName(), sizeof(szMapName));
	Q_strncpy(szHostName, GetHostName(), sizeof(szHostName));
	Q_strncpy(szPassword, GetPassword(), sizeof(szPassword));

	// save the config data
	if (m_pSavedData)
	{
		if (IsRandomMapSelected())
		{
			// it's set to random map, just save an
			m_pSavedData->SetString("map", "");
		}
		else
		{
			m_pSavedData->SetString("map", szMapName);
		}

		// save config to a file
		m_pSavedData->SaveToFile(g_pFullFileSystem, "ServerConfig.vdf", "GAME");
	}

	char szMapCommand[1024];



	// add bots
	int m_iBotAmount = GetBotAmount();
	char szBotAddCommand[64];

	Q_snprintf(szBotAddCommand, sizeof(szBotAddCommand), "tf_bot_quota %i\n", m_iBotAmount);

	// create the command to execute
	Q_snprintf(szMapCommand, sizeof(szMapCommand), "disconnect\nwait\nwait\nsv_lan 1\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nprogress_enable\nmp_winlimit %i\nmp_fraglimit %i\nmp_timelimit %i\nmp_maxrounds %i\nmap %s\n",
			   GetMaxPlayers(),
			   szPassword,
			   szHostName,
			   GetWinLimit(),
			   GetFragLimit(),
			   GetTimePerMap(),
			   GetRoundLimit(),
			   szMapName);

	Hide();

	engine->ClientCmd_Unrestricted(szBotAddCommand);

	// exec
	engine->ClientCmd_Unrestricted(szMapCommand);
}

int CreateServerPanel::GetMaxPlayers()
{
	char strValue[256];
	vgui::Panel *pMaxPlayersLabel = FindChildByName("MaxPlayersCombo", true);

	vgui::TextEntry *pMaxPlayersText = dynamic_cast<vgui::TextEntry *>(pMaxPlayersLabel);
	pMaxPlayersText->GetText(strValue, sizeof(strValue));

	return atoi(strValue);
}

const char *CreateServerPanel::GetHostName()
{
	static char strValue[256];
	vgui::Panel *pHostnamePanel = FindChildByName("HostnameCombo", true);

	vgui::TextEntry *pHostnameText = dynamic_cast<vgui::TextEntry *>(pHostnamePanel);
	pHostnameText->GetText(strValue, sizeof(strValue));

	return strValue;
}

const char *CreateServerPanel::GetPassword()
{
	static char strValue[256];
	vgui::Panel *pServerPasswordLabel = FindChildByName("ServerPasswordCombo", true);

	vgui::TextEntry *pServerPasswordText = dynamic_cast<vgui::TextEntry *>(pServerPasswordLabel);
	pServerPasswordText->GetText(strValue, sizeof(strValue));

	return strValue;
}

const char *CreateServerPanel::GetMapName()
{
	int count = m_pMapList->GetItemCount();

	// if there is only one entry it's the special "select random map" entry
	if (count <= 1)
		return NULL;

	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString("mapname");
	if (!strcmp(mapname, RANDOM_MAP))
	{
		int which = RandomInt(1, count - 1);
		mapname = m_pMapList->GetItemUserData(which)->GetString("mapname");
	}

	return mapname;
}

bool CreateServerPanel::IsRandomMapSelected()
{
	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString("mapname");
	if (!stricmp(mapname, RANDOM_MAP))
	{
		return true;
	}
	return false;
}

int CreateServerPanel::GetBotAmount()
{
	char strValue[4];
	vgui::Panel *pBotQuotaLabel = FindChildByName("BotQuotaCombo", true);

	vgui::TextEntry *pBotQuotaText = dynamic_cast<vgui::TextEntry *>(pBotQuotaLabel);
	pBotQuotaText->GetText(strValue, sizeof(strValue));

	vgui::CheckButton *pEnableBotsCheck = dynamic_cast<vgui::CheckButton *>(FindChildByName("EnableBotsCheck", true));
	if (pEnableBotsCheck->IsSelected())
	{
		return atoi(strValue);
	}
	else
	{
		return 0;
	}
}

int CreateServerPanel::GetWinLimit()
{
	char strValue[32];
	vgui::Panel *pWinLimitLabel = FindChildByName("WinLimitCombo",true);

	vgui::TextEntry *pWinLimitText = dynamic_cast<vgui::TextEntry *>(pWinLimitLabel);
	pWinLimitText->GetText(strValue, sizeof(strValue));

	return atoi(strValue);
}

int CreateServerPanel::GetFragLimit()
{
	char strValue[32];
	vgui::Panel *pFragLimitLabel = FindChildByName("FragLimitCombo", true);

	vgui::TextEntry *pFragLimitText = dynamic_cast<vgui::TextEntry *>(pFragLimitLabel);
	pFragLimitText->GetText(strValue, sizeof(strValue));

	return atoi(strValue);
}

int CreateServerPanel::GetTimePerMap()
{
	char strValue[32];
	vgui::Panel *pTimePerMapLabel = FindChildByName("TimePerMapCombo", true);

	vgui::TextEntry *pTimePerMapText = dynamic_cast<vgui::TextEntry *>(pTimePerMapLabel);
	pTimePerMapText->GetText(strValue, sizeof(strValue));

	return atoi(strValue);
}

int CreateServerPanel::GetRoundLimit()
{
	char strValue[32];
	vgui::Panel *pRoundLimitLabel = FindChildByName("RoundLimitCombo", true);

	vgui::TextEntry *pRoundLimitText = dynamic_cast<vgui::TextEntry *>(pRoundLimitLabel);
	pRoundLimitText->GetText(strValue, sizeof(strValue));

	return atoi(strValue);
}

//-----------------------------------------------------------------------------
// Purpose: loads the list of available maps into the map list
//-----------------------------------------------------------------------------
void CreateServerPanel::LoadMaps(const char *pszPathID)
{
	FileFindHandle_t findHandle = NULL;

	const char *pszFilename = g_pFullFileSystem->FindFirst("maps/*.bsp", &findHandle);
	while (pszFilename)
	{
		char mapname[256];
		char *ext, *str;

		// FindFirst ignores the pszPathID, so check it here
		// TODO: this doesn't find maps in fallback dirs
		Q_snprintf(mapname, sizeof(mapname), "maps/%s", pszFilename);
		if (!g_pFullFileSystem->FileExists(mapname, pszPathID))
		{
			goto nextFile;
		}

		// remove the text 'maps/' and '.bsp' from the file name to get the map name

		str = Q_strstr(pszFilename, "maps");
		if (str)
		{
			Q_strncpy(mapname, str + 5, sizeof(mapname) - 1); // maps + \\ = 5
		}
		else
		{
			Q_strncpy(mapname, pszFilename, sizeof(mapname) - 1);
		}
		ext = Q_strstr(mapname, ".bsp");
		if (ext)
		{
			*ext = 0;
		}

		//!! hack: strip out single player HL maps
		// this needs to be specified in a seperate file
		//if (!stricmp(ModInfo().GetGameName(), "Half-Life") && (mapname[0] == 'c' || mapname[0] == 't') && mapname[2] == 'a' && mapname[1] >= '0' && mapname[1] <= '5')
		//{
		//	goto nextFile;
		//}

		// strip out maps that shouldn't be displayed
		//if (hiddenMaps)
		//{
		//	if (hiddenMaps->GetInt(mapname, 0))
		//	{
		//		goto nextFile;
		//	}
		//}

		// add to the map list
		m_pMapList->AddItem(mapname, new KeyValues("data", "mapname", mapname));

		// get the next file
	nextFile:
		pszFilename = g_pFullFileSystem->FindNext(findHandle);
	}
	g_pFullFileSystem->FindClose(findHandle);
}

//-----------------------------------------------------------------------------
// Purpose: loads the list of available maps into the map list
//-----------------------------------------------------------------------------
void CreateServerPanel::LoadMapList()
{
	// clear the current list (if any)
	m_pMapList->DeleteAllItems();

	// add special "name" to represent loading a randomly selected map
	m_pMapList->AddItem(RANDOM_MAP, new KeyValues("data", "mapname", RANDOM_MAP));

	// iterate the filesystem getting the list of all the files
	// UNDONE: steam wants this done in a special way, need to support that
	const char *pathID = "MOD";
	//if (!stricmp(ModInfo().GetGameName(), "Half-Life"))
	//{
	//	pathID = NULL; // hl is the base dir
	//}

	// Load the GameDir maps
	LoadMaps(pathID);

	// If we're not the Valve directory and we're using a "fallback_dir" in gameinfo.txt then include those maps...
	// (pathID is NULL if we're "Half-Life")
	//const char *pszFallback = ModInfo().GetFallbackDir();
	//if (pathID && pszFallback[0])
	//{
	//	LoadMaps("GAME_FALLBACK");
	//}

	// set the first item to be selected
	m_pMapList->ActivateItem(0);
}

void CreateServerPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(true);
	LoadControlSettings("resource/ui/basemodui/createserverpanel.res");

	m_pMapList = static_cast<ComboBox*>( FindChildByName("MapList", true) );
	LoadMapList();

	m_pCheckButton = static_cast<CheckButton*>( FindChildByName("EnableBotsCheck", true) );
}