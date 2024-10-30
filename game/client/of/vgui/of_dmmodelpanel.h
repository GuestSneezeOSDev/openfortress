//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_DMMODELPANEL_H
#define OF_DMMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "of_modelpanel.h"

namespace vgui
{
	class DMModelPanel : public CTFModelPanel
	{
	private:
		DECLARE_CLASS_SIMPLE(DMModelPanel, CTFModelPanel);

	public:
		DMModelPanel(Panel *parent, const char *panelName);

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void ApplySettings(KeyValues* inResourceData);
		virtual void PerformLayout();
		virtual void PaintBackground();

		void SetLoadoutCosmetics();
		void SetCosmetic(int iCosmeticID, bool bSelected);
		void SetWeaponModel( const char *szWeapon, int iAnim );

		CUtlVector<float> m_flCosmetics;
		bool m_bUpdateCosmetics;
		int m_iCurrentParticle;

	private:
		int m_iTennisball;
		char szWeaponModel[128];
		int iWeaponAnim;
	};
}

#endif // OF_LOADOUT_H
