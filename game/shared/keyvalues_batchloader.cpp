#include "cbase.h"
#include "keyvalues_batchloader.h"

#include "filesystem.h"
#include "strtools.h"
#include "dbg.h"

namespace
{
	inline std::string ToFullPath(const std::string& basePath, const std::string& filename)
	{
		// Combines base path string and filename, then fixes the slashes to use platform-correct format
		std::string fullPath = {};
		
		// Combine the strings properly
		if (basePath.empty())
		{
			fullPath = filename;
		}
		else
		{
			if (basePath.back() == CORRECT_PATH_SEPARATOR || basePath.back() == INCORRECT_PATH_SEPARATOR)
			{
				fullPath = basePath + filename;
			}
			else
			{
				fullPath = fmt::format(
					FMT_STRING("{}{}{}")
					, basePath
					, CORRECT_PATH_SEPARATOR
					, filename
				);
			}
		}

		// Finally, fix the slashes
		std::replace(fullPath.begin(), fullPath.end(), INCORRECT_PATH_SEPARATOR, CORRECT_PATH_SEPARATOR);

		return fullPath;
	}

	inline std::string ExtractBasePathFromFilter(const std::string& filterString)
	{
		if (filterString.empty())
		{
			return std::string();
		}
		
		const size_t folderPosWin 
			= filterString.find_last_of(CORRECT_PATH_SEPARATOR);
		const size_t folderPosPosix 
			= filterString.find_last_of(INCORRECT_PATH_SEPARATOR);

		// That's assuming user code won't mix two together :^)
		const size_t folderPos = min(folderPosWin, folderPosPosix);

		return filterString.substr(0, folderPos);
	}
}

bool CKeyValuesBatchLoader::LoadFromResources(IFileSystem* fileSystem, const std::vector<Path>& resourcePaths, CKeyValuesBatchLoader::LoadPolicy loadPolicy)
{
	if (!fileSystem)
	{
		Warning("CKeyValuesBatchLoader::LoadFromResources() - filesystem not found!");
		return false;
	}

	ResetKeyValues();

	// Loads KV pairs from files, one by one
	for (const auto& pathIt : resourcePaths)
	{
		auto kvalues = KeyValuesUtils::Pointer(new KeyValues(pathIt.m_ResourceName.c_str()));
		bool isLoaded = kvalues->LoadFromFile(fileSystem, pathIt.m_ResourceName.c_str(), pathIt.m_PathId.c_str());
		
		if (isLoaded)
		{
			m_KeyValues.emplace(pathIt.m_ResourceName, std::move(kvalues));
		}
		else if (loadPolicy == LoadPolicy::Strict)
		{
			Warning(
				"CKeyValuesBatchLoader::LoadFromResources() - could not load key values from file %s, aborting"
				, pathIt.m_ResourceName.c_str()
			);
			
			ResetKeyValues();
			return false;
		}
		else
		{
			Warning(
				"CKeyValuesBatchLoader::LoadFromResources() - could not load key values from file %s, continuing"
				, pathIt.m_ResourceName.c_str()
			);
		}
	}

	return true;
}

bool CKeyValuesBatchLoader::LoadFromResources(IFileSystem* fileSystem, const std::string& resourceNameFilter, const std::string& resourcePathId, CKeyValuesBatchLoader::LoadPolicy loadPolicy)
{
	if (!fileSystem)
	{
		Warning("CKeyValuesBatchLoader::LoadFromResources() - filesystem not found!");
		return false;
	}

	const std::string basePath = ExtractBasePathFromFilter(resourceNameFilter);

	std::vector<Path> foundPaths = {};
	std::string foundFileName = {};

	// Start the search
	FileFindHandle_t findHandle;
	if (resourcePathId.empty())
	{
		foundFileName = fileSystem->FindFirst(resourceNameFilter.c_str(), &findHandle);
	}
	else
	{
		foundFileName = fileSystem->FindFirstEx(resourceNameFilter.c_str(), resourcePathId.c_str(), &findHandle);
	}

	// Build paths vector until filesystem runs out of matches 
	while (!foundFileName.empty())
	{
		foundPaths.push_back(Path{ ToFullPath(basePath, foundFileName), resourcePathId });

		const char* name = fileSystem->FindNext(findHandle);
		foundFileName = name ? name : "";
	}

	// Early out if no resources match
	if (foundPaths.empty())
	{
		Warning(
			"CKeyValuesBatchLoader::LoadFromResources() - no resources found with wildcard %s, path ID %s"
			, resourceNameFilter.c_str()
			, resourcePathId.c_str()
		);

		return false;
	}

	// Hand over processing to primary loader logic
	return LoadFromResources(fileSystem, foundPaths, loadPolicy);
}

const CKeyValuesBatchLoader::KeyValuesMap& CKeyValuesBatchLoader::GetKeyValues() const
{
	return m_KeyValues;
}

void CKeyValuesBatchLoader::ResetKeyValues()
{
	m_KeyValues.clear();
}
