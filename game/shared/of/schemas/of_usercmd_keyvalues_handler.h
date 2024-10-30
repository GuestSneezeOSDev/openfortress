#ifndef OF_CLIENT_TO_SERVER_KEYVALUES_H
#define OF_CLIENT_TO_SERVER_KEYVALUES_H
#ifdef _WIN32
#pragma once
#endif

class KeyValues;

class COFUserCMDKeyValueHandler
{
public:
	void AddMessage( KeyValues *pKV );
	void RemoveMessage( const char *szName );
public:
	CUtlDict<KeyValues *, unsigned short> m_Messages;
};

extern COFUserCMDKeyValueHandler g_UserCMDKeyValueHandler;
extern void SendKeyValuesToServer( KeyValues *pKV );

#define OF_KEYVALUE_MESSAGE_COUNT 1

extern const char *g_aValidUserKeyvalueNames[OF_KEYVALUE_MESSAGE_COUNT];

extern bool KeyValuesReadFromUserCMD( KeyValues *pKv, bf_read *buf, int nStackDepth = 0 );
extern void KeyValuesWriteToUserCMD( KeyValues *pKV, bf_write *buf );

#endif // OF_CLIENT_TO_SERVER_KEYVALUES_H