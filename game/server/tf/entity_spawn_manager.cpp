//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CBaseEntity - entity_spawn_manager
- m_iszEntityName (Offset 856) (Save|Key)(4 Bytes) - entity_name
- m_iEntityCount (Offset 860) (Save|Key)(4 Bytes) - entity_count
- m_iRespawnTime (Offset 864) (Save|Key)(4 Bytes) - respawn_time
- m_bDropToGround (Offset 868) (Save|Key)(1 Bytes) - drop_to_ground
- m_bRandomRotation (Offset 869) (Save|Key)(1 Bytes) - random_rotation
*/

/*
CBaseEntity - entity_spawn_point
- m_iszSpawnManagerName (Offset 860) (Save|Key)(4 Bytes) - spawn_manager_name
*/

#include "cbase.h"
#include "datacache/imdlcache.h"
#include "entityapi.h"
#include "entityoutput.h"
#include "TemplateEntities.h"
#include "ndebugoverlay.h"
#include "mapentities.h"
#include "IEffects.h"
#include "props.h"
#include "tf_generic_bomb.h"
#include "entitylist.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_DESTINATION_ENTS	128

//=========================================================
//=========================================================
class CEntitySpawnPoint : public CServerOnlyPointEntity 
{
	DECLARE_CLASS( CEntitySpawnPoint, CServerOnlyPointEntity );

public:
	CEntitySpawnPoint();
	bool IsAvailable();	
	void SpawnedEntity( void );

	string_t m_iszSpawnManagerName;
	string_t GetManagerName( void );

	float		m_ReuseDelay;
	float		m_TimeNextAvailable;

	DECLARE_DATADESC();
};

class CEntitySpawnManager : public CBaseEntity, public IEntityListener
{
public:
	DECLARE_CLASS( CEntitySpawnManager, CBaseEntity );
	DECLARE_DATADESC();

	CEntitySpawnManager();

	void Spawn( void );
	void SpawnerThink( void );
	bool SpaceAvailable( const Vector &vecLocation );
	bool CanMakeEntity( void );

	bool m_bDropToGround;
	bool m_bRandomRotation;
	CBaseEntity *pent;

	virtual CEntitySpawnPoint *FindSpawnDestination();
	virtual void MakeEntity( void );

	float	GetRespawnTime( void );

	string_t m_iszEntityName;

	int		m_iRespawnTime;	

	int		m_iSpawnCount;
	
	int		m_nLiveChildren;
	int		m_iEntityCount;

	bool	 m_bSpawned;

	void	OnEntityDeleted( CBaseEntity *pEntity );
	void	UpdateOnRemove( void );
};

LINK_ENTITY_TO_CLASS( entity_spawn_point, CEntitySpawnPoint );

