//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_TFCGRENADEBASE_H
#define OF_TFCGRENADEBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "basegrenade_shared.h"


class CTFCGrenadeBase : public CBaseGrenade
{
	DECLARE_CLASS( CTFCGrenadeBase, CBaseGrenade );
public:

	virtual void Precache();

	void Explode( trace_t *pTrace, int bitsDamageType );
	unsigned int	PhysicsSolidMaskForEntity( void ) const;
};

#endif // OF_TFCGRENADEBASE_H
