//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_TFCGRENADE_FRAG_H
#define OF_TFCGRENADE_FRAG_H
#ifdef _WIN32
#pragma once
#endif


#include "of_tfcgrenadebase.h"

class CTFCGrenadeFrag : public CTFCGrenadeBase
{
public:
	DECLARE_CLASS( CTFCGrenadeFrag, CTFCGrenadeBase );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	Precache( void );
	void	BounceSound( void );
	void	BounceTouch( CBaseEntity *pOther );

	void ShootTimed( CBaseCombatCharacter *pOwner, Vector vecVelocity, float flTime );
};

#endif // OF_TFCGRENADE_FRAG_H
