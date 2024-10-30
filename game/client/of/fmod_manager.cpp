#include "cbase.h"
#include "fmod_manager.h"
#include "of_sound_params.h"
#include "filesystem.h"

#define FOR_EACH_SOUND(pManager,pElement)																				\
for( int i = pManager->m_Sounds.First(); i != pManager->m_Sounds.InvalidIndex(); i = pManager->m_Sounds.Next(i) )		\
{																														\
	FOR_EACH_MAP( *pManager->m_Sounds[i], y )																			\
	{																													\
		if( pManager->m_Sounds[i]->IsValidIndex(y) )	 																\
		{																												\
			SoundData_t *pElement = pManager->m_Sounds[i]->Element(y);											

#define END_EACH_SOUND	\
		}				\
	}					\
}

using namespace FMOD;

// user configurable music volume
ConVar snd_announcervolume( "snd_announcervolume", "1.0", FCVAR_ARCHIVE, "Announcer volume", true, 0.0f, true, 1.0f );	

static const char *g_pVolumeConVars[OF_CHANNEL_COUNT] =
{
	"volume", // OF_CHANNEL_GAME
	"snd_musicvolume", // OF_CHANNEL_MUSIC
	"snd_announcervolume" // OF_CHANNEL_ANNOUNCER
};

System			*pSystem;
FMOD_RESULT		result;

#define OF_FMOD_COMMANDS

#ifdef OF_FMOD_COMMANDS
void ConPlayFmodSound(const CCommand &args)
{
	char szSound[1024];
	Q_strncpy(szSound, args.Arg(1),sizeof(szSound));
	FMODManager()->PlaySound(szSound);
}

static ConCommand fm_play("fm_play", ConPlayFmodSound, "Play a Sounscript File.", FCVAR_NONE);

void ConPlayLoopingFmodSound(const CCommand &args)
{
	char szSound[1024];
	Q_strncpy(szSound, args.Arg(1),sizeof(szSound));
	SoundData_t *pSound = FMODManager()->PlaySound(szSound);
	
	if( pSound )
		pSound->SetLooping(true);
}

static ConCommand fm_play_looping("fm_play_looping", ConPlayLoopingFmodSound, "Play a looping Sounscript File.", FCVAR_NONE);

void ConSetLoopingFmodSound(const CCommand &args)
{
	char szSound[1024];
	Q_strncpy(szSound, args.Arg(1),sizeof(szSound));
	bool bLoop = atoi(args.Arg(2)) > 0;
	FMODManager()->SetLoopSound(szSound, bLoop);
}

static ConCommand fm_set_looping("fm_set_looping", ConSetLoopingFmodSound, "Sets if a sound loops or not.", FCVAR_NONE);

void ConStopFmodSound(const CCommand &args)
{
	char szSound[1024];
	Q_strncpy(szSound, args.Arg(1), sizeof(szSound));
	FMODManager()->StopSound(szSound);
}

static ConCommand fm_stop("fm_stop", ConStopFmodSound, "Stop a Sounscript File.", FCVAR_NONE);

void ConEndFmodSound(const CCommand &args)
{
	char szSound[1024];
	Q_strncpy(szSound, args.Arg(1), sizeof(szSound));
	FMODManager()->ToEndSound(szSound);
}

static ConCommand fm_end("fm_end", ConEndFmodSound, "Send a Sounscript File to the outro.", FCVAR_NONE);

void CommSndStp( void )
{
	FMODManager()->StopAllSound();
}
static ConCommand fm_stop_all_sound( "fm_stop_all_sound", CommSndStp, "Stops every channel group", FCVAR_NONE );
#endif

// Handle Callbacks for intro/loop/outro
FMOD_RESULT F_CALLBACK SoundHandleCallback(FMOD_CHANNELCONTROL *channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void *commanddata1, void *commanddata2)
{
	if ( callbacktype == FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT )
	{
		// Get the sync point
		int iSyncIndex = (int)commanddata1;
		
		// Assume we only ever use channel control with this callback
		Channel *pChannel = (Channel*)channelcontrol;
		Sound *pSound;
		pChannel->getCurrentSound(&pSound);
		
		if( !pSound )
			return FMOD_OK;

		FMOD_SYNCPOINT *pPoint;

		pSound->getSyncPoint(iSyncIndex, &pPoint);

		char szTarget[128];
		unsigned int iOffset = 0;
		int iOffsetType = 0;
		
		pSound->getSyncPointInfo(pPoint, szTarget, sizeof(szTarget), &iOffset, iOffsetType);
		
		CCommand args;
		args.Tokenize(szTarget);
		char *szSoundscript = (char*) args[0];
		int iKey = atoi(args[1]);
		int iStage = atoi(args[2]);
		
		int iSoundscript = FMODManager()->m_Sounds.Find(szSoundscript);

		if( iSoundscript == FMODManager()->m_Sounds.InvalidIndex() )
			return FMOD_ERR_INVALID_PARAM;

		int iIndex = FMODManager()->m_Sounds[iSoundscript]->Find(iKey);

		if( 
			!FMODManager()->m_Sounds[iSoundscript]->IsValidIndex(iIndex) || 
			FMODManager()->m_Sounds[iSoundscript]->InvalidIndex() == iIndex )
			return FMOD_ERR_INVALID_PARAM;

		SoundData_t *pElement = FMODManager()->m_Sounds[iSoundscript]->Element(iIndex);

		switch( iStage )
		{
			// Intro
			case 0:
			// Transition to loop when you're done with the intro
			pElement->TransitionToLoop();
			break;

			// Loop
			case 1:
			// Check if we should transition to end
			pElement->TransitionToEnd();
			break;

			// Outro
			case 2:
			// Clear out all buffers and remove the song from our dictionary
			pElement->EndSound();
			break;
		}
	}
	return FMOD_OK;
}


