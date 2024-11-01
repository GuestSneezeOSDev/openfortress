//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( TEXT_MESSAGE_H )
#define TEXT_MESSAGE_H
#ifdef _WIN32
#pragma once
#endif

abstract_class IHudTextMessage
{
public:
	virtual const char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size ) = 0;
	virtual const char *BufferedLocaliseTextString( const char *msg ) = 0;
	virtual const char *LookupString( const char *msg_name, int *msg_dest = NULL ) = 0;
};

extern IHudTextMessage *hudtextmessage;
#endif // TEXT_MESSAGE_H