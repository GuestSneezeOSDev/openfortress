//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_weapon_sniperrifle.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "view.h"
	#include "beamdraw.h"
	#include "functionproxy.h"
	#include "toolframework_client.h"

	// forward declarations
	//void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#endif

#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define SNIPER_DOT_SPRITE_RED			"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE			"effects/sniperdot_blue.vmt"

#ifdef CLIENT_DLL
	ConVar of_holdtozoom( "of_holdtozoom", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Hold Mouse2 to zoom instead of Toggling it." );
#endif

ConVar of_headshots	( "of_headshots", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "Makes all hitscan weapons headshot when set. 2 will force headshots only damage." );
ConVar of_headshot_disable_railgun("of_headshot_disable_railgun", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Disables railgun headshots.");

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle, DT_TFSniperRifle )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle, DT_SniperRifleLocalData )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flChargedDamage), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargedDamage) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle, DT_TFSniperRifle )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flUnzoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flRezoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle );
//PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle );


IMPLEMENT_NETWORKCLASS_ALIASED( TFRailgun, DT_WeaponRailgun )

BEGIN_NETWORK_TABLE( CTFRailgun, DT_WeaponRailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_railgun, CTFRailgun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_railgun );


IMPLEMENT_NETWORKCLASS_ALIASED(TFCSniperRifle, DT_TFCSniperRifle)

BEGIN_NETWORK_TABLE(CTFCSniperRifle, DT_TFCSniperRifle)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFCSniperRifle)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tfc_weapon_sniper_rifle, CTFCSniperRifle);
//PRECACHE_WEAPON_REGISTER(tfc_weapon_sniper_rifle);

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::~CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Precache()
{
	BaseClass::Precache();
	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ResetTimers( void )
{
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::SetReloadTimer(float flReloadTime)
{
	float flSecondary = m_flNextSecondaryAttack;
	BaseClass::SetReloadTimer(flReloadTime);
	m_flNextSecondaryAttack = flSecondary;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Reload( void )
{
	// We currently don't reload.
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanHolster( void ) const
{
 	CTFPlayer *pPlayer = GetTFPlayerOwner();
 	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() ) && ( m_flNextPrimaryAttack > gpGlobals->curtime ) )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}

	m_flChargedDamage = 0.0f;
	ResetTimers();

	return BaseClass::Holster( pSwitchingTo );
}

void CTFSniperRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	ZoomOut();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if (
				(
					(GetWeaponID() != TFC_WEAPON_SNIPER_RIFLE) 
					&& 
					(pPlayer->m_Shared.InCond(TF_COND_AIMING))
				)
				||
				(
					(GetWeaponID() != TFC_WEAPON_SNIPER_RIFLE)
					&&
					(pPlayer->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY))
				)
			)
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
	}

	if ( m_flUnzoomTime > 0 && gpGlobals->curtime > m_flUnzoomTime )
	{
		if ( m_bRezoomAfterShot )
		{
			ZoomOutIn();
			m_bRezoomAfterShot = false;
		}
		else
		{
			ZoomOut();
		}
		m_flUnzoomTime = -1;
	}

	if ( m_flRezoomTime > 0 )
	{
		if ( gpGlobals->curtime > m_flRezoomTime )
		{
            ZoomIn();
			m_flRezoomTime = -1;
		}
	}
	int bHoldToZoom = 0;
#ifdef GAME_DLL
		bHoldToZoom = pPlayer->IsFakeClient() ? 0 : V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "of_holdtozoom"));
#else
		bHoldToZoom = V_atoi(of_holdtozoom.GetString());