SoundData_t::SoundData_t()
{
	pChannelGroupPtr = NULL;
	pChannelPtr = NULL;
	pIntroPtr = NULL;
	pBasePtr = NULL;
	pOutroPtr = NULL;
	
	pIntroBuf = NULL;
	pBaseBuf = NULL;
	pOutroBuf = NULL;

	pIntroFile = NULL;
	pBaseFile = NULL;
	pOutroFile = NULL;

	m_iIntroSize = -1;
	m_iBaseSize = -1;
	m_iOutroSize = -1;
	
	m_bIsLooping = false;
	
	m_nChannel = OF_CHANNEL_GAME;

	if( pSystem )
		pSystem->createChannelGroup("Parent", &pChannelGroupPtr);

	m_bResetWithRound = true;
}

SoundData_t::~SoundData_t()
{
	// We dont necessary have a start and end, so check first
	if( pIntroPtr )	pIntroPtr->release();
	if( pOutroPtr )	pOutroPtr->release();
	
	pChannelGroupPtr->stop();
	pChannelPtr->stop();

	pChannelGroupPtr->release();

	pBasePtr->release();

	if( pIntroBuf )	
		filesystem->FreeOptimalReadBuffer(pIntroBuf);

	if( pOutroBuf )	
		filesystem->FreeOptimalReadBuffer(pOutroBuf);

	filesystem->FreeOptimalReadBuffer(pBaseBuf);
};

bool SoundData_t::LoadSoundFiles( const char *szBaseWav, const char *szIntroWav, const char *szOutroWav )
{
	pBaseFile = szBaseWav;
	pBaseBuf = CFMODManager::GetRawSoundPointer(szBaseWav, &m_iBaseSize);

	if( !pBaseBuf )
		return false;

	if( szIntroWav )
	{
		pIntroFile = szIntroWav;
		pIntroBuf = CFMODManager::GetRawSoundPointer(szIntroWav, &m_iIntroSize);
		if( !pIntroBuf )
			return false;
	}

	if( szOutroWav )
	{
		pOutroFile = szOutroWav;
		pOutroBuf = CFMODManager::GetRawSoundPointer(szOutroWav, &m_iOutroSize);
		if( !pOutroBuf)
			return false;
	}

	return true;
}

bool SoundData_t::SetupStreams()
{
	// SoundEXInfo is required for the sound to know where the buffer ends
	FMOD_CREATESOUNDEXINFO infoLoop;
	memset(&infoLoop, 0, sizeof(infoLoop));
	infoLoop.length = m_iBaseSize;
	infoLoop.cbsize = sizeof(infoLoop);

	FMOD_RESULT res = FMOD_OK;

	res = pSystem->createStream( (const char *)pBaseBuf,
		FMOD_OPENMEMORY | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL,
		&infoLoop,
		&pBasePtr );

	if( res != FMOD_OK )
		return false;
		
	pBasePtr->setLoopPoints(0, FMOD_TIMEUNIT_PCM, CFMODManager::GetSoundLengthPCM(pBasePtr), FMOD_TIMEUNIT_PCM);

	// Don't loop by default
	pBasePtr->setLoopCount(0);

	// Setup Intro
	if( pIntroBuf )
	{
		FMOD_CREATESOUNDEXINFO infoIntro;
		memset(&infoIntro, 0, sizeof(infoIntro));
		infoIntro.length = m_iIntroSize;
		infoIntro.cbsize = sizeof(infoIntro);
		
		pSystem->createStream((const char *)pIntroBuf, FMOD_OPENMEMORY | FMOD_CREATESTREAM, &infoIntro, &pIntroPtr);	
	}

	if( res != FMOD_OK )
		return false;

	// Setup Outro
	if( pOutroBuf )
	{		
		FMOD_CREATESOUNDEXINFO infoOutro;
		memset(&infoOutro, 0, sizeof(infoOutro));
		infoOutro.length = m_iOutroSize;
		infoOutro.cbsize = sizeof(infoOutro);
		
		pSystem->createStream((const char *)pOutroBuf, FMOD_OPENMEMORY | FMOD_CREATESTREAM, &infoOutro, &pOutroPtr);
	}

	if( res != FMOD_OK )
		return false;

	return true;
}

