//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_tf_objective_resource.h"
#include "teamplayroundbased_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT( C_TFObjectiveResource, DT_TFObjectiveResource, CTFObjectiveResource)

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::C_TFObjectiveResource()
{
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_mercenary" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu_up" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red_up" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::~C_TFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPCappingSwipe( int index, int iCappingTeam )
{
	Assert( index < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "sprites/obj_icons/icon_obj_cap_red";	

	if ( iCappingTeam == TF_TEAM_MERCENARY )
		return "sprites/obj_icons/icon_obj_cap_mercenary";	

	return "sprites/obj_icons/icon_obj_cap_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarFG( int index, int iOwningTeam )
{
	Assert( index < m_iNumControlPoints );

	if ( iOwningTeam == TF_TEAM_RED )
		return "progress_bar_red";

	if ( iOwningTeam == TF_TEAM_BLUE )
		return "progress_bar_blu";

	if (iOwningTeam == TF_TEAM_MERCENARY)
		return "progress_bar_mercenary";

	return "progress_bar";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarBG( int index, int iCappingTeam )
{
	Assert( index < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "progress_bar_red";

	if ( iCappingTeam == TF_TEAM_MERCENARY )
		return "progress_bar_mercenary";

	return "progress_bar_blu";
}

void C_TFObjectiveResource::SetCappingTeam( int index, int team )
{
	//Display warning that someone is capping our point.
	//Only do this at the start of a cap and if WE own the point.
	//Also don't warn on a point that will do a "Last Point cap" warning.
	if ( GetNumControlPoints() > 0 && GetCapWarningLevel( index ) == CP_WARN_NORMAL && GetCPCapPercentage( index ) == 0.0f && team != TEAM_UNASSIGNED && GetOwningTeam( index ) != TEAM_UNASSIGNED )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			int iLocalTeam = pLocalPlayer->GetTeamNumber();

			if ( iLocalTeam != team )
			{
				TeamplayRoundBasedRules()->BroadcastSound( iLocalTeam, "ControlPointContested" ); // Stickynote: Figure out if this plays or not, i think not
			}
		}
	}

	BaseClass::SetCappingTeam( index, team );
}
