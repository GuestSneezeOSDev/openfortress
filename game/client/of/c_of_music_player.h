#ifndef OF_MUSIC_PLAYER_H
#define OF_MUSIC_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod_manager.h"

#include "saverestore_utlvector.h"

class C_TFMusicPlayer : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFMusicPlayer, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	C_TFMusicPlayer();
	~C_TFMusicPlayer();
	virtual void ClientThink(void);
	void HandleVolume(void);
	virtual void Spawn(void);
	virtual void OnPreDataChanged(DataUpdateType_t updateType);
	virtual void OnDataChanged(DataUpdateType_t updateType);
	int iSoundIndex;
private:

	int m_iPhase;
	
	bool m_bShouldBePlaying;
	bool m_bHardTransition;
	bool bIsPlaying;
	
	bool bParsed;
	
	float flLoopTick;
	float m_flDelay;
	float m_flVolume;

	char szLoopingSong[MAX_PATH] = {};
	char szOldLoopingSong[MAX_PATH] = {};

	bool bRestart;

	SoundData_t *pSound;
};

class C_TFDMMusicManager : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_TFDMMusicManager, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_TFDMMusicManager();
	~C_TFDMMusicManager();

	// Input handlers
	int m_iIndex;
	CNetworkHandle( C_TFMusicPlayer, pWaitingMusicPlayer );
	CNetworkHandle( C_TFMusicPlayer, pRoundMusicPlayer );
	
	char szWaitingForPlayerMusic[64];
	char szRoundMusic[64];
	
	char szWaitingMusicPlayer[64];
	char szRoundMusicPlayer[64];
};

extern C_TFDMMusicManager* DMMusicManager();

#endif //OF_MUSIC_PLAYER_H