bool SoundData_t::SetupSyncPoints()
{
	// Temp Pointer we just need to initialize a syncpoint
	FMOD_SYNCPOINT *ptr = NULL;
	FMOD_RESULT res = FMOD_OK;

	const char *szSoundName = FMODManager()->m_Sounds.GetElementName(m_iSoundscript);

	if( pIntroBuf )
	{
		res = pIntroPtr->addSyncPoint(CFMODManager::GetSoundLengthPCM(pIntroPtr),
			FMOD_TIMEUNIT_PCM,
			VarArgs("%s %d 0", szSoundName, m_iIndex), // 0 Stands for Intro, we use this to determine what to do in a callback
			&ptr);
	}

	if( res != FMOD_OK )
		return false;

	ptr = NULL;
	res = pBasePtr->addSyncPoint
	(
		CFMODManager::GetSoundLengthPCM(pBasePtr),
		FMOD_TIMEUNIT_PCM,
		VarArgs("%s %d 1", szSoundName, m_iIndex), // 1 Stands for LOOP, we use this to determine what to do in a callback
		&ptr
	);

	if( res != FMOD_OK )
		return false;
	
	if( pOutroBuf )
	{
		ptr = NULL;
		res = pOutroPtr->addSyncPoint(CFMODManager::GetSoundLengthPCM(pOutroPtr),
			FMOD_TIMEUNIT_PCM,
			VarArgs("%s %d 2", szSoundName, m_iIndex), // 2 Stands for Outro, we use this to determine what to do in a callback
			&ptr);
	}

	if( res != FMOD_OK )
		return false;

	return true;
}

bool SoundData_t::SetupCallback()
{
	return pChannelPtr->setCallback((FMOD_CHANNELCONTROL_CALLBACK)SoundHandleCallback) == FMOD_OK;
}

// Intro ended, start the mid point ( usually looping )
void SoundData_t::TransitionToLoop()
{
	pSystem->playSound(pBasePtr, pChannelGroupPtr, false, &pChannelPtr);
	pChannelPtr->setCallback((FMOD_CHANNELCONTROL_CALLBACK)SoundHandleCallback);
}

// Loop ended, transition to outro
// Loop ended, transition to outro
void SoundData_t::TransitionToEnd()
{
	if( m_bIsLooping )
		return;

	pBasePtr->release();
	pChannelPtr->stop();
		
	if( !pOutroPtr )
	{
		EndSound();
		return;
	}

	pSystem->playSound(pOutroPtr, pChannelGroupPtr, false, &pChannelPtr);
	pChannelPtr->setCallback((FMOD_CHANNELCONTROL_CALLBACK)SoundHandleCallback);
}

// Outro just ended, stop our sound and remove it from the manager
// Outro just ended, stop our sound and remove it from the manager
void SoundData_t::EndSound()
{
	FMODManager()->m_Sounds.Element(m_iSoundscript)->Remove(m_iIndex);
	if( FMODManager()->m_Sounds.Element(m_iSoundscript)->Count() <= 0 )
		FMODManager()->m_Sounds.RemoveAt(m_iSoundscript);

	delete this;
}

// Sets our sound to loop or not loop
// Sets our sound to loop or not loop
void SoundData_t::SetLooping( bool bLooping )
{
	m_bIsLooping = bLooping;
	
	pBasePtr->setLoopCount(bLooping ? -1 : 0);
}

// Sets the individual volume modifier of our sound ( later gets multiplied with the other settings )
// Sets the individual volume modifier of our sound ( later gets multiplied with the other settings )
void SoundData_t::SetVolume( float flVolume )
{
	m_flVolume = flVolume;
}

// Update the final volume based on our individual volume settings as well as the global volume modifier
// Update the final volume based on our individual volume settings as well as the global volume modifier
void SoundData_t::UpdateVolume()
{
	bool bMuted = false;
	pChannelGroupPtr->getMute(&bMuted);
	pChannelGroupPtr->setVolume(m_flVolume * FMODManager()->GetVolumeModifier( m_nChannel ));
	pChannelGroupPtr->setMute(bMuted);
}

CFMODManager gFMODMng;
CFMODManager* FMODManager()
{
	return &gFMODMng;
}

CFMODManager::CFMODManager()
{
	m_flVolume = 1.0f;
	m_bUnfocused = false;
}

CFMODManager::~CFMODManager()
{

}

