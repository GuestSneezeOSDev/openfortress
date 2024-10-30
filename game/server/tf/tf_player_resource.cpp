//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_OFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 12, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClassModifier ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClassModifier ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_vecColors ), SendPropVector( SENDINFO_ARRAY3( m_vecColors ), 12, SPROP_COORD ) ),
	// THIS IS UNOPTIMIZED LIKE SHIT BUT THERE'S LITERALLY NO OTHER WAY
	SendPropArray3( SENDINFO_ARRAY3( m_flCosmetics ), SendPropFloat( SENDINFO_ARRAY( m_flCosmetics ), 9, SPROP_UNSIGNED | SPROP_NOSCALE | SPROP_ROUNDUP ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCosmetics2 ), SendPropFloat( SENDINFO_ARRAY( m_flCosmetics2 ), 9, SPROP_UNSIGNED | SPROP_NOSCALE | SPROP_ROUNDUP ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCosmetics3 ), SendPropFloat( SENDINFO_ARRAY( m_flCosmetics3 ), 9, SPROP_UNSIGNED | SPROP_NOSCALE | SPROP_ROUNDUP ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCosmetics4 ), SendPropFloat( SENDINFO_ARRAY( m_flCosmetics4 ), 9, SPROP_UNSIGNED | SPROP_NOSCALE | SPROP_ROUNDUP ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

CTFPlayerResource::CTFPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	int i;

	BaseClass::UpdatePlayerData();

	for ( i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
			if ( pPlayerStats ) 
			{
				m_iMaxHealth.Set( i, pPlayer->GetPlayerClass()->GetMaxHealth() );
				m_iPlayerClass.Set( i, pPlayer->GetPlayerClass()->GetClassIndex() );
				m_iPlayerClassModifier.Set( i, pPlayer->GetPlayerClass()->GetModifier() );
				int iTotalScore = CTFGameRules::CalcPlayerScore( &pPlayerStats->statsAccumulated );
				m_iTotalScore.Set( i, iTotalScore );
				
				m_vecColors.Set( i, pPlayer->m_vecPlayerColor );
			}
		}
	}
}

void CTFPlayerResource::UpdatePlayerCosmetics( CTFPlayer *pPlayer )
{
	int iIndex = pPlayer->entindex();
	// Optimize this later, way too tired to do it now
	if ( iIndex < 48 )
	{
		if ( iIndex < 32 )
		{
			if ( iIndex < 16 )
			{
				int iPos = (iIndex - 1) * 33;
				m_flCosmetics.Set( iPos, pPlayer->m_flCosmetics.Count() );
				int iMaxCosmetics = min(pPlayer->m_flCosmetics.Count(), 32);
				for( int y = 0; y < iMaxCosmetics; y++ )
				{
					m_flCosmetics.Set( iPos + (y + 1), pPlayer->m_flCosmetics[y] );
				}
			}
			else
			{
				int iPos = (iIndex - 16) * 33;
				m_flCosmetics2.Set( iPos, pPlayer->m_flCosmetics.Count() );
				for( int y = 0; y < pPlayer->m_flCosmetics.Count(); y++ )
				{
					m_flCosmetics2.Set( iPos + (y + 1), pPlayer->m_flCosmetics[y] );
				}
			}
		}
		else
		{
			int iPos = (iIndex - 32) * 33;
			m_flCosmetics3.Set( iPos, pPlayer->m_flCosmetics.Count() );
			for( int y = 0; y < pPlayer->m_flCosmetics.Count(); y++ )
			{
				m_flCosmetics3.Set( iPos + (y + 1), pPlayer->m_flCosmetics[y] );
			}
		}
	}
	else
	{
			int iPos = (iIndex - 48) * 33;
			m_flCosmetics4.Set( iPos, pPlayer->m_flCosmetics.Count() );
			for( int y = 0; y < pPlayer->m_flCosmetics.Count(); y++ )
			{
				m_flCosmetics4.Set( iPos + (y + 1), pPlayer->m_flCosmetics[y] );
			}

	}	
}

void CTFPlayerResource::Spawn( void )
{
	int i;

	for ( i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iTotalScore.Set( i, 0 );
		m_iMaxHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iPlayerClass.Set( i, TF_CLASS_UNDEFINED );
		m_iPlayerClassModifier.Set( i, TF_CLASSMOD_NONE );
		m_vecColors.Set( i, Vector( 0.0, 0.0, 0.0 ) );
	}
	
	// im going to hazbin hotel for this
	for( i = 0; i < 528; i++ )
	{
		m_flCosmetics.Set( i, 0 );
		m_flCosmetics2.Set( i, 0 );
		m_flCosmetics3.Set( i, 0 );
		m_flCosmetics4.Set( i, 0 );
	}
	for( i = 528; i < 561; i++ )
		m_flCosmetics4.Set( i, 0 );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}

Color CTFPlayerResource::GetPlayerColor( int iIndex )
{
	if ( m_vecColors[iIndex].x < 0 || m_vecColors[iIndex].y < 0 || m_vecColors[iIndex].z < 0 )
	{
		return Color( RandomFloat( 0.0f, 1.0f ) * 255.0, RandomFloat( 0.0f, 1.0f ) * 255.0, RandomFloat( 0.0f, 1.0f ) * 255.0, 255 );
	}
	
	return Color( m_vecColors[iIndex].x * 255.0, m_vecColors[iIndex].y * 255.0, m_vecColors[iIndex].z * 255.0, 255 );
}