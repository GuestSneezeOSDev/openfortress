//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for all animating characters and objects.
//
//=============================================================================//

#include "cbase.h"
#include "of_baseschemaitem.h"
#include "of_items_game.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#else
#include "dt_utlvector_send.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
#define Shared_VarArgs	UTIL_VarArgs
#else
#define Shared_VarArgs	VarArgs
#endif

BEGIN_NETWORK_TABLE_NOBASE( CTFAttribute, DT_Attribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iID ) ),
	RecvPropString( RECVINFO( m_iszValue ) )
#else
	SendPropInt( SENDINFO( m_iID ) ),
	SendPropString( SENDINFO( m_iszValue ) )
#endif
END_NETWORK_TABLE()

// Maybe replace with Init()? - Kay
CTFAttribute::CTFAttribute()
{
	m_iID = -1;
	m_iszValue.GetForModify()[0] = '\0';
}

CTFAttribute::CTFAttribute( int iID, int iValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), Shared_VarArgs("%d", iValue), OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, bool bValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), Shared_VarArgs("%d", bValue), OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, float flValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), Shared_VarArgs("%f", flValue), OF_MAX_ATTRIBUTE_LENGHT);
}
#ifdef GAME_DLL
CTFAttribute::CTFAttribute( int iID, string_t szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), STRING(szValue), OF_MAX_ATTRIBUTE_LENGHT);
}
#endif
CTFAttribute::CTFAttribute( int iID, const char *szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), szValue, OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, char *szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), szValue, OF_MAX_ATTRIBUTE_LENGHT);
}

int CTFAttribute::GetInt( void )
{
	return Q_atoi(m_iszValue.Get());
}

bool CTFAttribute::GetBool( void )
{
	return !!Q_atoi(m_iszValue.Get());
}

float CTFAttribute::GetFloat( void )
{
	return Q_atof(m_iszValue.Get());
}

string_t CTFAttribute::GetString( void )
{
	return MAKE_STRING(m_iszValue.Get());
}

BEGIN_DATADESC( CBaseSchemaEntity )
	DEFINE_FIELD( m_iszSchemaName, FIELD_STRING ),
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( BaseSchemaEntity, DT_BaseSchemaEntity )

BEGIN_NETWORK_TABLE( CBaseSchemaEntity, DT_BaseSchemaEntity )
#ifdef GAME_DLL
	SendPropString( SENDINFO( m_iszSchemaName ) ),
	SendPropUtlVector( 
		SENDINFO_UTLVECTOR( m_Attributes ),
		32, // max elements
		SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( DT_Attribute ) ) )
#else
	RecvPropString( RECVINFO( m_iszSchemaName )),
	RecvPropUtlVector( 
		RECVINFO_UTLVECTOR( m_Attributes ), 
		32,
		RecvPropDataTable( NULL, 0, 0, &REFERENCE_RECV_TABLE( DT_Attribute ) ) )
#endif
END_NETWORK_TABLE()


CBaseSchemaEntity::CBaseSchemaEntity()
{
	m_iszSchemaName.GetForModify()[0] = '\0';
	// Maybe replace with Init()? - Kay
}

CBaseSchemaEntity::~CBaseSchemaEntity()
{
}

void CBaseSchemaEntity::AddAttribute( CTFAttribute hAttribute )
{
	m_Attributes.AddToTail(hAttribute);
}

void CBaseSchemaEntity::RemoveAttribute(int iID)
{
	FOR_EACH_VEC(m_Attributes, i)
	{
		if( m_Attributes[i].GetID() == iID )
		{
			m_Attributes.Remove(i);
			return;
		}
	}
}

void CBaseSchemaEntity::RemoveAttribute( char *szName )
{
	int iID = GetItemSchema()->GetAttributeID( szName );

	// Return if its an invalid Index
	if( iID == -1 )
		return;

	FOR_EACH_VEC(m_Attributes, i)
	{
		if( m_Attributes[i].GetID() == iID )
		{
			m_Attributes.Remove(i);
			return;
		}
	}
}

void CBaseSchemaEntity::RemoveAttribute( const char *szName )
{
	char szFullName[128];

	Q_strncpy(szFullName, szName, 128);
	RemoveAttribute(szFullName);
}

void CBaseSchemaEntity::PurgeAttributes()
{
	m_Attributes.Purge();
}

// Get a pointer to the Attribute
CTFAttribute *CBaseSchemaEntity::GetAttribute( const char *szName ) const
{
	int iFindID = GetItemSchema()->GetAttributeID(szName);

	// Return nothing if its an invalid Index
	if( iFindID == -1 )
		return NULL;

	FOR_EACH_VEC(m_Attributes, i)
	{
		CTFAttribute *pAttribute = const_cast<CTFAttribute *>(&m_Attributes[i]);
		if( pAttribute->GetID() == iFindID )
			return pAttribute;
	}

	// None Found, return Null
	return NULL;
}

bool CBaseSchemaEntity::GetAttributeValue_Int( const char *szName, int &iValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		iValue = pAttribute->GetInt();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_Bool( const char *szName, bool &bValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		bValue = pAttribute->GetBool();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_Float( const char *szName, float &flValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		flValue = pAttribute->GetFloat();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_String( const char *szName, string_t &szValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		szValue = pAttribute->GetString();

	return bSuccess;
}