#endif	

	if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		// If we're in the process of rezooming, just cancel it
		if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
		{
			// Prevent them from rezooming in less time than they would have
			m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
			m_flRezoomTime = -1;
		}
		else if ( (!pPlayer->m_Shared.InCond(TF_COND_AIMING) && !pPlayer->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY)) || bHoldToZoom == 0 )
		{
			Zoom();
		}
	}
	else if ( ( (pPlayer->m_Shared.InCond(TF_COND_AIMING) || pPlayer->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY)) && !(pPlayer->m_nButtons & IN_ATTACK2) ) && bHoldToZoom == 1 )
	{
		Zoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::ItemPostFrame( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;
	
	// If we're lowered, we're not allowed to fire
	if (m_bLowered)
		return;

	if (UsesClipsForAmmo1())
	{
		CheckReload();
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	//  Can only start the Reload Cycle after the firing cycle
	if (pPlayer->m_nButtons & IN_RELOAD
		&& m_flNextPrimaryAttack <= gpGlobals->curtime && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed or if we're loading a barrage, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// Check for reload singly interrupts.
	if (m_bReloadsSingly)
	{
		ReloadSinglyPostFrame();
	}

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot )
	{
		UpdateSniperDot();
	}
#endif

	bool bHoldToZoom = 0;
#ifdef GAME_DLL
	bHoldToZoom = pPlayer->IsFakeClient() ? 0 : (V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "of_holdtozoom")) > 0);
#else
	bHoldToZoom = of_holdtozoom.GetBool();
#endif	
	float flChargeAfter = m_flNextSecondaryAttack;
	if ( bHoldToZoom )
	  flChargeAfter = m_flNextPrimaryAttack;

	if ( flChargeAfter <= gpGlobals->curtime )
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !m_bRezoomAfterShot && !GetTFWpnData().m_bNoSniperCharge )
		{
			m_flChargedDamage = min( m_flChargedDamage + gpGlobals->frametime * GetDamage(), GetDamage() * 3 );
		}
		else
		{
			m_flChargedDamage = max( 0, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC );
		}
	}

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}

	if (!pPlayer->m_Shared.InCond(TF_COND_AIMING) && !pPlayer->m_Shared.InCond(TF_COND_AIMING_SCOPE_ONLY))
		SoftZoomCheck();

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK ) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}


void CTFSniperRifle::DoFireEffects( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// No muzzleflash when scoped in
	if ( !IsZoomed() )
		pPlayer->DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFSniperRifle::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
	}

	ToggleZoom();
	
	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = max( m_flNextPrimaryAttack, gpGlobals->curtime + 0.1 );
	
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomOutIn( void )
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->ShouldAutoRezoom() )
	{
		m_flRezoomTime = gpGlobals->curtime + 0.9;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( ReserveAmmo() <= 0 )
		return;

	BaseClass::ZoomIn();

	if (GetWeaponID() != TF_WEAPON_RAILGUN)
	{
		pPlayer->m_Shared.AddCond(TF_COND_AIMING);
		pPlayer->TeamFortress_SetSpeed();
	}
	else
	{
		pPlayer->m_Shared.AddCond(TF_COND_AIMING_SCOPE_ONLY);
		pPlayer->TeamFortress_SetSpeed();
	}
	pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;

#ifdef GAME_DLL
	// Create the sniper dot.
	CreateSniperDot();
	pPlayer->ClearExpression();
#endif
}

