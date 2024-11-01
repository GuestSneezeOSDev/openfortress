//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>
#include "tf_clientscoreboard.h"
#include "tf_hud_statpanel.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include <vgui/ILocalize.h>
#include "tf_playerclass_shared.h"

using namespace vgui;

#define SCOREBOARD_MAX_LIST_ENTRIES 12
#define PLAYERINFO_ELEMENTS 16

extern bool IsInCommentaryMode(void);
extern ConVar of_allowteams;
extern ConVar fraglimit;
extern ConVar of_allow_special_teams;
extern ConVar of_show_medals;
extern ConVar of_lives;
extern ConVar of_lives_frags_score;
extern ConVar of_arena_lives_multiplier;
extern ConVar of_arena_frags_multiplier;

const char *CTFClientScoreBoardDialog::szPlayerDetailsLabels[13] =
	{"#TF_ScoreBoard_KillsLabel", "#TF_ScoreBoard_DeathsLabel", "#TF_ScoreBoard_AssistsLabel", "#TF_ScoreBoard_DestructionLabel",
	 "#TF_ScoreBoard_CapturesLabel", "#TF_ScoreBoard_DefensesLabel", "#TF_ScoreBoard_DominationLabel", "#TF_ScoreBoard_RevengeLabel",
	 "#TF_ScoreBoard_InvulnLabel", "#TF_ScoreBoard_HeadshotsLabel", "#TF_ScoreBoard_TeleportsLabel", "#TF_ScoreBoard_HealingLabel",
	 "#TF_ScoreBoard_BackstabsLabel"};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::CTFClientScoreBoardDialog(IViewPort *pViewPort) : CClientScoreBoardDialog(pViewPort)
{
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	SetScheme("ClientScheme");

	m_pPlayerListBlue = new SectionedListPanel(this, "BluePlayerList");
	m_pPlayerListRed = new SectionedListPanel(this, "RedPlayerList");
	m_pPlayerListMercenary = new SectionedListPanel(this, "MercenaryPlayerList");
	m_pLabelPlayerName = new CExLabel(this, "PlayerNameLabel", "");
	m_pImagePanelHorizLine = new ImagePanel(this, "HorizontalLine");
	m_pClassImage = new CTFClassImage(this, "ClassImage");
	m_pClassImageColorless = new CTFClassImage(this, "ClassImageColorless");
	m_iImageDead = 0;
	m_iImageDominated = 0;
	m_iImageNemesis = 0;

	ListenForGameEvent("server_spawn");

	SetDialogVariable("server", "");

	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::~CTFClientScoreBoardDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

extern TFPlayerClassData_t s_aTFPlayerClassData[TF_CLASS_COUNT_ALL];

const char *g_aClassIcons[TF_CLASS_COUNT_ALL] =
	{
		"",
		"../hud/leaderboard_class_scout",
		"../hud/leaderboard_class_sniper",
		"../hud/leaderboard_class_soldier",
		"../hud/leaderboard_class_demo",
		"../hud/leaderboard_class_medic",
		"../hud/leaderboard_class_heavy",
		"../hud/leaderboard_class_pyro",
		"../hud/leaderboard_class_spy",
		"../hud/leaderboard_class_engineer",
		"../hud/leaderboard_class_mercenary",
		"../hud/leaderboard_class_civilian",
		"../hud/leaderboard_class_juggernaut"};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if (m_pImageList)
	{
		m_iImageDead = m_pImageList->AddImage(scheme()->GetImage("../hud/leaderboard_dead", true));
		m_iImageDominated = m_pImageList->AddImage(scheme()->GetImage("../hud/leaderboard_dominated", true));
		m_iImageNemesis = m_pImageList->AddImage(scheme()->GetImage("../hud/leaderboard_nemesis", true));

		for (int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++)
		{
			m_iClassIcon[i] = m_pImageList->AddImage(scheme()->GetImage(g_aClassIcons[i], true));
		}

		// resize the images to our resolution
		for (int i = 1; i < m_pImageList->GetImageCount(); i++)
		{
			int wide = 13, tall = 13;
			m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx(GetScheme(), wide), scheme()->GetProportionalScaledValueEx(GetScheme(), tall));
		}
	}

	SetPlayerListImages(m_pPlayerListBlue);
	SetPlayerListImages(m_pPlayerListRed);
	SetPlayerListImages(m_pPlayerListMercenary);

	SetBgColor(Color(0, 0, 0, 0));
	SetBorder(NULL);
	SetVisible(false);

	SetZPos(80); // guarantee it shows above the scope
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ShowPanel(bool bShow)
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	Reset();

	if (m_pImageList == NULL)
		InvalidateLayout(true, true);

	// Don't show in commentary mode
	if (IsInCommentaryMode())
	{
		bShow = false;
	}

	if (IsVisible() == bShow)
	{
		return;
	}

	int iRenderGroup = gHUD.LookupRenderGroupIndexByName("global");

	if (bShow)
	{
		SetVisible(true);
		MoveToFront();

		gHUD.LockRenderGroup(iRenderGroup);

		// Clear the selected item, this forces the default to the local player
		SectionedListPanel *pList = GetSelectedPlayerList();
		if (pList)
		{
			pList->ClearSelection();
		}
	}
	else
	{
		SetVisible(false);

		gHUD.UnlockRenderGroup(iRenderGroup);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scoreboard panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Reset()
{
	RemovePlayerList( m_pPlayerListBlue );
	RemovePlayerList( m_pPlayerListRed );
	RemovePlayerList( m_pPlayerListMercenary );
	if ( TFGameRules() )
	{
		bool bIsDM = TFGameRules()->IsDMGamemode();
		bool bAllowTeams = of_allowteams.GetBool();
		bool bIsTeamplay = TFGameRules()->IsTeamplay();

		if (!bIsDM || bAllowTeams || bIsTeamplay)
		{
			InitPlayerList( m_pPlayerListBlue );
			InitPlayerList( m_pPlayerListRed );
		}

		if (bIsDM && !bIsTeamplay)
		{
			InitPlayerList(m_pPlayerListMercenary);

			m_pPlayerListBlue->SetVisible(bAllowTeams);
			m_pPlayerListRed->SetVisible(bAllowTeams);
			if ((TFGameRules()->IsArenaGamemode()) && (TFGameRules()->DoesArenaCountLivesAndFrags()))
			{
				LoadControlSettings("Resource/UI/scoreboardarenadm.res");
			}
			else
				LoadControlSettings("Resource/UI/scoreboarddm.res");
			
		}
		else
		{
			m_pPlayerListMercenary->SetVisible(of_allow_special_teams.GetBool());
			LoadControlSettings("Resource/UI/scoreboard.res");
		}

		ClearPlayerDetails( bIsDM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::InitPlayerList(SectionedListPanel *pPlayerList)
{
	// Avatars are always displayed at 32x32 regardless of resolution
	if (ShowAvatars())
	{
		pPlayerList->AddColumnToSection(0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth);
	}
	pPlayerList->AddColumnToSection(0, "name", "#TF_Scoreboard_Name", 0, m_iNameWidth);
	pPlayerList->AddColumnToSection(0, "status", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iStatusWidth);
	pPlayerList->AddColumnToSection(0, "nemesis", "", SectionedListPanel::COLUMN_IMAGE, m_iNemesisWidth);

	const char *szScoreLabel[] =
		{
			"#TF_Scoreboard_Score",
			"#TF_Scoreboard_Kills",
			"#TF_ScoreBoard_GGLevel",
			"#TF_Scoreboard_Lives",
		};

	int iScoreLabel = 0;

	if (TFGameRules())
	{
		if (TFGameRules()->IsGGGamemode())
			iScoreLabel = 2;
		else if ((TFGameRules()->IsDMGamemode() && !TFGameRules()->InGametype(TF_GAMETYPE_TDM) && !TFGameRules()->DontCountKills()) && !TFGameRules()->IsArenaGamemode())
			iScoreLabel = 1;
		else if (TFGameRules()->IsArenaGamemode())
		{
			if (TFGameRules()->DoesArenaCountLivesAndFrags())
			{
				pPlayerList->AddColumnToSection(0, "kills", "#TF_Scoreboard_Kills", SectionedListPanel::COLUMN_CENTER, m_iScoreWidth);
				pPlayerList->AddColumnToSection(0, "lives", "#TF_Scoreboard_Lives", SectionedListPanel::COLUMN_CENTER, m_iScoreWidth);
				iScoreLabel = 0;
			}
			else
				iScoreLabel = 3;
		}
	}

	pPlayerList->AddColumnToSection(0, "score", szScoreLabel[iScoreLabel], SectionedListPanel::COLUMN_CENTER, m_iScoreWidth);
	pPlayerList->AddColumnToSection(0, "class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iClassWidth);
	pPlayerList->AddColumnToSection(0, "ping", "#TF_Scoreboard_Ping", SectionedListPanel::COLUMN_CENTER, m_iPingWidth);
}

void CTFClientScoreBoardDialog::RemovePlayerList(SectionedListPanel *pPlayerList)
{
	pPlayerList->SetVerticalScrollbar(false);
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection(0, "Players", TFPlayerSortFunc);
	pPlayerList->SetSectionAlwaysVisible(0, true);
	pPlayerList->SetSectionFgColor(0, Color(255, 255, 255, 255));
	pPlayerList->SetBgColor(Color(0, 0, 0, 0));
	pPlayerList->SetBorder(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Builds the image list to use in the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::SetPlayerListImages(vgui::SectionedListPanel *pPlayerList)
{
	pPlayerList->SetImageList(m_pImageList, false);
	pPlayerList->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Update()
{
	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateSpectatorList();
	UpdatePlayerDetails();
	MoveToCenterOfScreen();

	wchar_t string1[1024];
	wchar_t wzMaxLevel[128];
	wchar_t wzFragLimit[128];

	if (TFGameRules())
	{
		if (TFGameRules()->IsGGGamemode())
		{
			_snwprintf(wzMaxLevel, ARRAYSIZE(wzMaxLevel), L"%i", (int)TFGameRules()->m_iMaxLevel);
			g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#TF_ScoreBoard_LevelLimit"), 1, wzMaxLevel);
			SetDialogVariable("FragLimit", string1);
		}
		else if (TFGameRules()->IsArenaGamemode())
		{
			_snwprintf(wzMaxLevel, ARRAYSIZE(wzMaxLevel), L"%i", of_lives.GetInt());
			g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#TF_ScoreBoard_LivesLimit"), 1, wzMaxLevel);
			SetDialogVariable("FragLimit", string1);
		}
		else
		{
			_snwprintf(wzFragLimit, ARRAYSIZE(wzFragLimit), L"%i", fraglimit.GetInt());
			g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#TF_ScoreBoard_Fraglimit"), 1, wzFragLimit);
			if (!TFGameRules()->DontCountKills())
				SetDialogVariable("FragLimit", string1);
			else
				SetDialogVariable("FragLimit", "");
		}
	}

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateTeamInfo()
{
	// update the team sections in the scoreboard
	for (int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_MERCENARY; teamIndex++)
	{
		wchar_t *teamName = NULL;
		C_Team *team = GetGlobalTeam(teamIndex);
		if (team)
		{
			// choose dialog variables to set depending on team
			const char *pDialogVarTeamScore = NULL;
			const char *pDialogVarTeamPlayerCount = NULL;
			if (!TFGameRules()->IsDMGamemode() || TFGameRules()->IsTeamplay())
			{
				switch (teamIndex)
				{
				case TF_TEAM_RED:
					pDialogVarTeamScore = "redteamscore";
					pDialogVarTeamPlayerCount = "redteamplayercount";
					break;
				case TF_TEAM_BLUE:
					pDialogVarTeamScore = "blueteamscore";
					pDialogVarTeamPlayerCount = "blueteamplayercount";
					break;
				case TF_TEAM_MERCENARY:
					pDialogVarTeamScore = "mercenaryteamscore";
					pDialogVarTeamPlayerCount = "mercenaryteamplayercount";
					break;
				default:
					Assert(false);
					break;
				}
			}
			else
			{
				pDialogVarTeamScore = "mercenaryteamscore";
				pDialogVarTeamPlayerCount = "mercenaryteamplayercount";
			}
			// update # of players on each team
			wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];
			_snwprintf(wNumPlayers, ARRAYSIZE(wNumPlayers), L"%i", team->Get_Number_Players());
			if (!teamName && team)
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(team->Get_Name(), name, sizeof(name));
				teamName = name;
			}
			if (team->Get_Number_Players() == 1)
			{
				g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#TF_ScoreBoard_Player"), 1, wNumPlayers);
			}
			else
			{
				g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#TF_ScoreBoard_Players"), 1, wNumPlayers);
			}

			// set # of players for team in dialog
			SetDialogVariable(pDialogVarTeamPlayerCount, string1);

			// set team score in dialog
			SetDialogVariable(pDialogVarTeamScore, team->Get_Score());
		}
	}
}

bool AreEnemyTeams(int iTeam1, int iTeam2)
{
	if (iTeam1 == TF_TEAM_MERCENARY || iTeam2 == TF_TEAM_MERCENARY)
		return true;

	if (iTeam1 == TF_TEAM_RED && iTeam2 == TF_TEAM_BLUE)
		return true;

	if (iTeam1 == TF_TEAM_BLUE && iTeam2 == TF_TEAM_RED)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerList()
{
	int iSelectedPlayerIndex = GetLocalPlayerIndex();

	// Save off which player we had selected
	SectionedListPanel *pList = GetSelectedPlayerList();

	if (pList)
	{
		int itemID = pList->GetSelectedItem();

		if (itemID >= 0)
		{
			KeyValues *pInfo = pList->GetItemData(itemID);
			if (pInfo)
			{
				iSelectedPlayerIndex = pInfo->GetInt("playerIndex");
			}
		}
	}

	m_pPlayerListRed->RemoveAll();
	m_pPlayerListBlue->RemoveAll();
	m_pPlayerListMercenary->RemoveAll();

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return;

	int localteam = pLocalPlayer->GetTeamNumber();

	bool bMadeSelection = false;

	for (int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++)
	{
		if (g_PR->IsConnected(playerIndex))
		{
			SectionedListPanel *pPlayerList = NULL;
			switch (g_PR->GetTeam(playerIndex))
			{
			case TF_TEAM_BLUE:
				pPlayerList = m_pPlayerListBlue;
				break;
			case TF_TEAM_RED:
				pPlayerList = m_pPlayerListRed;
				break;
			case TF_TEAM_MERCENARY:
				pPlayerList = m_pPlayerListMercenary;
				break;
			}
			if (null == pPlayerList)
				continue;

			const char *szName = tf_PR->GetPlayerName(playerIndex);
			int score = tf_PR->GetTotalScore(playerIndex);
			int gglevel = tf_PR->GetGGLevel(playerIndex);
			int lives = tf_PR->GetLives(playerIndex);
			int kills = tf_PR->GetPlayerScore(playerIndex);
			int livefrags = ((lives * of_arena_lives_multiplier.GetInt()) + (kills * of_arena_frags_multiplier.GetInt()));

			KeyValues *pKeyValues = new KeyValues("data");

			pKeyValues->SetInt("playerIndex", playerIndex);
			pKeyValues->SetString("name", szName);

			bool bDMnoTeamplay = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay();

			if (TFGameRules()->IsGGGamemode())
				score = gglevel;
			else if (TFGameRules()->IsArenaGamemode())
			{
				if ((TFGameRules()->DoesArenaCountLivesAndFrags()))
				{
					score = livefrags;
					pKeyValues->SetInt("kills", kills);
					pKeyValues->SetInt("lives", lives);
				}
				else
					score = lives;
			}
			else if (bDMnoTeamplay && !TFGameRules()->DontCountKills())
				score = kills;

			pKeyValues->SetInt("score", score);

			// can only see class information if we're on the same team
			if (localteam == TF_TEAM_MERCENARY || (!AreEnemyTeams(g_PR->GetTeam(playerIndex), localteam) && !(localteam == TEAM_UNASSIGNED)))
			{
				// class name
				if (g_PR->IsConnected(playerIndex))
				{
					int iClass = tf_PR->GetPlayerClass(playerIndex);
					if (GetLocalPlayerIndex() == playerIndex && !tf_PR->IsAlive(playerIndex))
					{
						// If this is local player and he is dead, show desired class (which he will spawn as) rather than current class.
						C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
						int iDesiredClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();

						// use desired class unless it's random -- if random, his future class is not decided until moment of spawn
						if (TF_CLASS_RANDOM != iDesiredClass)
							iClass = iDesiredClass;
					}
					else
					{
						// for non-local players, show the current class
						iClass = tf_PR->GetPlayerClass(playerIndex);
					}

					if (iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_CLASS_COUNT_ALL)
						pKeyValues->SetInt("class", m_iClassIcon[iClass]);
				}
			}

			C_TFPlayer *pPlayerOther = ToTFPlayer(UTIL_PlayerByIndex(playerIndex));

			if (pPlayerOther && pPlayerOther->m_Shared.IsPlayerDominated(pLocalPlayer->entindex()))
			{
				// if local player is dominated by this player, show a nemesis icon
				pKeyValues->SetInt("nemesis", m_iImageNemesis);
			}
			else if (pLocalPlayer->m_Shared.IsPlayerDominated(playerIndex))
			{
				// if this player is dominated by the local player, show the domination icon
				pKeyValues->SetInt("nemesis", m_iImageDominated);
			}

			// display whether player is alive or dead (all players see this for all other players on both teams)
			pKeyValues->SetInt("status", tf_PR->IsAlive(playerIndex) ? 0 : m_iImageDead);

			if (g_PR->GetPing(playerIndex) < 1)
			{
				if (g_PR->IsFakePlayer(playerIndex))
					pKeyValues->SetString("ping", "#TF_Scoreboard_Bot");
				else
					pKeyValues->SetString("ping", "");
			}
			else
			{
				pKeyValues->SetInt("ping", g_PR->GetPing(playerIndex));
			}

			UpdatePlayerAvatar(playerIndex, pKeyValues);

			int itemID = pPlayerList->AddItem(0, pKeyValues);
			Color clr = bDMnoTeamplay ? tf_PR->GetPlayerColor(playerIndex) : g_PR->GetTeamColor(g_PR->GetTeam(playerIndex));
			pPlayerList->SetItemFgColor(itemID, clr);

			if (iSelectedPlayerIndex == playerIndex)
			{
				bMadeSelection = true;
				pPlayerList->SetSelectedItem(itemID);
			}

			pKeyValues->deleteThis();
		}
	}

	// If we're on spectator, undefine any selection
	if (!bMadeSelection)
	{
		if (m_pPlayerListBlue->GetItemCount() >= 0)
			m_pPlayerListBlue->SetSelectedItem(-1);
		else if (m_pPlayerListRed->GetItemCount() >= 0)
			m_pPlayerListRed->SetSelectedItem(-1);
		else if (m_pPlayerListMercenary->GetItemCount() >= 0)
			m_pPlayerListMercenary->SetSelectedItem(-1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateSpectatorList()
{
	char szSpectatorList[512] = "";
	int nSpectators = 0;
	for (int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++)
	{
		if (ShouldShowAsSpectator(playerIndex))
		{
			if (nSpectators > 0)
				Q_strncat(szSpectatorList, ", ", ARRAYSIZE(szSpectatorList));

			Q_strncat(szSpectatorList, g_PR->GetPlayerName(playerIndex), ARRAYSIZE(szSpectatorList));
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	if (nSpectators > 0)
	{
		const char *pchFormat = (1 == nSpectators ? "#ScoreBoard_Spectator" : "#ScoreBoard_Spectators");

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		_snwprintf(wzSpectatorCount, ARRAYSIZE(wzSpectatorCount), L"%i", nSpectators);
		g_pVGuiLocalize->ConvertANSIToUnicode(szSpectatorList, wzSpectatorList, sizeof(wzSpectatorList));
		g_pVGuiLocalize->ConstructString(wzSpectators, sizeof(wzSpectators), g_pVGuiLocalize->Find(pchFormat), 2, wzSpectatorCount, wzSpectatorList);
	}
	SetDialogVariable("spectators", wzSpectators);
}

//-----------------------------------------------------------------------------
// Purpose: Updates details about a player
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerDetails()
{
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return;
	int playerIndex = pLocalPlayer->entindex();

	// Make sure the selected player is still connected.
	if (!tf_PR->IsConnected(playerIndex))
		return;

	if (engine->IsHLTV())
	{
		SetDialogVariable("playername", tf_PR->GetPlayerName(playerIndex));
		return;
	}

	bool bIsDM = TFGameRules()->IsDMGamemode();

	if (bIsDM)
	{
		// Find medals that were awarded, those that have no count will not be drawn
		int iMedalCount = 0;
		CExLabel *pLabel = NULL;
		char szName[20];
		int size = sizeof(szName);

		int j = 1;
		for (int i = 0; i < DENIED + 1 || j > PLAYERINFO_ELEMENTS; i++)
		{
			// Has the medals at index i been awarded in the match?
			iMedalCount = g_medalsCounter[i];

			// if yes, show it on the scoreboard
			if (iMedalCount)
			{
				// Set the label to be visible...
				Q_snprintf(szName, size, "Label%02d", j);
				pLabel = dynamic_cast<CExLabel *>(FindChildByName(szName));
				pLabel->SetVisible(true);

				//...and change the label text, some editing needed
				Q_strcpy(szName, medalNames[i]);
				FormatMedalName(szName);
				pLabel->SetText(szName);

				// Set the counter to be visible...
				Q_snprintf(szName, size, "Count%02d", j);
				pLabel = dynamic_cast<CExLabel *>(FindChildByName(szName));
				pLabel->SetVisible(true);

				//...and change the text to the medal count
				Q_snprintf(szName, size, "%d", iMedalCount);
				pLabel->SetText(szName);

				j++;
			}
		}

		// no medals earned yet, show an explicative message on the first label
		if (j == 1)
		{
			pLabel = dynamic_cast<CExLabel *>(FindChildByName("Label01"));
			pLabel->SetVisible(true);
			pLabel->SetText("No medals");

			pLabel = dynamic_cast<CExLabel *>(FindChildByName("Count01"));
			pLabel->SetVisible(true);
			pLabel->SetText("earned, get killing!");

			j++;
		}

		// hide the possibly unused slots
		for (; j <= PLAYERINFO_ELEMENTS; j++)
		{
			Q_snprintf(szName, sizeof(szName), "Label%02d", j);
			SetControlVisible(szName, false);

			Q_snprintf(szName, sizeof(szName), "Count%02d", j);
			SetControlVisible(szName, false);
		}
	}
	else
	{
		// regular scoreboard stuff
		RoundStats_t &roundStats = GetStatPanel()->GetRoundStatsCurrentGame();

		UpdateLabelValue("Count01", tf_PR->GetPlayerScore(playerIndex));
		UpdateLabelValue("Count02", tf_PR->GetDeaths(playerIndex));
		UpdateLabelValue("Count03", roundStats.m_iStat[TFSTAT_KILLASSISTS]);
		UpdateLabelValue("Count04", roundStats.m_iStat[TFSTAT_BUILDINGSDESTROYED]);
		UpdateLabelValue("Count05", roundStats.m_iStat[TFSTAT_CAPTURES]);
		UpdateLabelValue("Count06", roundStats.m_iStat[TFSTAT_DEFENSES]);
		UpdateLabelValue("Count07", roundStats.m_iStat[TFSTAT_DOMINATIONS]);
		UpdateLabelValue("Count08", roundStats.m_iStat[TFSTAT_REVENGE]);
		UpdateLabelValue("Count09", roundStats.m_iStat[TFSTAT_INVULNS]);
		UpdateLabelValue("Count10", roundStats.m_iStat[TFSTAT_HEADSHOTS]);
		UpdateLabelValue("Count11", roundStats.m_iStat[TFSTAT_TELEPORTS]);
		UpdateLabelValue("Count12", roundStats.m_iStat[TFSTAT_HEALING]);
		UpdateLabelValue("Count13", roundStats.m_iStat[TFSTAT_BACKSTABS]);
	}

	SetDialogVariable("playername", tf_PR->GetPlayerName(playerIndex));
	SetDialogVariable("playerscore", GetPointsString(tf_PR->GetTotalScore(playerIndex)));
	Color clr = bIsDM && !TFGameRules()->IsTeamplay() ? tf_PR->GetPlayerColor(playerIndex) : g_PR->GetTeamColor(g_PR->GetTeam(playerIndex));

	m_pLabelPlayerName->SetFgColor(clr);
	m_pImagePanelHorizLine->SetFillColor(clr);

	int iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	int iTeam = pLocalPlayer->GetTeamNumber();
	if (iTeam >= FIRST_GAME_TEAM && iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_CLASS_COUNT_ALL)
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		TFPlayerClassData_t *pClassData = GetPlayerClassData(iClass, 0);
		if ( pPlayer )
			pClassData = pPlayer->GetPlayerClass()->GetData();

		if (iTeam == TF_TEAM_MERCENARY)
		{
			m_pClassImageColorless->SetClassColorless(iTeam, pClassData, 0);
			m_pClassImageColorless->SetVisible(true);
		}
		else
		{
			m_pClassImageColorless->SetVisible(false);
		}

		m_pClassImage->SetClass(iTeam, pClassData, 0);
		m_pClassImage->SetVisible(true);
	}
	else
	{
		m_pClassImage->SetVisible(false);
		m_pClassImageColorless->SetVisible(false);
	}
}

void CTFClientScoreBoardDialog::FormatMedalName(char *medalname)
{
	char szTemp[20] = {};
	char spaced[20] = {};
	V_strncpy(spaced, medalname, sizeof(spaced));
	int pos = 0;

	for (int i = 1; i < V_strlen(medalname); i++)
	{
		if (isupper(medalname[i]))
		{
			pos = i;
		}
	}

	if (pos > 0)
	{
		for (int j = Q_strlen(medalname); j >= pos; j--)
		{
			spaced[j] = spaced[j - 1];
		}
		spaced[pos] = ' ';
	}

	if (spaced)
	{
		Q_strcpy(szTemp, spaced);
		Q_snprintf(medalname, sizeof(szTemp), "%s:", szTemp);
	}
	else
	{
		Q_strcpy(szTemp, STRING(medalname));
		Q_snprintf(medalname, sizeof(szTemp), "%s:", szTemp);
	}
}

void CTFClientScoreBoardDialog::UpdateLabelValue(const char *name, const int value)
{
	CExLabel *pLabel = dynamic_cast<CExLabel *>(FindChildByName(name));
	char szValue[4];
	Q_snprintf(szValue, sizeof(szValue), "%d", value);
	pLabel->SetText(szValue);
}

//-----------------------------------------------------------------------------
// Purpose: Clears score details
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ClearPlayerDetails(bool isDM)
{
	m_pClassImage->SetVisible(false);
	m_pClassImageColorless->SetVisible(false);

	// HLTV has no game stats
	bool bVisible = !engine->IsHLTV();

	char szName[8];
	CExLabel *pLabel = NULL;
	int size = sizeof(szName);

	// nothing should draw
	if (!bVisible)
	{
		for (int i = 1; i < PLAYERINFO_ELEMENTS + 1; i++)
		{
			Q_snprintf(szName, size, "Count%02d", i);
			SetDialogVariable(szName, "");
			Q_snprintf(szName, size, "Label%02d", i);
			SetControlVisible(szName, bVisible);
		}
	}
	else if (!isDM)
	{
		int i = 1;
		// show the player details and give them the regular names
		for (; i < 14; i++)
		{
			// reset counter
			Q_snprintf(szName, size, "Count%02d", i);
			pLabel = dynamic_cast<CExLabel *>(FindChildByName(szName));
			if (pLabel == NULL)
				continue;
			pLabel->SetText("0");
			pLabel->SetVisible(true);

			// Make sure the correct text is used in the label
			Q_snprintf(szName, size, "Label%02d", i);
			pLabel = dynamic_cast<CExLabel *>(FindChildByName(szName));
			if (pLabel == NULL)
				continue;
			pLabel->SetText(szPlayerDetailsLabels[i - 1]);
			pLabel->SetVisible(true);
		}

		// hide the extra 3 details that are only used with medals
		for (; i < PLAYERINFO_ELEMENTS + 1; i++)
		{
			Q_snprintf(szName, size, "Count%02d", i);
			SetControlVisible(szName, false);
			Q_snprintf(szName, size, "Label%02d", i);
			SetControlVisible(szName, false);
		}
	}

	SetDialogVariable("playername", "");
	SetDialogVariable("playerscore", "");
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::TFPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare score
	int v1 = it1->GetInt("score");
	int v2 = it2->GetInt("score");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// if score is the same, use player index to get deterministic sort
	int iPlayerIndex1 = it1->GetInt("playerIndex");
	int iPlayerIndex2 = it2->GetInt("playerIndex");
	return (iPlayerIndex1 > iPlayerIndex2);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a localized string of form "1 point", "2 points", etc for specified # of points
//-----------------------------------------------------------------------------
const wchar_t *GetPointsString(int iPoints)
{
	wchar_t wzScoreVal[128];
	static wchar_t wzScore[128];
	_snwprintf(wzScoreVal, ARRAYSIZE(wzScoreVal), L"%i", iPoints);
	if (TFGameRules() && TFGameRules()->IsDMGamemode())
	{
		g_pVGuiLocalize->ConstructString(wzScore, sizeof(wzScore), g_pVGuiLocalize->Find("#TF_ScoreBoard_Points_NoLabel"), 1, wzScoreVal);
	}
	else
	{
		if (1 == iPoints)
			g_pVGuiLocalize->ConstructString(wzScore, sizeof(wzScore), g_pVGuiLocalize->Find("#TF_ScoreBoard_Point"), 1, wzScoreVal);
		else
			g_pVGuiLocalize->ConstructString(wzScore, sizeof(wzScore), g_pVGuiLocalize->Find("#TF_ScoreBoard_Points"), 1, wzScoreVal);
	}
	return wzScore;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the specified player index is a spectator
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::ShouldShowAsSpectator(int iPlayerIndex)
{
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return false;

	// see if player is connected
	if (tf_PR->IsConnected(iPlayerIndex))
	{
		// either spectating or unassigned team should show in spectator list
		int iTeam = tf_PR->GetTeam(iPlayerIndex);
		if (TEAM_SPECTATOR == iTeam)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (0 == Q_strcmp(type, "server_spawn"))
	{
		// set server name in scoreboard
		const char *hostname = event->GetString("hostname");
		wchar_t wzHostName[256] = {};
		wchar_t wzServerLabel[256] = {};
		g_pVGuiLocalize->ConvertANSIToUnicode(hostname, wzHostName, sizeof(wzHostName));
		g_pVGuiLocalize->ConstructString(wzServerLabel, sizeof(wzServerLabel), g_pVGuiLocalize->Find("#Scoreboard_Server"), 1, wzHostName);
		SetDialogVariable("server", wzServerLabel);
	}

	if (IsVisible())
	{
		Update();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SectionedListPanel *CTFClientScoreBoardDialog::GetSelectedPlayerList(void)
{
	SectionedListPanel *pList = NULL;

	// navigation
	if (m_pPlayerListBlue->GetSelectedItem() >= 0)
	{
		pList = m_pPlayerListBlue;
	}
	else if (m_pPlayerListRed->GetSelectedItem() >= 0)
	{
		pList = m_pPlayerListRed;
	}
	else if (m_pPlayerListMercenary->GetSelectedItem() >= 0)
	{
		pList = m_pPlayerListMercenary;
	}

	return pList;
}

#if defined(_X360)

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
int CTFClientScoreBoardDialog::HudElementKeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{
	if (!IsVisible())
		return 1;

	if (!down)
	{
		return 1;
	}

	SectionedListPanel *pList = GetSelectedPlayerList();

	switch (keynum)
	{
	case KEY_XBUTTON_UP:
	{
		if (pList)
		{
			pList->MoveSelectionUp();
		}
	}
		return 0;

	case KEY_XBUTTON_DOWN:
	{
		if (pList)
		{
			pList->MoveSelectionDown();
		}
	}
		return 0;

	case KEY_XBUTTON_RIGHT:
	{
		if (m_pPlayerListRed->GetItemCount() == 0)
			return 0;

		// move to the red list

		// get the row we're in now
		int iSelectedBlueItem = m_pPlayerListBlue->GetSelectedItem();

		m_pPlayerListBlue->ClearSelection();

		if (iSelectedBlueItem >= 0)
		{
			int row = m_pPlayerListBlue->GetRowFromItemID(iSelectedBlueItem);

			if (row >= 0)
			{
				int iNewItem = m_pPlayerListRed->GetItemIDFromRow(row);

				if (iNewItem >= 0)
				{
					m_pPlayerListRed->SetSelectedItem(iNewItem);
				}
				else
				{
					// we have fewer items. Select the last one
					int iLastRow = m_pPlayerListRed->GetItemCount() - 1;

					iNewItem = m_pPlayerListRed->GetItemIDFromRow(iLastRow);

					if (iNewItem >= 0)
					{
						m_pPlayerListRed->SetSelectedItem(iNewItem);
					}
				}
			}
		}
	}
		return 0;

	case KEY_XBUTTON_LEFT:
	{
		if (m_pPlayerListBlue->GetItemCount() == 0)
			return 0;

		// move to the blue list

		// get the row we're in now
		int iSelectedRedItem = m_pPlayerListRed->GetSelectedItem();

		if (iSelectedRedItem < 0)
			iSelectedRedItem = 0;

		m_pPlayerListRed->ClearSelection();

		if (iSelectedRedItem >= 0)
		{
			int row = m_pPlayerListRed->GetRowFromItemID(iSelectedRedItem);

			if (row >= 0)
			{
				int iNewItem = m_pPlayerListBlue->GetItemIDFromRow(row);

				if (iNewItem >= 0)
				{
					m_pPlayerListBlue->SetSelectedItem(iNewItem);
				}
				else
				{
					// we have fewer items. Select the last one
					int iLastRow = m_pPlayerListBlue->GetItemCount() - 1;

					iNewItem = m_pPlayerListBlue->GetItemIDFromRow(iLastRow);

					if (iNewItem >= 0)
					{
						m_pPlayerListBlue->SetSelectedItem(iNewItem);
					}
				}
			}
		}
	}
		return 0;

	case KEY_XBUTTON_B:
	{
		ShowPanel(false);
	}
		return 0;

	case KEY_XBUTTON_A: // Show GamerCard for the selected player
	{
		if (pList)
		{
			int iSelectedItem = pList->GetSelectedItem();

			if (iSelectedItem >= 0)
			{
				KeyValues *pInfo = pList->GetItemData(iSelectedItem);

				DevMsg(1, "XShowGamerCardUI for player '%s'\n", pInfo->GetString("name"));

				uint64 xuid = matchmaking->PlayerIdToXuid(pInfo->GetInt("playerIndex"));
				XShowGamerCardUI(XBX_GetPrimaryUserId(), xuid);
			}
		}
	}
		return 0;

	case KEY_XBUTTON_X: // Show player review for the selected player
	{
		if (pList)
		{
			int iSelectedItem = pList->GetSelectedItem();

			if (iSelectedItem >= 0)
			{
				KeyValues *pInfo = pList->GetItemData(iSelectedItem);

				DevMsg(1, "XShowPlayerReviewUI for player '%s'\n", pInfo->GetString("name"));

				uint64 xuid = matchmaking->PlayerIdToXuid(pInfo->GetInt("playerIndex"));
				XShowPlayerReviewUI(XBX_GetPrimaryUserId(), xuid);
			}
		}
	}
		return 0;

	default:
		break;
	}

	return 1;
}

#endif //_X360
