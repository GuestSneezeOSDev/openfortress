//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_dmmodelpanel.h"
#include "of_loadout.h"
#include "of_items_game.h"
#include "gameui/basemodui.h"

using namespace BaseModUI;
using namespace vgui;

extern ConVar of_tennisball;
extern ConVar of_respawn_particle;
extern ConVar of_announcer_override;

DECLARE_BUILD_FACTORY(DMModelPanel);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
DMModelPanel::DMModelPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName) 
{
	iWeaponAnim = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DMModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings(inResourceData);
	SetWeaponModel("models/weapons/w_models/w_supershotgun.mdl", 0);
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void DMModelPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetLoadoutCosmetics();
	KeyValues *pWeapons = GetLoadout()->FindKey("Weapons");
	if( pWeapons )
	{
		KeyValues *pMercenary = pWeapons->FindKey("mercenary");
		if( pMercenary )
		{
			int i = 1;
			while( i < 3 )
			{
				COFSchemaWeaponInfo *pWeapon = GetItemSchema()->GetWeapon( pMercenary->GetString( VarArgs("%d", i) ) );
				if( pWeapon )
				{
					if( pWeapon->m_iLoadoutAnim != -1 )
					{
						FileWeaponInfo_t *pWpnData = GetFileWeaponInfoFromHandle( pWeapon->m_nWeaponHandle );
						SetWeaponModel( pWpnData->szWorldModel, pWeapon->m_iLoadoutAnim );
						break;
					}
				}
				i++;
			}
		}
	}

	SetPaintBackgroundEnabled(true);

	// Set the animation.
	Update();

	m_iCurrentParticle = of_respawn_particle.GetInt();
}

void DMModelPanel::PerformLayout()
{
	BaseClass::PerformLayout();

}

void DMModelPanel::PaintBackground()
{
	BaseClass::PaintBackground();
	
	if( m_iCurrentParticle != of_respawn_particle.GetInt() )
	{
		m_iCurrentParticle = of_respawn_particle.GetInt();

		COFParticleInfo *pParticle = GetItemSchema()->GetRespawnParticle( m_iCurrentParticle );
		if( pParticle )
			m_flParticleZOffset = pParticle->m_flParticleZOffset;
		
		char pEffectName[32];
		pEffectName[0] = '\0';
		Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_%02d", m_iCurrentParticle );
		if ( pEffectName[0] != '\0' )
			SetParticleName(pEffectName);
	}

	if( of_tennisball.GetInt() != m_iTennisball )
	{
		m_iTennisball = of_tennisball.GetInt();
		m_BMPResData.m_nSkin = m_iTennisball == 1 ? 6 : 4;
		Update();
		// Since cosmetics can now have different tenisball skins, we need to update them
		// *sigh*, team classic
		m_bUpdateCosmetics = true;
	}

	if( m_bUpdateCosmetics )
	{
		if( !GetModelPtr()->IsValid() ) 
			return;

		ClearMergeMDLs();
		for( int i = 0; i < GetNumBodyGroups(); i++ )
		{
			SetBodygroup(i, 0);
		}
		// Set the animation.
		SetMergeMDL( szWeaponModel, NULL, 2 );
		for( int i = 0; i < m_flCosmetics.Count(); i++ )
		{
			COFCosmeticInfo *pCosmetic = GetItemSchema()->GetCosmetic( m_flCosmetics[i] );
			if( pCosmetic )
			{
				float flCosmetic = m_flCosmetics[i];
				flCosmetic += 0.001f;
				flCosmetic = flCosmetic - (int)m_flCosmetics[i];
				flCosmetic *= 100.0f;
				int iStyle = (int)flCosmetic;
				pCosmetic = pCosmetic->GetStyle( iStyle );

				int iVisibleTeam = 0;
				int iTeamCount = 1;

				if( pCosmetic->m_bTeamSkins )
				{
					iTeamCount = 3;
					iVisibleTeam = 2;
				}
				
				if( pCosmetic->m_bUsesBrightskins )
				{
					iTeamCount++;
					if( of_tennisball.GetInt() == 1 )
						iVisibleTeam = iTeamCount - 1;
				}
				
				int nSkin = iVisibleTeam < 0 ? 0 : iVisibleTeam;
				
				nSkin += iTeamCount * pCosmetic->m_iSkinOffset;

				if( strcmp( pCosmetic->m_szModel, "BLANK" ) && strcmp( pCosmetic->m_szModel, "" ) )
					SetMergeMDL( pCosmetic->m_szModel, NULL, nSkin );

				FOR_EACH_DICT( pCosmetic->m_Bodygroups, i )
				{
					int m_Bodygroup = FindBodygroupByName( pCosmetic->m_Bodygroups.GetElementName(i) );
					if ( m_Bodygroup >= 0 )
					{
						SetBodygroup( m_Bodygroup, pCosmetic->m_Bodygroups.Element(i) );
					}
				}
			}
		}
		Update();
		SetModelAnim( iWeaponAnim );
		
		m_bUpdateCosmetics = false;
	}
}

void DMModelPanel::SetCosmetic(int iCosmeticID, bool bSelected)
{
	if (bSelected)
	{
		for (int i = 0; i < m_flCosmetics.Count(); i++)
		{
			if (iCosmeticID == m_flCosmetics[i])
			{
				// Already has the cosmetic, don't add second time
				return;
			}
		}
		m_flCosmetics.AddToTail(iCosmeticID);
	}
	else
	{
		for (int i = 0; i < m_flCosmetics.Count(); i++)
		{
			if (iCosmeticID == m_flCosmetics[i])
			{
				m_flCosmetics.Remove(i);
				break;
			}
		}
	}
	m_bUpdateCosmetics = true;
}

void DMModelPanel::SetWeaponModel( const char *szWeapon, int iAnim )
{
	Q_strncpy(szWeaponModel, szWeapon, sizeof(szWeaponModel));
	SetModelAnim( iAnim );
	iWeaponAnim = iAnim;
	m_iAnimationIndex = iAnim;
	m_bUpdateCosmetics = true;
}

void DMModelPanel::SetLoadoutCosmetics()
{
	m_flCosmetics.RemoveAll();
	if (GetLoadout())
	{
		KeyValues *kvCosmetics = GetLoadout()->FindKey("Cosmetics");
		if (kvCosmetics)
		{
			KeyValues *kvMerc = kvCosmetics->FindKey("mercenary");
			if (kvMerc)
			{
				for (KeyValues *pData = kvMerc->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey())
				{
					m_flCosmetics.AddToTail(pData->GetFloat());
				}
			}
		}
	}
	m_bUpdateCosmetics = true;
}
