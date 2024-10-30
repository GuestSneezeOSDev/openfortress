// enabled by default on linux
#ifdef _WIN32
    #pragma once
#endif

// ---> HEY HEY LOOK AT ME <----
// READ THIS SHIT OK
//      if you enable dbging, YOU ARE EXPOSING ANTICHEAT FUNCTIONS UNOBSFUCATED TO CLIENTS
//      this is very obviously ONLY FOR DEBUGGING
//      -----> DO NOT SHIP ANY BINS TO ANYONE WITH THIS DEFINED EVER UNDER ANY CIRCUMSTANCES PLEASE <-----
//      -----> NOT EVEN OVER DISCORD, NOT EVER *EVER* IN SVN <-----
//      -----> COMPILE IT YOURSELF IF YOU ABSOLUTELY NEED TO <-----
//
//      Please.
// -sappho
////////// #define dbging yep //////////

#ifdef dbging
    #define goodcolor   Color(90, 240, 90, 255) // green
    #define okcolor     Color(246, 190, 0, 255) // yellow
#endif

#include <cbase.h>

#include "tier0/valve_minmax_off.h"

#include "hooks.h"

#include "tier0/valve_minmax_on.h"

// This is needed so that CAutoGameSystem knows that we're using it, apparently
CHooks g_CHooks;


#include "obfuscate.h"

#include <ac/ac_utils.h>
#include <icommandline.h>
#include <iconvar.h>

void* cmdline_ENCODED = nullptr;

// set this client as psudeo vac banned, client will have to reinstall the game to get "unbanned"
void CHooks::setofi()
{
    ConVar* ofi = (ConVar*)cvar->FindVar("ofi");
    ofi->SetValue("1");
    CHooks::setinsecure();
}

// turn on insecure if ofi == 1, this gets called whenever ofi changes, including on startup too!
void oficb(IConVar* icvar, const char* pOldValue, float flOldValue)
{
    ConVarRef cvr(icvar);
    if ( cvr.GetBool() )
    {
        CHooks::setinsecure();
    }
}

// set client in insecure mode for this session
void CHooks::setinsecure()
{
    ICommandLine* cmdline = reinterpret_cast<ICommandLine*>(xorptr(cmdline_ENCODED));

    // TODOTODO:
    // DETOUR TIER0 REMOVEPARM TO PREVENT REMOVING INSECURE/TEXTMODE
    if ( !cmdline->CheckParm( AY_OBFUSCATE("-insecure") ) )
    {
        cmdline->AppendParm( AY_OBFUSCATE("-insecure"), AY_OBFUSCATE("") );
    }
}

ConVar ofi("ofi", "0", FCVAR_HIDDEN | FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "", oficb);

// This is "" so that we don't have an id-able string
CHooks::CHooks() : CAutoGameSystem("")
{
    cmdline_ENCODED = xorptr(CommandLine());
    return;
}


