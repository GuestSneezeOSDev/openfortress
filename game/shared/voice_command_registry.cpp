#include "cbase.h"
#include "voice_command_registry.h"

#include "mp_shareddefs.h"
#include "keyvalues_batchloader.h"


namespace
{
	inline bool IsMenuItemValid(const SVoiceCommand& menuItem)
	{
		// Valid if concept is recognized and menu label is defined
#ifdef CLIENT_DLL
		return !std::string(menuItem.m_MenuLabel).empty();
#else
		return menuItem.m_ConceptId != MP_CONCEPT_NONE;
#endif // CLIENT_DLL
	}

	inline bool IsWildcardPath(const std::string& filename)
	{
		return filename.find_first_of('*') != std::string::npos;
	}

	inline std::string ExtractFilename(const std::string& fullPath)
	{
		if (fullPath.empty())
		{
			return std::string("");
		}
		
		const size_t nameStartWin
			= fullPath.find_last_of(CORRECT_PATH_SEPARATOR);
		const size_t nameStartPosix
			= fullPath.find_last_of(INCORRECT_PATH_SEPARATOR);

		// Expected filename is scripts/voicecommands*.txt, drop everything else
		const size_t nameStart
			= min(nameStartWin, nameStartPosix);
		const size_t nameEnd 
			= fullPath.find_last_of('.');

		if (nameStart == std::string::npos || (nameStart + 1) >= fullPath.size())
		{
			return std::string("");
		}

		// Rest of the string will be selected if nameEnd == npos, even with offset
		return fullPath.substr(nameStart + 1, nameEnd - (nameStart + 1));
	}
}

CVoiceCommandRegistry::CVoiceCommandRegistry(const std::string& scriptPath)
	: m_VoiceCommandScriptsPath(scriptPath)
{
}

void CVoiceCommandRegistry::Initialize()
{
	// Gamerules get reloaded every time client joins a server
	CKeyValuesBatchLoader loader = {};
	bool loadSucceeded = false;

	// Choose the proper loading mode in accordance to passed script path
	if (IsWildcardPath(m_VoiceCommandScriptsPath))
	{
		loadSucceeded = 
			loader.LoadFromResources(
				filesystem
				, m_VoiceCommandScriptsPath
				, "GAME"
				, CKeyValuesBatchLoader::LoadPolicy::Permissive
			);
	}
	else
	{
		loadSucceeded =
			loader.LoadFromResources(
				filesystem
				, std::vector<CKeyValuesBatchLoader::Path>{ CKeyValuesBatchLoader::Path{ m_VoiceCommandScriptsPath, "GAME" } }
				, CKeyValuesBatchLoader::LoadPolicy::Permissive
			);
	}

	if (!loadSucceeded)
	{
		Warning("CVoiceCommandRegistry::Initialize - Could not load voice command scripts, aborting");
		return;
	}

	// Parse loaded voice commands
	const auto& commandDefs = loader.GetKeyValues();
	for (const auto& def : commandDefs)
	{
		const auto& menuSetPath = def.first;
		const auto& menuSetData = def.second;

		const auto menuSetName = ExtractFilename(menuSetPath);
		if (menuSetName.empty())
		{
			Warning("CVoiceCommandRegistry::Initialize - Unexpected filename format for %s, skipping", menuSetPath.c_str());
			continue;
		}

		CVoiceCommandMenuSet parsedMenuSet = ParseMenuSet(menuSetPath, menuSetData);

		if (parsedMenuSet.IsEmpty())
		{
			Warning("CVoiceCommandRegistry::Initialize - Could not load menu set from %s, skipping", menuSetPath.c_str());
			continue;
		}

		m_LoadedMenuSets[menuSetName] = std::move(parsedMenuSet);
	}
}

void CVoiceCommandRegistry::Shutdown()
{
	// Placeholder
	m_LoadedMenuSets.clear();
}

const CVoiceCommandMenuSet& CVoiceCommandRegistry::GetMenu(int gametypeField) const
{
	auto setIt = m_LoadedMenuSets.find(ToMenuSetName(gametypeField));
	if (setIt == m_LoadedMenuSets.end())
	{
		AssertMsg(false, "Trying to fetch voice commands for unknown gametype 0x%.8X", gametypeField);
	}

	return setIt->second;
}

