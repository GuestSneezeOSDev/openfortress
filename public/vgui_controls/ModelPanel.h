//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ANIMCONTMODELPANEL_H
#define ANIMCONTMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include "tier1/interface.h"
#include "KeyValues.h"

namespace vgui
{
	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	abstract_class AnimContModelPanel : public IBaseInterface
	{
		public:
			
			virtual void SetHUDModelPos(float x, float y, float z) = 0;
			virtual void GetHUDModelPos(float &x, float &y, float &z) = 0;
			
			virtual void SetHUDModelAng(float x, float y, float z) = 0;
			virtual void GetHUDModelAng(float &x, float &y, float &z) = 0;

			virtual void SetHUDModelAnimPos(float x, float y, float z) = 0;
			virtual void GetHUDModelAnimPos(float& x, float& y, float& z) = 0;

			virtual void SetHUDModelAnimAng(float x, float y, float z) = 0;
			virtual void GetHUDModelAnimAng(float &x, float &y, float &z) = 0;
			
			virtual void ResetAnim() = 0;
	};
}
#endif // ANIMCONTMODELPANEL_H
