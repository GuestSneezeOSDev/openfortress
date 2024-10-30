//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "tf_player.h"
#include "of_trigger_set_weapon_attributes.h"
#include "tf_weaponbase.h"
#include "of_items_game.h"
#include "tf_gamerules.h"

BEGIN_DATADESC( CTriggerWeaponAttributes )

	DEFINE_ENTITYFUNC( StartTouch ),
	DEFINE_ENTITYFUNC( EndTouch ),

	DEFINE_KEYFIELD( m_szWeaponName, FIELD_STRING, "WeaponName" ),
	DEFINE_KEYFIELD( m_bReplaceAttributes, FIELD_BOOLEAN, "ReplaceAttributes" ),
	DEFINE_KEYFIELD( m_bRemoveAttributes, FIELD_BOOLEAN, "RemoveAttributes" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_set_weapon_attributes, CTriggerWeaponAttributes );

CTriggerWeaponAttributes::CTriggerWeaponAttributes()
{
	m_szWeaponName = MAKE_STRING("");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerWeaponAttributes::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

bool CTriggerWeaponAttributes::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !Q_strncmp( szKeyName, "AttributeName", 13 ) )
	{
		m_hAttributes.AddToTail(AllocPooledString(szValue));
	}
	else if( !Q_strncmp( szKeyName, "Value", 5 ) )
	{
		m_hValues.AddToTail(AllocPooledString(szValue));
	}
	else
		BaseClass::KeyValue( szKeyName, szValue );
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerWeaponAttributes::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !m_bDisabled && PassesTriggerFilters( pOther ) && pPlayer )
	{
		
		CTFWeaponBase *pWeapon = NULL;

		if( m_szWeaponName.ToCStr()[0] == '\0' )
			pWeapon = pPlayer->GetActiveTFWeapon();
		else
		{
			char szName[64];
			Q_strncpy(szName, m_szWeaponName.ToCStr(), sizeof(szName));

			int iSlot = -1, iPos = -1;
			TFGameRules()->GetWeaponSlot(szName, iSlot, iPos, pPlayer);

			pWeapon = pPlayer->GetWeaponInSlot(iSlot, iPos);
		}

		if( !pWeapon )
			return;

		if( m_bReplaceAttributes || m_bRemoveAttributes )
			pWeapon->PurgeAttributes();

		if( !m_bRemoveAttributes )
		{
			for( int i = 0; i < m_hAttributes.Count() && i < m_hValues.Count(); i++ )
			{
				pWeapon->AddAttribute(CTFAttribute(GetItemSchema()->GetAttributeID(m_hAttributes[i].ToCStr()), m_hValues[i].ToCStr()));
			}
		}

		BaseClass::StartTouch( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerWeaponAttributes::EndTouch( CBaseEntity *pOther )
{

}