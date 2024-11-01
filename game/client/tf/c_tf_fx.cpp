//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_fx.h"
#include "tf_fx_shared.h"
#include "tier0/vprof.h"

IMPLEMENT_CLIENTCLASS_EVENT( C_TEFireBullets, DT_TEFireBullets, CTEFireBullets );

BEGIN_RECV_TABLE_NOBASE( C_TEFireBullets, DT_TEFireBullets )
RecvPropVector( RECVINFO( m_vecOrigin ) ),
RecvPropFloat( RECVINFO( m_vecAngles[0] ) ),
RecvPropFloat( RECVINFO( m_vecAngles[1] ) ),
RecvPropInt( RECVINFO( m_iWeaponID ) ),
RecvPropInt( RECVINFO( m_iMode ) ), 
RecvPropInt( RECVINFO( m_iSeed ) ),
RecvPropInt( RECVINFO( m_iPlayer ) ),
RecvPropFloat( RECVINFO( m_flSpread ) ),	
RecvPropBool( RECVINFO( m_bFixedSpread ) ),
RecvPropInt( RECVINFO( m_iBullets ) ),
RecvPropInt( RECVINFO( m_iAmmoType ) ),
RecvPropFloat( RECVINFO( m_flRange ) ),
RecvPropFloat( RECVINFO( m_flDamage ) ),
RecvPropInt( RECVINFO( m_bCritical ) ),
RecvPropBool( RECVINFO( m_bFirstShot ) ),
RecvPropInt( RECVINFO( m_iTracerFrequency ) ),
END_RECV_TABLE()

void C_TEFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEFireBullets::PostDataUpdate" );

	// Create the effect.
	m_vecAngles.z = 0;
	FX_FireBullets( m_iPlayer+1, m_vecOrigin, m_vecAngles, m_iWeaponID, m_iMode, m_iSeed, m_flSpread, m_bFixedSpread, m_iBullets, m_flRange, m_iAmmoType, m_flDamage, m_bCritical, m_bFirstShot, m_iTracerFrequency );
}