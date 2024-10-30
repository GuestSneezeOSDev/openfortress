//=============================================================================//
//
// Purpose: Streamlines batch loading KeyValues from multiple resource instances
//
//=============================================================================//


#ifndef KEYVALUES_BATCH_LOADER_H
#define KEYVALUES_BATCH_LOADER_H

#ifdef _WIN32
#pragma once
#endif

#include "keyvalues_utils.h"
#include <inc_cpp_stdlib.h>

class CKeyValuesBatchLoader
{
public:
	using KeyValuesMap = std::map<std::string, KeyValuesUtils::Pointer>;

	struct Path
	{
		std::string m_ResourceName;
		std::string m_PathId;
	};

	enum class LoadPolicy
	{
		// Result is valid only when all resources were successfully loaded
		Strict,

		// Result is valid when at least one resource was successfully loaded
		Permissive
	};

public:
	CKeyValuesBatchLoader() = default;
	~CKeyValuesBatchLoader() = default;

	// Attempts to load KeyValues from resources provided in vecResourcePaths
	// Returns true on full success, false on failure
	bool LoadFromResources(IFileSystem* pFileSystem, const std::vector<Path>& vecResourcePaths, LoadPolicy ePolicy = LoadPolicy::Strict);

	// Attempts to load KeyValues from resources that match strResourceNameFilter
	// Returns true on full success, false on failure
	bool LoadFromResources(IFileSystem* pFileSystem, const std::string& strResourceNameFilter, const std::string& strResourcePathId = std::string(), LoadPolicy ePolicy = LoadPolicy::Strict);

	// Returns loaded KeyValues map
	// Maps as filename - KeyValues instance
	const KeyValuesMap& GetKeyValues() const;

	// Resets loaded KeyValues map
	void ResetKeyValues();

private:
	KeyValuesMap m_KeyValues;
};

#endif // KEYVALUES_BATCH_LOADER_H 