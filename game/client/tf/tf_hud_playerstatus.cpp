//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "tf_hud_playerstatus.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "basemodelpanel.h"
#include "filesystem.h"

using namespace vgui;


extern ConVar tf_max_health_boost;
extern ConVar of_tennisball;
extern ConVar of_armor;

static KeyValues *inRenderModels = new KeyValues("Models");

static const char *g_szEmpty[] = 
{ 
	"../hud/empty"
};

void HUDPlayerModelCallback(IConVar *var, const char *pOldString, float flOldValue)
{
	IGameEvent *event = gameeventmanager->CreateEvent("refresh_hud_model");
	if( event )
		gameeventmanager->FireEventClientSide(event);
}

ConVar cl_hud_playerclass_use_playermodel("cl_hud_playerclass_use_playermodel", "0", FCVAR_USERINFO | FCVAR_ARCHIVE ,"Use player model in player class HUD.", HUDPlayerModelCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerClass::CTFHudPlayerClass( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pClassImage = new CTFClassImage( this, "PlayerStatusClassImage" );
	m_pClassImageColorless = new CTFClassImage( this, "PlayerStatusClassImageColor" );
	m_pSpyImage = new CTFImagePanel( this, "PlayerStatusSpyImage" );
	m_pSpyOutlineImage = new CTFImagePanel( this, "PlayerStatusSpyOutlineImage" );

	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nModifiers = TF_CLASSMOD_NONE;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_flNextThink = 0.0f;
	m_nWeaponAttachment = -1;
	m_nHudRef = -1;

	ListenForGameEvent( "localplayer_changedisguise" );
	ListenForGameEvent( "weapon_switched" );
	//ListenForGameEvent( "refresh_hud_model" );
	//ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "localplayer_changecosmetics" );
	
	inRenderModels->LoadFromFile(filesystem, "resource/ui/winpaneldm_objects.txt");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudSpyDisguiseHide" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudPlayerClass.res" );
	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nModifiers = TF_CLASSMOD_NONE;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_flNextThink = 0.0f;
	m_nCloakLevel = 0;
	
	CModelPanel *pModel = dynamic_cast<CModelPanel*>(FindChildByName("classmodelpanel"));
	if( pModel && m_nWeaponAttachment != -1 )
		pModel->RemoveAttachment(m_nWeaponAttachment);
	
	m_nWeaponAttachment = -1;
	m_nHudRef = -1;

	BaseClass::ApplySchemeSettings( pScheme );
}

