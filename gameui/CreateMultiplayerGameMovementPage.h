//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMEMOVEMENTPAGE_H
#define CREATEMULTIPLAYERGAMEMOVEMENTPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

//-----------------------------------------------------------------------------
// Purpose: Advanced Movement properties page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameMovementPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CCreateMultiplayerGameMovementPage, vgui::PropertyPage);

public:
	CCreateMultiplayerGameMovementPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameMovementPage();

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


#endif // CREATEMULTIPLAYERGAMEMOVEMENTPAGE_H
