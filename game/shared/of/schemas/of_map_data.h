#ifndef OF_MAP_DATA_H
#define OF_MAP_DATA_H
#ifdef _WIN32
#pragma once
#endif

class KeyValues;

extern void InitMapDataManager();

class COFMapDataManager
{
public:
	COFMapDataManager(){ m_MapData = NULL; }

	void InitMapData();
	void ParseMapDataSchema();

	KeyValues *GetEntity( const char *szClassname, int iIndex );
	KeyValues *GetBotTemlpate( const char *szBotName );
	KeyValues *GetWavePreset( const char *szWaveName );
private:
	KeyValues* m_MapData;
};

extern COFMapDataManager *GetMapData( void );

#endif // OF_MAP_DATA_H