FMOD_RESULT F_CALLBACK DebugCallback(
	FMOD_DEBUG_FLAGS flags,
	const char* file,
	int line,
	const char* func,
	const char* message
)
{
	char strMessage[1024];
	Q_snprintf( strMessage, sizeof( strMessage ), "FMOD Warning/Error:\n" );

	if( file )
		Q_snprintf(strMessage, sizeof(strMessage), "%sFile: %s\n", strMessage, file);

	if( func )
		Q_snprintf(strMessage, sizeof(strMessage), "%sIn function: %s\n", strMessage, func);

	Q_snprintf(strMessage, sizeof(strMessage), "%sAtLine: %d\n", strMessage, line);

	if( message )
		Q_snprintf(strMessage, sizeof(strMessage), "%sWith message: %s\n", strMessage, message);

	DevWarning( "%s\n", strMessage );

	return FMOD_OK;
}

// Starts FMOD
void CFMODManager::InitFMOD(void)
{
	result = System_Create(&pSystem); // Create the main system object.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System creation failed!\n");
	else
		DevMsg("FMOD system successfully created.\n");

	result = pSystem->init(100, FMOD_INIT_NORMAL, 0);   // Initialize FMOD system.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: Failed to initialize properly!\n");
	else
		DevMsg("FMOD initialized successfully.\n");

	FMOD_DEBUG_FLAGS flags = FMOD_DEBUG_LEVEL_ERROR | FMOD_DEBUG_DISPLAY_LINENUMBERS;

	Debug_Initialize( flags, FMOD_DEBUG_MODE_CALLBACK, &DebugCallback );

	// Listeners
	// Function events
	ListenForGameEvent( "teamplay_round_start" );
	// User called events
	ListenForGameEvent( "play_fmod_sound" );
	ListenForGameEvent( "stop_fmod_sound" );
}

// Stops FMOD
void CFMODManager::ExitFMOD(void)
{
	result = pSystem->release();

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System did not terminate properly!\n");
	else
		DevMsg("FMOD system terminated successfully.\n");
}

// Game event listener responses
void CFMODManager::FireGameEvent( IGameEvent *event )
{
	const char *name = event->GetName();

	// Stops all round based (non global) sound
	if( !Q_strcmp(name, "teamplay_round_start") )
	{
		if( event->GetBool( "full_reset" ) )
		{
			StopAllSound( true );
		}
	}
	// Event that can be called by a Programmer/Mapper to start playing a sound
	else if( !Q_strcmp(name, "play_fmod_sound") )
	{
		char *szSoundScript = (char*) event->GetString( "soundscript" );

		OF_SOUND_CHANNEL nChan = (OF_SOUND_CHANNEL)event->GetInt( "channel" );

		SoundData_t *pSound = FMODManager()->PlaySound( szSoundScript, nChan );
		pSound->SetLooping( event->GetBool( "looping" ) );

	}
	// Event that can be called by a Programmer/Mapper to stop playing a sound
	else if( !Q_strcmp(name, "stop_fmod_sound") )
	{
		char *szSoundScript = (char*)event->GetString( "soundscript" );
		if( !szSoundScript )
			return;

		FMODManager()->StopSound( szSoundScript );
		//event->GetBool( "set_not_loop" );
	}
}

// Called every frame
// Manages volume and mute
void CFMODManager::Think(void)
{
	const ConVar *snd_mute_losefocus = NULL;
	snd_mute_losefocus = g_pCVar->FindVar("snd_mute_losefocus");

	if( m_bUnfocused != engine->IsActiveApp() )
	{
		m_bUnfocused = engine->IsActiveApp();

		if( snd_mute_losefocus->GetBool() )
		{
			FOR_EACH_SOUND(this, pElement)
				pElement->pChannelGroupPtr->setMute( !engine->IsActiveApp() );
			END_EACH_SOUND
		}
	}

	UpdateSoundVolumes();

	pSystem->update();
}