CFuncList EngineFuncs[] =
{
    #ifdef _WIN32
        // detoured, don't touch 
        //{
        //    // CNetChan::Shutdown
        //    //
        //    // String: "NetChannel removed."
        //    //
        //    // sub_101C7880 + 3   push    offset aNetchannelRemo ; "NetChannel removed."
        //    // sub_101C7880 + 8   mov     dword ptr[edi], offset ? ? _7CNetChan@@6B@; const CNetChan::`vftable'
        //    // sub_101C7880 + E   call  -->sub_101CCA10<--
        //    //
        //    // Signature for sub_101CCA10:
        //    // 55 8B EC 83 EC 10 56 8B F1 83 BE 8C 00 00 00 00
        //    AY_OBFUSCATE("\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\x83\xBE\x8C\x00\x00\x00\x00"),
        //    16
        //},
        {
            // CNetChan::SendNetMsg
            //
            // At LEAST used for spoofing m_nFriendsID, if nothing else. Probably more.
            // 
            // String: "NetMsg"
            // 
            // Signature for sub_101CBB30:
            // 55 8B EC 57 8B F9 8D 8F 98 00 00 00 
            AY_OBFUSCATE("\x55\x8B\xEC\x57\x8B\xF9\x8D\x8F\x98\x00\x00\x00"),
            12
        },
        {
            // ::CL_Move
            //
            // String: "CL_Move" followed by "%s"
            //
            // Signature for sub_100BC1E0:
            // 55 8B EC 83 EC 34 83 3D ? ? ? ? 02
            AY_OBFUSCATE("\x55\x8B\xEC\x83\xEC\x34\x83\x3D\x2A\x2A\x2A\x2A\x02"),
            13
        },
    #else
        // detoured, don't touch
        //{
        //    // CNetChan::Shutdown
        //    //
        //    // Signature for _ZN8CNetChan8ShutdownEPKc:
        //    // 55 89 E5 57 56 53 83 EC 3C 8B 5D 08 8B 75 0C 8B 8B 8C 00 00 00
        //    AY_OBFUSCATE("\x55\x89\xE5\x57\x56\x53\x83\xEC\x3C\x8B\x5D\x08\x8B\x75\x0C\x8B\x8B\x8C\x00\x00\x00"),
        //    21
        //},
        {
            // CNetChan::SendNetMsg
            //
            // At LEAST used for spoofing m_nFriendsID, if nothing else. Probably more.
            // 
            // String: "NetMsg"
            //
            // Signature for sub_4DA370:
            // 55 89 E5 83 EC 38 0F B6 45 14 89 5D F4
            AY_OBFUSCATE("\x55\x89\xE5\x83\xEC\x38\x0F\xB6\x45\x14\x89\x5D\xF4"),
            13
        },
        {
            // ::CL_Move
            //
            // String: "CL_Move" followed by "%s"
            //
            // Signature for sub_3B7070:
            // 55 89 E5 81 EC A8 00 00 00 83 3D ? ? ? ? 01
            AY_OBFUSCATE("\x55\x89\xE5\x81\xEC\xA8\x00\x00\x00\x83\x3D\x2A\x2A\x2A\x2A\x01"),
            16
        },
    #endif
};

/*
CFuncList ClientFuncs[] =
{
    #ifdef _WIN32
        {
            // CUserCmd::GetUserCmd
            //
            // String: "WARNING! User command buffer overflow(%"
            // It's a bit further up
            //
            // sub_10166A60 + 99
            // sub_10166A60 + 99       loc_10166AF9:
            // sub_10166A60 + 99   058 push    esi
            // sub_10166A60 + 9A   05C push    edi
            // sub_10166A60 + 9B   060 mov     ecx, ebx
            // sub_10166A60 + 9D   060 call -->sub_10166A00<--
            //
            // Signature for sub_10166A00:
            // 55 8B EC 53 56 8B D9 8B 4D 08 57 E8 ? ? ? ? 8B 75 0C 8B F8 B8 B7 60 0B B6
            AY_OBFUSCATE("\x55\x8B\xEC\x53\x56\x8B\xD9\x8B\x4D\x08\x57\xE8\x2A\x2A\x2A\x2A\x8B\x75\x0C\x8B\xF8\xB8\xB7\x60\x0B\xB6"),
            26
        },
        {
            // CInput::CreateMove
            // This is the function we use to network our "ban me i am cheating" message to the server!
            // Make sure it's intact... 
            // Sig aquired with PDBs.
            // 55 8B EC 83 EC 50 53 56 8B 75 08 B8 B7 60 0B B6 F7 EE 8B D9 03
            AY_OBFUSCATE("\x55\x8B\xEC\x83\xEC\x50\x53\x56\x8B\x75\x08\xB8\xB7\x60\x0B\xB6\xF7\xEE\x8B\xD9\x03"),
            21
        },
    #else
        {
            // CUserCmd::GetUserCmd
            //
            // STUB! GetUserCmd is inlined! Good!
            // This is WriteUserCmdDelta etc
            //
            // Signature for _ZN6CInput25WriteUsercmdDeltaToBufferEP8bf_writeiib:
            // 55 C5 F8 57 C0 89 E5 57 56 8D 4D A8
            AY_OBFUSCATE("\x55\xC5\xF8\x57\xC0\x89\xE5\x57\x56\x8D\x4D\xA8"),
            12
        },
        {
            // CInput::CreateMove
            // This is the function we use to network our "ban me i am cheating" message to the server!
            //
            // Signature for _ZN6CInput10CreateMoveEifb:
            // 55 BA B7 60 0B B6 C5 F8 57 C0 89 D0
            AY_OBFUSCATE("\x55\xBA\xB7\x60\x0B\xB6\xC5\xF8\x57\xC0\x89\xD0"),
            12
        },
    #endif
};
*/

