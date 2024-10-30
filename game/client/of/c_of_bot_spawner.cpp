//====== Copyright 2023, Open Fortres, All rights reserved. =======//
//
// Purpose: Bot spawner
//
//=============================================================================//

#include "cbase.h"
#include "KeyValues.h"
#include "of_map_data.h"
#include "props_shared.h"

#include "tier0/memdbgon.h"

extern void UTIL_PrecacheSchemaWeapon(const char* szName);

#define MAX_BOT_TEMPLATES 5

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------
class C_TFBotSpawner : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFBotSpawner, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFBotSpawner()
	{
		bPrecached = false;
	}

	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PrecacheTemplate( KeyValues* kvTemplate );
	virtual bool LoadAndPrecacheTemplates();
private:
	char		m_iszWavePreset[128];
	char		m_iszBotTemplate[MAX_BOT_TEMPLATES][128];
	int			m_iTemplate;
	int			m_iMaxTemplates;
	int 		m_iWaveSize;

	KeyValues	*inBotTemplate[MAX_BOT_TEMPLATES];
	KeyValues	*inWavePreset;

	bool bPrecached;
};

IMPLEMENT_CLIENTCLASS_DT( C_TFBotSpawner, DT_BotSpawner, CTFBotSpawner )
	RecvPropInt( RECVINFO( m_iTemplate ) ),
	RecvPropInt( RECVINFO( m_iMaxTemplates ) ),
	RecvPropInt( RECVINFO( m_iWaveSize ) ),
	RecvPropString( RECVINFO( m_iszWavePreset ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_iszBotTemplate[0]) ), m_iszBotTemplate ),
END_RECV_TABLE()

// Inputs.
LINK_ENTITY_TO_CLASS( of_bot_spawner, C_TFBotSpawner );

void C_TFBotSpawner::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if( bPrecached )
		return;

	bPrecached = LoadAndPrecacheTemplates();
}

void C_TFBotSpawner::PrecacheTemplate( KeyValues *kvTemplate )
{
	int iModel = PrecacheModel( kvTemplate->GetString("Model", "") );
	PrecacheGibsForModel( iModel );

	if( kvTemplate->GetString("Weapons") )
	{
		CCommand args;
		args.Tokenize(kvTemplate->GetString("Weapons"));
		for( int i = 0; i < args.ArgC(); i++ )
		{
			UTIL_PrecacheSchemaWeapon( args[i] );
		}
	}
}

bool C_TFBotSpawner::LoadAndPrecacheTemplates( void )
{
	// If we have a wave preset, load that
	if( m_iszWavePreset[0] != '\0' )
	{
		inWavePreset = GetMapData()->GetWavePreset(m_iszWavePreset);
		if( !inWavePreset )
		{
			Warning("Preset %s not found, aborting\n", m_iszWavePreset);
			return false;
		}

		m_iWaveSize = 0;
		m_iTemplate = 0;

		int iCurrentTemplate = 0;
		FOR_EACH_VALUE( inWavePreset, kvValue )
		{
			inBotTemplate[iCurrentTemplate] = GetMapData()->GetBotTemlpate( kvValue->GetName() );

			if( !inBotTemplate[iCurrentTemplate] )
			{
				Warning("Bot template %s not found\n", kvValue->GetName());
				continue;
			}

			PrecacheTemplate( inBotTemplate[iCurrentTemplate] );
			iCurrentTemplate++;
			m_iWaveSize += kvValue->GetInt();
		}

		return true;
	}

	bool ret = false;

	// Otherwise, load it through templates
	int iCurrentTemplate = 0;
	for( int i = 0; i < MAX_BOT_TEMPLATES; i++ )
	{
		if( m_iszBotTemplate[i][0] == '\0' )
		{
			Warning("BotTemplate%d not specified, skipping\n", i+1);
			continue;
		}

		inBotTemplate[iCurrentTemplate] = GetMapData()->GetBotTemlpate( m_iszBotTemplate[i] );

		if( inBotTemplate[iCurrentTemplate] ) // unless its the first template, copy the last template if we dont find the targeted one
		{
			PrecacheTemplate(inBotTemplate[iCurrentTemplate]);
			iCurrentTemplate++;
		}

		ret = true;
	}
	m_iMaxTemplates = iCurrentTemplate;

	return ret;
}