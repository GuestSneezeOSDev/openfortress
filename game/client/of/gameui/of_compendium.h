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

	class COFLoreCompendium : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE(COFLoreCompendium, CBaseModFrame);

	public:
		COFLoreCompendium(Panel *parent, const char *panelName);
		~COFLoreCompendium();

		virtual void Activate() override;

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void ApplySettings( KeyValues *pResource ) override;
		void OnCommand(const char *command);

		void SetPage( int iPage );
		int GetPage() { return m_iCurrentPage; }

	private:
		EditablePanel *m_pContainer;

		int m_iCurrentPage;
		int m_iPageCount;
	};

}