CFuncList VmatFuncs[] =
{
    #ifdef _WIN32
        {
            // I don't know the exact function name but it's x::StartDrawing
            // So I'm going to call it VMat::StartDrawing, since it's in the vguimatsurfaces lib
            //
            // String: "-pixel_offset_x"
            //
            // Signature for sub_100472F0:
            // 55 8B EC 64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 14
            AY_OBFUSCATE("\x55\x8B\xEC\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x2A\x2A\x2A\x2A\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x14"),
            27
        },
        {
            // I don't know the exact function name but it's x::EndDrawing
            // So I'm going to call it VMat::EndDrawing
            //
            // String: "Too many popups! Rendering will be bad!"
            // It's a bit further down
            //
            // sub_10045620+24F  02C mov     eax, [esi]
            // sub_10045620+251  02C mov     ecx, esi
            // sub_10045620+253  02C push    0
            // sub_10045620+255  030 call    dword ptr [eax+1D4h]
            // sub_10045620+25B  02C mov     ecx, ebx
            // sub_10045620+25D  02C call -->sub_10042510<--
            // Signature for sub_10042510:
            // 55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 6A 00
            AY_OBFUSCATE("\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x56\x6A\x00"),
            28
        },
    #else
        {
            // I don't know the exact function name but it's x::StartDrawing
            // So I'm going to call it VMat::StartDrawing, since it's in the vguimatsurfaces lib
            //
            // String: "-pixel_offset_x"
            //
            // Signature for sub_93D60:
            // 55 89 E5 53 83 EC 74
            AY_OBFUSCATE("\x55\x89\xE5\x53\x83\xEC\x74"),
            7
        },
        {
            // THIS IS A STUB!!! ENDDRAWING IS INLINED,
            // so I'm just using the parent function signature
            //
            // String: "Too many popups! Rendering will be bad!"
            // 
            //
            // Signature for sub_94150:
            // 55 89 E5 57 56 53 81 EC AC 00 00 00 A1 ? ? ? ?
            AY_OBFUSCATE("\x55\x89\xE5\x57\x56\x53\x81\xEC\xAC\x00\x00\x00\xA1\x2A\x2A\x2A\x2A"),
            17
        },
    #endif
};

CFuncList::CFuncList(char* signature, size_t sigsize)
{
    m_pSignature    = signature;
    m_iSize         = sigsize;
}

bool CFuncList::CheckHook(modbin* mbin)
{
    if (memy::FindPattern(mbin, m_pSignature, m_iSize, 0x0))
    {
        return true;
    }
    return false;
}

// if we fail first init, a sig probably wasnt found because of OUR mistake, not their cheating
// just knock this person into insecure instead of banning them
bool shouldCheckHooks = true;

void CHooks::PostInit()
{
    shouldCheckHooks = GetAllHooks();
    if (!shouldCheckHooks)
    {
        CHooks::setinsecure();
    }
}

bool CHooks::GetAllHooks()
{
    if (!shouldCheckHooks)
    {
        return true;
    }
    else
    {
        // make sure there's more than one person on the server
        // this deters cheaters from hopping on empty servers
        // to poke and prod at the ac
        bool checkcheatflag = false;
        int clients = ROBUST_GetRealClientsOnServer();
        #ifdef dbging
            Warning("CHooks::GetAllHooks -> clients = %i\n\n", clients);
        #endif
        if (clients > 1)
        {
            checkcheatflag = true;
        }

        #ifdef dbging
            checkcheatflag = true;
            Warning("Setting checkcheatflag = trutru\n");
        #endif

        // it would be funny if cheaters just set this to false. i would laugh
        if (!checkcheatflag)
        {
            return true;
        }
    }

    // backtrace found somethin wonky!
    // if ( !CHooks::DoBT() )
    // {
    //    #ifdef dbging
    //        Warning("CHooks::GetAllHooks -> backtrace found somethin wonky.\n");
    //    #endif
    //    return false;
    // }

    // engine
    for (int i = 0; i < (sizeof(EngineFuncs) / sizeof(*EngineFuncs)); i++)
    {
        if (!EngineFuncs[i].CheckHook(engine_bin))
        {
            #ifdef dbging
                Warning("CHooks::GetAllHooks -> Failed on engine sig %i.\n", i);
            #endif
            return false;
        }
    }
    #ifdef dbging
        ConColorMsg(goodcolor, "CHooks::GetAllHooks -> Got all engine hooks.\n");
    #endif

    // vmat
    for (int i = 0; i < (sizeof(VmatFuncs) / sizeof(*VmatFuncs)); i++)
    {
        if (!VmatFuncs[i].CheckHook(vgui_bin))
        {
            #ifdef dbging
                Warning("CHooks::GetAllHooks -> Failed on vmat sig %i.\n", i);
            #endif
            return false;
        }
    }
    #ifdef dbging
        ConColorMsg(goodcolor, "CHooks::GetAllHooks -> Got all vmat hooks.\n");
    #endif

    return true;
}


