//=============================================================================//
//
// Purpose: Contains convenience definition of voice command menu
//	Mostly used by GameRules and VoiceCommandRegistry objects
//
//=============================================================================//

#ifndef VOICE_COMMAND_MENU_SET_H
#define VOICE_COMMAND_MENU_SET_H

#ifdef _WIN32
#pragma once
#endif

#include "voice_command.h"
#include <inc_cpp_stdlib.h>

namespace VoiceCommand
{
	constexpr size_t MaxMenusInSet = 3;     // same as in tf2, Z/X/C keys menus
	constexpr size_t MaxMenuItems = 9;
	constexpr size_t InvalidIndex = static_cast<size_t>(-1);
} // namespace VoiceCommand

struct SVoiceCommandMenuPosition
{
	inline bool IsValid() const
	{
		return m_MenuIndex != VoiceCommand::InvalidIndex && m_ItemIndex != VoiceCommand::InvalidIndex;
	}

	size_t m_MenuIndex = VoiceCommand::InvalidIndex;
	size_t m_ItemIndex = VoiceCommand::InvalidIndex;
};

class CVoiceCommandMenuSet
{
public:
	using VoiceCommandVector = std::vector<SVoiceCommand>;

public:
	CVoiceCommandMenuSet();
	~CVoiceCommandMenuSet() = default;

	size_t AddItem(size_t menuIndex, SVoiceCommand&& item);

	const VoiceCommandVector* GetMenu(size_t menuIndex) const;

	const SVoiceCommand* GetItem(size_t menuIndex, size_t itemIndex) const;
	const SVoiceCommand* GetItem(const std::string& idString) const;

	SVoiceCommandMenuPosition GetItemPosition(const std::string& idString) const;

	bool IsEmpty() const;
	bool IsEmpty(size_t menuIndex) const;

private:
	std::vector<VoiceCommandVector> m_Menus;
	std::unordered_map<std::string, SVoiceCommandMenuPosition> m_ItemIdBinds;
};

#endif // VOICE_COMMAND_MENU_SET_H