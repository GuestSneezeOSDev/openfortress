//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TF_PlayerResource, DT_OFPlayerResource, CTFPlayerResource )
	RecvPropArray3( RECVINFO_ARRAY( m_iTotalScore ), RecvPropInt( RECVINFO( m_iTotalScore[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxHealth ), RecvPropInt( RECVINFO( m_iMaxHealth[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerClass ), RecvPropInt( RECVINFO( m_iPlayerClass[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerClassModifier ), RecvPropInt( RECVINFO( m_iPlayerClassModifier[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_vecColors ), RecvPropVector( RECVINFO( m_vecColors[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCosmetics ), RecvPropFloat( RECVINFO( m_flCosmetics[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCosmetics2 ), RecvPropFloat( RECVINFO( m_flCosmetics2[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCosmetics3 ), RecvPropFloat( RECVINFO( m_flCosmetics3[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCosmetics4 ), RecvPropFloat( RECVINFO( m_flCosmetics4[0] ) ) ),
END_RECV_TABLE()

extern ConVar of_tennisball;
extern ConVar of_force_cosmetics;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TF_PlayerResource::C_TF_PlayerResource()
{
	m_Colors[TEAM_UNASSIGNED] = COLOR_TF_SPECTATOR;
	m_Colors[TEAM_SPECTATOR] = COLOR_TF_SPECTATOR;
	m_Colors[TF_TEAM_RED] = COLOR_RED;
	m_Colors[TF_TEAM_BLUE] = COLOR_BLUE;
	m_Colors[TF_TEAM_MERCENARY] = COLOR_MERCENARY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TF_PlayerResource::~C_TF_PlayerResource()
{
}
const Vector &C_TF_PlayerResource::GetPlayerColorVector( int iIndex )
{
	if( of_tennisball.GetInt() > 1 )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer )
			iIndex = pPlayer->entindex();
	}
	
	if ( !IsConnected( iIndex ) )
	{
		static Vector White( 1, 1, 1 );
		return White;
	}
	return m_vecColors[iIndex];
}
Color C_TF_PlayerResource::GetPlayerColor( int iIndex )
{
	const Vector &vecColor = GetPlayerColorVector( iIndex );
	return Color( vecColor.x * 255.0f, vecColor.y * 255.0f, vecColor.z * 255.0f, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetArrayValue( int iIndex, int *pArray, int iDefaultVal )
{
	if ( !IsConnected( iIndex ) )
	{
		return iDefaultVal;
	}
	return pArray[iIndex];
}
//-----------------------------------------------------------------------------
// Purpose: Oh boy, gets the cosmetic index for a specified player
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetPlayerCosmetic( int iPlayerIndex, int iIndex )
{
	if( of_force_cosmetics.GetBool() )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer )
			iPlayerIndex = pPlayer->entindex();
	}

	if ( !IsConnected( iPlayerIndex ) )
	{
		return -1;
	}
	
	// Cant have more than 32 cosmetics
	if( iIndex > 32 )
		return -1;
	// Quick rundown, We take the player index, and multiply it by the amount of data a single person can take up
	// which is 32 for cosmetics, and 1 for the cosmetic count, then we skip the count and continue afterwards with the Index

	iIndex++; // skip the first index since its used for cosmetic count
	
	int iPos = 0;
	
	if ( iPlayerIndex < 48 )
	{
		if ( iPlayerIndex < 32 )
		{
			if ( iPlayerIndex < 16 )
			{
				iPos = (iPlayerIndex - 1) * 33;
				return m_flCosmetics[iPos + iIndex];
			}
			else
			{
				iPos = (iPlayerIndex - 16) * 33;
				return m_flCosmetics2[iPos + iIndex];
			}
		}
		else
		{
			iPos = (iPlayerIndex - 32) * 33;
			return m_flCosmetics3[iPos + iIndex];
		}
	}
	else
	{
			iPos = (iPlayerIndex  - 48) * 33;
			return m_flCosmetics4[iPos + iIndex];
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Oh boy, gets the cosmetic index for a specified player
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetPlayerCosmeticSkin( int iPlayerIndex, int iIndex )
{
	if( of_force_cosmetics.GetBool() )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer )
			iPlayerIndex = pPlayer->entindex();
	}
	
	if ( !IsConnected( iPlayerIndex ) )
	{
		return 0;
	}
	
	// Cant have more than 32 cosmetics
	if( iIndex > 32 )
		return 0;
	// Quick rundown, We take the player index, and multiply it by the amount of data a single person can take up
	// which is 32 for cosmetics, and 1 for the cosmetic count, then we skip the count and continue afterwards with the Index

	iIndex++; // skip the first index since its used for cosmetic count
	
	int iPos = 0;
	
	float flCosmetic = 0;
	
	if ( iPlayerIndex < 48 )
	{
		if ( iPlayerIndex < 32 )
		{
			if ( iPlayerIndex < 16 )
			{
				iPos = (iPlayerIndex - 1) * 33;
				flCosmetic = m_flCosmetics[iPos + iIndex];
			}
			else
			{
				iPos = (iPlayerIndex - 16) * 33;
				flCosmetic = m_flCosmetics2[iPos + iIndex];
			}
		}
		else
		{
			iPos = (iPlayerIndex - 32) * 33;
			flCosmetic = m_flCosmetics3[iPos + iIndex];
		}
	}
	else
	{
			iPos = (iPlayerIndex  - 48) * 33;
			flCosmetic = m_flCosmetics4[iPos + iIndex];
	}

	// This is real hacky but more often than not the float is missing 1 for some reason
	// So just add that back, it gets rounded down anyways so we dont need to worry about it
	flCosmetic += 0.001;

	flCosmetic = flCosmetic - (int)flCosmetic;
	
	flCosmetic *= 100;

	return flCosmetic;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the amounts of cosmetics a player has equipped
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetPlayerCosmeticCount( int iIndex )
{
	if( of_force_cosmetics.GetBool() )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer )
			iIndex = pPlayer->entindex();
	}
	
	if ( !IsConnected( iIndex ) )
	{
		return -1;
	}
	// Quick rundown, We take the player index, and multiply it by the amount of data a single person can take up
	// which is 32 for cosmetics, and 1 for the cosmetic count, then we skip the count and continue afterwards with the Index

	int iPos = 0;
	
	if ( iIndex < 48 )
	{
		if ( iIndex < 32 )
		{
			if ( iIndex < 16 )
			{
				iPos = (iIndex - 1) * 33;
				return m_flCosmetics[iPos];
			}
			else
			{
				iPos = (iIndex - 16) * 33;
				return m_flCosmetics2[iPos];
			}
		}
		else
		{
			iPos = (iIndex - 32) * 33;
			return m_flCosmetics3[iPos];
		}
	}
	else
	{
			iPos = (iIndex - 48) * 33;
			return m_flCosmetics4[iPos];
	}	
	
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer /*=false*/ )
{
	int count = 0;
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	for ( int i = 1 ; i <= MAX_PLAYERS ; i++ )
	{
		if ( bExcludeLocalPlayer && ( i == iLocalPlayerIndex ) )
		{
			continue;
		}

		if ( ( GetTeam( i ) == iTeam ) && ( GetPlayerClass( i ) == iClass ) )
		{
			count++;
		}
	}

	return count;
}

void C_TF_PlayerResource::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}

void CommPlayerGetCosmetics( const CCommand &args )
{
	int value = Q_atoi(args[1]);
	
	int iCount = GetTFPlayerResource()->GetPlayerCosmeticCount( value );
	for( int i = 0; i < iCount; i++ )
	{
		DevMsg( "%d ", GetTFPlayerResource()->GetPlayerCosmetic( value, i ) );
	}
	DevMsg("\n");
}
static ConCommand getplayercosmetics( "getplayercosmetics", CommPlayerGetCosmetics, "fuck", FCVAR_NONE );