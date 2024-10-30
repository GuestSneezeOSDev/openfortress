//=============================================================================//
//
// Purpose: Extends CVoiceCommandRegistry by introducing OF gamemode-specific menu set selection
//
//=============================================================================//


#ifndef OF_VOICE_COMMAND_REGISTRY_H
#define OF_VOICE_COMMAND_REGISTRY_H

#ifdef _WIN32
#pragma once
#endif

#include "voice_command_registry.h"

class COFVoiceCommandRegistry : public CVoiceCommandRegistry
{
public:
	COFVoiceCommandRegistry();
	~COFVoiceCommandRegistry() = default;

protected:
	std::string ToMenuSetName(int gametypeField) const override;
};

#endif // OF_VOICE_COMMAND_REGISTRY_H 