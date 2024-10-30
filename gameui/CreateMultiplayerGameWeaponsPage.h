//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMEWEAPONSPAGE_H
#define CREATEMULTIPLAYERGAMEWEAPONSPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

//-----------------------------------------------------------------------------
// Purpose: Advanced Weapons properties page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameWeaponsPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CCreateMultiplayerGameWeaponsPage, vgui::PropertyPage);

public:
	CCreateMultiplayerGameWeaponsPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameWeaponsPage();

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


#endif // CREATEMULTIPLAYERGAMEWEAPONSPAGE_H
