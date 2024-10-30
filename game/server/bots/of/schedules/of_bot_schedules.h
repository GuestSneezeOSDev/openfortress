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

#ifndef OF_BOT_SCHEDULES_H
#define OF_BOT_SCHEDULES_H

#ifdef _WIN32
#pragma once
#endif

#include "bots/interfaces/ibotschedule.h"

//================================================================================
// Take a weapon from a spawner
//================================================================================
class CPickupWeaponSpawnerSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CPickupWeaponSpawnerSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_PICKUP_SPAWNER );

    CPickupWeaponSpawnerSchedule( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual float GetDesire() const;

    virtual void TaskStart();
	virtual void TaskRun();
};

//================================================================================
// Roam around the map
//================================================================================
class CFreeRoamSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CFreeRoamSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_FREE_ROAM);

	CFreeRoamSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;
	virtual void TaskRun();
};

//================================================================================
// Take a Powerup from a spawner
//================================================================================
class CPickupPowerupSpawnerSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CPickupPowerupSpawnerSchedule, IBotSchedule );
	DECLARE_SCHEDULE( SCHEDULE_PICKUP_POWERUP );

	CPickupPowerupSpawnerSchedule( IBot *bot ) : BaseClass( bot )
	{
	}

public:
	virtual float GetDesire() const;

	virtual void TaskStart();
	virtual void TaskRun();
};

//================================================================================
// Find health while raoming
//================================================================================
class CPickupHealthSpawnerSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CPickupHealthSpawnerSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_FIND_HEALTH);

	CPickupHealthSpawnerSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;

	virtual void TaskStart();
	virtual void TaskRun();
};

//================================================================================
// Find ammo while raoming
//================================================================================
class CPickupAmmoSpawnerSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CPickupAmmoSpawnerSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_FIND_AMMO);

	CPickupAmmoSpawnerSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;

	virtual void TaskStart();
	virtual void TaskRun();
};
//================================================================================
// Roam around the map
//================================================================================
class CCaptureFlagSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CCaptureFlagSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_CAPTURE_FLAG);

	CCaptureFlagSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;
	virtual void TaskRun();
};
//================================================================================
// Roam around the map
//================================================================================
class CGotoFlagSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CGotoFlagSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_GOTO_FLAG);

	CGotoFlagSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;
	virtual void TaskRun();
};
//================================================================================
// Roam around the map
//================================================================================
class CDefendFlagSchedule : public IBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT(CDefendFlagSchedule, IBotSchedule);
	DECLARE_SCHEDULE(SCHEDULE_DEFEND_OWN_FLAG);

	CDefendFlagSchedule(IBot *bot) : BaseClass(bot)
	{
	}

public:
	virtual float GetDesire() const;
	virtual void TaskRun();
	//virtual void TaskComplete();
};
#endif // OF_BOT_SCHEDULES_H