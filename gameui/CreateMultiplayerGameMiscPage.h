//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMEMISCPAGE_H
#define CREATEMULTIPLAYERGAMEMISCPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

//-----------------------------------------------------------------------------
// Purpose: Advanced Misc properties page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameMiscPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CCreateMultiplayerGameMiscPage, vgui::PropertyPage);

public:
	CCreateMultiplayerGameMiscPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameMiscPage();

protected:
	virtual void OnApplyChanges();

private:
	const char *GetValue(const char *cvarName, const char *defaultValue);
	void LoadGameOptionsList();
	void GatherCurrentValues();

	CDescription *m_pDescription;
	mpcontrol_t *m_pList;
	CPanelListPanel *m_pOptionsList;
};


#endif // CREATEMULTIPLAYERGAMEMISCPAGE_H