/*
	Used for setting delay
	int  iOutputRate = 0;
	unsigned int iDspBufferSize = 0;
	result = pSystem->getSoftwareFormat(&iOutputRate, 0, 0);
	result = pSystem->getDSPBufferSize(&iDspBufferSize, 0);
	unsigned long long iClockTime = 0;
	float flFrequency = 0;
	unsigned int iSoundLenght = 0;
	
	result = pNewSound->pIntroPtr->getDefaults(&flFrequency, 0);
	
	result = pNewSound->pChannelPtr->getDSPClock(0, &iClockTime);

	// Current clock time
	iClockTime += (iDspBufferSize * 2);

	// 30000.0f is the amount needed to convert seconds to DSP
	float flDelay = 0.0f;
	iSoundLenght = (unsigned int)( flDelay * 30000.0f );
	iSoundLenght = (unsigned int)( (float)iSoundLenght / flFrequency * iOutputRate );
	// Set the clock to the end of the 
	iClockTime += iSoundLenght;
*/
bool CFMODManager::GetSoundscriptWavs( COFSoundParameters *pSoundscript, const char *&szBaseWav, const char *&szIntroWav, const char *&szOutroWav )
{
	// Don't play anything if we dont have a base wave, even if we have intros and outros
	szBaseWav = pSoundscript->GetRandomWav();

	// Remove start of file flags
	// String we use to format the song paths
	while( szBaseWav[0] == '#' || szBaseWav[0] == ')' )
	{
		szBaseWav++;
	}
	
	// I don't like doing this, however, 
	// for some reason, when there's no intro,
	// FMOD encounters an engine level crash
	// when you play a previously played sound
	// So,
	// HACK HACK: If no intro is set, set it to the Null sound
	COFSoundParameters *pStageScript = OFSoundManifest()->GetSoundscript(pSoundscript->GetIntro()[0] == '\0' ? "Null" : pSoundscript->GetIntro());
	if( !pStageScript )
		return false;
		
	szIntroWav = pStageScript->GetRandomWav();

	// Remove start of file flags
	// String we use to format the song paths
	while( szIntroWav[0] == '#' || szIntroWav[0] == ')' )
	{
		szIntroWav++;
	}
	// Note: This probably has something to do with memory allocation
	// Since this only happens when all instances of this sound stop playing
	// Playing the same sound before a previous one ended works without problem

	pStageScript = OFSoundManifest()->GetSoundscript( pSoundscript->GetOutro() );
	if( pStageScript )
	{
		szOutroWav = pStageScript->GetRandomWav();
		// Remove start of file flags
		// String we use to format the song paths
		while( szOutroWav[0] == '#' || szOutroWav[0] == ')' )
		{
			szOutroWav++;
		}
	}

	return true;
}

// Put a sound into our system and start playing it
// To make it loop, use StartLoop( "Soundscript Name", iArrayPos )
SoundData_t *CFMODManager::PlaySound( const char *szSoundName, OF_SOUND_CHANNEL nChan, float flDelay )
{
	// Assume the string parsed is valid
	COFSoundParameters *pSoundscript = OFSoundManifest()->GetSoundscript( szSoundName );
	if( !pSoundscript )
		// Soundscript does not exist :(
		return NULL;

	const char *szIntroWav = NULL;
	const char *szBaseWav = NULL;
	const char *szOutroWav = NULL;

	if( !GetSoundscriptWavs( pSoundscript, szBaseWav, szIntroWav, szOutroWav ) )
		return NULL;

	SoundData_t *pNewSound = new SoundData_t();

	if( !pNewSound->LoadSoundFiles( szBaseWav, szIntroWav, szOutroWav ) )
	{
		delete pNewSound;
		return NULL;
	}

	if( !pNewSound->SetupStreams() )
	{
		delete pNewSound;
		return NULL;
	}
	
	int iIndex = m_Sounds.Find(szSoundName);

	if( iIndex == m_Sounds.InvalidIndex() )
	{
		iIndex = m_Sounds.Insert(szSoundName);
		m_Sounds[iIndex] = (SOUNDSCRIPT_CONTAINER)new CUtlMap < int, SoundData_t* >;
		SetDefLessFunc( *m_Sounds[iIndex] );
	}
	
	static int g_iUniqueSoundID = 0;
	int iElementIndex = g_iUniqueSoundID;
	g_iUniqueSoundID++;

	m_Sounds[iIndex]->Insert(iElementIndex, pNewSound);

	// Set the Soundscript pos and Index
	pNewSound->m_iSoundscript = iIndex;
	pNewSound->m_iIndex = iElementIndex;

	if( !pNewSound->SetupSyncPoints() )
	{
		pNewSound->EndSound();
		return NULL;
	}

	FMOD_RESULT res = FMOD_OK;

	// Start the sound ( with the intro if we have one )
	if( pNewSound->pIntroPtr )
		res = pSystem->playSound(pNewSound->pIntroPtr, pNewSound->pChannelGroupPtr, false, &pNewSound->pChannelPtr);
	else
		res = pSystem->playSound(pNewSound->pBasePtr, pNewSound->pChannelGroupPtr, false, &pNewSound->pChannelPtr);

	if( res != FMOD_OK )
	{
		pNewSound->EndSound();
		return NULL;
	}

	if( !pNewSound->SetupCallback() )
	{
		pNewSound->EndSound();
		return NULL;
	}

	const ConVar *snd_mute_losefocus = NULL;
	snd_mute_losefocus = g_pCVar->FindVar("snd_mute_losefocus");

	pNewSound->SetVolume( pSoundscript ? pSoundscript->GetVolume().Random() : 1 );
	pNewSound->UpdateVolume();
	pNewSound->pChannelGroupPtr->setMute( !engine->IsActiveApp() && snd_mute_losefocus->GetBool() );

	pNewSound->m_nChannel = nChan;

	if( flDelay != 0.0f )
	{
		int  samplerate = 0;
		float freq = 0;

		unsigned int dsp_buffer_len = 0;
		unsigned long long clock_start = 0;

		// Info
		pSystem->getDSPBufferSize(&dsp_buffer_len, 0);

		pSystem->getSoftwareFormat(&samplerate, 0, 0);

		if( pNewSound->pIntroPtr )
			pNewSound->pIntroPtr->getDefaults(&freq, 0);
		else
			pNewSound->pBasePtr->getDefaults(&freq, 0);
		//

		pNewSound->pChannelGroupPtr->getDSPClock(0, &clock_start);

		// this is the start
		unsigned long long dsp_delay_time = clock_start + (dsp_buffer_len * 2);

		// Lenght of the delay, DSP is in Kilo Herz per second, flDelay is our seconds, sample rate is how much of it there is, go figure
		unsigned int slen = (unsigned int)(flDelay * samplerate);
		//slen = (unsigned int)((float)slen / freq * samplerate);
		// Add our delay to our base time
		dsp_delay_time += slen;

		// delay
		pNewSound->pChannelPtr->setDelay(dsp_delay_time, 0, false);
	}

	pNewSound->pChannelPtr->setPaused(false);
	
	return pNewSound;
}

