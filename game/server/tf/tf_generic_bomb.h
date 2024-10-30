//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exploding bomb
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
-m_flDamage(Offset 1188) (Save | Key)(4 Bytes) - damage
- m_flRadius(Offset 1196) (Save | Key)(4 Bytes) - radius
- m_nHealth(Offset 1192) (Save | Key)(4 Bytes) - health
- m_strExplodeParticleName(Offset 1200) (Save | Key)(4 Bytes) - explode_particle
- m_strExplodeSoundName(Offset 1208) (Save | Key)(4 Bytes) - sound
- m_eWhoToDamage(Offset 1212) (Save | Key)(4 Bytes) - friendlyfire
- m_OnDetonate(Offset 1156) (Save | Key | Output)(0 Bytes) - OnDetonate
- Detonate(Offset 0) (Input)(0 Bytes) - Detonate
*/

#ifndef CTFGenericBomb_H
#define CTFGenericBomb_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

//==================================================================================================================
// Entity Bomb
//==================================================================================================================
class CTFGenericBomb : public CDynamicProp
{
public:
	DECLARE_CLASS( CTFGenericBomb, CDynamicProp );
	DECLARE_DATADESC();

	CTFGenericBomb();
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void InputDetonate( inputdata_t &inputdata) ;
	virtual void Event_Killed( const CTakeDamageInfo &info );

protected:
	// virtual int OnTakeDamage( const CTakeDamageInfo &info );
	// virtual void	Event_Killed( const CTakeDamageInfo &info );
	// virtual void	InputDetonate( inputdata_t &inputdata );
	float m_flDamage;
	float m_flRadius;
	string_t m_strExplodeParticleName;
	string_t m_strExplodeSoundName;
	// fixme: this isn't a bool in TF2 - m_eWhoToDamage
	bool m_eWhoToDamage;
	bool m_bDead;
	COutputEvent Detonate;
};

//==================================================================================================================
// Pumpkin Bomb
//==================================================================================================================

class CTFPumpkinBomb : public CDynamicProp
{
public:
	DECLARE_CLASS( CTFPumpkinBomb, CDynamicProp );
	DECLARE_DATADESC();

	CTFPumpkinBomb();
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Event_Killed( const CTakeDamageInfo &info );

protected:
	// virtual int OnTakeDamage( const CTakeDamageInfo &info );
	// virtual void	Event_Killed( const CTakeDamageInfo &info );
	// virtual void	InputDetonate( inputdata_t &inputdata );
	float m_flDamage;
	float m_flRadius;
};
#endif // CTFGenericBomb_H