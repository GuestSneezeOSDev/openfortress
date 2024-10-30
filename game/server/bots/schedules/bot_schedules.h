//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017
//
// Conjunto de tareas, aqu� es donde esta la I.A. de los Bots, cada conjunto
// de tareas se activa seg�n el nivel de deseo que devuelve la funci�n
// GetDesire(), el conjunto con m�s deseo se activar� y empezar� a realizar cada
// una de las tareas que se le fue asignada.
//
// Si la funci�n ItsImportant() devuelve false entonces cualquier otro conjunto
// que tenga m�s deseo terminara y reemplazara la activa.
//
// El funcionamiento predeterminado de cada tarea se encuentra en 
// el archivo bot_schedules.cpp. Se puede sobreescribir el funcionamiento
// de una tarea al devolver true en las funciones StartTask y RunTask de CBot
//
//=============================================================================//

#ifndef BOT_SCHEDULES_H
#define BOT_SCHEDULES_H

#ifdef _WIN32
#pragma once
#endif

#include "bots/interfaces/ibotschedule.h"

//================================================================================
// Investigar una ubicaci�n y regresar a donde estabamos si no hab�a nada
//================================================================================
class CInvestigateLocationSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CInvestigateLocationSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_INVESTIGATE_LOCATION );

    CInvestigateLocationSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
	virtual bool ItsImportant() { return true; }
    virtual float GetDesire() const;
};

//================================================================================
// Perseguir a nuestro enemigo
//================================================================================
class CHuntEnemySchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CHuntEnemySchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HUNT_ENEMY );

    CHuntEnemySchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
};

//================================================================================
// Recargar
//================================================================================
class CReloadSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CReloadSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_RELOAD );

    CReloadSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
};

//================================================================================
// Cubrirse
//================================================================================
class CCoverSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCoverSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_COVER );

    CCoverSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
	virtual bool ItsImportant() { return true; }
};

//================================================================================
// Ocultarse, como cubrirse pero sin volver a nuestra ubicaci�n original
//================================================================================
class CHideSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCoverSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HIDE );

    CHideSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
	virtual bool ItsImportant() { return true; }
};

//================================================================================
// Tomar el arma en el suelo que estamos viendo.
//================================================================================
class CChangeWeaponSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CChangeWeaponSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_CHANGE_WEAPON );

    CChangeWeaponSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

	enum
	{
		TASK_GRAB_WEAPON = BCUSTOM_TASK
	};

public:
    virtual float GetDesire() const;

    virtual void TaskStart();
	virtual void TaskRun();
};

//================================================================================
// Moverse a un lado
//================================================================================
class CMoveAsideSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CMoveAsideSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_MOVE_ASIDE );

    CMoveAsideSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
	virtual void Start();


protected:
	CountdownTimer m_nMoveAsideTimer;
};

//================================================================================
// Retirarse y llamar refuerzos
//================================================================================
class CCallBackupSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCallBackupSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_CALL_FOR_BACKUP );

    CCallBackupSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const {
        return BOT_DESIRE_NONE;
    }
};

//================================================================================
// Defender el punto de aparici�n
//================================================================================
class CDefendSpawnSchedule : public IBotSchedule
{
public:
    DECLARE_CLASS_GAMEROOT( CDefendSpawnSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_DEFEND_SPAWN );

    CDefendSpawnSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;
};

#endif // BOT_SCHEDULES_H