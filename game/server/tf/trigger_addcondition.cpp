//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "tf_player.h"
#include "trigger_addcondition.h"

BEGIN_DATADESC( CTriggerAddTFPlayerCondition )

	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),

	DEFINE_ENTITYFUNC( StartTouch ),
	DEFINE_ENTITYFUNC( EndTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_add_tf_player_condition, CTriggerAddTFPlayerCondition );

BEGIN_DATADESC( CTriggerRemoveTFPlayerCondition )

	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_remove_tf_player_condition, CTriggerRemoveTFPlayerCondition );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerAddTFPlayerCondition::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerAddTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !m_bDisabled && PassesTriggerFilters( pOther ) && pPlayer )
	{
		if ( m_nCondition == -1 )
		{
			Warning( "Invalid Condition ID[-1] in trigger %s\n", GetClassname() );
		}
		else
		{
			pPlayer->m_Shared.AddCond( m_nCondition, m_flDuration );
			BaseClass::StartTouch( pOther );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerAddTFPlayerCondition::EndTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !m_bDisabled && PassesTriggerFilters( pOther ) && pPlayer )
	{
		if ( m_nCondition == -1 )
		{
			Warning( "Invalid Condition ID[-1] in trigger %s\n", GetClassname() );
		}
		else
		{
			if (m_flDuration == -1)
			{
				pPlayer->m_Shared.RemoveCond(m_nCondition);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerRemoveTFPlayerCondition::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTriggerRemoveTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !m_bDisabled && PassesTriggerFilters( pOther ) && pPlayer )
	{
		if ( m_nCondition == -1 )
		{
			pPlayer->m_Shared.RemoveAllCond( pPlayer );
		}
		else
		{
			pPlayer->m_Shared.RemoveCond( m_nCondition );
		}
	}
}