//=============================================================================//
//
// Purpose: Contains voice command definition loaded from script
//	Used by GameRules object to emit voiceline and play player gesture if applicable
//
//=============================================================================//

#ifndef VOICE_COMMAND_H
#define VOICE_COMMAND_H

#ifdef _WIN32
#pragma once
#endif


namespace VoiceCommand
{
	constexpr size_t MaxGlobalIdentifierLength = 64;
	constexpr size_t MaxSubtitleLength = 256;
	constexpr size_t MaxSpeakConceptLength = 64;
	constexpr size_t MaxGestureActivityLength = 64;
} // namespace VoiceCommand

struct SVoiceCommand
{
	// global string that binds the same voice command across different menu versions
	char m_GlobalIdentifier[VoiceCommand::MaxGlobalIdentifierLength];

#ifdef CLIENT_DLL
	// localizable subtitle
	char m_Subtitle[VoiceCommand::MaxSubtitleLength];

	// localizable string for menu
	char m_MenuLabel[VoiceCommand::MaxSpeakConceptLength];
#else
	// concept to speak
	int	 m_ConceptId;

	// play subtitle?
	bool m_ShowsSubtitle;
	bool m_IsSubtitleDistanceBased;

	char m_GestureActivity[VoiceCommand::MaxGestureActivityLength];
#endif // CLIENT_DLL
};

#endif // VOICE_COMMAND_H