bool CTFSniperRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomOut( void )
{
	BaseClass::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if (GetWeaponID() != TF_WEAPON_RAILGUN)
	{
		pPlayer->m_Shared.RemoveCond(TF_COND_AIMING);
		pPlayer->TeamFortress_SetSpeed();
	}
	else
	{
		pPlayer->m_Shared.RemoveCond(TF_COND_AIMING_SCOPE_ONLY);
		pPlayer->TeamFortress_SetSpeed();
	}
	pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;

#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	pPlayer->ClearExpression();
#endif

	// if we are thinking about zooming, cancel it
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
	m_flChargedDamage = 0.0f;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Fire( CTFPlayer *pPlayer )
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ((ReserveAmmo() <= 0 && !UsesClipsForAmmo1()) || (Clip1() <= 0 && UsesClipsForAmmo1()))
	{
		HandleFireOnEmpty();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	if ( IsZoomed() )
	{
		// If we have more bullets, zoom out, play the bolt animation and zoom back in
		if( ReserveAmmo() > 0 )
		{
			if (UsesClipsForAmmo1())
			{
				if (Clip1() <= 0)
					SetRezoom(true, 0.25f);
			}
			else
				SetRezoom( true, 0.5f );	// zoom out in 0.5 seconds, then rezoom
		}
		else	
		{
			//just zoom out
			SetRezoom( false, 0.5f );	// just zoom out in 0.5 seconds
		}
	}
	else
	{
		// Prevent primary fire preventing zooms
	int bHoldToZoom = 0;
#ifdef GAME_DLL
		bHoldToZoom = pPlayer->IsFakeClient() ? 0 : V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "of_holdtozoom"));
#else
		bHoldToZoom = V_atoi(of_holdtozoom.GetString());
#endif

		m_flNextSecondaryAttack = bHoldToZoom > 0 ? gpGlobals->curtime : gpGlobals->curtime + GetFireRate();
	}

	m_flChargedDamage = 0.0f;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::SetRezoom( bool bRezoom, float flDelay )
{
	int HoldToZoom = 0;
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
#ifdef GAME_DLL
	if( pPlayer )
		HoldToZoom = pPlayer->IsFakeClient() ? 0 : V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "of_holdtozoom"));
#else
	HoldToZoom = V_atoi(of_holdtozoom.GetString());