BEGIN_DATADESC( CEntitySpawnPoint )
	DEFINE_KEYFIELD( m_iszSpawnManagerName, FIELD_STRING, "spawn_manager_name" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEntitySpawnPoint::CEntitySpawnPoint()
{
	m_TimeNextAvailable = gpGlobals->curtime;
	m_ReuseDelay = 10;

	CEntitySpawnManager *pManager = dynamic_cast<CEntitySpawnManager *>( gEntList.FindEntityByName( NULL, STRING( m_iszSpawnManagerName ) ) );
	if ( pManager )
		m_ReuseDelay = pManager->GetRespawnTime();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEntitySpawnPoint::IsAvailable( void )
{
	if( m_TimeNextAvailable > gpGlobals->curtime )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::SpawnedEntity( void )
{
	m_TimeNextAvailable = gpGlobals->curtime + m_ReuseDelay;
}

string_t CEntitySpawnPoint::GetManagerName( void )
{
	return m_iszSpawnManagerName;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define MAX_SPAWN_POINTS 128

LINK_ENTITY_TO_CLASS( entity_spawn_manager, CEntitySpawnManager );

BEGIN_DATADESC( CEntitySpawnManager )
	DEFINE_KEYFIELD( m_iszEntityName, FIELD_STRING, "entity_name" ),
	DEFINE_KEYFIELD( m_iEntityCount, FIELD_INTEGER, "entity_count" ),
	DEFINE_KEYFIELD( m_iRespawnTime, FIELD_INTEGER, "respawn_time" ),
	DEFINE_KEYFIELD( m_bDropToGround, FIELD_BOOLEAN, "drop_to_ground" ),
	DEFINE_KEYFIELD( m_bRandomRotation, FIELD_BOOLEAN, "random_rotation" ),
	DEFINE_THINKFUNC( SpawnerThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEntitySpawnManager::CEntitySpawnManager()
{
	m_iszEntityName = NULL_STRING;
	m_iEntityCount = 0;
	m_iSpawnCount = 0;
	m_iRespawnTime = 10;
	m_bDropToGround = false;
	m_bRandomRotation = false;
	m_bSpawned = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::Spawn( void )
{
	m_bSpawned = false;

	m_nLiveChildren = 0;

	gEntList.AddListenerEntity( this );

	CEntitySpawnPoint *pDestinations[ MAX_DESTINATION_ENTS ];
	CBaseEntity *pEnt2 = NULL;

	int	counter = 0;

	pEnt2 = gEntList.FindEntityByClassname( NULL, "entity_spawn_point" );

	if( !pEnt2 )
	{
		Warning("CEntitySpawnManager (%s) doesn't have any spawn points!\n", GetDebugName() );
		gEntList.RemoveListenerEntity( this );
		UTIL_Remove( this );
	}
	
	while( pEnt2 )
	{
		CEntitySpawnPoint *pDest;

		pDest = dynamic_cast <CEntitySpawnPoint*>( pEnt2 );

		if ( pDest && pDest->GetManagerName() == GetEntityName() )
		{
			pDestinations[ counter ] = pDest;
			counter++;
			m_iSpawnCount = counter;
		}

		pEnt2 = gEntList.FindEntityByClassname( pEnt2, "entity_spawn_point" );
	}


	SetThink ( &CEntitySpawnManager::SpawnerThink );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

float CEntitySpawnManager::GetRespawnTime( void )
{
	return m_iRespawnTime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEntitySpawnManager::SpaceAvailable( const Vector &vecLocation )
{
	CBaseEntity *ent = NULL;

	for ( CEntitySphereQuery sphere( vecLocation, 5 ); ( ent = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( ent )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEntitySpawnManager::CanMakeEntity( )
{
	if ( m_iEntityCount > 0 && m_nLiveChildren >= m_iEntityCount )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::SpawnerThink ( void )
{
	if ( ( !m_bSpawned && m_iEntityCount > 0 && m_nLiveChildren >= m_iSpawnCount ) || m_nLiveChildren >= m_iEntityCount )
	{
		m_bSpawned = true;
		SetNextThink( gpGlobals->curtime + 8.0f );
	}

	if ( !m_bSpawned )
	{
		SetNextThink( gpGlobals->curtime + 0.05f );
	}

	if ( m_bSpawned )
	{
		SetNextThink( gpGlobals->curtime + 8.0f );
	}

	MakeEntity();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

CEntitySpawnPoint *CEntitySpawnManager::FindSpawnDestination()
{
	CEntitySpawnPoint *pDestinations[ MAX_DESTINATION_ENTS ];
	CBaseEntity *pEnt = NULL;

	int	count = 0;

	pEnt = gEntList.FindEntityByClassname( NULL, "entity_spawn_point" );

	if( !pEnt )
	{
		Warning("CEntitySpawnManager (%s) doesn't have any spawn points!\n", GetDebugName() );
		return NULL;
	}
	
	while( pEnt )
	{
		CEntitySpawnPoint *pDestination;

		pDestination = dynamic_cast <CEntitySpawnPoint*>(pEnt);

		if ( pDestination && pDestination->IsAvailable() && pDestination->GetManagerName() == GetEntityName() )
		{
			pDestinations[ count ] = pDestination;
			count++;
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "entity_spawn_point" );
	}

	if ( count < 1 )
		return NULL;
		for( int i = 0 ; i < 3 ; i++ )
		{
			CEntitySpawnPoint *pRandomDest = pDestinations[ rand() % count ];

			if( SpaceAvailable( pRandomDest->GetAbsOrigin() ) )
			{
				return pRandomDest;
			}
		}

	return NULL;
}

float GetBottomZ( const Vector &origin ) 
{
	trace_t	tr;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID_BRUSHONLY,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &tr );

	// This trace is ONLY used if we hit an entity flagged with FL_WORLDBRUSH
	trace_t	trEnt;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &trEnt );

	
	// Did we hit something closer than the floor?
	if ( trEnt.fraction < tr.fraction )
	{
		// If it was a world brush entity, copy location
		if ( trEnt.m_pEnt )
		{
			CBaseEntity *e = trEnt.m_pEnt;
			if ( e && (e->GetFlags() & FL_WORLDBRUSH) )
			{
				tr.endpos = trEnt.endpos;
			}
		}
	}

	return tr.endpos.z;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntitySpawnManager::MakeEntity( void )
{
	if ( !CanMakeEntity() )
		return;

	CEntitySpawnPoint *pDestination = NULL;

	pDestination = FindSpawnDestination();
	if ( !pDestination )
	{
		return;
	}

	CBaseEntity *pent = dynamic_cast< CBaseEntity * >( CreateEntityByName( STRING( m_iszEntityName ) ) );

	if ( !pent )
	{
		Warning( "CEntitySpawnManager attempting to spawn NULL entity \n");
		return;
	}

	pent->Precache();

	if ( m_bDropToGround )
	{
		Vector bottomPos = pDestination->GetAbsOrigin();
		bottomPos.z = GetBottomZ( bottomPos );
		pent->SetAbsOrigin( bottomPos );
	}
	else
	{
		pent->SetAbsOrigin( pDestination->GetAbsOrigin() );
	}

	if ( m_bRandomRotation )
	{
		pent->SetAbsAngles( QAngle( 0, ( random->RandomFloat( 0, 360 ) ), 0 ) );
	}

	pent->SetName( GetEntityName() );

	DispatchSpawn( pent );

	pDestination->SpawnedEntity();

	m_nLiveChildren++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::OnEntityDeleted( CBaseEntity *pEntity )
{
	if ( pEntity && pEntity->NameMatches( GetEntityName() )  )
	{
		if (m_nLiveChildren > 0)
			m_nLiveChildren--;
	}
	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::UpdateOnRemove( void )
{
	gEntList.RemoveListenerEntity( this );

	BaseClass::UpdateOnRemove();
}