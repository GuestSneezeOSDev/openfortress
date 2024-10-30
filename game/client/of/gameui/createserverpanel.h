//=============================================================================
//
// Purpose: Fancy create server panel
//
//=============================================================================

//#ifdef _WIN32
//#pragma once
//#endif

#include "basemodframe.h"

class CModelPanel;

namespace BaseModUI
{

	class CreateServerPanel : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE(CreateServerPanel, CBaseModFrame);

	public:
		CreateServerPanel(Panel *parent, const char *panelName);
		~CreateServerPanel();

		virtual void Activate() override;

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		void OnCommand(const char *command);

		MESSAGE_FUNC_PARAMS(OnHudAnimationEnd, "AnimEnd", pKV);

		void StartServer();
		void LoadMapList();
		void LoadMaps(const char *pszPathID);
		const char *GetMapName();
		const char *GetHostName();
		const char *GetPassword();
		int GetMaxPlayers();
		int GetBotAmount();
		int GetWinLimit();
		int GetFragLimit();
		int GetTimePerMap();
		int GetRoundLimit();
		bool IsRandomMapSelected();

	private:
		KeyValues *m_pSavedData;
		vgui::ComboBox* m_pMapList;
		vgui::CheckButton* m_pCheckButton;
	protected:
		virtual void OnClose();
	};

}