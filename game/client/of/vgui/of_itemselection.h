//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_ITEMSELECTION_H
#define OF_ITEMSELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "of_editablebutton.h"

struct ItemTemplate_t
{
	char wide[8];
	char tall[8];
	char border_idle[64];
	char border_hover[64];
	char border_pressed[64];
	char border_selected[64];
	
	char button_wide[8];
	char button_tall[8];
	char button_xpos[8];
	char button_ypos[8];
	char button_zpos[8];
	
	char extra_wide[8];
	char extra_tall[8];
	char extra_xpos[8];
	char extra_ypos[8];
	char extra_zpos[8];
};

class CTFItemSelection : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFItemSelection, CTFEditableButton );

public:
	CTFItemSelection(vgui::Panel *parent, const char *panelName, const int iID);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();	
	virtual void Paint();
	void	SetItemID( int iID );
	void	SetSkin( int iS ){ iSkin = iS; };
	void	SetSkinCount( int iCount ){ iSkinCount = iCount; };
	virtual void SetSelected( bool bSelected );
	virtual void OnCommand(const char *command);

public:
	int		iItemID;
	int		iSkin;
	int		iSkinCount;
	char	szCommand[64];
	bool	bParsedBPImage;
	
	CTFImagePanel	*pItemImage;
	vgui::Button	*pSkin;
};



#endif // OF_LOADOUT_H
