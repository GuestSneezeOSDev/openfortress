//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_DOM_H
#define TF_HUD_DOM_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "of_imageprogressbar.h"
#include "tf_controls.h"

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudDOM : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudDOM, vgui::EditablePanel );

public:

	CTFHudDOM( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateDOMLabel( bool bScore );

private:

	float							m_flNextThink;

	int								m_nScore;

	CTFImageProgressBar* m_pProgressRed;
	CTFImageProgressBar* m_pProgressBlu;
	CExLabel* m_pScore;
	CExLabel* m_pScoreShadow;
};



#endif	// TF_HUD_DOM_H