// Enables/Disables looping of a given sound
void CFMODManager::SetLoopSound( const char *szSoundName, bool bLoop, int iIndex )
{
	SOUNDSCRIPT_CONTAINER soundscript = NULL;
	SoundData_t* pElement = NULL;

	switch( GetSoundInstance(&pElement, &soundscript, szSoundName, iIndex) )
	{
		case SOUND_INSTANCE_SOUND:
			pElement->SetLooping(bLoop);
		break;

		case SOUND_INSTANCE_SOUNDSCAPE:
			FOR_EACH_MAP( *soundscript, i )
			{
				(*soundscript)[i]->SetLooping(bLoop);
				(*soundscript)[i]->UpdateVolume();
			}
		break;
	}
}

// Sets the volume of a given sound
void CFMODManager::SetSoundVolume( float flVolume, char *szSoundName, int iIndex )
{
	SOUNDSCRIPT_CONTAINER soundscript = NULL;
	SoundData_t* pElement = NULL;

	switch( GetSoundInstance(&pElement, &soundscript, szSoundName, iIndex) )
	{
		case SOUND_INSTANCE_SOUND:
			pElement->SetVolume(flVolume);
			pElement->UpdateVolume();
		break;

		case SOUND_INSTANCE_SOUNDSCAPE:
			FOR_EACH_MAP( *soundscript, i )
			{
				(*soundscript)[i]->SetVolume(flVolume);
				(*soundscript)[i]->UpdateVolume();
			}
		break;
	}
}

//  Stops a given  sound
void CFMODManager::StopSound(const char *szSoundName, int iIndex)
{
	SOUNDSCRIPT_CONTAINER soundscript = NULL;
	SoundData_t* pElement = NULL;

	switch( GetSoundInstance(&pElement, &soundscript, szSoundName, iIndex) )
	{
		case SOUND_INSTANCE_SOUND:
			pElement->EndSound();
		break;

		case SOUND_INSTANCE_SOUNDSCAPE:
			FOR_EACH_MAP( *soundscript, i )
			{
				(*soundscript)[i]->EndSound();
			}
		break;
	}
}

// Transitions a given sound to its outro phase immediately
void CFMODManager::ToEndSound(const char *szSoundName, int iIndex)
{
	SOUNDSCRIPT_CONTAINER soundscript = NULL;
	SoundData_t* pElement = NULL;

	switch( GetSoundInstance( &pElement, &soundscript, szSoundName, iIndex ) )
	{
		case SOUND_INSTANCE_SOUND:
			pElement->TransitionToEnd();
		break;

		case SOUND_INSTANCE_SOUNDSCAPE:
			FOR_EACH_MAP( *soundscript, i )
			{
				(*soundscript)[i]->TransitionToEnd();
			}
		break;
	}
}

// Stops all currently playing sound
void CFMODManager::StopAllSound( bool bRound )
{
	for( unsigned short i = m_Sounds.First(); i != m_Sounds.InvalidIndex(); )
	{
		int iNext = m_Sounds.Next(i);

		int iIndex = m_Sounds[i]->FirstInorder();
		while( iIndex != m_Sounds[i]->InvalidIndex() )
		{
			SoundData_t *pElement = m_Sounds[i]->Element(iIndex);

			// Has to be before EndSound because end sound deletes the index
			iIndex = m_Sounds[i]->NextInorder(iIndex);

			if( !bRound || pElement->m_bResetWithRound )
				pElement->EndSound();
		}

		// Next is set manually with the value at the start in case EndSound removes the current m_Sounds entry
		i = iNext;
	}
//	m_Sounds.PurgeAndDeleteElements();
//	pChannelGroup->setPaused(true);
}

