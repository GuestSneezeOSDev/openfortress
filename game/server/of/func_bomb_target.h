//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: De_ Bomb Plant Zone.
//
//=============================================================================//
#ifndef FUNC_BOMB_TARGET_H
#define FUNC_BOMB_TARGET_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "props.h"

bool InBombTargetZone( const Vector &vecPoint );

//=============================================================================
//
// CTF Regenerate Zone class.
//
class CBombTargetZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CBombTargetZone, CBaseTrigger );

	CBombTargetZone();

	void	Spawn( void );
	void	Activate( void );
	void	Touch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

private:

	DECLARE_DATADESC();
};

#endif // FUNC_BOMB_TARGET_H