#endif	

	if( HoldToZoom )
		return;
	if( pPlayer )
		pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + 0.25f;
	
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProjectileDamage( void )
{
	// Uncharged? Min damage.
	return max( m_flChargedDamage, GetDamage() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifle::GetDamageType( void ) const
{
	if ((GetWeaponID() == TF_WEAPON_RAILGUN) || (GetWeaponID() == TFC_WEAPON_SNIPER_RIFLE))
		return BaseClass::GetDamageType();

	// Only do hit location damage if we're zoomed
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) ) || of_headshots.GetBool() )
		return BaseClass::GetDamageType();

	return ( BaseClass::GetDamageType() & ~DMG_USE_HITLOCATIONS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::CreateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Check to see if we have already been created?
	if ( m_hSniperDot )
		return;

	// Get the owning player (make sure we have one).
	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	// Create the sniper dot, but do not make it visible yet.
	m_hSniperDot = CSniperDot::Create( GetAbsOrigin(), GetDamage() , pPlayer, true );
	m_hSniperDot->ChangeTeam( pPlayer->GetTeamNumber() );

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::DestroySniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Destroy the sniper dot.
	if ( m_hSniperDot )
	{
		UTIL_Remove( m_hSniperDot );
		m_hSniperDot = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::UpdateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Get the start and endpoints.
	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, ( MASK_SHOT & ~CONTENTS_WINDOW ), GetOwner(), COLLISION_GROUP_NONE, &trace );

	// Update the sniper dot.
	if ( m_hSniperDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
		}

		m_hSniperDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanFireCriticalShot( bool bIsHeadshot )
{
	// can only fire a crit shot if this is a headshot
	if ( !bIsHeadshot )
		return false;
	
	if( of_headshots.GetBool() )
		return true;

	// Railgun headshots are disabled?
	if (GetWeaponID() == TF_WEAPON_RAILGUN && of_headshot_disable_railgun.GetBool() == true)
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// no crits if they're not zoomed
		if (((GetWeaponID() != TF_WEAPON_RAILGUN) && (GetWeaponID() != TFC_WEAPON_SNIPER_RIFLE)) 
			&& (pPlayer->GetFOV() >= pPlayer->GetDefaultFOV()))
		{
			return false;
		}

		// no crits for 0.2 seconds after starting to zoom
		if (((GetWeaponID() != TF_WEAPON_RAILGUN) && (GetWeaponID() != TFC_WEAPON_SNIPER_RIFLE))
			&& ((gpGlobals->curtime - pPlayer->GetFOVTime()) < TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME))
		{
			return false;
		}
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCSniperRifle::ItemPostFrame(void)
{

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if( !pPlayer )
		return;
	
	// If we're lowered, we're not allowed to fire
	if (m_bLowered)
		return;

	if( !CanAttack() )
	{
		if (IsZoomed())
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	if (m_hSniperDot)
	{
		UpdateSniperDot();
	}
#endif

	if (pPlayer->m_nButtons & IN_ATTACK)
	{
	    //Added the charge statement here to allow the charge to start on primary hold. So now the charge after float will go up with the dot until both reach the correct float number and then on the release the float will send the multiplied damage to fire

        float flChargeAfter = m_flNextPrimaryAttack;
        if ( flChargeAfter <= gpGlobals->curtime) {
            // Don't start charging in the time just after a shot before we unzoom to play rack anim.
			if (pPlayer->m_Shared.InCond(TF_COND_AIMING_TFC) && !m_bRezoomAfterShot && !GetTFWpnData().m_bNoSniperCharge) {
                m_flChargedDamage = min((m_flChargedDamage + gpGlobals->frametime * GetDamage()), GetDamage() * 3);
            } else {
                m_flChargedDamage = max(0,m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC);
            }
        }
		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
        {

			pPlayer->m_Shared.AddCond(TF_COND_AIMING_TFC);
			pPlayer->TeamFortress_SetSpeed();
			#ifdef GAME_DLL
				// Create the sniper dot.
				CreateSniperDot(); 
				pPlayer->ClearExpression();
			#endif
		}
	}

	// Fire.
	if (pPlayer->m_afButtonReleased & IN_ATTACK)
	{

		Fire(pPlayer);
		pPlayer->m_Shared.RemoveCond(TF_COND_AIMING_TFC);
		pPlayer->TeamFortress_SetSpeed();
		#ifdef GAME_DLL
			// Create the sniper dot.
			DestroySniperDot();
			pPlayer->ClearExpression();
		#endif
	}

	if (!((pPlayer->m_afButtonReleased & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2)))
	{
		if (!ReloadOrSwitchWeapons() && (m_bInReload == false))
		{
			WeaponIdle();
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFCSniperRifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#endif

	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (pPlayer && pPlayer->m_Shared.InCond(TF_COND_AIMING_TFC))
	{
		pPlayer->m_Shared.RemoveCond(TF_COND_AIMING_TFC);
		pPlayer->TeamFortress_SetSpeed();
	#ifdef GAME_DLL
		// Create the sniper dot.
		DestroySniperDot();
		pPlayer->ClearExpression();
	#endif
	}

	m_flChargedDamage = 0.0f;
	ResetTimers();

	return BaseClass::Holster(pSwitchingTo);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCSniperRifle::Fire(CTFPlayer *pPlayer)
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( ReserveAmmo() <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	if ( IsZoomed() )
	{
		// If we have more bullets, zoom out, play the bolt animation and zoom back in
		if( ReserveAmmo() < 0 )
		{
			//just zoom out
			SetRezoom( false, 0.5f );	// just zoom out in 0.5 seconds
		}
	}
	else
	{
		// Prevent primary fire preventing zooms
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	}

	m_flChargedDamage = 0.0f;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#endif
}
//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFCSniperRifle::Zoom(void)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (pPlayer)
	{
		pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = max(m_flNextPrimaryAttack, gpGlobals->curtime + 0.1);

	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCSniperRifle::ZoomOutIn(void)
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (pPlayer && pPlayer->ShouldAutoRezoom())
	{
		m_flRezoomTime = gpGlobals->curtime + 0.9;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCSniperRifle::ZoomIn(void)
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if (!pPlayer)
		return;

	if (ReserveAmmo() <= 0)
		return;

	CTFWeaponBaseGun::ZoomIn();

	pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;

	pPlayer->m_Shared.AddCond(TF_COND_AIMING_SCOPE_ONLY);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCSniperRifle::ZoomOut(void)
{
	CTFWeaponBaseGun::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if (!pPlayer)
		return;

	pPlayer->m_Shared.m_flNextZoomTime = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;

	// if we are thinking about zooming, cancel it
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;

	pPlayer->m_Shared.RemoveCond(TF_COND_AIMING_SCOPE_ONLY);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCSniperRifle::SetRezoom(bool bRezoom, float flDelay)
{
	BaseClass::SetRezoom(bRezoom, flDelay);
}

// We need to set the damage amount that is stored to the charge float which is what the Primary attack in fire reads- Sir Matrix
float CTFCSniperRifle::GetProjectileDamage( void )
{
    // Uncharged? Min damage.
    return max( m_flChargedDamage, GetDamage() );
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetHUDDamagePerc( void )
{
	return ( m_flChargedDamage / ( GetDamage() * 3 ));
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCSniperRifle::GetHUDDamagePerc(void)
{
	return (m_flChargedDamage / (GetDamage() * 3));
}
//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( pPlayer )
	{
		//can someone please find a better way of doing this
		CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
		if (pWpn->GetWeaponID() == TFC_WEAPON_SNIPER_RIFLE)
		{
			CTFCSniperRifle *pWeapon = assert_cast<CTFCSniperRifle*>(pPlayer->GetActiveTFWeapon());
			if (pWeapon)
			{
				float flChargeValue = ((1.0 - pWeapon->GetHUDDamagePerc()) * 0.8) + 0.6;

				VMatrix mat, temp;

				Vector2D center(0.5, 0.5);
				MatrixBuildTranslation(mat, -center.x, -center.y, 0.0f);

				// scale
				{
					Vector2D scale(1.0f, 0.25f);
					MatrixBuildScale(temp, scale.x, scale.y, 1.0f);
					MatrixMultiply(temp, mat, mat);
				}

				MatrixBuildTranslation(temp, center.x, center.y, 0.0f);
				MatrixMultiply(temp, mat, mat);

				// translation
				{
					Vector2D translation(0.0f, flChargeValue);
					MatrixBuildTranslation(temp, translation.x, translation.y, 0.0f);
					MatrixMultiply(temp, mat, mat);
				}

				m_pResult->SetMatrixValue(mat);
			}
		}
		else
		{
			CTFSniperRifle *pWeapon = assert_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
			if (pWeapon)
			{
				float flChargeValue = ((1.0 - pWeapon->GetHUDDamagePerc()) * 0.8) + 0.6;

				VMatrix mat, temp;

				Vector2D center(0.5, 0.5);
				MatrixBuildTranslation(mat, -center.x, -center.y, 0.0f);

				// scale
				{
					Vector2D scale(1.0f, 0.25f);
					MatrixBuildScale(temp, scale.x, scale.y, 1.0f);
					MatrixMultiply(temp, mat, mat);
				}

				MatrixBuildTranslation(temp, center.x, center.y, 0.0f);
				MatrixMultiply(temp, mat, mat);

				// translation
				{
					Vector2D translation(0.0f, flChargeValue);
					MatrixBuildTranslation(temp, translation.x, translation.y, 0.0f);
					MatrixMultiply(temp, mat, mat);
				}

				m_pResult->SetMatrixValue(mat);
			}
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CProxySniperRifleCharge, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( SniperDot, DT_SniperDot )

BEGIN_NETWORK_TABLE( CSniperDot, DT_SniperDot )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeStartTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_sniperdot, CSniperDot );

BEGIN_DATADESC( CSniperDot )
DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSniperDot::CSniperDot( void )
{
	m_vecSurfaceNormal.Init();
	m_hTargetEnt = NULL;

#ifdef CLIENT_DLL
	m_hSpriteMaterial = NULL;
#endif
	m_nDamage = -1;

	ResetChargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CSniperDot::~CSniperDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CSniperDot
//-----------------------------------------------------------------------------
CSniperDot *CSniperDot::Create( const Vector &origin, int damage, CBaseEntity *pOwner, bool bVisibleDot )
{
// Client specific.
#ifdef CLIENT_DLL

	return NULL;

// Server specific.
#else
	// Create the sniper dot entity.
	CSniperDot *pDot = static_cast<CSniperDot*>( CBaseEntity::Create( "env_sniperdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	//Create the graphic
	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	pDot->m_nDamage = damage;
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	// Set owner.
	pDot->SetOwnerEntity( pOwner );

	// Force updates even though we don't have a model.
	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	return pDot;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal )
{
	SetAbsOrigin( vecOrigin );
	m_vecSurfaceNormal = vecNormal;
	m_hTargetEnt = pTarget;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// TFTODO: Make the sniper dot get brighter the more damage it will do.
//-----------------------------------------------------------------------------
int CSniperDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return -1;

	// Get the sprite rendering position.
	Vector vecEndPos;

	float flSize = 6.0;

	if ( !pPlayer->IsDormant() )
	{
		Vector vecAttachment, vecDir;
		QAngle angles;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			// Clamp the forward distance for the sniper's firstperson
			flDist = 384;

			flSize = 2.0;
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors( angles, &vecDir );
		}

		trace_t tr;
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

		// Backup off the hit plane, towards the source
		vecEndPos = tr.endpos + vecDir * -4;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	float flStrength = RemapValClamped( flLifeTime, 0.0, m_nDamage * 3 / m_nDamage, 0.1, 1.0 );

	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSniperDot::ShouldDraw( void )			
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	// Don't draw the sniper dot when in thirdperson.
	if ( ::input->CAM_IsThirdPerson() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if (GetTeamNumber() == TF_TEAM_BLUE)
		{
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_BLUE, TEXTURE_GROUP_CLIENT_EFFECTS);
		}
		else
		{
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_RED, TEXTURE_GROUP_CLIENT_EFFECTS);
		}
	}
}

#endif

acttable_t CTFSniperRifle::m_acttableSniperRifle[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_RAILGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_RAILGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_RAILGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_RAILGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_RAILGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_RAILGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_RAILGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_RAILGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_RAILGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_RAILGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_RAILGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_RAILGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_RAILGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_RAILGUN, false },

	{ ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED, ACT_MERC_ATTACK_CROUCH_RAILGUN_DEPLOYED, false },
	{ ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, ACT_MERC_ATTACK_STAND_RAILGUN_DEPLOYED, false },
	{ ACT_MP_CROUCHWALK_DEPLOYED, ACT_MERC_CROUCHWALK_RAILGUN_DEPLOYED, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MERC_CROUCH_RAILGUN_DEPLOYED_IDLE, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MERC_RAILGUN_DEPLOYED_IDLE, false },
	{ ACT_MP_DEPLOYED, ACT_MERC_RAILGUN_DEPLOYED, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MERC_RAILGUN_SWIM_DEPLOYED, false },
};

acttable_t *CTFSniperRifle::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableSniperRifle);
		return m_acttableSniperRifle;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t CTFRailgun::m_acttableRailgun[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_RAILGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_RAILGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_RAILGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_RAILGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_RAILGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_RAILGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_RAILGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_RAILGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_RAILGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_RAILGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_RAILGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_RAILGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_RAILGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_RAILGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED, ACT_MERC_ATTACK_CROUCH_RAILGUN_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED, ACT_MERC_ATTACK_STAND_RAILGUN_DEPLOYED, false },
	{ ACT_MP_CROUCHWALK_DEPLOYED, ACT_MERC_CROUCHWALK_RAILGUN_DEPLOYED, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MERC_CROUCH_RAILGUN_DEPLOYED_IDLE, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MERC_RAILGUN_DEPLOYED_IDLE, false },
	{ ACT_MP_DEPLOYED, ACT_MERC_RAILGUN_DEPLOYED, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MERC_RAILGUN_SWIM_DEPLOYED, false },
};

acttable_t *CTFRailgun::ActivityList(int &iActivityCount)
{
	if( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableRailgun);
		return m_acttableRailgun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}