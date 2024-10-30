//=============================================================================//
//
// Purpose: Music manager for OFD. (Mainly Deathmatch mode.)
//
//=============================================================================//
#include "cbase.h"
#if 0
#include "gameinterface.h"
#include "KeyValues.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_MUSIC_FILE	"scripts/music/default.txt"

/*
TODO:	Parse from something like this, maybe even add in a system so
		waiting music and active music are linked together somehow.

==============scripts/music/dm_2fort.txt=====================================
"dm_2fort" // Not really important. Unless we wan't to vertify we're actually
		   // loading the correct file or something.
{
	"waiting"
	{
		"wave"		"#music/deathmatch/dmchase_waiting.wav"
		"group"		//Maybe define groups? aka group1, group2 etc.
		{
			"wave"	"#music/deathmatch/map01_waiting.wav"
		}
	}
	"active"
	{
		"wave"		"#music/deathmatch/e2m1_loop.wav"
		"wave"		"#music/deathmatch/facility_loop.wav"
		"group"
		{
			"wave"	"#music/deathmatch/map01_loop.wav"
		}
	}
}
===========================================================================
*/

class COFMusicManager : public CAutoGameSystem // Change me
{
public:
	virtual bool	Init();

private:
	void	LoadMusicFromScript(char const *filesystem);
};

bool COFMusicManager::Init()
{
	LoadMusicFromScript( NULL );
	return true;
}

// Code ripped from decals.cpp
void COFMusicManager::LoadMusicFromScript(char const *filename)
{
	KeyValues *kv = new KeyValues(filename);
	Assert(kv);
	if (kv)
	{
		const char *mapname = STRING(gpGlobals->mapname);
		KeyValues *translation = NULL;

		if (kv->LoadFromFile(filesystem, filename))
		{
			KeyValues *pKeys = kv;
			while (pKeys)
			{
				if (pKeys->GetFirstSubKey())
				{
					char const *keyname = pKeys->GetName();

					if (!Q_stricmp(keyname, mapname))
					{
						translation = pKeys;
					}
					else
					{
						// TODO: Loop over "active" and "waiting"
						for (KeyValues *sub = pKeys->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
						{
							if (!Q_stricmp(sub->GetName(), "wave"))
							{
								// TODO
							}
							if (!Q_stricmp(sub->GetName(), "group"))
							{
								// TODO: Give a unique identifier for each group we encounter. So we can compare and match.
							}
						}
					}
				}
			}
		}
	}
}
#endif
