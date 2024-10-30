//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================
#ifndef TF_PLAYERCLASS_SHARED_H
#define TF_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#define TF_NAME_LENGTH		128

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerClassShared );

#endif


//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------

#define MAX_PLAYERCLASS_SOUND_LENGTH	128

struct TFPlayerClassData_t
{
	char 	m_szClassName[ TF_NAME_LENGTH ];

	char 	m_szModelName[ TF_NAME_LENGTH ];
	char 	m_szArmModelName[ TF_NAME_LENGTH ];
	char 	m_szLocalizableName[ TF_NAME_LENGTH ];
	char 	m_szJumpSound[ TF_NAME_LENGTH ];

	float 	m_flMaxSpeed;
	int 	m_nMaxHealth;
	int 	m_nMaxArmor;
	int 	m_nArmorType;
	int 	m_nStarterArmor;
	
	int 	m_aWeapons[ TF_PLAYER_WEAPON_COUNT ];
	
	int 	m_iWeaponCount;
	
	int 	m_aGrenades[ TF_PLAYER_GRENADE_COUNT ];
	int 	m_aAmmoMax[ TF_AMMO_COUNT ];
	int 	m_aBuildable[ TF_PLAYER_BUILDABLE_COUNT ];

	int 	m_nCapNumber;
	int 	m_nMaxAirDashCount;
	bool 	m_bDontDoAirwalk;
	bool 	m_bDontDoNewJump;

	bool 	m_bSpecialClass;

	char 	m_szClassSelectImageRed[ TF_NAME_LENGTH ];
	char 	m_szClassSelectImageBlue[ TF_NAME_LENGTH ];
	char 	m_szClassSelectImageMercenary[ TF_NAME_LENGTH ];	
	
	char 	m_szClassImageRed[ TF_NAME_LENGTH ];
	char 	m_szClassImageBlue[ TF_NAME_LENGTH ];
	char 	m_szClassImageMercenary[ TF_NAME_LENGTH ];
	char 	m_szClassImageColorless[ TF_NAME_LENGTH ];
	char 	m_szClassImageTennis[ TF_NAME_LENGTH ];
	
	char 	m_szClassIcon[ TF_NAME_LENGTH ];

	int 	m_nViewVector;

	bool 	m_bParsed;

#ifdef GAME_DLL
	// sounds
	char	m_szDeathSound[ MAX_PLAYERCLASS_SOUND_LENGTH ];
	char	m_szCritDeathSound[ MAX_PLAYERCLASS_SOUND_LENGTH ];
	char	m_szMeleeDeathSound[ MAX_PLAYERCLASS_SOUND_LENGTH ];
	char	m_szExplosionDeathSound[ MAX_PLAYERCLASS_SOUND_LENGTH ];
#endif

	TFPlayerClassData_t();
	const char *GetModelName() const;
	const char *GetArmModelName() const { return m_szArmModelName; }
	const char *GetClassSelectImageRed() const { return m_szClassSelectImageRed; }
	const char *GetClassSelectImageBlue() const { return m_szClassSelectImageBlue; }
	const char *GetClassSelectImageMercenary() const { return m_szClassSelectImageMercenary; }
	
	const char *GetClassImageRed() const { return m_szClassImageRed; }
	const char *GetClassImageBlue() const { return m_szClassImageBlue; }
	const char *GetClassImageMercenary() const { return m_szClassImageMercenary; }
	const char *GetClassImageColorless() const { return m_szClassImageColorless; }
	const char *GetClassImageTennis() const { return m_szClassImageTennis; }
	
	const char *GetClassIcon() const { return m_szClassIcon; }
	
	const char *GetJumpSound() const { return m_szJumpSound; }
	virtual void Parse( const char *pszClassName, int iClass );

private:

	// Parser for the class data.
	void ParseData( KeyValues *pKeyValuesData );
};

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTFPlayerClassShared
{
public:

	CTFPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerClassShared );

	bool		Init( int iClass, int iModifier = 0 );
	void		SetModifier( int iModifier );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void ) const							{ return m_iClass; }
	
	char		*GetName( void );
#ifndef CLIENT_DLL
	void		SetCustomModel( const char *pszModelName );
	void		SetCustomArmModel( const char *pszModelName );
	
	void		SetCustomHealth( int iHealth );
	void		SetCustomSpeed( float flSpeed );
#endif	
	char		*GetModelName( void ) const;
	char 		*GetSetCustomModel ( void ) const;		
	bool 		UsesCustomModel ( void ) const;	
	char 		*GetSetCustomArmModel ( void ) const;		
	bool 		UsesCustomArmModel ( void ) const;		

	char		*GetArmModelName( void ) const;

	float		GetMaxSpeed( void  )const;
	int			GetMaxHealth( void ) const;
	int			GetMaxArmor( void ) const;
	int			GetArmorType( void ) const;
	bool		IsSpecialClass( void ) const;
	bool		CanAirDash( void ) const;
	int			MaxAirDashCount( void ) const;
	int			GetCapNumber( void ) const;
	const char *GetJumpSound() const;

	TFPlayerClassData_t  *GetData( void ) const;

	// If needed, put this into playerclass scripts
	bool CanBuildObject( int iObjectType ) const;
	
	const int GetClass ( void ) const { return m_iClass; }
	const int GetModifier( void ) const { return m_iModifier; }
	void SetClass ( int iClass ) { m_iClass = iClass; }
	
protected:


	CNetworkVar( int,	m_iClass );
	CNetworkVar( int,	m_iModifier );

	CNetworkVar( int,	m_iSetCustomHealth );
	CNetworkVar( float,	m_flSetCustomSpeed );
#ifdef CLIENT_DLL
	char	m_iszSetCustomModel[MAX_PATH];
	char	m_iszSetCustomArmModel[MAX_PATH];
#else
	CNetworkVar(string_t, m_iszSetCustomModel);
	CNetworkVar(string_t, m_iszSetCustomArmModel);
#endif
};

void InitPlayerClasses( void );
TFPlayerClassData_t *GetPlayerClassData( int iClass, int iModifier );

#endif // TF_PLAYERCLASS_SHARED_H