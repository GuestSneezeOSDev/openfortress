//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "tf_playerclass_shared.h"
#include "of_items_game.h"

ConVar of_airdashcount( "of_airdashcount", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED );

#define TF_CLASS_UNDEFINED_FILE			""
#define TF_CLASS_SCOUT_FILE				"scripts/playerclasses/scout"
#define TF_CLASS_SNIPER_FILE			"scripts/playerclasses/sniper"
#define TF_CLASS_SOLDIER_FILE			"scripts/playerclasses/soldier"
#define TF_CLASS_DEMOMAN_FILE			"scripts/playerclasses/demoman"
#define TF_CLASS_MEDIC_FILE				"scripts/playerclasses/medic"
#define TF_CLASS_HEAVYWEAPONS_FILE		"scripts/playerclasses/heavyweapons"
#define TF_CLASS_PYRO_FILE				"scripts/playerclasses/pyro"
#define TF_CLASS_SPY_FILE				"scripts/playerclasses/spy"
#define TF_CLASS_ENGINEER_FILE			"scripts/playerclasses/engineer"
#define TF_CLASS_MERCENARY_FILE			"scripts/playerclasses/mercenary"
#define TF_CLASS_CIVILIAN_FILE			"scripts/playerclasses/civilian"
#define TF_CLASS_JUGGERNAUT_FILE		"scripts/playerclasses/juggernaut"

const char *s_aPlayerClassFiles[] =
{
	TF_CLASS_UNDEFINED_FILE,
	TF_CLASS_SCOUT_FILE,
	TF_CLASS_SNIPER_FILE,
	TF_CLASS_SOLDIER_FILE,
	TF_CLASS_DEMOMAN_FILE,
	TF_CLASS_MEDIC_FILE,
	TF_CLASS_HEAVYWEAPONS_FILE,
	TF_CLASS_PYRO_FILE,
	TF_CLASS_SPY_FILE,
	TF_CLASS_ENGINEER_FILE,
	TF_CLASS_MERCENARY_FILE,
	TF_CLASS_CIVILIAN_FILE,
	TF_CLASS_JUGGERNAUT_FILE,
};