std::string CVoiceCommandRegistry::ToMenuSetName(int gametypeField) const
{
	// Must be extended by implementation if new menu sets are added
	return std::string("voicecommands");
}

CVoiceCommandMenuSet CVoiceCommandRegistry::ParseMenuSet(const std::string& menuSetPath, const KeyValuesUtils::Pointer& menuSetData) const
{
	// KeyValues should *really* be replaced by something else that preferably supports STL iteration
	auto       menuSet = CVoiceCommandMenuSet();
	size_t     menuIndex = 0;
	KeyValues* loadedMenu = menuSetData->GetFirstSubKey();

	// Iterate over menus
	while (loadedMenu)
	{
		size_t     itemCount = 0;
		KeyValues* loadedMenuItem = loadedMenu->GetFirstSubKey();

		if (menuIndex >= VoiceCommand::MaxMenusInSet)
		{
			Warning(
				"CVoiceCommandRegistry::ParseMenuSet - Trying to load more than %u menus in %s, extras ignored"
				, VoiceCommand::MaxMenusInSet
				, menuSetPath.c_str()
			);

			break;
		}

		// Iterate over first MaxSubmenuItems in current menu
		while (loadedMenuItem && itemCount <= VoiceCommand::MaxMenuItems)
		{
			auto parsedMenuItem = ParseItem(loadedMenuItem);

			if (IsMenuItemValid(parsedMenuItem))
			{
				menuSet.AddItem(menuIndex, std::move(parsedMenuItem));
			}
			else
			{
				Warning(
					"CVoiceCommandRegistry::ParseMenuSet - Trying to load invalid menu item %s in %s/%s, skipping"
					, loadedMenuItem->GetName()
					, menuSetPath.c_str()
					, loadedMenu->GetName()
				);
			}

			++itemCount;
			loadedMenuItem = loadedMenuItem->GetNextKey();

			if (itemCount > VoiceCommand::MaxMenuItems)
			{
				Warning(
					"CVoiceCommandRegistry::ParseMenuSet - Trying to load more than %u menu items in %s/%s, extras ignored"
					, VoiceCommand::MaxMenuItems
					, menuSetPath.c_str()
					, loadedMenu->GetName()
				);
			}
		}

		++menuIndex;
		loadedMenu = loadedMenu->GetNextKey();
	}

	return menuSet;
}

SVoiceCommand CVoiceCommandRegistry::ParseItem(KeyValues* menuItem) const
{
	auto parsedCommand = SVoiceCommand{};

	Q_snprintf(parsedCommand.m_GlobalIdentifier, VoiceCommand::MaxGlobalIdentifierLength, "%s", menuItem->GetString("globalidentifier", ""));

#ifdef CLIENT_DLL
	Q_snprintf(parsedCommand.m_Subtitle, VoiceCommand::MaxSubtitleLength, "%s", menuItem->GetString("subtitle", ""));
	Q_snprintf(parsedCommand.m_MenuLabel, VoiceCommand::MaxSpeakConceptLength, "%s", menuItem->GetString("menu_label", ""));
#else
	std::string conceptString = menuItem->GetString("concept", "");
	int conceptId = GetMPConceptIndexFromString(conceptString.c_str());

	if (conceptId == MP_CONCEPT_NONE)
	{
		Warning(
			"CVoiceCommandRegistry::ParseItem - Item %s attempting to use unknown concept %s, concept needs to be defined in code"
			, menuItem->GetName()
			, conceptString.c_str()
		);
	}

	parsedCommand.m_ConceptId = conceptId;

	parsedCommand.m_ShowsSubtitle = (menuItem->GetInt("show_subtitle", 0) > 0);
	parsedCommand.m_IsSubtitleDistanceBased = (menuItem->GetInt("distance_check_subtitle", 0) > 0);

	Q_snprintf(parsedCommand.m_GestureActivity, VoiceCommand::MaxGestureActivityLength, "%s", menuItem->GetString("activity", ""));
#endif // CLIENT_DLL

	return parsedCommand;
}