// Stops all currently playing announcer sounds
void CFMODManager::StopAllAnnouncerSounds()
{
	for( unsigned short i = m_Sounds.First(); i != m_Sounds.InvalidIndex(); )
	{
		int iNext = m_Sounds.Next(i);

		int iIndex = m_Sounds[i]->FirstInorder();
		while( iIndex != m_Sounds[i]->InvalidIndex() )
		{
			SoundData_t *pElement = m_Sounds[i]->Element(iIndex);
			iIndex = m_Sounds[i]->NextInorder(iIndex);
			if (pElement->m_nChannel == OF_CHANNEL_ANNOUNCER)
			{
				pElement->EndSound();
			}
		}
		i = iNext;
	}
}

// Get the volume modifier of a given channel, based on player cvars
float CFMODManager::GetVolumeModifier( OF_SOUND_CHANNEL nChan )
{
	static const ConVar *volume_cvar = NULL;
	volume_cvar = g_pCVar->FindVar(g_pVolumeConVars[nChan]);

	return volume_cvar->GetFloat();
}

// Updates the volumes of each sound
void CFMODManager::UpdateSoundVolumes()
{
	FOR_EACH_SOUND(this,pElement)
		pElement->UpdateVolume();
	END_EACH_SOUND
}

// Returns the lenght of a given sound in PCM, very accurate
// Used by PlaySound to setup perfect loops
unsigned int CFMODManager::GetSoundLengthPCM(Sound *sound)
{
	unsigned int templength = 0;
	sound->getLength(&templength, FMOD_TIMEUNIT_PCM);
	return templength - 1;
}

// Returns the lenght of a given SoundScript
// Use this if you need the sound lenght of something
unsigned int CFMODManager::GetSoundLength( const char *szSoundPath )
{	
	// Don't play anything if we dont have a base wave, even if we have intros and outros
	if( szSoundPath == NULL )
		return 0;

	// String we use to format the song paths
	char szSongName[1024];
	
	Q_strncpy(szSongName, szSoundPath, sizeof(szSongName));
	if( szSongName[0] == '#' || szSongName[0] == ')' )
	{
		memmove(&szSongName, &szSongName[1], strlen(szSongName));
	}
	
	// Buffer for 
	int iBuffSize = 0;
	byte *pBaseBuf = FMODManager()->GetRawSoundPointer(szSongName, &iBuffSize);

	Sound* pBasePtr;

	// SoundEXInfo is required for the sound to know where the buffer ends
	FMOD_CREATESOUNDEXINFO infoLoop;
	memset(&infoLoop, 0, sizeof(infoLoop));
	infoLoop.length = iBuffSize;
	infoLoop.cbsize = sizeof(infoLoop);
	
	pSystem->createStream( (const char *)pBaseBuf,
		FMOD_OPENMEMORY | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL,
		&infoLoop,
		&pBasePtr );
		
	unsigned int templength = 0;
	pBasePtr->getLength( &templength, FMOD_TIMEUNIT_MS );

	pBasePtr->release();
	filesystem->FreeOptimalReadBuffer(pBaseBuf);

	return templength - 1;
}

short CFMODManager::GetSoundInstance( SoundData_t **sound, SOUNDSCRIPT_CONTAINER *soundscripts, const char* szSoundName, int iIndex )
{
	unsigned short nSoundscript = m_Sounds.Find(szSoundName);
	if( nSoundscript == m_Sounds.InvalidIndex() )
		return 0;

	*soundscripts = m_Sounds.Element(nSoundscript);

	if( iIndex > -1 )
	{
		unsigned short nIndex = (*soundscripts)->Find(iIndex);
		if( nIndex == (*soundscripts)->InvalidIndex() )
			return 0;

		*sound = (*soundscripts)->Element(nIndex);
		return SOUND_INSTANCE_SOUND;
	}
	else
	{
		return SOUND_INSTANCE_SOUNDSCAPE;
	}

	return 0;
}

// Returns the raw sound data, required to be able to load sounds packed inside a vpk or bsp
// Dealocation handled by the SoundData_t class
byte *CFMODManager::GetRawSoundPointer(const char* pathToFileFromModFolder, int *pLength )
{
	char fullpath[512];
	Q_snprintf(fullpath, sizeof(fullpath), "sound/%s", pathToFileFromModFolder);

	void *buffer = NULL;

	int length = filesystem->ReadFileEx( fullpath, "GAME", &buffer, true, true );

	if ( pLength )
	{
		*pLength = length;
	}

	return static_cast<byte*>(buffer);
}

