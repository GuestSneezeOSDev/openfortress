//=============================================================================//
//
// Purpose: Utilities and helpers for KeyValues use, mostly with STD types
//
//=============================================================================//

#ifndef KEYVALUES_UTILS_H
#define KEYVALUES_UTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include <inc_cpp_stdlib.h>

namespace KeyValuesUtils
{
	class Deleter
	{
	public:
		// Default destructor is privated, so it needs to be called manually
		inline void operator()(KeyValues* ptr) const { ptr->deleteThis(); }
	};

	using Pointer = std::unique_ptr<KeyValues, KeyValuesUtils::Deleter>;
}

#endif // KEYVALUES_UTILS_H