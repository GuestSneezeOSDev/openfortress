//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Glow effect
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CTFGlow - tf_glow
- m_iMode (Offset 856) (Save|Key)(4 Bytes) - Mode
- m_glowColor (Offset 860) (Save|Key)(4 Bytes) - GlowColor
- m_bDisabled (Offset 864) (Save|Key)(1 Bytes) - StartDisabled
- InputEnable (Offset 0) (Input)(0 Bytes) - Enable
- InputDisable (Offset 0) (Input)(0 Bytes) - Disable
- InputSetGlowColor (Offset 0) (Input)(0 Bytes) - SetGlowColor
*/

#ifndef TF_GLOW_H
#define TF_GLOW_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTF Glow class.
//

class CTFGlow : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFGlow, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CTFGlow();

	color32 m_glowColor;

	CBaseEntity *pGlower;
	CBaseAnimating *pProp;

	virtual void	Spawn( void );
	virtual void	Activate( void );

	void AddGlow( void );
	void RemoveGlow( void );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetGlowColor( inputdata_t &inputdata );

	CNetworkHandle( CBaseAnimating, m_hglowEntity );

	// 0 - always 
	// 1 - only when occluded
	// 2 - only when visible
	CNetworkVar( int, m_iMode );

	CNetworkVar( bool, m_bDisabled );
};

#endif // TF_GLOW_H


