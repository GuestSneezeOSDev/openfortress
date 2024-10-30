//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PLAYERRESOURCE_H
#define C_TF_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "c_playerresource.h"

class C_TF_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_TF_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_TF_PlayerResource();
	virtual ~C_TF_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetMaxHealth( int iIndex )   { return GetArrayValue( iIndex, m_iMaxHealth, TF_HEALTH_UNDEFINED ); }
	int GetPlayerClass( int iIndex ) { return GetArrayValue( iIndex, m_iPlayerClass, TF_CLASS_UNDEFINED ); }
	int GetPlayerClassModifier( int iIndex ) { return GetArrayValue( iIndex, m_iPlayerClassModifier, TF_CLASSMOD_NONE ); }
	int GetPlayerCosmetic( int iPlayerIndex ,int iIndex );
	int GetPlayerCosmeticSkin( int iPlayerIndex, int iIndex );
	int GetPlayerCosmeticCount( int iIndex );
	const Vector &GetPlayerColorVector( int iIndex );
	Color GetPlayerColor( int iIndex );
	
	int GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer = false );
	virtual void OnDataChanged( DataUpdateType_t updateType );
protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );

	int		m_iTotalScore[MAX_PLAYERS+1];
	int		m_iMaxHealth[MAX_PLAYERS+1];
	int		m_iPlayerClass[MAX_PLAYERS+1];
	int		m_iPlayerClassModifier[MAX_PLAYERS+1];
	Vector	m_vecColors[MAX_PLAYERS+1];
public:
	float m_flCosmetics[(16)*33];
	float m_flCosmetics2[(16)*33];
	float m_flCosmetics3[(16)*33];
	float m_flCosmetics4[(17)*33];
};

inline C_TF_PlayerResource *GetTFPlayerResource( void )
{
	if ( !g_PR )
		return NULL;

	return assert_cast<C_TF_PlayerResource *>( g_PR );
}

#endif // C_TF_PLAYERRESOURCE_H