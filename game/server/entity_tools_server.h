//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CServerTools
//
//=============================================================================//
#ifndef ENTITY_TOOLS_SERVER_H
#define ENTITY_TOOLS_SERVER_H

#ifdef _WIN32
#pragma once
#endif

#include "icliententity.h"
#include "iserverentity.h"
#include "toolframework/itoolentity.h"

//-----------------------------------------------------------------------------
// Interface from engine to tools for manipulating entities
//-----------------------------------------------------------------------------
class CServerTools : public IServerTools
{
public:
	// Inherited from IServerTools
	virtual IServerEntity *GetIServerEntity( IClientEntity *pClientEntity );
	virtual bool GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer = NULL );
	virtual bool SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer = NULL );
	virtual int GetPlayerFOV( IClientEntity *pClientPlayer = NULL );
	virtual bool SetPlayerFOV( int fov, IClientEntity *pClientPlayer = NULL );
	virtual bool IsInNoClipMode( IClientEntity *pClientPlayer = NULL );
	virtual CBaseEntity *FirstEntity( void );
	virtual CBaseEntity *NextEntity( CBaseEntity *pEntity );
	virtual CBaseEntity *FindEntityByHammerID( int iHammerID );
	virtual bool GetKeyValue( CBaseEntity *pEntity, const char *szField, char *szValue, int iMaxLen );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const char *szValue );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, float flValue );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const Vector &vecValue );
	virtual CBaseEntity *CreateEntityByName( const char *szClassName );
	virtual void DispatchSpawn( CBaseEntity *pEntity );
	virtual void ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen );
	virtual void AddOriginToPVS( const Vector &org );
	virtual void MoveEngineViewTo( const Vector &vPos, const QAngle &vAngles );
	virtual bool DestroyEntityByHammerId( int iHammerID );
	virtual CBaseEntity *GetBaseEntityByEntIndex( int iEntIndex );
	virtual void RemoveEntity( CBaseEntity *pEntity );
	virtual void RemoveEntityImmediate( CBaseEntity *pEntity );
	virtual IEntityFactoryDictionary *GetEntityFactoryDictionary( void );
	virtual void SetMoveType( CBaseEntity *pEntity, int val );
	virtual void SetMoveType( CBaseEntity *pEntity, int val, int moveCollide );
	virtual void ResetSequence( CBaseAnimating *pEntity, int nSequence );
	virtual void ResetSequenceInfo( CBaseAnimating *pEntity );
	virtual void ClearMultiDamage( void );
	virtual void ApplyMultiDamage( void );
	virtual void AddMultiDamage( const CTakeDamageInfo &pTakeDamageInfo, CBaseEntity *pEntity );
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

	virtual ITempEntsSystem *GetTempEntsSystem( void );
	virtual CBaseTempEntity *GetTempEntList( void );
	virtual CGlobalEntityList *GetEntityList( void );
	virtual bool IsEntityPtr( void *pTest );
	virtual CBaseEntity *FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName );
	virtual CBaseEntity *FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL );
	virtual CBaseEntity *FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius );
	virtual CBaseEntity *FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName );
	virtual CBaseEntity *FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName );
	virtual CBaseEntity *FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius );
	virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius );
	virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs );
	virtual CBaseEntity *FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold );
	virtual CBaseEntity *FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname );
	virtual CBaseEntity *FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CServerTools g_ServerTools;
#endif // ENTITY_TOOLS_SERVER_H