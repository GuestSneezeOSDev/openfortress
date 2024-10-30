#ifndef OF_ITEMS_GAME_H
#define OF_ITEMS_GAME_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

typedef unsigned short ItemsGameHandle;

class KeyValues;

#define DeallocStringIfExists( string ) \
	if( string ) \
		delete string;
		
extern void InitItemSchema();

class COFParticleInfo
{
public:
	COFParticleInfo()
	{
		m_flLoopTime = 1.2f;
		m_flParticleZOffset = 0.0f;
	}

	void Parse( KeyValues *pKV )
	{
		m_flLoopTime = pKV->GetFloat( "loop_time", 1.2f );
		m_flParticleZOffset = pKV->GetFloat( "particle_z_offset", 0.0f );
	}
public:
	float m_flLoopTime;
	float m_flParticleZOffset;
};

class COFCosmeticInfo
{
public:
	COFCosmeticInfo()
	{
		m_szModel = NULL;
		m_szViewModel = NULL;
		m_szRegion = NULL;
#ifdef CLIENT_DLL
		m_szBackpackIcon = NULL;
#endif
		m_bTeamSkins = true;
		m_bUsesBrightskins = false;
		m_iSkinOffset = 0;
	}

	~COFCosmeticInfo()
	{
		DeallocStringIfExists( m_szModel );
		DeallocStringIfExists( m_szViewModel );
#ifdef CLIENT_DLL
		DeallocStringIfExists( m_szBackpackIcon );
#endif
		m_StyleInfo.Purge();
	}

	void Parse( KeyValues *pKV );

	COFCosmeticInfo *GetStyle( int iStyle )
	{
		if( iStyle == 0 )
			return this;

		iStyle--;
		if( m_StyleInfo.IsValidIndex(iStyle) )
			return &m_StyleInfo[iStyle];

		return this;
	}
public:
	const char *m_szModel;
	const char *m_szViewModel;
	const char *m_szRegion;
#ifdef CLIENT_DLL
	const char *m_szBackpackIcon;
#endif
	bool m_bTeamSkins;
	bool m_bUsesBrightskins;
	int m_iSkinOffset;
	CUtlDict<int, ItemsGameHandle> m_Bodygroups;
	CUtlVector<COFCosmeticInfo> m_StyleInfo;
};

class COFSchemaWeaponInfo
{
public:
	COFSchemaWeaponInfo()
	{
		m_szWeaponName = NULL;
		m_szWeaponClass = NULL;
		m_szBackpackIcon = NULL;
		m_bShowInLoadout = false;
		m_iLoadoutAnim = 0;
		m_bParsedThroughFile = false;

		memset(m_iWeaponSlot, -1, TF_CLASS_COUNT * sizeof(int));
	}

	virtual ~COFSchemaWeaponInfo()
	{
		DeallocStringIfExists( m_szWeaponName );
		DeallocStringIfExists( m_szWeaponClass );

		if( !m_bParsedThroughFile )
			DeallocStringIfExists( m_szBackpackIcon );
	}

	virtual void Parse( KeyValues *pKV, KeyValues *pBase = NULL );
	virtual void ParseFromFile( IFileSystem* filesystem, const char *szWeaponName, const unsigned char *pICEKey );

public:

	const char *m_szWeaponName;
	const char *m_szWeaponClass;
	const char *m_szBackpackIcon;
	bool m_bShowInLoadout;
	int m_iLoadoutAnim;
	WEAPON_FILE_INFO_HANDLE m_nWeaponHandle;
	int m_iWeaponSlot[TF_CLASS_COUNT];

	bool m_bParsedThroughFile;

	class COFAttributeInfo
	{
	public:
		COFAttributeInfo()
		{
			m_szName = NULL;
			m_szValue = NULL;
		}

		~COFAttributeInfo()
		{
			DeallocStringIfExists( m_szName );
			DeallocStringIfExists( m_szValue );
		}

		char const *m_szName;
		char const *m_szValue;
	};

	CUtlVector<COFAttributeInfo> m_Attributes;
};

class CTFItemSchema
{
public:
	CTFItemSchema();

	void ParseItemsGame( void );
	void ParseLevelItems( void );
	void PurgeSchema( void );
	void PurgeLevelItems( void );

	void OnMapChange( void );
	
	void AddWeapon( const char *szWeaponName );
	void AddLevelWeapon( const char *szWeaponName );
	COFSchemaWeaponInfo *GetWeapon( unsigned short iID );
	COFSchemaWeaponInfo *GetWeapon( const char *szWeaponName );

	COFCosmeticInfo *GetCosmetic( int iIndex );
	int GetCosmeticInOrder( int iIndex )
	{
		return m_CosmeticOrder[iIndex];
	}
	int GetCategoryCount(){ return m_Categories.Count(); }

	bool CategoryExists( const char *szName ){ return CategoryExists( const_cast<char*>(szName) ); }
	bool CategoryExists( char *szName )
	{
		int i = GetCategoryID( szName );
		return i != m_Categories.InvalidIndex();
	}

	int GetCategoryID( char *szName )
	{ 
		int iRegion = m_Categories.InvalidIndex();
		FOR_EACH_VEC( m_Categories, i )
		{
			if( !Q_strcasecmp(m_Categories[i], szName) )
			{
				iRegion = i;
				break;
			}
		}

		return iRegion;
	}
	int GetCategoryID( const char *szName )
	{ 
		return GetCategoryID( const_cast<char*>(szName) );
	}
	char *GetCategoryName( int i ){ return m_Categories[i]; }

	COFParticleInfo *GetRespawnParticle( int i );

	void AddAttribute( int iID, const char *szName );
	int GetAttributeID( char *szName );
	int GetAttributeID( const char *szName );

	bool WeaponIDExists( ItemsGameHandle handle ){ return handle != m_Weapons.InvalidIndex(); }
	ItemsGameHandle GetWeaponID( const char *szWeaponName );
	const char *GetWeaponName( ItemsGameHandle iID );
	
	int GetWeaponCount( void ){ return m_Weapons.Count(); }
	int GetCosmeticCount( void ){ return m_Cosmetics.Count(); }
	int GetRespawnParticleCount( void ){ return m_RespawnParticles.Count(); }
	char *AddOrReturnRegionName( const char *szName );

	// Primarily used for weapon parsing oustide of the initial load
	// do NOT overuse this
	//KeyValues *GetWeaponData( char *szWeaponName );	
private:
	bool m_bWeaponTesting;

	friend void ReloadItemsSchema();

	CUtlDict< COFSchemaWeaponInfo*, ItemsGameHandle > m_Weapons;
	CUtlVector< ItemsGameHandle > m_LevelWeapons;

	CUtlMap<int, COFCosmeticInfo *>	m_Cosmetics;
	CUtlVector<int>	m_CosmeticOrder;
	CUtlVector<int>	m_LevelCosmetics;

	CUtlStringList m_Categories;

	CUtlMap<int, COFParticleInfo *>	m_RespawnParticles;
	CUtlDict<int, ItemsGameHandle> m_AttributeList;
};

extern void InitItemSchema();
extern CTFItemSchema *GetItemSchema();

#undef DeallocStringIfExists

#endif // OF_ITEMS_GAME_H