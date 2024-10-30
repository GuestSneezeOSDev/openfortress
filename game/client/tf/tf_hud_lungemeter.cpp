//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_weapon_grapple.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_crowbar.h"
#include "tf_controls.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudLungeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudLungeMeter, EditablePanel );

public:
	CHudLungeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	ContinuousProgressBar *m_pChargeMeter;
};

DECLARE_HUDELEMENT( CHudLungeMeter );

extern ConVar of_zombie_lunge_delay;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudLungeMeter::CHudLungeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudLungeMeter" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "LungeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLungeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudLungeMeter.res" );
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudLungeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	C_TFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	bool bHookWeapon = false;
	if (pWeapon)
	{
		int iWeaponID = pWeapon->GetWeaponID();
		if (iWeaponID == TF_WEAPON_GRAPPLE || iWeaponID == TF_WEAPON_ETERNALSHOTGUN || iWeaponID == TF_WEAPON_LEAD_PIPE)
			bHookWeapon = true;
	}

	if (!pPlayer->m_Shared.IsZombie() && !bHookWeapon)
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLungeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !m_pChargeMeter )
		return;

	CExLabel *pLabel = dynamic_cast<CExLabel *>(FindChildByName("LungeLabel"));

	if (!pLabel)
		return;

	float flProgress = 0.f;
	if ( pPlayer->m_Shared.IsZombie() ) //zombie lunge meter
	{
		pLabel->SetText("#TF_Lunge");

		float flProgress = pPlayer->m_Shared.GetNextLungeTime() <= gpGlobals->curtime ? 1.0f : 1.0f - (pPlayer->m_Shared.GetNextLungeTime() - gpGlobals->curtime) / of_zombie_lunge_delay.GetFloat();
		m_pChargeMeter->SetProgress(flProgress);
	}
	else
	{
		C_TFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
		if (!pWeapon)
			return;

		int iWeaponID = pWeapon->GetWeaponID();

		if (iWeaponID == TF_WEAPON_GRAPPLE)
		{
			pLabel->SetText("Charge");

			C_WeaponGrapple *pHook = dynamic_cast<C_WeaponGrapple *>(pWeapon);
			if (!pHook)
				return;

			flProgress = pHook->GetGrappleCharge();

			//when below 25% charge set color to a not too strong red
			m_pChargeMeter->SetFgColor(flProgress <= 0.25 ? Color(242, 67, 29, 255) : Color(255, 255, 255, 255));
			m_pChargeMeter->SetProgress(flProgress);
		}
		else if (iWeaponID == TF_WEAPON_ETERNALSHOTGUN)
		{
			pLabel->SetText("Hook");

			C_TFEternalShotgun *pHook = dynamic_cast<C_TFEternalShotgun *>(pWeapon);
			if (!pHook)
				return;

			flProgress = pHook->GetGrappleCharge();

			//when below 25% charge set color to a not too strong red
			m_pChargeMeter->SetFgColor(flProgress > 0.f ? Color(255, 153, 0, 255) : Color(255, 255, 255, 255));
			m_pChargeMeter->SetProgress(1.f - flProgress);
		}
		if (iWeaponID == TF_WEAPON_LEAD_PIPE)
		{
			pLabel->SetText("Charge");

			C_TFLeadPipe *pHook = dynamic_cast<C_TFLeadPipe *>(pWeapon);
			if (!pHook)
				return;

			flProgress = pHook->GetCharge();

			//when below 25% charge set color to a not too strong red
			m_pChargeMeter->SetFgColor(flProgress <= 0.5 ? Color(242, 67, 29, 255) : Color(255, 255, 255, 255));
			m_pChargeMeter->SetProgress(flProgress);
		}
	}
}