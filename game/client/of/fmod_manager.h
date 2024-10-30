#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"

using namespace FMOD;

struct COFSoundParameters;

enum
{
	SOUND_INSTANCE_SOUND = 1,
	SOUND_INSTANCE_SOUNDSCAPE
};

enum OF_SOUND_CHANNEL
{
	OF_CHANNEL_GAME = 0,
	OF_CHANNEL_MUSIC,
	OF_CHANNEL_ANNOUNCER,
	
	OF_CHANNEL_COUNT
};

class SoundData_t
{
public:
	SoundData_t();
	~SoundData_t();

	virtual bool LoadSoundFiles( const char *szBaseWav, const char *szIntroWav, const char *szOutroWav );
	virtual bool SetupStreams();
	// Has to be called *after* soundscript and index have been set
	virtual bool SetupSyncPoints();
	virtual bool SetupCallback();

	virtual void TransitionToLoop();
	virtual void TransitionToEnd();
	virtual void EndSound();

	virtual void SetLooping( bool bLooping );
	virtual void SetVolume( float flVolume );

	virtual void UpdateVolume();

public:
	// Channel we use to stream the sounds
	ChannelGroup *pChannelGroupPtr;
	Channel		 *pChannelPtr;

	Sound		*pIntroPtr;
	Sound		*pBasePtr;
	Sound		*pOutroPtr;
	
	byte		*pIntroBuf;
	byte		*pBaseBuf;
	byte		*pOutroBuf;

	const char	*pIntroFile;
	const char	*pBaseFile;
	const char	*pOutroFile;

	int			m_iIntroSize;
	int			m_iBaseSize;
	int			m_iOutroSize;
	
	int			m_iSoundscript;
	int			m_iIndex;

	// Unused for now, setup in the transition functions if needed
	int			m_iStage;

	// Parameters
	float				m_flVolume;
	OF_SOUND_CHANNEL	m_nChannel;
	bool				m_bIsLooping;
	bool				m_bResetWithRound;
};

typedef CUtlMap<int, SoundData_t*>* SOUNDSCRIPT_CONTAINER;

class CFMODManager : public CGameEventListener
{
public:
	CFMODManager();
	~CFMODManager();

	void InitFMOD();
	void ExitFMOD();

	void FireGameEvent( IGameEvent *event ) override;

	void Think();
	
	SoundData_t *PlaySound(const char *szSoundName, OF_SOUND_CHANNEL nChan = OF_CHANNEL_GAME, float flDelay = 0.0f);

	static bool GetSoundscriptWavs( COFSoundParameters *pSoundscript, const char *&szBaseWav, const char *&szIntroWav, const char *&szOutroWav );

	void SetLoopSound( const char *szSoundName, bool bLoop, int iIndex = -1 );
	void SetSoundVolume( float flVolume, char *szSoundName, int iIndex = -1 );
	
	void StopSound( const char *szSoundName, int iIndex = -1 );
	void StopAllAnnouncerSounds( void );
	void ToEndSound( const char *szSoundName, int iIndex = -1 );

	void StopAllSound( bool bRound = false );
	
	virtual float GetVolumeModifier( OF_SOUND_CHANNEL nChan = OF_CHANNEL_GAME );
	void UpdateSoundVolumes();
	
	static unsigned int GetSoundLengthPCM( Sound *sound );	// In PCM
	static unsigned int GetSoundLength( const char* szSoundPath ); // In miliseconds

	short GetSoundInstance( SoundData_t **sound, SOUNDSCRIPT_CONTAINER* soundscripts, const char *szSoundName, int iIndex = -1 );
	
	//void PlayLoopingMusic( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic,const char* pIntroMusic = NULL, float flDelay = 0 , bool fadeIn = false);
	//void PlayMusicEnd( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, bool bDelay = false, Channel *pLoopingChannel = NULL );

	static byte *GetRawSoundPointer( const char* pathToFileFromModFolder, int *pLength );
public:
	CUtlDict< SOUNDSCRIPT_CONTAINER, unsigned short > m_Sounds;
	
private:
	bool m_bUnfocused;
	float m_flVolume;
};

extern CFMODManager* FMODManager();
 
#endif //FMOD_MANAGER_H