TFPlayerClassData_t s_aTFPlayerClassData[TF_CLASSMOD_LAST][TF_CLASS_COUNT_ALL];
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TFPlayerClassData_t::TFPlayerClassData_t()
{
//	m_szClassName.Set( NULL_STRING );
//	m_szModelName.Set( NULL_STRING );
//	m_szArmModelName.Set( NULL_STRING );
//	m_szLocalizableName.Set( NULL_STRING );
//	m_szJumpSound.Set( NULL_STRING );
	m_flMaxSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_nMaxArmor = 0;
	m_nArmorType = 0;

	m_nMaxAirDashCount = 0;
	
	m_nCapNumber = 1;

#ifdef GAME_DLL
	m_szDeathSound[0] = '\0';
	m_szCritDeathSound[0] = '\0';
	m_szMeleeDeathSound[0] = '\0';
	m_szExplosionDeathSound[0] = '\0';
#endif

//	m_szClassSelectImageRed.Set( NULL_STRING );
//	m_szClassSelectImageBlue.Set( NULL_STRING );
//	m_szClassSelectImageMercenary.Set( NULL_STRING );
	
//	m_szClassImageRed.Set( NULL_STRING );
//	m_szClassImageBlue.Set( NULL_STRING );
//	m_szClassImageMercenary.Set( NULL_STRING );
//	m_szClassImageColorless.Set( NULL_STRING );

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		m_aWeapons[iWeapon] = TF_WEAPON_NONE;
	}
	
	m_iWeaponCount = 0;
	
	for ( int iGrenade = 0; iGrenade < TF_PLAYER_GRENADE_COUNT; ++iGrenade )
	{
		m_aGrenades[iGrenade] = TF_WEAPON_NONE;
	}

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax[iAmmo] = TF_AMMO_DUMMY;
	}

	for ( int iBuildable = 0; iBuildable < TF_PLAYER_BUILDABLE_COUNT; ++iBuildable )
	{
		m_aBuildable[iBuildable] = OBJ_LAST;
	}

	m_bParsed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::Parse( const char *szName, int iClass )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	
	// No filesystem at this point????  Hmmmm......

	// Parse class file.

	const unsigned char *pKey = NULL;

	if ( g_pGameRules )
	{
		pKey = g_pGameRules->GetEncryptionKey();
	}

	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szName, pKey );
	
	if ( pKV )
	{
		ParseData( pKV );
		for( int iMod = TF_CLASSMOD_NONE+1; iMod < TF_CLASSMOD_LAST; iMod++ )
		{
			TFPlayerClassData_t *pClassModData = &s_aTFPlayerClassData[iMod][iClass];
			KeyValues *pModKV = pKV->FindKey( g_aPlayerMutatorNames[iMod] );
			if( !pModKV )
				continue;

			KeyValues *pModKVFinal = new KeyValues( szName );
			pKV->CopySubkeys( pModKVFinal );
			
			FOR_EACH_SUBKEY( pModKV, kvSubKey )
			{
				pModKVFinal->RemoveSubKey( pModKVFinal->FindKey(kvSubKey->GetName()) );
				pModKVFinal->AddSubKey( kvSubKey->MakeCopy() );
			}
			pClassModData->ParseData( pModKVFinal );
			pModKVFinal->deleteThis();
		}
		pKV->deleteThis();
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TFPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL	
	return m_szModelName;
	
#else
	return m_szModelName;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Loads up class.txt @modelsetc
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	Q_strncpy( m_szClassName, pKeyValuesData->GetString( "name" ), TF_NAME_LENGTH );

	// Load the high res model or the lower res model.
	Q_strncpy( m_szArmModelName, pKeyValuesData->GetString( "arm_model" ), TF_NAME_LENGTH );
	Q_strncpy( m_szModelName, pKeyValuesData->GetString( "model" ), TF_NAME_LENGTH );
	Q_strncpy( m_szLocalizableName, pKeyValuesData->GetString( "localize_name" ), TF_NAME_LENGTH );
	Q_strncpy( m_szJumpSound, pKeyValuesData->GetString( "jump_sound" ), TF_NAME_LENGTH );
	
	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_nMaxArmor = pKeyValuesData->GetInt( "armor_max" );
	m_nArmorType = pKeyValuesData->GetInt("armor_type");

	m_nMaxAirDashCount = pKeyValuesData->GetInt( "MaxAirDashCount" );
	
	m_nCapNumber = pKeyValuesData->GetInt( "CapMultiplier" );
	m_iWeaponCount = pKeyValuesData->GetInt( "WeaponCount", 1 );
	
	// Weapons.
	int i;
	char buf[32];
	for ( i=0;i<TF_PLAYER_WEAPON_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "weapon%d", i+1 );
		int iID = GetItemSchema()->GetWeaponID( pKeyValuesData->GetString(buf) );
		m_aWeapons[i] = iID;
	}

	// Grenades.
	for( i = 0; i < 3; i++ )
	{
		m_aGrenades[0] = GetWeaponId(pKeyValuesData->GetString( UTIL_VarArgs("grenade%d", i) ));
	}

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if ( pAmmoKeyValuesData )
	{
		for ( int iAmmo = 1; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax[iAmmo] = pAmmoKeyValuesData->GetInt( g_aAmmoNames[iAmmo], 0 );
		}
	}

	// Buildables
	for ( i=0;i<TF_PLAYER_BUILDABLE_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "buildable%d", i+1 );		
		m_aBuildable[i] = GetBuildableId( pKeyValuesData->GetString( buf ) );
	}

	// Temp animation flags
	m_bDontDoAirwalk = ( pKeyValuesData->GetInt( "DontDoAirwalk", 0 ) > 0 );
	m_bDontDoNewJump = ( pKeyValuesData->GetInt( "DontDoNewJump", 0 ) > 0 );
	
	// Open Fortress
	
	m_nViewVector = pKeyValuesData->GetInt( "ViewVector" );
	
	m_bSpecialClass = ( pKeyValuesData->GetInt( "SpecialClass", 0 ) > 0 );

