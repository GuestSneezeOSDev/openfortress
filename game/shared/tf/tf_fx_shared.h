//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//  
//
//=============================================================================
#ifndef TF_FX_SHARED_H
#define TF_FX_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

void FX_WeaponSound ( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo );
void StartGroupingSounds( void );
void EndGroupingSounds( void );


// This runs on both the client and the server.
// On the server, it only does the damage calculations.
// On the client, it does all the effects.
void FX_FireBullets( int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, bool bFixedSpread = true, int iBullets = 1, float flRange = 8192, int iAmmoType = 0, float flDamage = -1.0f, int bCritical = false, bool bFirstShot = false, int iTracerFrequency = 2 );

#endif // TF_FX_SHARED_H
