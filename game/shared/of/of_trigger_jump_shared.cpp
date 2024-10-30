#include "cbase.h"
#include "movevars_shared.h"

#ifdef GAME_DLL
	#include "of_trigger_jump.h"
	#include "tf_player.h"
#else
	#include "c_of_trigger_jump.h"
	#include "c_tf_player.h"

	#define COFDTriggerJump C_OFDTriggerJump
#endif


void COFDTriggerJump::StartTouch( CBaseEntity *pOther )
{
	if( pOther->GetMoveType() != MOVETYPE_WALK && pOther->GetMoveType() != MOVETYPE_VPHYSICS && pOther->GetMoveType() != MOVETYPE_FLYGRAVITY && pOther->GetMoveType() != MOVETYPE_STEP )
		return;

	// ignore grappling hooks or tf ents
	// WTF?
	if ( !pOther->IsPlayer() )
	{
		if ( V_strncmp( "gr", pOther->GetClassname(), 2 ) == 0 ) 
			return;
		if ( V_strncmp( "tf", pOther->GetClassname(), 2 ) == 0 ) 
			return;
	}

	// Just slam any projectiles to 1, not pretty but no big reason not to.
	if( pOther->GetMoveType() == MOVETYPE_FLYGRAVITY )
		pOther->SetGravity( 1.0 );

	Vector startPos = pOther->GetAbsOrigin();
	Vector endPos = m_vecTarget;
	float flGravity = GetCurrentGravity();

	//assume a peak for our air travel, this is conceptually garbage but there is no other way
	//without getting a second value set from the mapper
	float flPeak = max( endPos.z, startPos.z ) + 720.f;

	//assume a time depending on what it takes to fall from the peak to the lowest point
	float flTime = sqrt( 2.f * abs(min( endPos.z, startPos.z ) - flPeak) / flGravity );

	Vector2D vecTargetVelXY = ( Vector2D( endPos.x, endPos.y ) - Vector2D( startPos.x, startPos.y ) ) / flTime;
	Vector vecTargetVel = Vector( vecTargetVelXY.x, vecTargetVelXY.y, ( endPos.z - startPos.z ) / flTime + ( flGravity * flTime ) / 2.f );

	pOther->SetGroundEntity(NULL);

	//add the current player velocity to the jumppad output
	if( m_bNoCompensation )
    {
		Vector vecCurrentVel = pOther->GetAbsVelocity();
        if (!m_bDontReduceBackwardsAccel)
        {
            Vector2D vecCurrentDir2D(vecCurrentVel.x, vecCurrentVel.y);
            vecCurrentDir2D.NormalizeInPlace();

            Vector2D vecTargetDir2D(vecTargetVelXY);
            vecTargetDir2D.NormalizeInPlace();

            Vector2D vecDeltaDir = vecTargetDir2D + vecCurrentDir2D;

            if (vecDeltaDir.Length() < 1)
            {
                vecTargetVel.x += vecCurrentVel.x * vecDeltaDir.Length();
                vecTargetVel.y += vecCurrentVel.y * vecDeltaDir.Length();
            }
            else
            {
                vecTargetVel.x += vecCurrentVel.x;
                vecTargetVel.y += vecCurrentVel.y;
            }
        }
        else
        {
            vecTargetVel.x += vecCurrentVel.x;
            vecTargetVel.y += vecCurrentVel.y;
        }
    }
	else
	{
		//for safety
		pOther->SetAbsVelocity( Vector(0.f, 0.f, 0.f) );
	}
    CTFPlayer *pPlayer = ToTFPlayer(pOther);
	//remove air control while in the air
	if ( m_bNoAirControl && m_bImSureAboutNoAirControl )
	{
		if (pPlayer)
		{
			pPlayer->m_Shared.SetNoAirControl(true);
		}
	}
	if ( vecTargetVel.IsValid() )
	{
		if (!pOther->IsCombatCharacter() && pOther->VPhysicsGetObject())
		{
			pOther->VPhysicsGetObject()->SetVelocityInstantaneous( &vecTargetVel, NULL );
		}
		else
		{
			pOther->SetAbsVelocity(vecTargetVel);
		}
	}

#ifdef GAME_DLL
	m_OnJump.FireOutput( pOther, this );

	if (!m_bDontReduceBackwardsAccel && pPlayer)
	{
		pPlayer->m_Shared.SetLaunchedFromJumppad(true);
	}
	/*
	CTFBot *actor = ToTFBot( pOther );
	if ( actor )
	{
		actor->m_bTouchedJumpPad = true;
		actor->TouchJumpPad();
	}
	*/

	pOther->EmitSound( "JumpPadSound" );
#endif
}
