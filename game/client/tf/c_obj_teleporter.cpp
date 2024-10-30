//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectTeleporter
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_teleporter.h"
#include "soundenvelope.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define TELEPORTER_MINS			Vector( -24, -24, 0)
#define TELEPORTER_MAXS			Vector( 24, 24, 12)	

//-----------------------------------------------------------------------------
// Purpose: Teleporter object
//-----------------------------------------------------------------------------

IMPLEMENT_CLIENTCLASS_DT(C_ObjectTeleporter, DT_ObjectTeleporter, CObjectTeleporter)
	RecvPropInt( RECVINFO(m_iState) ),
	RecvPropTime( RECVINFO(m_flRechargeTime) ),
	RecvPropInt( RECVINFO(m_iTimesUsed) ),
	RecvPropFloat( RECVINFO(m_flYawToExit) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectTeleporter::C_ObjectTeleporter()
{
	m_pChargedEffect = NULL;
	m_pDirectionEffect = NULL;
	m_pChargedLeftArmEffect = NULL;
	m_pChargedRightArmEffect = NULL;
	m_pDamageEffects = NULL;

	m_iDirectionArrowPoseParam = 0;

	m_pSpinSound = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpdateOnRemove( void )
{
	StopActiveEffects();
	StopChargedEffects();

	if ( m_pSpinSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSpinSound );
	}

	BaseClass::UpdateOnRemove();
}

void C_ObjectTeleporter::GetStatusText( wchar_t *pStatus, int iMaxStatusLen )
{
	float flHealthPercent = (float)GetHealth() / (float)GetMaxHealth();
	wchar_t wszHealthPercent[32];
	_snwprintf(wszHealthPercent, sizeof(wszHealthPercent)/sizeof(wchar_t) - 1, L"%d%%", (int)( flHealthPercent * 100 ) );

	if ( IsBuilding() )
	{
		wchar_t *pszState = g_pVGuiLocalize->Find( "#TF_ObjStatus_Teleporter_Building" );

		if ( pszState )
		{
			g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszState,
				1,
				wszHealthPercent );
		}
	}
	else
	{
		const wchar_t *pszState = NULL;

		switch( m_iState )
		{
			case TELEPORTER_STATE_IDLE:
				pszState = g_pVGuiLocalize->Find( "#TF_Obj_Teleporter_State_Idle" );
				break;

			case TELEPORTER_STATE_READY:
			case TELEPORTER_STATE_SENDING:
				pszState = g_pVGuiLocalize->Find( "#TF_Obj_Teleporter_State_Ready" );
				break;

			case TELEPORTER_STATE_RECHARGING:
				pszState = g_pVGuiLocalize->Find( "#TF_Obj_Teleporter_State_Charging" );
				break;

			default:
				// how can this happen?
			case TELEPORTER_STATE_BUILDING:
				pszState = L"unknown";
				break;
		}

		if ( pszState )
		{
			wchar_t *pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_Teleporter" );

			if ( pszTemplate )
			{
				g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
					3,
					m_iUpgradeLevel,
					wszHealthPercent,
					pszState );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldState = m_iState;
}

void C_ObjectTeleporter::StartChargedEffects()
{
	if ( GetAltMode() == 0 )
	{
		char szEffect[128];
		if ( GetTeamNumber() == TF_TEAM_RED ) Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_charged", "red" );
		else if ( GetTeamNumber() == TF_TEAM_BLUE )Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_charged", "blue" );
		else Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_charged", "mercenary" );
		

		Assert( m_pChargedEffect == NULL );
		m_pChargedEffect = ParticleProp()->Create( szEffect, PATTACH_ABSORIGIN );
	}
}

void C_ObjectTeleporter::StartActiveEffects()
{
	char szEffect[128];
		if ( GetTeamNumber() == TF_TEAM_RED ) 	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_%s", "red",
		( GetAltMode() == 0 ) ? "entrance" : "exit" );
		else if ( GetTeamNumber() == TF_TEAM_BLUE )	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_%s", "blue",
		( GetAltMode() == 0 ) ? "entrance" : "exit" );
		else Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_%s", "mercenary",
		( GetAltMode() == 0 ) ? "entrance" : "exit" );


	Assert( m_pDirectionEffect == NULL );
	m_pDirectionEffect = ParticleProp()->Create( szEffect, PATTACH_ABSORIGIN );

	// arm glow effects
		if ( GetTeamNumber() == TF_TEAM_RED ) Q_snprintf( szEffect, sizeof(szEffect), "teleporter_arms_circle_%s", "red" );
		else if ( GetTeamNumber() == TF_TEAM_BLUE )Q_snprintf( szEffect, sizeof(szEffect), "teleporter_arms_circle_%s", "blue" );
		else Q_snprintf( szEffect, sizeof(szEffect), "teleporter_arms_circle_%s", "mercenary" );
		
	Assert( m_pChargedLeftArmEffect == NULL );
	m_pChargedLeftArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 1 );

	Assert( m_pChargedRightArmEffect == NULL );
	m_pChargedRightArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 3 );

	if ( !m_pSpinSound )
	{
		// init the spin sound
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		const char *shootsound = NULL;

		if ( GetUpgradeLevel() == 1 )
			shootsound = "Building_Teleporter.SpinLevel1";
		else if ( GetUpgradeLevel() == 2 )
			shootsound = "Building_Teleporter.SpinLevel2";
		else
			shootsound = "Building_Teleporter.SpinLevel3";

		CLocalPlayerFilter filter;
		m_pSpinSound = controller.SoundCreate( filter, entindex(), shootsound );
		controller.Play( m_pSpinSound, 1.0, 100 );
	}
}

