#ifndef OF_SOUND_PARAMS_H
#define OF_SOUND_PARAMS_H
#ifdef _WIN32
#pragma once
#endif

#include "SoundEmitterSystem/isoundemittersystembase.h"

class KeyValues;

extern void InitSoundManifest();

// Our sound params just add onto the existing ones
struct COFSoundParameters : public CSoundParametersInternal
{
public:
	COFSoundParameters() : CSoundParametersInternal()
	{
		m_szMusicName[0] = '\0';
		m_szArtist[0] = '\0';
		m_szIntro[0] = '\0';
		m_szOutro[0] = '\0';
		m_szWinMusic[0] = '\0';
	}

	const char *GetWav( int i = 0 );
	const char *GetRandomWav();

	char *GetMusicName(){ return m_szMusicName; }
	char *GetArtist(){ return m_szArtist; }
	char *GetIntro(){ return m_szIntro; }
	char *GetOutro(){ return m_szOutro; }
	char *GetWinMusic(){ return m_szWinMusic; }

	void SetMusicName( const char *szMusicName ){ Q_strncpy(m_szMusicName, szMusicName, sizeof(m_szMusicName)); }
	void SetArtist( const char *szArtist ){ Q_strncpy(m_szArtist, szArtist, sizeof(m_szArtist)); }
	void SetIntro( const char *szIntro ){ Q_strncpy(m_szIntro, szIntro, sizeof(m_szIntro)); }
	void SetOutro( const char *szOutro ){ Q_strncpy(m_szOutro, szOutro, sizeof(m_szOutro)); }
	void SetWinMusic( const char *szWinMusic ){ Q_strncpy(m_szWinMusic, szWinMusic, sizeof(m_szWinMusic)); }
private:
	char m_szMusicName[64];
	char m_szArtist[128];
	char m_szIntro[64];
	char m_szOutro[64];
	char m_szWinMusic[64];
};

class CTFSoundManifest
{
public:
	void Clear(){ m_Soundscripts.PurgeAndDeleteElements(); }
	void ParseFile( KeyValues *pKV, bool bLevelSounds = false );
	COFSoundParameters *CreateSoundEntry(){ return new COFSoundParameters; };
	void ParseSoundEntry( KeyValues *pKV, COFSoundParameters *pSound );

	COFSoundParameters *GetSoundscript( const char *szName );
	
	void AddSoundsFromFile( const char *szFile );
	void ReParsePermanentSoundFiles();
	void ReAddPermanentSoundFiles();
private:
	CUtlDict< COFSoundParameters*, unsigned short > m_Soundscripts;
	CUtlDict< COFSoundParameters*, unsigned short > m_LevelSoundscripts;

	CUtlStringList m_AddedSoundScripts;
};

extern CTFSoundManifest *OFSoundManifest();
extern void ParseSoundManifest( void );
extern void ParseLevelSoundManifest( void );
#ifdef CLIENT_DLL
extern void PrecacheUISoundScript( char *szSoundScript );
#endif

#endif // OF_SOUND_PARAMS_H