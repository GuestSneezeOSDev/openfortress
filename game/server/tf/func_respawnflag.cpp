//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Regenerate Zone.
//
//=============================================================================//

// datamap dump data https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
//CBaseEntity - func_respawnflag
//- CFuncRespawnFlagZoneTouch(Offset 0) (FunctionTable)(0 Bytes)

#include "cbase.h"
#include "tf_player.h"
#include "entity_capture_flag.h"
#include "tf_item.h"

#include "func_respawnflag.h"
#include "func_bomb_target.h"

LINK_ENTITY_TO_CLASS( func_respawnflag, CFuncRespawnFlagZoneTouch );

//=============================================================================
//
// CTF Regenerate Zone tables.
//

BEGIN_DATADESC( CFuncRespawnFlagZoneTouch )
	DEFINE_FUNCTION( Touch ),
END_DATADESC();

//=============================================================================
//
// CTF Regenerate Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

// is the flag in this trigger zone?
bool InRespawnFlagZone( const Vector &vecPoint )
{
	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_respawnflag" ) ) != NULL )
	{
		CFuncRespawnFlagZoneTouch *pFlag = ( CFuncRespawnFlagZoneTouch * )pEntity;

		if ( !pFlag->IsDisabled() && pFlag->PointIsWithin( vecPoint ) )
		{
			// return false;
			return true;	
		}
	}

	// return true;
	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncRespawnFlagZoneTouch::CFuncRespawnFlagZoneTouch()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::Spawn(void)
{
	Precache();
	InitTrigger();
	SetTouch( &CFuncRespawnFlagZoneTouch::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::Activate(void)
{
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );

		if ( pPlayer && pPlayer->HasTheFlag() )
		{
			CTFItem *pItem = pPlayer->GetItem();

			// drop flag if the player has it and then reset it
			if ( pItem )
			{
				CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pItem );

				pPlayer->DropFlag();

				if ( pFlag )
				{
					pFlag->Reset();

					pFlag->ResetMessage();
				}
			}
			else
			{
					pPlayer->DropFlag();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncRespawnFlagZoneTouch::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZoneTouch::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}