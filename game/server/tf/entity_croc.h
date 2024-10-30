//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The crocodile model that spawns from a func_croc
//
//=============================================================================//

#ifndef EntityCroc_H
#define EntityCroc_H
#ifdef _WIN32
#pragma once
#endif

#define CROC_MODEL "models/props_island/crocodile/crocodile.mdl"

//==================================================================================================================
// Entity Crocodile
//==================================================================================================================
class CEntityCroc : public CBaseAnimating
{
	DECLARE_CLASS( CEntityCroc, CBaseAnimating );
	DECLARE_DATADESC();

public:
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Think ( void );
};

#endif // EntityCroc_H
