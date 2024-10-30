//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: De_ Bomb Plant Zone.
//
//=============================================================================//

#include "cbase.h"
#include "tf_item.h"

#include "func_bomb_target.h"

LINK_ENTITY_TO_CLASS( func_bomb_target, CBombTargetZone );

//=============================================================================
//
// De_ Bomb Plant Zone tables.
//

BEGIN_DATADESC( CBombTargetZone )
	DEFINE_FUNCTION( Touch ),
END_DATADESC();

//=============================================================================
//
// De_ Bomb Plant Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

// is the flag in this trigger zone?
bool InBombTargetZone( const Vector &vecPoint )
{
	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_bomb_target" ) ) != NULL )
	{
		CBombTargetZone *pBombZone = ( CBombTargetZone * )pEntity;

		if ( !pBombZone->IsDisabled() && pBombZone->PointIsWithin( vecPoint ) )
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
CBombTargetZone::CBombTargetZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CBombTargetZone::Spawn(void)
{
	Precache();
	InitTrigger();
	SetTouch( &CBombTargetZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBombTargetZone::Activate(void)
{
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBombTargetZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		// Nutin here yet
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBombTargetZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBombTargetZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBombTargetZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBombTargetZone::InputToggle( inputdata_t &inputdata )
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
void CBombTargetZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}