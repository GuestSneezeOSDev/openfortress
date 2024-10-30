#include "cbase.h"
#include "voice_command_menu_set.h"

CVoiceCommandMenuSet::CVoiceCommandMenuSet()
	// Could (should!) be an std::array<VoiceCommandVector, VoiceCommand::MaxMenusInSet>
	: m_Menus{ VoiceCommandVector{}, VoiceCommandVector{}, VoiceCommandVector{} }
{ 
	for (auto& menu : m_Menus)
	{
		menu.reserve(VoiceCommand::MaxMenuItems);
	}
};

size_t CVoiceCommandMenuSet::AddItem(size_t menuIndex, SVoiceCommand&& item)
{
	// Check the usual index/size constraints
	if (m_Menus.size() <= menuIndex)
	{
		return VoiceCommand::InvalidIndex;
	}

	VoiceCommandVector& menu = m_Menus.at(menuIndex);

	if (menu.size() >= VoiceCommand::MaxMenuItems)
	{
		return VoiceCommand::InvalidIndex;
	}

	// Finally, move the item into menu and update ID map if necessary
	menu.emplace_back(std::move(item));

	const auto itemIt = std::prev(menu.end());
	const auto idString = std::string(itemIt->m_GlobalIdentifier);

	const size_t itemIndex = std::distance(menu.begin(), itemIt);

	if (!idString.empty())
	{
		m_ItemIdBinds[idString] = SVoiceCommandMenuPosition{ menuIndex, itemIndex };
	}

	return itemIndex;
}

const CVoiceCommandMenuSet::VoiceCommandVector* CVoiceCommandMenuSet::GetMenu(size_t menuIndex) const
{
	return m_Menus.size() > menuIndex ? &m_Menus[menuIndex] : nullptr;
}

const SVoiceCommand* CVoiceCommandMenuSet::GetItem(size_t menuIndex, size_t itemIndex) const
{
	const auto* menu = GetMenu(menuIndex);
	if (!menu)
	{
		return nullptr;
	}

	return (menu->size() > itemIndex) ? &(menu->at(itemIndex)) : nullptr;
}

const SVoiceCommand* CVoiceCommandMenuSet::GetItem(const std::string& idString) const
{
	const auto itemIt = m_ItemIdBinds.find(idString);
	if (itemIt == m_ItemIdBinds.end())
	{
		return nullptr;
	}

	const auto posIt = itemIt->second;

	return &(m_Menus[posIt.m_MenuIndex][posIt.m_ItemIndex]);
}

SVoiceCommandMenuPosition CVoiceCommandMenuSet::GetItemPosition(const std::string& idString) const
{
	const auto itemIt = m_ItemIdBinds.find(idString);
	if (itemIt == m_ItemIdBinds.end())
	{
		return SVoiceCommandMenuPosition{};
	}

	return itemIt->second;
}

bool CVoiceCommandMenuSet::IsEmpty() const
{
	return std::all_of(
		m_Menus.begin(), 
		m_Menus.end(), 
		[](const auto& menu) { return menu.empty(); }
	);
}

bool CVoiceCommandMenuSet::IsEmpty(size_t menuIndex) const
{
	const auto* menu = GetMenu(menuIndex);

	if (menu)
	{
		return menu->empty();
	}
	
	return true;
}
