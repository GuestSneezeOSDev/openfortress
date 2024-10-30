//=============================================================================//
//
// Purpose: Holds voice command definitions defined in game scripts
//	Also responsible for loading voice commands on game launch
//
//=============================================================================//


#ifndef VOICE_COMMAND_REGISTRY_H
#define VOICE_COMMAND_REGISTRY_H

#ifdef _WIN32
#pragma once
#endif

#include "keyvalues_utils.h"
#include "voice_command.h"
#include "voice_command_menu_set.h"

class CVoiceCommandRegistry
{
public:
	CVoiceCommandRegistry(const std::string& scriptPath);
	virtual ~CVoiceCommandRegistry() = default;

	void Initialize();
	void Shutdown();

	const CVoiceCommandMenuSet& GetMenu(int gametypeField) const;

protected:
	virtual std::string ToMenuSetName(int gametypeField) const;

private:
	CVoiceCommandMenuSet ParseMenuSet(const std::string& menuSetPath, const KeyValuesUtils::Pointer& menuSetData) const;
	SVoiceCommand        ParseItem(KeyValues* menuItem) const;

private:
	const std::string m_VoiceCommandScriptsPath;

	std::unordered_map<std::string, CVoiceCommandMenuSet> m_LoadedMenuSets;
};

#endif // VOICE_COMMAND_REGISTRY_H 