void C_ObjectTeleporter::StopChargedEffects()
{
	if ( m_pChargedEffect )
	{
		ParticleProp()->StopEmission( m_pChargedEffect );
		m_pChargedEffect = NULL;
	}
}

void C_ObjectTeleporter::StopActiveEffects()
{
	if ( m_pDirectionEffect )
	{
		ParticleProp()->StopEmission( m_pDirectionEffect );
		m_pDirectionEffect = NULL;
	}

	if ( m_pChargedLeftArmEffect )
	{
		ParticleProp()->StopEmission( m_pChargedLeftArmEffect );
		m_pChargedLeftArmEffect = NULL;
	}

	if ( m_pChargedRightArmEffect )
	{
		ParticleProp()->StopEmission( m_pChargedRightArmEffect );
		m_pChargedRightArmEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iOldState != m_iState )
	{
		if ( m_iState == TELEPORTER_STATE_READY )
		{
			StartChargedEffects();
		}
		else
		{
			StopChargedEffects();
		}

		if ( m_iState > TELEPORTER_STATE_IDLE && m_iOldState <= TELEPORTER_STATE_IDLE )
		{
			StartActiveEffects();
		}
		else if ( m_iState <= TELEPORTER_STATE_IDLE && m_iOldState > TELEPORTER_STATE_IDLE )
		{
			StopActiveEffects();
		}

		m_iOldState = m_iState;
	}

	// update the pitch based on our playback rate
	if ( m_pSpinSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundChangePitch( m_pSpinSound, GetPlaybackRate() * 100.0f, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float C_ObjectTeleporter::GetChargeTime( void )
{
	float flTime = m_flRechargeTime - gpGlobals->curtime;

	if ( flTime < 0 )
		return 0;

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_ObjectTeleporter::GetTimesUsed( void )
{
	return m_iTimesUsed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_ObjectTeleporter::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iDirectionArrowPoseParam = LookupPoseParameter( "direction" );

	if ( GetAltMode() == 0 )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Update the direction arrow
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::ClientThink( void )
{
	if ( GetAltMode() == 0 && m_iState >= TELEPORTER_STATE_READY )
	{
		SetPoseParameter( m_iDirectionArrowPoseParam, m_flYawToExit);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::GetTargetIDString( wchar_t *sIDString, int iMaxLenInBytes )
{
	sIDString[0] = '\0';

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	int iTeamNumber = GetTeamNumber();

	if ( iTeamNumber == pLocalPlayer->GetTeamNumber() )
	{
		wchar_t wszBuilderName[ MAX_PLAYER_NAME_LENGTH ];

		const char *pszStatusName = GetStatusName();
		const char *pszModeName = GetModeName();
		const wchar_t *wszObjectName = g_pVGuiLocalize->Find( pszStatusName );
		const wchar_t *wszModeName = g_pVGuiLocalize->Find( pszModeName );

		if ( !wszObjectName )
		{
			wszObjectName = L"";
		}

		if ( !wszModeName )
		{
			wszObjectName = L"";
		}

		C_BasePlayer *pBuilder = GetOwner();

		if ( pBuilder )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pBuilder->GetPlayerName(), wszBuilderName, sizeof(wszBuilderName) );
		}
		else
		{
			wszBuilderName[0] = '\0';
		}

		// building or live, show health
		const char *printFormatString = "#TF_playerid_object_mode";

		g_pVGuiLocalize->ConstructString( sIDString, iMaxLenInBytes, g_pVGuiLocalize->Find(printFormatString),
			3,
			wszObjectName,
			wszBuilderName,
			wszModeName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes )
{
	sDataString[0] = '\0';

	if ( m_iState == TELEPORTER_STATE_RECHARGING && gpGlobals->curtime < m_flRechargeTime )
	{
		float rechargetime;

		if ( GetUpgradeLevel() == 1 )
			rechargetime = (float)TELEPORTER_RECHARGE_TIME;
		else if ( GetUpgradeLevel() == 2 )
			rechargetime = (float)TELEPORTER_RECHARGE_TIME / 2;
		else
			rechargetime = (float)TELEPORTER_RECHARGE_TIME / 4;
			
		float flPercent = clamp( ( m_flRechargeTime - gpGlobals->curtime ) / rechargetime, 0.0f, 1.0f );

		wchar_t wszRecharging[ 32 ];
		_snwprintf( wszRecharging, ARRAYSIZE(wszRecharging) - 1, L"%.0f", 100 - (flPercent * 100) );
		wszRecharging[ ARRAYSIZE(wszRecharging)-1 ] = '\0';

		const char *printFormatString = "#TF_playerid_object_recharging";

		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( printFormatString ),
			1,
			wszRecharging );
	}	
	else if ( m_iState == TELEPORTER_STATE_IDLE )
	{
		// RIP
		g_pVGuiLocalize->ConstructString( sDataString, MAX_ID_STRING, g_pVGuiLocalize->Find("#TF_playerid_teleporter_entrance_nomatch" ),
		0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_pDamageEffects )
	{
		m_pDamageEffects->StopEmission( false, false );
		m_pDamageEffects = NULL;
	}

	const char *pszEffect = "";

	switch( damageLevel )
	{
	case BUILDING_DAMAGE_LEVEL_LIGHT:
		pszEffect = "tpdamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "tpdamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "tpdamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "tpdamage_4";
		break;

	default:
		break;
	}

	if ( Q_strlen(pszEffect) > 0 )
	{
		m_pDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool C_ObjectTeleporter::IsPlacementPosValid( void )
{
	bool bResult = BaseClass::IsPlacementPosValid();

	if ( !bResult )
	{
		return false;
	}

	// m_vecBuildOrigin is the proposed build origin

	// start above the teleporter position
	Vector vecTestPos = m_vecBuildOrigin;
	vecTestPos.z += TELEPORTER_MAXS.z;

	// make sure we can fit a player on top in this pos
	trace_t tr;
	UTIL_TraceHull( vecTestPos, vecTestPos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID | CONTENTS_PLAYERCLIP, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return ( tr.fraction >= 1.0 );
}