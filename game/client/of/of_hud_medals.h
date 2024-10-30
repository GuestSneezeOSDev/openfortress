#ifndef TF_HUD_MEDALS_H
#define TF_HUD_MEDALS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose:  Displays Deathmatch Medals
//-----------------------------------------------------------------------------

enum { FIRSTBLOOD,	PERFECT,		IMPRESSIVE,		PERFORATED,		HUMILIATION,	KAMIKAZE,	MIDAIR,				HEADSHOT,		EXCELLENT,		MULTIKILL,	ULTRAKILL,
	   HOLYSHIT,	KILLINGSPREE,	RAMPAGE,		DOMINATING,		UNSTOPPABLE,	GODLIKE,	POWERUPMASSACRE,	SHOWSTOPPER,	PARTYBREAKER,	DENIED,					
	   MEDAL_COUNT };

//these two are needed in the scoreboard to display medals count
extern int g_medalsCounter[DENIED + 1];
extern const char *medalNames[DENIED + 1];

class CTFHudMedals : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFHudMedals, EditablePanel);

public:

	CTFHudMedals(const char *pElementName);

	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void FireGameEvent(IGameEvent *event);
	virtual void Reset();
	virtual bool ShouldDraw(void);

protected:

	virtual void OnThink(void);

private:

	virtual void AddMedal(int medalIndex);

	bool bDied;
	float flDrawTime;

	struct  medal_info
	{
		const char *medal_name;
		const char *medal_sound;
	};
	CUtlVector<medal_info> medalsQueue;

	static const char *medalPaths[DENIED + 1];

	ImagePanel *m_pMedalImage;
};

#endif	// TF_HUD_MEDALS_H