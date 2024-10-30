#pragma once



#ifdef _WIN32
#define FORCEINLINE_AC __forceinline
#elif POSIX
#define FORCEINLINE_AC inline
#else
#error "implement me"
#endif
#include <obfuscate.h>
#include <cstdint>
static FORCEINLINE_AC void* xorptr(void* inptr)
{
    uintptr_t uinptr = reinterpret_cast<uintptr_t>(inptr);
    unsigned int xorjunk = static_cast<unsigned int>(ay::generate_key(reinterpret_cast<unsigned int>(__TIME__)));
    return reinterpret_cast<void*>(uinptr ^ xorjunk);
}



/*
#ifdef _WIN32

#pragma once
#pragma warning( disable : 4530 )

// winapi
#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>

typedef unsigned char byte;
typedef uintptr_t     addr;

void EnumProcDlls(DWORD pid);
DWORD GetProcessByName(PCSTR name) ;
addr* FindThingInProcessMem(DWORD pid, const void* data, size_t len, int& matches) ;
addr* FindThingInProcessMem(DWORD pid, const void* data, size_t len, int& matches) ;

// backtrace stuff
// DoBT
#endif
*/

#ifdef OF_DLL
void obfuscated_ac_ban(int userid) ;
#endif

#include <cbase.h>

#ifdef OF_CLIENT_DLL
    #include <c_tf_player.h>
#else
    #include <tf_player.h>
    #include <player.h>
    #include <playerinfomanager.h>
#endif

#include <inc_cpp_stdlib.h>

int ROBUST_GetRealClientsOnServer(bool retearly=false);
