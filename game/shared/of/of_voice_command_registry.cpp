#include "cbase.h"
#include "of_voice_command_registry.h"

#include "tf_shareddefs.h"

namespace
{
	constexpr int Tf2Gametypes = 0x00
		| 1 << TF_GAMETYPE_CTF
		| 1 << TF_GAMETYPE_CP
		| 1 << TF_GAMETYPE_PAYLOAD
		| 1 << TF_GAMETYPE_ARENA
		| 1 << TF_GAMETYPE_MVM
		| 1 << TF_GAMETYPE_RD
		| 1 << TF_GAMETYPE_PASSTIME
		| 1 << TF_GAMETYPE_PD;

	inline bool IsInGametype(int gametypeBitfield, int gametypeId)
	{
		return (gametypeBitfield & (1 << gametypeId)) != 0;
	}

	inline bool IsInAtLeastOneGametype(int gametypeBitfield, int gametypesMask)
	{
		return (gametypeBitfield & gametypesMask) != 0;
	}
}

COFVoiceCommandRegistry::COFVoiceCommandRegistry()
	: CVoiceCommandRegistry("scripts/voicecommands_*.txt") // OF has multiple separate voicecommand definitions
{
}

std::string COFVoiceCommandRegistry::ToMenuSetName(int gametypeField) const
{
	// We don't expect TF2 gametype pool to grow much, so OF types are considered as everything else
	const int OfGametypes = ~Tf2Gametypes;

	// OF gametype can have both OF- and TF2-specific bits, so we check for OF stuff first
	if (IsInAtLeastOneGametype(gametypeField, OfGametypes))
	{
		// Can be set by map or cvar switch
		if (IsInGametype(gametypeField, TF_GAMETYPE_TDM))
		{
			return std::string("voicecommands_of_team");
		}
		
		return std::string("voicecommands_of_ffa");
	}
	
	// Otherwise we're definitely in TF mode
	return std::string("voicecommands_tf");
}
