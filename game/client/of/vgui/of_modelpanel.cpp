//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "renderparm.h"
#include "animation.h"
#include "of_modelpanel.h"
#include "c_tf_player.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFModelPanel );

CTFModelPanel::CTFModelPanel( Panel *pParent, const char *pszName )
	: CBaseModelPanel( pParent, pszName ), m_rawinput("m_rawinput")
{
	SetParent( pParent );
	SetScheme( "ClientScheme" );
	SetProportional( true );
	SetVisible( true );
	SetThumbnailSafeZone( false );

	m_flParticleZOffset = 0.0f;
	m_flLoopParticleAfter = 0.0f;
	m_iAnimationIndex = 0;
	m_rawinput_stored = true;
}

void CTFModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	
	m_bLoopParticle = inResourceData->GetBool( "particle_loop" );
	m_flLoopTime = inResourceData->GetFloat( "particle_loop_time" );
	m_flParticleZOffset = inResourceData->GetFloat( "particle_z_offset" );
		
	Color bgColor = inResourceData->GetColor( "bgcolor_override" );
	SetBackgroundColor( bgColor );
	

	KeyValues *pModelData = inResourceData->FindKey( "model" );
	if( pModelData )
	{
		m_vecDefPosition = Vector( pModelData->GetFloat( "origin_x", 0 ), pModelData->GetFloat( "origin_y", 0 ), pModelData->GetFloat( "origin_z", 0 ) );
		m_vecDefAngles = QAngle( pModelData->GetFloat( "angles_x", 0 ), pModelData->GetFloat( "angles_y", 0 ), pModelData->GetFloat( "angles_z", 0 ) );
		SetModelName( ReadAndAllocStringValue ( pModelData, "modelname" ), pModelData->GetInt("skin", 0) );
	}
}

void CTFModelPanel::OnThink()
{
	BaseClass::OnThink();

	// TODO: autorotation?
}

void CTFModelPanel::Update()
{
	if (m_BMPResData.m_pszModelName)
	{
		MDLHandle_t hSelectedMDL = g_pMDLCache->FindMDL(m_BMPResData.m_pszModelName);
		g_pMDLCache->PreloadModel(hSelectedMDL);
		SetMDL(hSelectedMDL);


		if (m_iAnimationIndex < m_BMPResData.m_aAnimations.Size())
		{
			SetModelAnim(m_iAnimationIndex);
		}

		SetSkin(m_BMPResData.m_nSkin);
	}
}

void CTFModelPanel::Paint()
{
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	pRenderContext->SetFlashlightMode(false);
	
	if( m_bLoopParticle )
	{
		if( m_flLoopParticleAfter < gpGlobals->curtime )
		{
			SetParticleName(szLoopingParticle);
		}
	}
	BaseClass::Paint();
}

// Update our Studio Hdr to our current model
void CTFModelPanel::RefreshModel()
{
	//Fenteale: using m_RootMDL.m_MDL.GetStudioHdr() doesnt work in linux.  use this instead
	studiohdr_t *pStudioHdr = GetStudioHdr();
	
	if ( !pStudioHdr )
		return;

	CStudioHdr StudioHdr( pStudioHdr, g_pMDLCache );
	m_StudioHdr = StudioHdr;
}

CStudioHdr *CTFModelPanel::GetModelPtr()
{
	RefreshModel();
	return &m_StudioHdr;
}

// Most of the bodygroup stuff was copied over from c_BaseAnimating
// The only stuff removed is Dynamic loading tests
// The following comment is from that

// SetBodygroup is not supported on pending dynamic models. Wait for it to load!
// XXX TODO we could buffer up the group and value if we really needed to. -henryg

void CTFModelPanel::SetBodygroup( int iGroup, int iValue )
{

	Assert( GetModelPtr() );
	::SetBodygroup( GetModelPtr( ), m_RootMDL.m_MDL.m_nBody, iGroup, iValue );
}

int CTFModelPanel::GetBodygroup( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroup( GetModelPtr( ), m_RootMDL.m_MDL.m_nBody, iGroup );
}

const char *CTFModelPanel::GetBodygroupName( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroupName( GetModelPtr( ), iGroup );
}

int CTFModelPanel::FindBodygroupByName( const char *name )
{
	Assert( GetModelPtr() );
	return ::FindBodygroupByName( GetModelPtr( ), name );
}

int CTFModelPanel::GetBodygroupCount( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroupCount( GetModelPtr( ), iGroup );
}

int CTFModelPanel::GetNumBodyGroups( void )
{
	Assert( GetModelPtr() );
	return ::GetNumBodyGroups( GetModelPtr( ) );
}

void CTFModelPanel::SetModelName( const char* pszModelName, int nSkin )
{
	m_BMPResData.m_pszModelName = pszModelName;
	m_BMPResData.m_nSkin = nSkin;
}

extern ConVar of_tennisball;

void CTFModelPanel::SetParticleName(const char* name)
{
	m_bUseParticle = true;

	if (m_pData)
	{
		SafeDeleteParticleData(&m_pData);
	}

	m_pData = CreateParticleData(name);
	if( m_bLoopParticle )
	{
		Q_strncpy( szLoopingParticle, name ,sizeof(szLoopingParticle) );
		m_flLoopParticleAfter = gpGlobals->curtime + m_flLoopTime; 
	}
	// We failed at creating that particle for whatever reason, bail (!)
	if (!m_pData) return;

	CUtlVector<int> vecAttachments;

	Vector vecParticleOffset( 0, 0, m_flParticleZOffset );
	
#ifndef LINUX
	m_pData->UpdateControlPoints( GetModelPtr(), &m_RootMDL.m_MDLToWorld, vecAttachments, 0, vecParticleOffset);
#endif
	int iRed =  of_tennisball.GetInt() == 1 ? 0 : of_color_r.GetInt();
	int iGreen = of_tennisball.GetInt() == 1 ? 255 : of_color_g.GetInt();
	int iBlue =  of_tennisball.GetInt() == 1 ? 0 : of_color_b.GetInt();
#ifndef LINUX	
	m_pData->SetParticleColor( GetModelPtr(), &m_RootMDL.m_MDLToWorld, iRed, iGreen, iBlue );
#endif
	m_pData->m_bIsUpdateToDate = true;
}

void CTFModelPanel::OnMousePressed(vgui::MouseCode code)
{
	//Fenteale: this is a hack to get the rotation speed not out of control when
	//having m_rawinput 1 on linux.  For a more indepth explanation look at my comments
	//in of_hud_weaponwheel.cpp

	//yeah in linux there is a "jump" in rotation when you first click on the model,
	//but to fix this it would either require a way more in-depth workaround, or engine
	//access lol.  I believe its due to the fact that setting m_rawinput like below actually
	//is delayed from happening for like a frame.  I think for many, being able to rotate
	//at reasonable speeds is enough.
#ifdef LINUX
	m_rawinput_stored = m_rawinput.GetBool();
	m_rawinput.SetValue(false);
#endif
	CBaseModelPanel::OnMousePressed(code);
}

void CTFModelPanel::OnMouseReleased(vgui::MouseCode code)
{
	//dito as above
	CBaseModelPanel::OnMouseReleased(code);
#ifdef LINUX
	m_rawinput.SetValue(m_rawinput_stored);
#endif
}