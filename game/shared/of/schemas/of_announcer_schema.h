#ifndef OF_ANNOUNCER_SCHEMA_H
#define OF_ANNOUNCER_SCHEMA_H
#ifdef _WIN32
#pragma once
#endif

#define DeallocStringIfExists( string ) \
	if( string ) \
		delete string;
		
extern void InitAnnouncerSchema();

class COFAnnouncerInfo
{
public:
	COFAnnouncerInfo()
	{
		m_szSoundScriptFile = NULL;
		m_iSupportedGamemodes = NULL;
	}

	~COFAnnouncerInfo()
	{
		DeallocStringIfExists( m_szSoundScriptFile );
	}

	bool SupportsCurrentGamemodes( void );
	bool SupportsGamemode( int nGametype );
	void Parse( KeyValues *pKV );

public:
	const char *m_szSoundScriptFile;
	int m_iSupportedGamemodes;
	bool m_bUniversal;
};

class COFAnnouncerSchema
{
public:
	COFAnnouncerInfo *GetAnnouncer( const char *szName );
	void ParseAnnouncers();

public:
	CUtlDict< COFAnnouncerInfo*, unsigned short > m_Announcers;
	CUtlVector<unsigned short> m_HandleOrder;
};

extern COFAnnouncerSchema *GetAnnouncers();

#undef DeallocStringIfExists

#endif // OF_ANNOUNCER_SCHEMA_H