#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	Q_strncpy( m_szDeathSound, pKeyValuesData->GetString( "sound_death", "Player.Death" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szCritDeathSound, pKeyValuesData->GetString( "sound_crit_death", "TFPlayer.CritDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szMeleeDeathSound, pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szExplosionDeathSound, pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
#endif

	Q_strncpy( m_szClassSelectImageRed, 		pKeyValuesData->GetString( "ClassSelectImageRed" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassSelectImageBlue, 		pKeyValuesData->GetString( "ClassSelectImageBlue" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassSelectImageMercenary, 	pKeyValuesData->GetString( "ClassSelectImageMercenary" ), 	TF_NAME_LENGTH );	

	Q_strncpy( m_szClassImageRed, 		pKeyValuesData->GetString( "ClassImageRed" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageBlue, 		pKeyValuesData->GetString( "ClassImageBlue" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageMercenary, pKeyValuesData->GetString( "ClassImageMercenary" ), TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageColorless, pKeyValuesData->GetString( "ClassImageColorless" ), TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageTennis, pKeyValuesData->GetString( "ClassImageTennis" ), TF_NAME_LENGTH );
	
	Q_strncpy( m_szClassIcon, pKeyValuesData->GetString( "ClassIcon", "../hud/leaderboard_class_tank" ), 	TF_NAME_LENGTH );
	
	// The file has been parsed.
	m_bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class data (keep a cache).
//-----------------------------------------------------------------------------
void InitPlayerClasses( void )
{
	// Special case the undefined class.
	TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[0][TF_CLASS_UNDEFINED];
	Assert( pClassData );
	Q_strncpy( pClassData->m_szClassName, "undefined", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szModelName, "models/error.mdl", TF_NAME_LENGTH );	// Undefined players still need a model
	Q_strncpy( pClassData->m_szArmModelName, "models/error.mdl", TF_NAME_LENGTH );	// Undefined players still need a Arm model
	Q_strncpy( pClassData->m_szLocalizableName, "undefined", TF_NAME_LENGTH );

	Q_strncpy( pClassData->m_szClassSelectImageRed, "class_sel_sm_civilian_red", TF_NAME_LENGTH );	// Undefined players still need a class Image
	Q_strncpy( pClassData->m_szClassSelectImageBlue, "class_sel_sm_civilian_blu", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szClassSelectImageMercenary, "class_sel_sm_civilian_mercenary", TF_NAME_LENGTH );
	
	// Initialize the classes.
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[0][iClass];
		Assert( pClassData );
		pClassData->Parse( s_aPlayerClassFiles[iClass], iClass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TFPlayerClassData_t *GetPlayerClassData( int iClass, int iModifier )
{
	Assert ( ( iClass >= 0 ) && ( iClass < TF_CLASS_COUNT_ALL ) && ( iModifier >= 0 ) && ( iModifier < TF_CLASSMOD_LAST ) );
	return &s_aTFPlayerClassData[iModifier][iClass];
}

//=============================================================================
//
// Shared player class data.
//

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	RecvPropInt( RECVINFO( m_iClass ) ),
	RecvPropInt( RECVINFO( m_iModifier ) ),

	RecvPropInt( RECVINFO( m_iSetCustomHealth ) ),
	RecvPropFloat( RECVINFO( m_flSetCustomSpeed ) ),

	RecvPropString( RECVINFO( m_iszSetCustomModel ) ),
	RecvPropString( RECVINFO( m_iszSetCustomArmModel ) ),
END_RECV_TABLE()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	SendPropInt( SENDINFO( m_iClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iModifier ),  Q_log2( TF_CLASSMOD_LAST )+1 , SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_iSetCustomHealth ) ),
	SendPropFloat( SENDINFO( m_flSetCustomSpeed ) ),

	SendPropStringT( SENDINFO( m_iszSetCustomModel ) ),
	SendPropStringT( SENDINFO( m_iszSetCustomArmModel ) ),
END_SEND_TABLE()
#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerClassShared::CTFPlayerClassShared()
{
	m_iClass.Set( TF_CLASS_UNDEFINED );
	m_iModifier.Set( 0 );
	m_iSetCustomHealth = -1;
	m_flSetCustomSpeed = -1.0f;
#ifdef CLIENT_DLL
	m_iszSetCustomModel[0] = '\0';
#else
	m_iszSetCustomModel.Set( NULL_STRING );
#endif
}

#ifndef CLIENT_DLL
void CTFPlayerClassShared::SetCustomModel( const char *pszModelName )
{
	if( pszModelName && pszModelName[0] )
	{
		bool bPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CBaseEntity::PrecacheModel( pszModelName );
		CBaseEntity::SetAllowPrecache( bPrecache );
		m_iszSetCustomModel.Set( AllocPooledString( pszModelName ) );
	}
	else
		m_iszSetCustomModel.Set( NULL_STRING );
}

void CTFPlayerClassShared::SetCustomArmModel( const char *pszModelName )
{
	if (pszModelName && pszModelName[0])
	{
		bool bPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CBaseEntity::PrecacheModel( pszModelName );
		CBaseEntity::SetAllowPrecache( bPrecache );
		m_iszSetCustomArmModel.Set( AllocPooledString( pszModelName ) );
	}
	else
		m_iszSetCustomArmModel.Set( NULL_STRING );
}

void CTFPlayerClassShared::SetCustomHealth( int iHealth )
{
	m_iSetCustomHealth = iHealth;
	
}

void CTFPlayerClassShared::SetCustomSpeed( float flSpeed )
{
	m_flSetCustomSpeed = flSpeed;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class.
//-----------------------------------------------------------------------------
bool CTFPlayerClassShared::Init( int iClass, int iModifier )
{
	Assert ( ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_CLASS_COUNT_ALL - 1 ) );
	m_iClass = iClass;
	m_iModifier = iModifier;
#ifdef CLIENT_DLL
	m_iszSetCustomModel[0] = '\0';
#else
	m_iszSetCustomModel.Set( NULL_STRING );
#endif
	return true;
}

void CTFPlayerClassShared::SetModifier( int iModifier )
{
	m_iModifier = iModifier;
}

// If needed, put this into playerclass scripts
bool CTFPlayerClassShared::CanBuildObject( int iObjectType ) const
{
	bool bFound = false;

	TFPlayerClassData_t  *pData = GetData();

	int i;
	for ( i=0;i<TF_PLAYER_BUILDABLE_COUNT;i++ )
	{
		if ( iObjectType == pData->m_aBuildable[i] )
		{
			bFound = true;
			break;
		}
	}

	return bFound;
}

char	*CTFPlayerClassShared::GetModelName( void ) const				
{ 
#ifdef CLIENT_DLL
	if ( m_iszSetCustomModel[0] ) 
		return const_cast<char*>(m_iszSetCustomModel);
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomModel.Get() ), sizeof( tmp ) );
	if ( m_iszSetCustomModel.Get() != NULL_STRING ) 
		return tmp;
#endif
	static char modelFilename[ 256 ];
	Q_strncpy( modelFilename, GetData()->GetModelName(), sizeof( modelFilename ) );
	return modelFilename;
}

char *CTFPlayerClassShared::GetSetCustomModel( void ) const
{ 
#ifdef CLIENT_DLL
	return const_cast<char*>( m_iszSetCustomModel );
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING(m_iszSetCustomModel.Get()), sizeof(tmp) );
	return tmp;
#endif
}

bool CTFPlayerClassShared::UsesCustomModel( void ) const
{
#ifdef CLIENT_DLL
	if ( m_iszSetCustomModel[0] )
		return true;
#else
	if ( m_iszSetCustomModel.Get() != NULL_STRING )
		return true;
#endif	
	return false;
}

char *CTFPlayerClassShared::GetArmModelName( void ) const					
{ 
#ifdef CLIENT_DLL
	if( m_iszSetCustomArmModel[0] ) 
		return const_cast<char*>( m_iszSetCustomArmModel );
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomArmModel.Get() ), sizeof( tmp ) );
	if( m_iszSetCustomArmModel.Get() != NULL_STRING )
		return tmp;
#endif
	static char modelFilename[ 256 ];
	Q_strncpy( modelFilename, GetData()->GetArmModelName(), sizeof( modelFilename ) );
	
	return modelFilename;
}

