//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "of_shared_schemas.h"
#include "of_items_game.h"
#include "of_sound_params.h"
#include "of_map_data.h"
#include "of_announcer_schema.h"
#include "of_loadout.h"

#include "tier0/memdbgon.h"

void InitSharedSchemas()
{
#ifdef GAME_DLL
	InitLoadoutManager();
#endif
	InitItemSchema();
	InitSoundManifest();
	InitAnnouncerSchema();
	InitMapDataManager();
}

// Load all schemas on client/server start
void ParseSharedSchemas()
{
	InitSharedSchemas();
#ifdef CLIENT_DLL
	// Loadout is only clientside
	ParseLoadout();
#endif
	GetItemSchema()->ParseItemsGame();
	ParseSoundManifest();
	GetAnnouncers()->ParseAnnouncers();
}

void ParseLevelSchemas()
{
	ParseLevelSoundManifest();
	GetMapData()->ParseMapDataSchema();
	GetItemSchema()->OnMapChange();
}