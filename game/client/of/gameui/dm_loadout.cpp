//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_commandbutton.h"
#include "dm_loadout.h"
#include "of_loadout.h"
#include "of_items_game.h"
#include "of_announcer_schema.h"
#include <convar.h>
#include "vgui_controls/AnimationController.h"
#include "filesystem.h"
#include "activitylist.h"

#include "tier0/dbg.h"

using namespace BaseModUI;
using namespace vgui;

extern ConVar of_tennisball;
extern ConVar of_respawn_particle;
extern ConVar of_announcer_override;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
DMLoadout::DMLoadout(Panel *parent, const char *panelName) : BaseClass(parent, panelName) 
{
	///
	SetProportional( true );	
	///
	m_pCloseButton = new Button( this, "CloseButton", "" );	
	pCosmeticPanel = new EditablePanel( this, "CosmeticPanel" );
	pArsenalPanel = new EditablePanel( this, "ArsenalPanel" );
	pVisualPanel = new EditablePanel( this, "VisualPanel" );
	pParticleList = new CTFScrollablePanelList( pVisualPanel, "ParticleList" );
	pAnnouncerList = new CTFScrollablePanelList( pVisualPanel, "AnnouncerList" );
	
	pPrimaryToggle = new CTFSelectionPanel( pArsenalPanel, "PrimaryToggle" );
	pSecondaryToggle = new CTFSelectionPanel( pArsenalPanel, "SecondaryToggle" );
	pMeleeToggle = new CTFSelectionPanel( pArsenalPanel, "MeleeToggle" );
	
	for( int i = 0; i < 3; i++ )
	{
		pWeaponList[i] = new CTFScrollablePanelList( pArsenalPanel, VarArgs("WeaponList%d", i) );
	}

	m_pItemHeader = new CTFLoadoutHeader( pCosmeticPanel, "ItemHeader" );
	
	m_bControlsLoaded = false;
	m_bInteractive = false;
	m_pSelectedOptions = NULL;
	m_bParsedParticles = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DMLoadout::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	GetAnimationController()->SetScriptFile( GetVPanel(), "scripts/HudAnimations_tf.txt" );
	
	KeyValues *inNewResourceData = new KeyValues("ResourceData");	
	if( !inNewResourceData->LoadFromFile( filesystem, m_ResourceName) )
		return;	
		
	KeyValues *inCosmeticPanel = inNewResourceData->FindKey("CosmeticPanel");
	if( !inCosmeticPanel )
		return;
	
	if( !GetItemSchema() )
		return;

	if( !GetItemSchema()->GetCosmeticCount() )
		return;
	
	m_pItemHeader->ApplySettings(inCosmeticPanel->FindKey("ItemHeader"));
	
	m_pItemHeader->ClearCategoryList();
	for( int i = 0; i < m_pItemCategories.Count(); i++ )
	{
		m_pItemCategories[i]->ClearItemList();
		m_pItemCategories[i]->DeletePanel();
	}
	m_pItemCategories.Purge();
	
	m_pItemHeader->pSelectedHeader = NULL;

	for( int i = 0; i < GetItemSchema()->GetCategoryCount(); i++ )
	{
		char *szRegion = GetItemSchema()->GetCategoryName(i);
		CTFScrollableItemList *pNew = new CTFScrollableItemList( m_pItemHeader->GetParent(), VarArgs("%sList", szRegion) );
		pNew->ApplySettings(inCosmeticPanel->FindKey("ListTemplate"));
		pNew->AddActionSignalTarget(this);
		Q_strncpy(pNew->szCategoryName, szRegion, sizeof(pNew->szCategoryName));
		m_pItemCategories.AddToTail(pNew);
		m_pItemHeader->AddCategory(szRegion);
		if( m_pItemCategories.Count() > 0 )
		{
			if( m_pItemCategories.Count() == 1 )
			{	
				m_pItemHeader->m_hCategories[0].pHeaderItem->SetSelected(true);
				m_pItemHeader->m_hCategories[0].pHeaderItem->OnReleasedUnselected();
			}
			else
			{
				if( m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem )
					m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem->OnReleasedSelected();
			}
		}
	}

	CUtlVector<const char*> hCategories;
	for( int i = 0; i < GetItemSchema()->GetCosmeticCount(); i++ )
	{
		int iID = GetItemSchema()->GetCosmeticInOrder(i);
		COFCosmeticInfo *pCosmetic = GetItemSchema()->GetCosmetic(iID);
		if( !pCosmetic )
			continue;
		
		int iExistingLocation = GetItemSchema()->GetCategoryID( pCosmetic->m_szRegion );
		bool bSelected = false;

		if( GetLoadout() )
		{
			KeyValues *kvCosmetics = GetLoadout()->FindKey("Cosmetics");
			if( kvCosmetics )
			{
				KeyValues *kvMerc = kvCosmetics->FindKey("mercenary");
				if( kvMerc )
				{
					if( iID == kvMerc->GetInt(m_pItemCategories[iExistingLocation]->szCategoryName) )
					{
						bSelected = true;
					}
				}
			}
		}

		m_pItemCategories[iExistingLocation]->AddItem(iID, bSelected);
	}
	
	KeyValues *inVisualPanel = inNewResourceData->FindKey("VisualPanel");
	if( !inVisualPanel )
		return;
	
	if( !m_bParsedParticles )
	{
		m_bParsedParticles = true;
		KeyValues *inParticleList = inVisualPanel->FindKey("ParticleList");
		if( !inParticleList )
			return;	

		if( pParticleList )
		{
			pParticleList->ClearItemList();
			
			pParticleList->ApplySettings( inParticleList );
			
			KeyValues *kvTemp = new KeyValues("Resource");
			
			kvTemp->SetString( "fieldName", "ItemTemplate" );
			kvTemp->SetString( "wide", "50" );
			kvTemp->SetString( "tall", "50" );
			kvTemp->SetString( "autoResize", "0" );
			kvTemp->SetString( "pinCorner", "2" );
			kvTemp->SetString( "visible", "1" );
			kvTemp->SetString( "enabled", "1" );
			kvTemp->SetString( "tabPosition", "0" );
			kvTemp->SetString( "proportionalToParent", "1" );
			kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
			kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
			kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
			kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
			kvTemp->SetString( "command", "of_respawn_particle 0" );
			kvTemp->SetString( "sound_chances", "2" );
			kvTemp->SetString( "pressed_sound", "Player.Spawn" );
			
			KeyValues *kvButtTemp = new KeyValues("Button");
			kvButtTemp->SetString( "wide", "50" );
			kvButtTemp->SetString( "tall", "50" );
			kvButtTemp->SetString( "xpos", "c-25" );
			kvButtTemp->SetString( "ypos", "c-25" );
			kvButtTemp->SetString( "zpos", "10" );
			kvButtTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvModelTemp = new KeyValues("ParticleModel");
			kvModelTemp->SetString( "wide", "50" );
			kvModelTemp->SetString( "tall", "50" );
			kvModelTemp->SetString( "xpos", "c-25" );
			kvModelTemp->SetString( "ypos", "c-25" );
			kvModelTemp->SetString( "zpos", "6" );
			kvModelTemp->SetString( "fov", "25" );
			kvModelTemp->SetString( "render_texture", "0" );
			kvModelTemp->SetString( "allow_rot", "1" );
			kvModelTemp->SetString( "use_particle", "1" );
			kvModelTemp->SetString( "particle_loop", "1" );
			kvModelTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvModelModelTemp = new KeyValues("model");
			kvModelModelTemp->SetString( "modelname", "models/empty.mdl" );
			kvModelModelTemp->SetString( "force_pos", "1" );
			kvModelModelTemp->SetString( "skin"	,"4" );
			kvModelModelTemp->SetString( "origin_z"	,"-40" );
			kvModelModelTemp->SetString( "origin_x", "450");
			
			KeyValues *kvModelAnimTemp = new KeyValues("animation");
			kvModelAnimTemp->SetString( "name", "PRIMARY" );
			kvModelAnimTemp->SetString( "activity", "ACT_MERC_LOADOUT" );
			kvModelAnimTemp->SetString( "default", "1" );
			
			kvModelModelTemp->AddSubKey( kvModelAnimTemp );
			kvModelTemp->AddSubKey( kvModelModelTemp );
			
			kvTemp->AddSubKey( kvButtTemp );
			
			for( int i = 1; i <= GetItemSchema()->GetRespawnParticleCount(); i++ )
			{
				CTFCommandButton *pTemp = new CTFCommandButton( pParticleList, "Temp" );
				kvTemp->SetString( "command", VarArgs( "of_respawn_particle %d", i ) );
				kvTemp->SetString( "convref", "of_respawn_particle" );
				kvTemp->SetString( "targetval", VarArgs( "%d", i ) );
				pTemp->ApplySettings( kvTemp );
				
				pParticleList->AddItem( pTemp );

				if( of_respawn_particle.GetInt() == i )
				{
					pTemp->SetSelected(true);
					pParticleList->pSelectedItem = pTemp;
				}
				
				COFParticleInfo *pParticle = GetItemSchema()->GetRespawnParticle( i );
				if( pParticle )
				{	
					kvModelTemp->SetFloat("particle_loop_time", pParticle->m_flLoopTime );
					kvModelTemp->SetFloat("particle_z_offset", pParticle->m_flParticleZOffset );
					CTFModelPanel *pTempMDL = new CTFModelPanel( pTemp, "ParticleModel" );
					pTempMDL->ApplySettings( kvModelTemp );
					
					char pEffectName[32];
					pEffectName[0] = '\0';
					Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_%02d", i );
					if ( pEffectName[0] != '\0' )
						Q_strncpy( pTempMDL->szLoopingParticle, pEffectName, sizeof(pTempMDL->szLoopingParticle) );
					
					pTempMDL->Update();
				}
				else
				{
					kvModelTemp->deleteThis();
				}
			}
		}
	}
	modelinfo->FindOrLoadModel( "models/player/mercenary.mdl" );
	
	KeyValues *inAnnouncerList = inVisualPanel->FindKey("AnnouncerList");
	if( !inAnnouncerList )
		return;	

	if( pAnnouncerList )
	{
		pAnnouncerList->ClearItemList();
		
		pAnnouncerList->ApplySettings( inAnnouncerList );
		
		KeyValues *kvTemp = new KeyValues("Resource");
		
		kvTemp->SetString( "fieldName", "ItemTemplate" );
		kvTemp->SetString( "wide", "50" );
		kvTemp->SetString( "tall", "50" );
		kvTemp->SetString( "autoResize", "0" );
		kvTemp->SetString( "pinCorner", "2" );
		kvTemp->SetString( "visible", "1" );
		kvTemp->SetString( "enabled", "1" );
		kvTemp->SetString( "tabPosition", "0" );
		kvTemp->SetString( "proportionalToParent", "1" );
		kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
		kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
		kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
		kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
		kvTemp->SetString( "command", "of_announcer_override \"\"" );
		
		KeyValues *kvButtTemp = new KeyValues("Button");
		kvButtTemp->SetString( "wide", "50" );
		kvButtTemp->SetString( "tall", "50" );
		kvButtTemp->SetString( "xpos", "c-25" );
		kvButtTemp->SetString( "ypos", "c-25" );
		kvButtTemp->SetString( "zpos", "10" );
		kvButtTemp->SetString( "proportionalToParent", "1" );
		
		KeyValues *kvImageTemp = new KeyValues("AnnouncerImage");
		kvImageTemp->SetString( "wide", "50" );
		kvImageTemp->SetString( "tall", "50" );
		kvImageTemp->SetString( "xpos", "c-25" );
		kvImageTemp->SetString( "ypos", "c-25" );
		kvImageTemp->SetString( "zpos", "6" );
		kvImageTemp->SetString( "scaleImage", "1" );
		kvImageTemp->SetString( "proportionalToParent", "1" );
		
		kvTemp->AddSubKey( kvButtTemp );
		
		COFAnnouncerSchema* pSupport = GetAnnouncers();
		
		if( pSupport )
		{
			FOR_EACH_VEC( pSupport->m_HandleOrder, i )
			{
				unsigned short handle = pSupport->m_HandleOrder[i];
				const char *szName = pSupport->m_Announcers.GetElementName(handle);
				CTFCommandButton *pTemp = new CTFCommandButton( pAnnouncerList, "Temp" );
				kvTemp->SetString( "command", VarArgs( "of_announcer_override %s", szName ) );
				kvTemp->SetString( "convref", "of_announcer_override" );
				kvTemp->SetString( "targetval", VarArgs( "%s", szName ) );
				pTemp->ApplySettings( kvTemp );
				pTemp->AddOnPressSound( VarArgs( "%s.Deathmatch", szName ) );
				pTemp->AddOnPressSound( VarArgs( "%s.Impressive", szName ) );
				pTemp->AddOnPressSound( VarArgs( "%s.Excellent", szName ) );
				pTemp->AddOnPressSound( VarArgs( "%s.Dominating", szName ) );
				pTemp->AddOnPressSound( VarArgs( "%s.DMRoundStart", szName ) );
				pTemp->AddOnPressSound( VarArgs( "%s.DMRoundPrepare", szName ) );
			
				kvImageTemp->SetString( "image", VarArgs( "../backpack/announcers/%s", szName ) );
				CTFImagePanel *pTempImage = new CTFImagePanel( pTemp, "AnnouncerImage" );
				pTempImage->ApplySettings( kvImageTemp );
			
				pAnnouncerList->AddItem( pTemp );

				if( !Q_stricmp( of_announcer_override.GetString(), szName ) )
				{
					pTemp->SetSelected(true);
					pAnnouncerList->pSelectedItem = pTemp;
				}
			}
		}
	}
	
	KeyValues *inArsenalPanel = inNewResourceData->FindKey("ArsenalPanel");
	if( !inArsenalPanel )
		return;	
	
	KeyValues *inWeaponList = inArsenalPanel->FindKey("WeaponList");
	if( !inWeaponList )
		return;	

	for( int i = 0; i < 3; i++ )
	{
		if( pWeaponList[i] )
		{
			pWeaponList[i]->ClearItemList();
			
			inWeaponList->SetString("fieldName",VarArgs("WeaponList%d", i));
			
			inWeaponList->SetInt( "visible", !i );
			
			pWeaponList[i]->ApplySettings( inWeaponList );
			
			KeyValues *kvTemp = new KeyValues("Resource");
			
			kvTemp->SetString( "fieldName", "ItemTemplate" );
			kvTemp->SetString( "wide", "82" );
			kvTemp->SetString( "tall", "60" );
			kvTemp->SetString( "autoResize", "0" );
			kvTemp->SetString( "pinCorner", "2" );
			kvTemp->SetString( "visible", "1" );
			kvTemp->SetString( "enabled", "1" );
			kvTemp->SetString( "tabPosition", "0" );
			kvTemp->SetString( "proportionalToParent", "1" );
			kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
			kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
			kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
			kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
			kvTemp->SetString( "command", "loadout_equip weapons mercenary \"\"" );

			KeyValues *kvButtTemp = new KeyValues("Button");
			kvButtTemp->SetString( "wide", "50" );
			kvButtTemp->SetString( "tall", "50" );
			kvButtTemp->SetString( "xpos", "c-25" );
			kvButtTemp->SetString( "ypos", "c-25" );
			kvButtTemp->SetString( "zpos", "10" );
			kvButtTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvImageTemp = new KeyValues("WeaponImage");
			kvImageTemp->SetString( "wide", "100" );
			kvImageTemp->SetString( "tall", "50" );
			kvImageTemp->SetString( "xpos", "c-41" );
			kvImageTemp->SetString( "ypos", "c-25" );
			kvImageTemp->SetString( "zpos", "6" );
			kvImageTemp->SetString( "scaleImage", "1" );
			kvImageTemp->SetString( "proportionalToParent", "1" );
			
			kvTemp->AddSubKey( kvButtTemp );
			
			for( int y = 0; y < GetItemSchema()->GetWeaponCount(); y++ )
			{
				COFSchemaWeaponInfo *pWpnInfo = GetItemSchema()->GetWeapon(y);
				if( !pWpnInfo )
					continue;

				if( !pWpnInfo->m_bShowInLoadout )
					continue;

				if( pWpnInfo->m_iWeaponSlot[TF_CLASS_MERCENARY] != -1 && pWpnInfo->m_iWeaponSlot[TF_CLASS_MERCENARY] != i + 1 )
					continue;

				// Don't let melee slot have non melee stuff
				if( i == 2 && pWpnInfo->m_iWeaponSlot[TF_CLASS_MERCENARY] != 3 )
					continue;

				CTFCommandButton *pTemp = new CTFCommandButton( pWeaponList[i], "Temp" );
				kvTemp->SetString( "fieldName", pWpnInfo->m_szWeaponName );

				kvTemp->SetString( "command", VarArgs( "loadout_equip weapons mercenary %s %d", pWpnInfo->m_szWeaponName, i + 1 ) );
				pTemp->ApplySettings( kvTemp );
				
				KeyValues *pWeapons = GetLoadout()->FindKey("Weapons");
				if( pWeapons )
				{
					KeyValues *pMercenary = pWeapons->FindKey("mercenary");
					if( pMercenary )
					{
						if( !Q_stricmp(pMercenary->GetString(VarArgs("%d", i+1 ) ), pWpnInfo->m_szWeaponName) )
						{
							pTemp->SetSelected(true);

							switch( i )
							{
								case 0:
									pPrimaryToggle->pImage->SetImage( pWpnInfo->m_szBackpackIcon ? pWpnInfo->m_szBackpackIcon : "../backpack/blocked" );
									break;
								case 1:
									pSecondaryToggle->pImage->SetImage( pWpnInfo->m_szBackpackIcon ? pWpnInfo->m_szBackpackIcon : "../backpack/blocked" );
									break;
								case 2:
									pMeleeToggle->pImage->SetImage( pWpnInfo->m_szBackpackIcon ? pWpnInfo->m_szBackpackIcon : "../backpack/blocked" );
									break;								
							}
						}
					}
				}
				kvImageTemp->SetString( "image", pWpnInfo->m_szBackpackIcon ? pWpnInfo->m_szBackpackIcon : "../backpack/blocked" );
				CTFImagePanel *pTempImage = new CTFImagePanel( pTemp, "WeaponImage" );
				pTempImage->ApplySettings( kvImageTemp );
				
				pWeaponList[i]->AddItem( pTemp );
			}
		}
	}
}

