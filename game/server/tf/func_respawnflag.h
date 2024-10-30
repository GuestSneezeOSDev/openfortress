//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Regenerate Zone.
//
//=============================================================================//
#ifndef FUNC_RESPAWNFLAG_H
#define FUNC_RESPAWNFLAG_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "props.h"

bool InRespawnFlagZone( const Vector &vecFlagOrigin );

//=============================================================================
//
// CTF Regenerate Zone class.
//
class CFuncRespawnFlagZoneTouch : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFuncRespawnFlagZoneTouch, CBaseTrigger );

	CFuncRespawnFlagZoneTouch();

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

#endif // FUNC_RESPAWNFLAG_H












