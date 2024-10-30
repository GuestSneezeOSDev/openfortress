//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_BASESCHEMAITEM_H
#define OF_BASESCHEMAITEM_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#define CBaseSchemaEntity C_BaseSchemaEntity
	#define CTFAttribute C_TFAttribute
	
	// Since CBaseAnimating doesn't do this on it's own
	#define CBaseAnimating C_BaseAnimating

	#include "c_baseanimating.h"
#else
	#include "baseanimating.h"
#endif

//=============
// Copied from CTFPlayerShared
// Assuming this is sometimes needed if we have another entity calling for DT_Attribute
// If you're sure we wont ever need it, feel free to remove
//=============

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_Attribute );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_Attribute );
#endif

#define OF_MAX_ATTRIBUTE_LENGHT 64

class CTFAttribute
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CTFAttribute );

	// Mandatory shared
	CTFAttribute();

	// ========================
	// If necessary, could probably move these to only be in the server
	CTFAttribute( int iID, int iValue );
	CTFAttribute( int iID, bool bValue );
	CTFAttribute( int iID, float flValue );
#ifdef GAME_DLL
	// string_t is serverside only
	CTFAttribute( int iID, string_t szValue );
#endif
	CTFAttribute( int iID, const char *szValue );
	CTFAttribute( int iID, char *szValue );
	// ========================

	// Mandatory Shared
	int GetID() { return m_iID; };

	int GetInt( void );
	bool GetBool( void );
	float GetFloat( void );
	string_t GetString( void );
	
private:
	CNetworkVar( int, m_iID );
	CNetworkString( m_iszValue, OF_MAX_ATTRIBUTE_LENGHT );
};

class CBaseSchemaEntity : public CBaseAnimating
{
public:
	DECLARE_CLASS( CBaseSchemaEntity, CBaseAnimating );

	CBaseSchemaEntity();
	~CBaseSchemaEntity();

	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

public:

	const char *GetSchemaName()
	{
		return m_iszSchemaName.GetForModify()[0] != '\0' ? (char*)m_iszSchemaName.Get() : GetClassname();
	};
	
	void SetSchemaName( char *szSchemaName )
	{
		Q_strncpy( m_iszSchemaName.GetForModify(), szSchemaName, 128);
	};
	
	void SetSchemaName( const char *szSchemaName )
	{
		Q_strncpy( m_iszSchemaName.GetForModify(), szSchemaName, 128);
	};
	
	bool IsSchemaItem()
	{
		return m_iszSchemaName.GetForModify()[0] != '\0';
	};

	// ========================
	// If necessary, could probably move these to only be in the server
	void AddAttribute( CTFAttribute hAttribute );
	void RemoveAttribute( int iID );
	void RemoveAttribute( char *szName );
	void RemoveAttribute( const char *szName );
	void PurgeAttributes();
	// ========================

	// Mandatory Shared
	CTFAttribute *GetAttribute( const char *szName ) const;

	CTFAttribute *GetAttribute( int i )  const{ return i < m_Attributes.Count() ? const_cast<CTFAttribute *>(&m_Attributes[i]) : NULL; }
	int GetAttributeCount( void ){ return m_Attributes.Count(); }

	bool GetAttributeValue_Int( const char *szName, int &iValue ) const;
	bool GetAttributeValue_Bool( const char *szName, bool &bValue ) const;
	bool GetAttributeValue_Float( const char *szName, float &flValue ) const;

	// string_t is const char* on the Client, just a note
	bool GetAttributeValue_String( const char *szName, string_t &szValue ) const;

public:

protected:
	CNetworkString(m_iszSchemaName, 128);
	
	CUtlVector< CTFAttribute > m_Attributes;

	friend class CTFPlayer;
	friend class CTFPlayerShared;
};

#endif // OF_BASESCHEMAITEM_H