// DEPRECATED: Replaced by PlaySound
/*
void CFMODManager::PlayLoopingMusic( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, const char* pIntroMusic, float flDelay , bool fadeIn)
{
	Channel *pTempChannel;
	Channel *pTempLoopChannel;
	
	if ( !pNewChannelGroup )
		return;
	
	int  outputrate = 0;
	result = pSystem->getSoftwareFormat(&outputrate, 0, 0);
	unsigned int dsp_block_len = 0;
	result = pSystem->getDSPBufferSize(&dsp_block_len, 0);
	unsigned long long clock_start = 0;
	float freq = 0;
	unsigned int slen = 0;
	
	if( pIntroMusic )
	{
		Sound *pIntroSound = NULL;
		Sound *pLoopingSound = NULL;
		
		int iLengthIntro = 0;
		byte *vBufferIntro = GetRawSoundPointer(pIntroMusic, &iLengthIntro);
		
		FMOD_CREATESOUNDEXINFO infoIntro;
		memset(&infoIntro, 0, sizeof(infoIntro));
		infoIntro.length = iLengthIntro;
		infoIntro.cbsize = sizeof(infoIntro);
		
		result = pSystem->createStream((const char *)vBufferIntro, FMOD_OPENMEMORY | FMOD_CREATESTREAM , &infoIntro, &pIntroSound);
		pSystem->playSound(pIntroSound, pNewChannelGroup, true, &pTempChannel);

		result = pIntroSound->getDefaults(&freq, 0);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);

		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		clock_start += slen;
		
		result = pTempChannel->setDelay(clock_start, 0, false);
		result = pTempChannel->setPaused(false);

		int iLengthLoop = 0;
		byte *vBufferLoop = GetRawSoundPointer(pLoopingMusic, &iLengthLoop);
		
		FMOD_CREATESOUNDEXINFO infoLoop;
		memset(&infoLoop, 0, sizeof(infoLoop));
		infoLoop.length = iLengthLoop;
		infoLoop.cbsize = sizeof(infoLoop);

		result = pSystem->createStream((const char *)vBufferLoop, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, &infoLoop, &pLoopingSound);
		result = pLoopingSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLengthPCM(pLoopingSound), FMOD_TIMEUNIT_PCM);
		pSystem->playSound(pLoopingSound, pNewChannelGroup, true, &pTempLoopChannel);

		result = pIntroSound->getLength(&slen, FMOD_TIMEUNIT_PCM);
		result = pIntroSound->getDefaults(&freq, 0);

		slen = (unsigned int)((float)slen / freq * outputrate);
		clock_start += slen;

		result = pTempLoopChannel->setDelay(clock_start, 0, false);
		result = pTempLoopChannel->setPaused(false);
		pTempChannel->setChannelGroup(pNewChannelGroup);
		pTempLoopChannel->setChannelGroup(pNewChannelGroup);
	}
	else
	{
		int iLengthLoop = 0;
		byte *vBufferLoop = GetRawSoundPointer(pLoopingMusic, &iLengthLoop);
		
		FMOD_CREATESOUNDEXINFO infoLoop;
		memset(&infoLoop, 0, sizeof(infoLoop));
		infoLoop.length = iLengthLoop;
		infoLoop.cbsize = sizeof(infoLoop);

		Sound *pSound = NULL;

		result = pSystem->createStream((const char *)vBufferLoop, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM , &infoLoop, &pSound);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", pLoopingMusic, result);
			return;
		}
		result = pSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLengthPCM(pSound), FMOD_TIMEUNIT_PCM);
		result = pSystem->playSound(pSound, pNewChannelGroup, true, &pTempChannel);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);
		pSound->getDefaults(&freq, 0);
		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		clock_start += slen;

		result = pTempChannel->setDelay(clock_start, 0, false);
		
		pTempChannel->setPosition(0, FMOD_TIMEUNIT_PCM);
		pTempChannel->setPaused(false);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", pLoopingMusic, result);
			return;
		}
		pTempChannel->setChannelGroup(pNewChannelGroup);
	}
	//	result = pSound->setLoopCount( -1 );
	//	
	m_flSongStart = gpGlobals->realtime;
	currentSound = pLoopingMusic;
}
*/


// DEPRECATED: Now automatically done by the sound itself
/*
void CFMODManager::PlayMusicEnd( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, bool bDelay, Channel *pLoopingChannel )
{
	Channel *pTempChannel;
	
	if ( !pNewChannelGroup )
		return;
	
	if (pLoopingMusic)
	{
		Sound *pIntroSound = NULL;
		
		int iLength = 0;
		byte *vBuffer = GetRawSoundPointer(pLoopingMusic, &iLength);
		
		FMOD_CREATESOUNDEXINFO info;
		memset(&info, 0, sizeof(info));
		info.length = iLength;
		info.cbsize = sizeof(info);			
		
		result = pSystem->createStream((const char*)vBuffer, FMOD_CREATESTREAM | FMOD_OPENMEMORY, &info, &pIntroSound);
		pSystem->playSound(pIntroSound, pNewChannelGroup, true, &pTempChannel);
		
		result = pTempChannel->setPaused(false);
		pTempChannel->setChannelGroup(pNewChannelGroup);
}
*/