int CTFPlayerClassShared::MaxAirDashCount( void ) const
{
	if ( of_airdashcount.GetInt() > GetData()->m_nMaxAirDashCount )
		return of_airdashcount.GetInt();
	else
		return GetData()->m_nMaxAirDashCount; 
}

bool CTFPlayerClassShared::CanAirDash( void ) const
{
	return GetData()->m_nMaxAirDashCount > 0 || of_airdashcount.GetInt() > 0; 
}

char *CTFPlayerClassShared::GetSetCustomArmModel( void ) const					
{ 
#ifdef CLIENT_DLL
	return const_cast<char*>( m_iszSetCustomArmModel );
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomArmModel.Get() ), sizeof( tmp ) );
	return tmp;
#endif
}

bool CTFPlayerClassShared::UsesCustomArmModel( void ) const
{
#ifdef CLIENT_DLL
	if ( m_iszSetCustomArmModel[0] ) 
		return true;
#else
	if ( m_iszSetCustomArmModel.Get() != NULL_STRING ) 
		return true;
#endif	
	return false;
}

TFPlayerClassData_t *CTFPlayerClassShared::GetData( void ) const
{ 
	return GetPlayerClassData( m_iClass, m_iModifier );
}

int	CTFPlayerClassShared::GetCapNumber( void ) const
{ 
	return GetData()->m_nCapNumber; 
}

bool CTFPlayerClassShared::IsSpecialClass( void ) const
{ 
	return GetData()->m_bSpecialClass; 
}

int	CTFPlayerClassShared::GetMaxArmor( void ) const
{ 
	return GetData()->m_nMaxArmor; 
}

int	CTFPlayerClassShared::GetArmorType( void ) const
{
	return GetData()->m_nArmorType;
}

int	CTFPlayerClassShared::GetMaxHealth( void ) const
{
	if( m_iSetCustomHealth > -1 )
		return m_iSetCustomHealth;
	else
		return GetData()->m_nMaxHealth; 
}

float CTFPlayerClassShared::GetMaxSpeed( void ) const
{
	if( m_flSetCustomSpeed > -1 )
		return m_flSetCustomSpeed;
	else
		return GetData()->m_flMaxSpeed; 
}

char *CTFPlayerClassShared::GetName( void )
{ 
	return GetData()->m_szClassName; 
}

const char *CTFPlayerClassShared::GetJumpSound( void ) const
{ 
	return GetData()->GetJumpSound(); 
}