// backtrace stuff
// #define MAX_FRAMES 16

// bool doretadr = false
/*
bool CHooks::DoBT( bool doReturnAddrs )
{
    // stub
    return true;


    uintptr_t* eng_min = (uintptr_t*)CEngineBinary2::ModuleBase;
    uintptr_t* eng_max = eng_min + CEngineBinary2::ModuleSize;

    uintptr_t* cli_min = (uintptr_t*)CClientBinary::ModuleBase;
    uintptr_t* cli_max = cli_min + CEngineBinary2::ModuleSize;

    #ifdef _WIN32
    void* bt_ptr[MAX_FRAMES] = { nullptr };

    int frames = 0;

    // CHECK FUNC ADDRESSES
    if ( !doReturnAddrs )
    {
        // https://docs.microsoft.com/en-us/windows/win32/debug/capturestackbacktrace
        frames = CaptureStackBackTrace(
            0,          // ULONG  FramesToSkip,
            MAX_FRAMES, // ULONG  FramesToCapture,
            bt_ptr,     // PVOID * BackTrace,
            NULL        // PULONG BackTraceHash
        );

        #ifdef dbging
            // Warning("== STACK ADDRS == \n");
        #endif
    }
    // CHECK RET ADDRESSES
    else
    {
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcapturestackbacktrace
        frames = RtlCaptureStackBackTrace(
            0,          // ULONG  FramesToSkip,
            MAX_FRAMES, // ULONG  FramesToCapture,
            bt_ptr,     // PVOID * BackTrace,
            NULL        // PULONG BackTraceHash
        );

        #ifdef dbging
            // Warning("== RETURN ADDRS == \n");
        #endif
    }
    #ifdef dbging
        // Warning("FRAMES = %i, bt_ptr = %p\n\n", frames, bt_ptr);
    #endif

    for (int thisframe = 0; thisframe < frames; thisframe++)
    {
        DWORD64 address = (DWORD64)(bt_ptr[thisframe]);
        uintptr_t* addr_ptr = reinterpret_cast<uintptr_t*>( (bt_ptr[thisframe]) );

        Warning("FRAME %i -> ADDR %p\n", thisframe, (uintptr_t*)address);

        if
        (
            (
                // addr is above engine start
                addr_ptr >= eng_min
                &&
                // addr is below engine stop
                addr_ptr <= eng_max
            )
            ||
            (
                // addr is above client start
                addr_ptr >= cli_min
                &&
                // addr is below client stop
                addr_ptr <= cli_max
            )
        )
        {
            #ifdef dbging
                // Warning("ADDR %p inside CLIENT or ENGINE. cli (min %p max %p) || eng (min %p max %p)\n", addr_ptr, cli_min, cli_max, eng_min, eng_max);
            #endif
        }
        else
        {
            #ifdef dbigng
                Warning("ADDR %p OUTSIDE CLIENT (min %p max %p)\n", addr_ptr, cli_min, cli_max);
                Warning("ADDR %p OUTSIDE ENGINE (min %p max %p)\n", addr_ptr, eng_min, eng_max);
            #endif
            return false;
        }
    }
    
    if (!doReturnAddrs)
    {
        CHooks::DoBT(true);
    }

    #else // _WIN32
        void* buffer[MAX_FRAMES];

        int num_ptrs = backtrace(buffer, MAX_FRAMES);
        Warning("backtrace() returned %d addresses\n", num_ptrs);

        for (int j = 0; j < num_ptrs; j++)
        {
            Warning("FRAME -> %i || ADDR -> %p\n", j, buffer[j]);
        }
        return true;
    #endif
    return true;
}
*/
