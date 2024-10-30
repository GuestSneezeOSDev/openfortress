#ifndef OF_LOADOUT_MANAGER_H
#define OF_LOADOUT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
extern void SendLoadoutToServer();

extern KeyValues* GetLoadout();
extern void ParseLoadout( void );
extern void ResetLoadout( const char *szCatName );
#else
	
#define OF_LOADOUT_REQUEST_TIMER 1.0f

class CTFPlayer;

extern void InitLoadoutManager();

class COFClassLoadout
{
public:
	CUtlDict< float, unsigned short > m_Cosmetics;
	CUtlVector< int > m_Weapons;
};

class COFPlayerLoadout
{
public:
	COFClassLoadout m_ClassLoadout[TF_CLASS_COUNT_ALL];
};

class COFLoadoutManager
{
public:
	COFLoadoutManager()
	{
		SetDefLessFunc( m_PlayerLoadout );
	};
	bool RegisterPlayer( CTFPlayer *pPlayer );
	bool RemovePlayer( CTFPlayer *pPlayer );
	uint64 GetPlayerID( CTFPlayer *pPlayer );
	COFPlayerLoadout *GetLoadout( CTFPlayer *pPlayer );
public:
	
	CUtlMap< uint64, COFPlayerLoadout * > m_PlayerLoadout;
};

COFLoadoutManager *LoadoutManager();
#endif

#endif // OF_LOADOUT_MANAGER_H