//=============================================================================
void DMLoadout::OnCommand(const char *command)
{
	if (Q_stricmp("Back", command) == 0)
	{
		// OnApplyChanges();
		//OnKeyCodePressed(KEY_XBUTTON_B);
		engine->ClientCmd_Unrestricted("gameui_allowescape\n");
		Hide();
	}
	else if (Q_stricmp("Cancel", command) == 0)
	{
		//OnKeyCodePressed(KEY_XBUTTON_B);
		engine->ClientCmd_Unrestricted("gameui_allowescape\n");
		Hide();
	}
	else if (Q_strncmp(command, "loadout_equip", strlen("loadout_equip")) == 0)
	{
		GetClassModel()->SetLoadoutCosmetics();
	}
	else if (Q_strncmp(command, "loadout_unequip", strlen("loadout_unequip")) == 0)
	{
		GetClassModel()->SetLoadoutCosmetics();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void DMLoadout::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pClassModel = dynamic_cast<vgui::DMModelPanel*>(FindChildByName("classmodelpanel"));
	m_bControlsLoaded = true;
	
	// required for new style
	SetPaintBackgroundEnabled(true);
	// SetupAsDialogStyle();
	
	if( !pVisualPanel )
		return;
}

void DMLoadout::PerformLayout()
{	
	BaseClass::PerformLayout();

	GetAnimationController()->StartAnimationSequence( this, "LoadoutPopup" );
	
//	GetAnimationController()->StartAnimationSequence("LoadoutPopup");
}

#define QUICK_CVAR(x) ConVar x(#x, "0", FCVAR_NONE);
QUICK_CVAR(of_bodygroup)
QUICK_CVAR(of_bodygroup_value)

void DMLoadout::PaintBackground()
{
	BaseClass::PaintBackground();

}

void DMLoadout::SelectWeapon( int iSlot, const char *szWeapon, bool bChangeSelection )
{
	vgui::Panel *pPanel = NULL;
	switch( iSlot )
	{
		case 1:
		pPanel = GetArsenalPanel()->FindChildByName("PrimaryToggle");
		break;
		case 2:
		pPanel = GetArsenalPanel()->FindChildByName("SecondaryToggle");
		break;
		case 3:
		pPanel = GetArsenalPanel()->FindChildByName("MeleeToggle");
		break;
	}
	
	if( pPanel )
	{
		CTFImagePanel *pImage = dynamic_cast<CTFImagePanel*>( pPanel->FindChildByName("Image"));
		if( pImage )
		{
			COFSchemaWeaponInfo *pWpnInfo = GetItemSchema()->GetWeapon(szWeapon);
			if( pWpnInfo )
			{
				pImage->SetImage( pWpnInfo->m_szBackpackIcon ? pWpnInfo->m_szBackpackIcon : "../backpack/blocked" );
				FileWeaponInfo_t *pWpnData = GetFileWeaponInfoFromHandle( pWpnInfo->m_nWeaponHandle );
				if( pWpnData )
				{
					int iWeaponAnim = pWpnInfo->m_iLoadoutAnim;
					const char *szWeaponModel = iWeaponAnim ? pWpnData->szWorldModel : "models/weapons/w_models/w_supershotgun.mdl";
					
					GetClassModel()->SetWeaponModel(szWeaponModel, iWeaponAnim);

					if( !engine->IsInGame() )
					{
						CBaseModFrame* mainMenu = CBaseModPanel::GetSingleton().GetWindow(WT_MAINMENU);
						vgui::DMModelPanel* pClassModel = dynamic_cast<vgui::DMModelPanel*>(mainMenu->FindChildByName("classmodelpanel"));
						if (pClassModel)
						{
							pClassModel->SetWeaponModel(szWeaponModel, iWeaponAnim);
						}
					}
				}
			}
		}
	}
	
	if( bChangeSelection )
	{
		CTFCommandButton *pWeapon = dynamic_cast<CTFCommandButton*>(pWeaponList[iSlot-1]->FindChildByName(szWeapon));
		if( pWeapon )
		{
			pWeapon->SetSelected(true);
		}
	}
}