extern void ApplyCosmeticsToModelPanel(CModelPanel *pModel, int iPlayerIndex, int iTeamOverride = -1 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::OnThink()
{
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		bool bTeamChange = false;

		if ( pPlayer )
		{
			// Look, i know that this is unoptimized as hell, but it works for now, and I'm really not in the mood
			CModelPanel *pModel = dynamic_cast<CModelPanel*>(FindChildByName("classmodelpanel"));
			if( pModel )
			{
				pModel->SetVisible(cl_hud_playerclass_use_playermodel.GetBool());
				m_pClassImage->SetVisible(!cl_hud_playerclass_use_playermodel.GetBool());
				m_pClassImageColorless->SetVisible(!cl_hud_playerclass_use_playermodel.GetBool());
				if( pModel->IsVisible() && pModel->m_hModel.Get() )
				{
					// We update these in real time because of inputs like SetCustomModel
					// Since all of these are just setting numbers it shouldn't be too hard on processing power
					C_TF_PlayerResource *g_TFPR = GetTFPlayerResource();
					if( g_TFPR )
					{
						pModel->m_hModel.Get()->SetModelColor(g_TFPR->GetPlayerColorVector(pPlayer->entindex()));
					}
					pModel->m_hModel.Get()->m_nSkin = pPlayer->GetSkin();
					pModel->m_hModel.Get()->SetModelIndex(pPlayer->GetModelIndex());
					pModel->m_hModel.Get()->m_nBody = pPlayer->GetBody();
					if( cl_hud_playerclass_use_playermodel.GetInt() == 3 )
					{
						pModel->m_hModel.Get()->SetSequence(pPlayer->GetSequence());

						int iPoseParams = min(pPlayer->GetModelPtr()->GetNumPoseParameters(), pModel->m_hModel.Get()->GetModelPtr()->GetNumPoseParameters());
						for (int i = 0; i < iPoseParams; i++)
						{
							pModel->m_hModel.Get()->SetPoseParameter(i, pPlayer->GetPoseParameter(i));
						}
					}
				}
			}
			// set our background colors
			if ( m_nTeam != pPlayer->GetTeamNumber() )
			{
				bTeamChange = true;
				m_nTeam = pPlayer->GetTeamNumber();
			}
			
			if( of_tennisball.GetInt() != m_nTennisball )
			{
				m_nTennisball = of_tennisball.GetInt();
				bTeamChange = true;
			}
			int nCloakLevel = 0;
			bool bCloakChange = false;
			float flInvis = pPlayer->GetPercentInvisible();

			if ( flInvis > 0.9 )
			{
				nCloakLevel = 2;
			}
			else if ( flInvis > 0.1 )
			{
				nCloakLevel = 1;
			}

			if ( nCloakLevel != m_nCloakLevel )
			{
				m_nCloakLevel = nCloakLevel;
				bCloakChange = true;
			}

			// set our class image
			if ( m_nClass != pPlayer->GetPlayerClass()->GetClassIndex() || bTeamChange || bCloakChange 
				|| m_nModifiers != pPlayer->GetPlayerClass()->GetModifier() 
				|| ( m_nClass == TF_CLASS_SPY && m_nDisguiseClass != pPlayer->m_Shared.GetDisguiseClass() ) 
				|| ( m_nClass == TF_CLASS_SPY && m_nDisguiseTeam != pPlayer->m_Shared.GetDisguiseTeam() ) )
			{
				m_nClass = pPlayer->GetPlayerClass()->GetClassIndex();
				m_nModifiers = pPlayer->GetPlayerClass()->GetModifier();
				TFPlayerClassData_t *pClassData = pPlayer->GetPlayerClass()->GetData();

				SetupClassModel();

				if ( m_nClass == TF_CLASS_SPY && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
				{
					if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
					{
						m_nDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();
						m_nDisguiseClass = pPlayer->m_Shared.GetDisguiseClass();
					}
				}
				else
				{
					m_nDisguiseTeam = TEAM_UNASSIGNED;
					m_nDisguiseClass = TF_CLASS_UNDEFINED;
				}

				if ( m_pClassImage && m_pSpyImage )
				{
					int iCloakState = 0;
					if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
					{
						iCloakState = m_nCloakLevel;
					}

					if ( m_nDisguiseTeam != TEAM_UNASSIGNED || m_nDisguiseClass != TF_CLASS_UNDEFINED )
					{
						m_pSpyImage->SetVisible( true );
						CTFPlayer *pDisguiseTarget = ToTFPlayer( pPlayer->m_Shared.GetDisguiseTarget() );
						if( pDisguiseTarget )
							pClassData = pDisguiseTarget->GetPlayerClass()->GetData();
						else
							pClassData = GetPlayerClassData( m_nDisguiseClass, pPlayer->m_Shared.GetDisguiseClassMod() );

						m_pClassImage->SetClass( m_nDisguiseTeam, pClassData, iCloakState );
						m_pClassImageColorless->SetClassColorless( m_nDisguiseTeam, pClassData, iCloakState );
					}
					else
					{
						m_pSpyImage->SetVisible( false );
						m_pClassImage->SetClass( m_nTeam, pClassData, iCloakState );
						m_pClassImageColorless->SetClassColorless( m_nTeam, pClassData, iCloakState );
					}
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}

void CTFHudPlayerClass::SetupClassModel(void)
{
	if( !cl_hud_playerclass_use_playermodel.GetBool() )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	CModelPanel *pModel = dynamic_cast<CModelPanel*>(FindChildByName("classmodelpanel"));
	if( !(pPlayer && pModel) )
		return;

	if( !pModel->m_hModel.Get() )
	{
		pModel->SetPanelDirty();
		pModel->UpdateModel();
	}

	if( !pModel->m_hModel.Get() )
		return;

	if( m_nWeaponAttachment != -1 )
	{
		pModel->RemoveAttachment(m_nWeaponAttachment);
		m_nWeaponAttachment = -1;
	}
	pModel->PurgeAttachedModels();
	pModel->PurgeAttachedModelsInfo();
	ApplyCosmeticsToModelPanel(pModel, pPlayer->entindex(), pPlayer->GetTeamNumber());
	pModel->SetPanelDirty();
	pModel->UpdateModel();
	
	// Set this early here so we get the proper HudRef sequence
	pModel->m_hModel.Get()->SetModelIndex(pPlayer->GetModelIndex());
	pModel->m_hModel.Get()->m_nBody = pPlayer->GetBody();

	m_nHudRef = pPlayer->LookupSequence("hud_ref");

	int iModelMode = cl_hud_playerclass_use_playermodel.GetInt();
	if( m_nHudRef == -1 )
		iModelMode = 1;

	switch( iModelMode )
	{
		default:
		case 1:
			if( pPlayer->GetActiveWeapon() )
			{
				int iTemp;
				m_nWeaponAttachment = pModel->QuickAddAttachment(pPlayer->GetActiveWeapon()->GetWorldModel(), pPlayer->GetActiveWeapon()->GetSkin());
				pModel->m_hModel.Get()->SetSequence(pPlayer->SelectWeightedSequence(pPlayer->GetActiveWeapon()->ActivityList(iTemp)[0].weaponAct));
			}
			break;
		case 2:
			const char *szWeaponModel = NULL;
			szWeaponModel = inRenderModels->GetString(VarArgs("%s_render_weapon",g_aPlayerClassNames_NonLocalized[pPlayer->GetPlayerClass()->GetClassIndex()]), 0 );
			if ( !szWeaponModel )
				break;
			if( Q_strlen(szWeaponModel) > 4 )
				m_nWeaponAttachment = pModel->QuickAddAttachment(szWeaponModel, pPlayer->GetTeamNumber() - 2);
			pModel->m_hModel.Get()->SetSequence(m_nHudRef);
			
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::FireGameEvent( IGameEvent * event )
{
	if( FStrEq( "localplayer_changedisguise", event->GetName() ) )
	{
		if ( m_pSpyImage && m_pSpyOutlineImage )
		{
			bool bFadeIn = event->GetBool( "disguised", false );

			if ( bFadeIn )
			{
				m_pSpyImage->SetAlpha( 0 );
			}
			else
			{
				m_pSpyImage->SetAlpha( 255 );
			}

			m_pSpyOutlineImage->SetAlpha( 0 );
			
			m_pSpyImage->SetVisible( true );
			m_pSpyOutlineImage->SetVisible( true );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( bFadeIn ? "HudSpyDisguiseFadeIn" : "HudSpyDisguiseFadeOut" );
		}
	}
	else if( FStrEq("weapon_switched", event->GetName()) )
	{
		if( cl_hud_playerclass_use_playermodel.GetInt() != 1 && cl_hud_playerclass_use_playermodel.GetInt() != 3 && m_nHudRef != -1 )
			return;

		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		CModelPanel *pModel = dynamic_cast<CModelPanel*>(FindChildByName("classmodelpanel"));

		if( pPlayer && pModel && pModel->m_hModel.Get() )
		{
			if( pPlayer->GetActiveWeapon() )
			{
				if( m_nWeaponAttachment != -1 )
					pModel->RemoveAttachment(m_nWeaponAttachment);

				int iTemp;
				m_nWeaponAttachment = pModel->QuickAddAttachment(pPlayer->GetActiveWeapon()->GetWorldModel(), pPlayer->GetActiveWeapon()->GetSkin());
				pModel->m_hModel.Get()->SetSequence(pPlayer->SelectWeightedSequence(pPlayer->GetActiveWeapon()->ActivityList(iTemp)[0].weaponAct));
			}
		}
	}
	else if( FStrEq("refresh_hud_model", event->GetName()) || FStrEq("localplayer_changeclass", event->GetName()) )
	{
		SetupClassModel();
	}
	else if( FStrEq("localplayer_changecosmetics", event->GetName()) )
	{
		SetupClassModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHealthPanel::CTFHealthPanel( Panel *parent, const char *name ) : CTFImagePanel( parent, name )
{
	m_flHealth = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/health_color" );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/health_color", true, false );

	m_iDeadMaterialIndex = surface()->DrawGetTextureId( "hud/health_dead" );
	if ( m_iDeadMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iDeadMaterialIndex = surface()->CreateNewTextureID();	
	}
	surface()->DrawSetTextureFile( m_iDeadMaterialIndex, "hud/health_dead", true, false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealthPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 0.0f;
	float uv2 = 1.0f;
	int xpos = 0, ypos = 0;

	if ( m_flHealth <= 0 )
	{
		// Draw the dead material
		surface()->DrawSetTexture( m_iDeadMaterialIndex );
		
		vert[0].Init( Vector2D( xpos, ypos ), Vector2D( uv1, uv1 ) );
		vert[1].Init( Vector2D( xpos + w, ypos ), Vector2D( uv2, uv1 ) );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), Vector2D( uv2, uv2 ) );				
		vert[3].Init( Vector2D( xpos, ypos + h ), Vector2D( uv1, uv2 ) );

		surface()->DrawSetColor( Color(255,255,255,255) );
	}
	else
	{
/*
		float flDamageX = w * ( m_flHealth );

		// blend in the red "damage" part
		surface()->DrawSetTexture( m_iMaterialIndex );


//		11------------21
//		|             |
//		|             |
//		|             |
//		|             |
//		12------------22


		Vector2D uv11( uv1, uv1 );
		Vector2D uv21( uv2 - ( 1.0f - m_flHealth ), uv1 );
		Vector2D uv22( uv2 - ( 1.0f - m_flHealth ), uv2 );
		Vector2D uv12( uv1, uv2 );

		vert[0].Init( Vector2D( xpos, ypos ), uv11 );
		vert[1].Init( Vector2D( flDamageX, ypos ), uv21 );
		vert[2].Init( Vector2D( flDamageX, ypos + h ), uv22 );				
		vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );
		*/

		// UP/DOWN
		float flDamageY = h * ( 1.0f - m_flHealth );
		
		// blend in the red "damage" part
		surface()->DrawSetTexture( m_iMaterialIndex );

		Vector2D uv11( uv1, uv2 - m_flHealth );
		Vector2D uv21( uv2, uv2 - m_flHealth );
		Vector2D uv22( uv2, uv2 );
		Vector2D uv12( uv1, uv2 );

		vert[0].Init( Vector2D( xpos, flDamageY ), uv11 );
		vert[1].Init( Vector2D( xpos + w, flDamageY ), uv21 );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
		vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

		surface()->DrawSetColor( GetFgColor() );
	}

	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMegahealPanel::CTFMegahealPanel( Panel *parent, const char *name ) : CTFImagePanel( parent, name )
{
	m_flHealth = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/megahealth_color" );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/megahealth_color", true, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMegahealPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 0.0f;
	float uv2 = 1.0f;
	int xpos = 0, ypos = 0;

	float flDamageY = h * ( 1.0f - m_flHealth );

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );

	Vector2D uv11( uv1, uv2 - m_flHealth );
	Vector2D uv21( uv2, uv2 - m_flHealth );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	vert[0].Init( Vector2D( xpos, flDamageY ), uv11 );
	vert[1].Init( Vector2D( xpos + w, flDamageY ), uv21 );
	vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
	vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

	surface()->DrawSetColor( GetFgColor() );

	surface()->DrawTexturedPolygon( 4, vert );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFArmorPanel::CTFArmorPanel(Panel *parent, const char *name) : vgui::Panel(parent, name)
{
	m_flArmor = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId("hud/armor_light");
	if (m_iMaterialIndex == -1) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}

	surface()->DrawSetTextureFile(m_iMaterialIndex, "hud/armor_light", true, false);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArmorPanel::SetArmorMaterial(int iType)
{
	const char* pArmorType = g_aArmorTypes[iType];

	const char* pArmorMaterial = VarArgs("hud/armor_%s", pArmorType);
	m_iMaterialIndex = surface()->DrawGetTextureId(pArmorMaterial);
	if (m_iMaterialIndex == -1) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}

	surface()->DrawSetTextureFile(m_iMaterialIndex, pArmorMaterial, true, false);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArmorPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds(x, y, w, h);

	Vertex_t vert[4];
	float uv1 = 0.0f;
	float uv2 = 1.0f;
	int xpos = 0, ypos = 0;

	float flDamageY = h * (1.0f - m_flArmor);

	// blend in the red "damage" part
	surface()->DrawSetTexture(m_iMaterialIndex);

	Vector2D uv11(uv1, uv2 - m_flArmor);
	Vector2D uv21(uv2, uv2 - m_flArmor);
	Vector2D uv22(uv2, uv2);
	Vector2D uv12(uv1, uv2);

	vert[0].Init(Vector2D(xpos, flDamageY), uv11);
	vert[1].Init(Vector2D(xpos + w, flDamageY), uv21);
	vert[2].Init(Vector2D(xpos + w, ypos + h), uv22);
	vert[3].Init(Vector2D(xpos, ypos + h), uv12);

	surface()->DrawSetColor(GetFgColor());

	surface()->DrawTexturedPolygon(4, vert);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerHealth::CTFHudPlayerHealth( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pHealthImage = new CTFHealthPanel( this, "PlayerStatusHealthImage" );	
	m_pHealthImageBG = new ImagePanel( this, "PlayerStatusHealthImageBG" );
	m_pHealthBonusImage = new CTFImagePanel( this, "PlayerStatusHealthBonusImage" );
	m_pMegaHealthBonusImage = new CTFImagePanel( this, "PlayerStatusMegaHealthBonusImage" );

	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_nHealth = -1;
	m_nMegaHealth = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFilename() );

	if ( m_pHealthBonusImage )
	{
		m_pHealthBonusImage->GetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
	}
	if ( m_pMegaHealthBonusImage )
	{
		m_pMegaHealthBonusImage->GetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
	}
	m_flNextThink = 0.0f;

	BaseClass::ApplySchemeSettings( pScheme );
}

ConVar of_pill_health_r("of_pill_health_r", "255", 	FCVAR_ARCHIVE );
ConVar of_pill_health_g("of_pill_health_g", "100", 	FCVAR_ARCHIVE );
ConVar of_pill_health_b("of_pill_health_b", "0", 	FCVAR_ARCHIVE );
ConVar of_pill_health_a("of_pill_health_a", "64", 	FCVAR_ARCHIVE );
ConVar of_pill_overheal_percent("of_pill_overheal_percent", "0", FCVAR_ARCHIVE, "Color Pill overheal to stand out from regular overheal.");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::SetHealth( int iNewHealth, int iMaxHealth, int	iMaxBuffedHealth )
{
	int nPrevHealth = m_nHealth;

	// set our health
	m_nHealth = iNewHealth;
	m_nMaxHealth = iMaxHealth;
	m_pHealthImage->SetHealth( (float)(m_nHealth) / (float)(m_nMaxHealth) );

	if ( m_pHealthImage )
	{
		m_pHealthImage->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	if ( m_nHealth <= 0 )
	{
		if ( m_pHealthImageBG->IsVisible() )
		{
			m_pHealthImageBG->SetVisible( false );
		}
		HideHealthBonusImage();
	}
	else
	{
		if ( !m_pHealthImageBG->IsVisible() )
		{
			m_pHealthImageBG->SetVisible( true );
		}

		// are we getting a health bonus?
		if ( m_nHealth > m_nMaxHealth )
		{
			if ( m_pHealthBonusImage )
			{
				if ( !m_pHealthBonusImage->IsVisible() )
				{
					m_pHealthBonusImage->SetVisible( true );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );
				}

				m_pHealthBonusImage->SetFgColor( Color( 255, 255, 255, 255 ) );

				// scale the flashing image based on how much health bonus we currently have
				float flBoostMaxAmount = ( iMaxBuffedHealth ) - m_nMaxHealth;
				float flPercent = ( m_nHealth - m_nMaxHealth ) / flBoostMaxAmount;
				if (flPercent > 1.0f)
					flPercent = 1.0f;

				int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
				int nSizeAdj = 2 * nPosAdj;

				m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
					m_nBonusHealthOrigY - nPosAdj, 
					m_nBonusHealthOrigW + nSizeAdj,
					m_nBonusHealthOrigH + nSizeAdj );
			}
			C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
			if ( m_pMegaHealthBonusImage && pPlayer->m_iMegaOverheal > 0 )
			{
				if ( m_pMegaHealthBonusImage->IsVisible() != of_pill_overheal_percent.GetBool() )
				{
					m_pMegaHealthBonusImage->SetVisible( of_pill_overheal_percent.GetBool() );
			//		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );
				}

				m_pMegaHealthBonusImage->SetFgColor( Color( of_pill_health_r.GetInt(), of_pill_health_g.GetInt(), of_pill_health_b.GetInt(), of_pill_health_a.GetInt() ) );

				// scale the flashing image based on how much health bonus we currently have
				float flBoostMaxAmount = ( iMaxBuffedHealth ) - m_nMaxHealth;
				float flPercent = pPlayer->m_iMegaOverheal / flBoostMaxAmount;
				if (flPercent > 1.0f)
					flPercent = 1.0f;

				int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
				int nSizeAdj = 2 * nPosAdj;

				m_pMegaHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
					m_nBonusHealthOrigY - nPosAdj, 
					m_nBonusHealthOrigW + nSizeAdj,
					m_nBonusHealthOrigH + nSizeAdj );
			}			
		}
		// are we close to dying?
		else if ( m_nHealth < m_nMaxHealth * m_flHealthDeathWarning )
		{
			if ( m_pHealthBonusImage )
			{
				if ( !m_pHealthBonusImage->IsVisible() )
				{
					m_pHealthBonusImage->SetVisible( true );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulse" );
				}

				m_pHealthBonusImage->SetFgColor( m_clrHealthDeathWarningColor );

				// scale the flashing image based on how much health bonus we currently have
				float flBoostMaxAmount = m_nMaxHealth * m_flHealthDeathWarning;
				float flPercent = ( flBoostMaxAmount - m_nHealth ) / flBoostMaxAmount;

				int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
				int nSizeAdj = 2 * nPosAdj;

				m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
					m_nBonusHealthOrigY - nPosAdj, 
					m_nBonusHealthOrigW + nSizeAdj,
					m_nBonusHealthOrigH + nSizeAdj );
			}

			if ( m_pHealthImage )
			{
				m_pHealthImage->SetFgColor( m_clrHealthDeathWarningColor );
			}
		}
		// turn it off
		else
		{
			HideHealthBonusImage();
		}
	}

	// set our health display value
	if ( nPrevHealth != m_nHealth )
	{
		if ( m_nHealth > 0 )
		{
			SetDialogVariable( "Health", m_nHealth );
		}
		else
		{
			SetDialogVariable( "Health", "" );
		}	
	}
}
/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::SetMegaHealth( int iNewHealth, int iMaxHealth, int	iMaxBuffedHealth )
{
	if( iNewHealth <= 0 )
	{
		if ( m_pMegahealImage->IsVisible() )
		{
			m_pMegahealImage->SetVisible( false );
			m_pMegahealImageBG->SetVisible( false );
		}
		if( m_pMegahealLabel )
			m_pMegahealLabel->SetVisible( false );
		SetDialogVariable( "MegaHealth", "" );
		return;
	}
	else
	{
		if ( !m_pMegahealImage->IsVisible() )
		{
			m_pMegahealImage->SetVisible( true );
			m_pMegahealImageBG->SetVisible( true );
		}
		if( m_pMegahealLabel )
			m_pMegahealLabel->SetVisible( true );
	}
	int nPrevHealth = m_nMegaHealth;

	// set our health
	m_nMegaHealth = iNewHealth;
	m_nMaxMegaHealth = iMaxHealth;
	m_pMegahealImage->SetHealth( (float)(m_nMegaHealth) / (float)(m_nMaxMegaHealth) );

	if ( m_pMegahealImage )
	{
		m_pMegahealImage->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	// are we getting a health bonus?
	// implement later when we get armor
	if ( m_nMegaHealth > m_nMaxMegaHealth )
	{

		if ( m_pHealthBonusImage )
		{
			if ( !m_pHealthBonusImage->IsVisible() )
			{
				m_pHealthBonusImage->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );
			}
			m_pHealthBonusImage->SetFgColor( Color( 255, 255, 255, 255 ) );
			// scale the flashing image based on how much health bonus we currently have
			float flBoostMaxAmount = ( iMaxBuffedHealth ) - m_nMaxMegaHealth;
			float flPercent = ( m_nMegaHealth - m_nMaxMegaHealth ) / flBoostMaxAmount;
			if (flPercent > 1.0f)
				flPercent = 1.0f;

			int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
			int nSizeAdj = 2 * nPosAdj;

			m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
			m_nBonusHealthOrigY - nPosAdj, 
			m_nBonusHealthOrigW + nSizeAdj,
			m_nBonusHealthOrigH + nSizeAdj );
		}
		
	}

	// set our health display value
	if ( nPrevHealth != m_nMegaHealth )
	{
		if ( m_nMegaHealth > 0 )
		{
			SetDialogVariable( "MegaHealth", m_nMegaHealth );
		}
		else
		{
			SetDialogVariable( "MegaHealth", "" );
		}	
	}
}
*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::HideHealthBonusImage( void )
{
	if ( m_pHealthBonusImage && m_pHealthBonusImage->IsVisible() )
	{
		m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
		m_pHealthBonusImage->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );
	}
	if ( m_pMegaHealthBonusImage && m_pMegaHealthBonusImage->IsVisible() )
	{
		m_pMegaHealthBonusImage->SetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
		m_pMegaHealthBonusImage->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pPlayer )
		{
//			float flMegaHeal = pPlayer->m_Shared.m_flMegaOverheal;
//			if( pPlayer->GetHealth() < pPlayer->GetPlayerClass()->GetMaxHealth() )
//				flMegaHeal = 0.0f;
//			else if( pPlayer->GetHealth() - flMegaHeal < pPlayer->GetPlayerClass()->GetMaxHealth() )
//				flMegaHeal -= pPlayer->GetPlayerClass()->GetMaxHealth() - ( pPlayer->GetHealth() - flMegaHeal );

			if ( pPlayer->GetHealth() < pPlayer->m_Shared.GetDefaultHealth() * m_flHealthDeathWarning
			&& m_pHealthBonusImage 
			&& !m_pHealthBonusImage->IsVisible() )
			{
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, pPlayer->entindex(), "TFPlayer.LowHealth" );
			}

			SetHealth( pPlayer->GetHealth(), pPlayer->m_Shared.GetDefaultHealth(), pPlayer->m_Shared.GetMaxBuffedHealthDM() );
//			SetMegaHealth( pPlayer->m_Shared.m_flMegaOverheal, pPlayer->GetPlayerClass()->GetMaxHealth(), pPlayer->m_Shared.GetMaxBuffedHealthDM()/ 2 );
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerArmor::CTFHudPlayerArmor(Panel *parent, const char *name) : EditablePanel(parent, name)
{
	m_pArmorImage = new CTFArmorPanel(this, "PlayerStatusArmorImage");
	m_pArmorImageBG = new ImagePanel(this, "PlayerStatusArmorImageBG");
	m_pArmorWarningImage = new CTFArmorPanel(this, "PlayerStatusArmorWarningImage");

	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_nArmor = -1;
	m_nMaxArmor = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings(GetResFilename());

	m_flNextThink = 0.0f;
	m_nType = ARMOR_NONE;

	//if (m_pArmorWarningImage)
	//{
	//	m_pArmorWarningImage->GetBounds(m_nWarningArmorOrigX, m_nWarningArmorOrigY, m_nWarningArmorOrigW, m_nWarningArmorOrigH);
	//}

	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::SetArmor(int iNewArmor, int iMaxArmor)
{
	int nPrevArmor = m_nArmor;

	C_TFPlayer *pPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);

	// set our health
	m_nArmor = iNewArmor;
	m_nMaxArmor = iMaxArmor;
	m_pArmorImage->SetArmor((float)(m_nArmor) / (float)(m_nMaxArmor));

	if (m_pArmorImage)
	{
		m_pArmorImage->SetFgColor(Color(255, 255, 255, 255));
	}
		// are we close to dying?
	if (m_nArmor < m_nMaxArmor * m_flArmorDeathWarning)
	{
		if (m_pArmorWarningImage)
		{
			if (!m_pArmorWarningImage->IsVisible())
			{
					m_pArmorWarningImage->SetVisible(true);
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudArmorDyingPulse");
			}

			m_pArmorWarningImage->SetFgColor(m_clrArmorDeathWarningColor);

			// scale the flashing image based on how much health bonus we currently have
			/*float flBoostMaxAmount = m_nMaxArmor * m_flArmorDeathWarning;
			float flPercent = (flBoostMaxAmount - m_nArmor) / flBoostMaxAmount;

			int nPosAdj = RoundFloatToInt(flPercent * m_nArmorWarningPosAdj);
			int nSizeAdj = 2 * nPosAdj;

			m_pArmorWarningImage->SetBounds(m_nWarningArmorOrigX - nPosAdj,
			m_nWarningArmorOrigY - nPosAdj,
			m_nWarningArmorOrigW + nSizeAdj,
			m_nWarningArmorOrigH + nSizeAdj);*/
		}

		if (m_pArmorImage)
		{
			m_pArmorImage->SetFgColor(m_clrArmorDeathWarningColor);
		}
	}
	// turn it off
	else
	{
		HideArmorWarningImage();
	}

	// set our health display value
	if (nPrevArmor != m_nArmor)
	{
		if (m_nArmor > 0)
		{
			SetDialogVariable("Armor", m_nArmor);
		}
		else
		{
			if (
				!(of_armor.GetInt() == 0) && !(pTFPlayer->GetPlayerClass()->GetArmorType() == 0)
				)
			{
				SetDialogVariable("Armor", "0");
			}
			else
				SetDialogVariable("Armor", "");
		}
	}

	if (
		!(of_armor.GetInt() == 0) && !(pTFPlayer->GetPlayerClass()->GetArmorType() == ARMOR_NONE)
		)
	{
		//DevMsg("Armor is enabled or Player has an armor type above 0\n");
		m_pArmorImage->SetVisible(true);
		m_pArmorImageBG->SetVisible(true);
	}
	else
	{
		//DevMsg("Armor is disabled or Player has an armor type that is 0\n");
		m_pArmorImage->SetVisible(false);
		m_pArmorImageBG->SetVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::HideArmorWarningImage(void)
{
	if (m_pArmorWarningImage && m_pArmorWarningImage->IsVisible())
	{
		m_pArmorWarningImage->SetVisible(false);
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudArmorWarningPulseStop");
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudArmorDyingPulseStop");
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArmorPanel::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);

	if (pPlayer)
	{
		if (
			!(of_armor.GetInt() == 0) && !(pTFPlayer->GetPlayerClass()->GetArmorType() == 0)
			)
		{
			return true;
		}
		else
			return false;
	}
	else
		return false;

	//This may be a bit redundant
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::SetArmorImageType(int iType)
{
	m_nType = iType;

	m_pArmorImage->SetArmorMaterial(iType);
	m_pArmorWarningImage->SetArmorMaterial(iType);
	m_pArmorImageBG->SetImage(VarArgs("../hud/armor_%s_bg", g_aArmorTypes[iType]));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerArmor::OnThink()
{
	if (m_flNextThink < gpGlobals->curtime)
	{
		C_TFPlayer *pPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (pPlayer)
		{

			int nType = pPlayer->GetPlayerClass()->GetArmorType();

			if (pPlayer->m_Shared.GetTFCArmor() < pPlayer->GetPlayerClass()->GetMaxArmor() * m_flArmorDeathWarning
				&& m_pArmorWarningImage
				&& !m_pArmorWarningImage->IsVisible())
			{
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound(filter, pPlayer->entindex(), "TFPlayer.LowArmor");
			}

			SetArmor(pPlayer->m_Shared.GetTFCArmor(), pPlayer->GetPlayerClass()->GetMaxArmor());
			SetArmorImageType(nType);
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}
DECLARE_HUDELEMENT( CTFHudPlayerStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerStatus::CTFHudPlayerStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPlayerStatus" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudPlayerClass = new CTFHudPlayerClass( this, "HudPlayerClass" );
	m_pHudPlayerHealth = new CTFHudPlayerHealth( this, "HudPlayerHealth" );
	m_pHudPlayerArmor = new CTFHudPlayerArmor(this, "HudPlayerArmor");

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerStatus::FireGameEvent(IGameEvent *event)
{
	if (!Q_stricmp(event->GetName(), "localplayer_changeclass"))
	{
		C_TFPlayer* pPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (pPlayer && pPlayer->IsAlive())
		{
			m_pHudPlayerArmor->SetArmorImageType(pPlayer->GetPlayerClass()->GetArmorType());
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// HACK: Work around the scheme application order failing
	// to reload the player class hud element's scheme in minmode.
	ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		m_pHudPlayerClass->InvalidateLayout( false, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerStatus::Reset()
{
	if ( m_pHudPlayerClass )
	{
		m_pHudPlayerClass->Reset();
	}

	if ( m_pHudPlayerHealth )
	{
		m_pHudPlayerHealth->Reset();
	}

	if (m_pHudPlayerArmor)
	{
		m_pHudPlayerArmor->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassImage::SetClass( int iTeam, TFPlayerClassData_t *iClassData, int iCloakstate )
{
	char szImage[128];
	szImage[0] = '\0';
	
	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( iClassData->GetClassImageBlue() )
			Q_strncpy( szImage, iClassData->GetClassImageBlue(), sizeof(szImage) );
	}
	else if ( iTeam == TF_TEAM_MERCENARY )
	{
		if( of_tennisball.GetInt() == 1 && iClassData->GetClassImageTennis()[0] != '\0' )
			Q_strncpy( szImage, iClassData->GetClassImageTennis(), sizeof(szImage) );
		else if ( iClassData->GetClassImageMercenary() )
			Q_strncpy( szImage, iClassData->GetClassImageMercenary(), sizeof(szImage) );
	}
	else
	{
		if ( iClassData->GetClassImageRed() )
			Q_strncpy( szImage, iClassData->GetClassImageRed(), sizeof(szImage) );
	}

	switch( iCloakstate )
	{
	case 2:
		Q_strncat( szImage, "_cloak", sizeof(szImage), COPY_ALL_CHARACTERS );
		break;
	case 1:
		Q_strncat( szImage, "_halfcloak", sizeof(szImage), COPY_ALL_CHARACTERS );
		break;
	default:
		break;
	}

	if ( Q_strlen( szImage ) > 0 )
	{
		SetImage( szImage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassImage::SetClassColorless( int iTeam, TFPlayerClassData_t *iClassData, int iCloakstate )
{
	char szImageColorless[128];
	szImageColorless[0] = '\0';

	if ( iTeam == TF_TEAM_MERCENARY && !( of_tennisball.GetInt() == 1 && iClassData->GetClassImageTennis()[0] != '\0' ) )
	{
		Q_strncpy( szImageColorless, iClassData->GetClassImageColorless(), sizeof(szImageColorless) );
		switch( iCloakstate )
		{
			case 2:
				Q_strncat( szImageColorless, "_cloak", sizeof(szImageColorless), COPY_ALL_CHARACTERS );
				break;
			case 1:
				Q_strncat( szImageColorless, "_halfcloak", sizeof(szImageColorless), COPY_ALL_CHARACTERS );
				break;
			default:
				break;
		}
	}
	else
		Q_strncpy( szImageColorless, g_szEmpty[ 0 ], sizeof(szImageColorless) );

	if ( Q_strlen( szImageColorless ) > 0 )
	{
		SetImage( szImageColorless );
	}
}