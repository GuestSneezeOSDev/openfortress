#ifdef _WIN32
#pragma once
#endif
#include <ac/ac_utils.h>
#include "obfuscate.h"
/*
#ifdef _WIN32
void EnumProcDlls(DWORD pid)
{
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (!proc || proc == INVALID_HANDLE_VALUE)
    {
        return;
    }
    HMODULE mods[512];
    if (!mods)
    {
        return;
    }

    DWORD totalneeded = 0;

    // BOOL EnumProcessModules(
    // [in]  HANDLE  hProcess,
    // [out] HMODULE *lphModule,
    // [in]  DWORD   cb,
    // [out] LPDWORD lpcbNeeded);
    if (!EnumProcessModules(proc, mods, sizeof(mods), &totalneeded))
    {
        return;
    }
    MODULEINFO modinfo;
    CHAR szModuleName[MAX_PATH];
    const size_t totalmods = totalneeded / sizeof(HMODULE);
    for (size_t i = 0; i < totalmods; ++i)
    {
        if (!GetModuleInformation(proc, mods[i], &modinfo, sizeof(modinfo)))
        {
            return;
        }
        // DWORD GetModuleBaseName(
        // [in]           HANDLE  hProcess,
        // [in, optional] HMODULE hModule,
        // [out]          LPWSTR  lpBaseName,
        // [in]           DWORD   nSize );
        //if (GetModuleBaseNameA(proc, mods[i], szModuleName, sizeof(szModuleName) / sizeof(CHAR)))
        //{
        //    // printf("Name = %s, Base = %X, Size = %X\n", szModuleName, reinterpret_cast<unsigned int>(mods[i]), modinfo.SizeOfImage);
        //}
        if (GetModuleFileNameExA(proc, mods[i], szModuleName, sizeof(szModuleName) / sizeof(CHAR)))
        {
            //addr* base = (addr*)mods[i];
            //size_t size = modinfo.SizeOfImage;
            //const char* name = szModuleName;

            // checkDlls(base, size, name);
            // printf("Name = %s, Base = %X, Size = %X\n", szModuleName, reinterpret_cast<unsigned int>(mods[i]), modinfo.SizeOfImage);
        }
    }
    return;
}

DWORD GetProcessByName(PCSTR name)
{
    DWORD pid = 0;

    // Create toolhelp snapshot.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    // Walkthrough all processes.
    if (Process32First(snapshot, &process))
    {
        do
        {
            printf("%S\n", process.szExeFile);

            std::string str_exefile(process.szExeFile);

            std::string str_name(name);

            // Compare process.szExeFile based on format of name, i.e., trim file path
            // trim .exe if necessary, etc.
            if (str_exefile == str_name)
            {
                pid = process.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    if (pid != 0)
    {
        return pid;
    }

    // Not found


    return NULL;
}

#include <algorithm>
#include <vector>
#include <cstring>

#define max_matches 32
addr* FindThingInProcessMem(DWORD pid, const void* data, size_t len, int& matches)
{
    matches = -1;
    static addr ptrarray[max_matches];
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
    {
        return nullptr;
    }

    matches = 0;
    // sysinfo so we can determine where to stop reading from (approx)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    addr pagesize = (addr)sysinfo.dwPageSize;
    addr minapp = (addr)sysinfo.lpMinimumApplicationAddress;
    addr maxapp = (addr)sysinfo.lpMaximumApplicationAddress;

    printf("pagesize = %i, minapp = %i, maxapp = %i\n", pagesize, minapp, maxapp);

    // meminfo 
    MEMORY_BASIC_INFORMATION meminfo = {};

    // vector that we store our memory chunks in
    // since we're storing bytes, each element only needs to store 0 - 255
    // aka a unsigned char aka a byte
    std::vector<byte>vec_chunk = { 0 };

    // size of our current memory chunk
    // this is our initial, first chunk
    SIZE_T regionsize = meminfo.RegionSize;

    // loop thru our memory chunks, regionsize at a time
    for (byte* p = 0; (addr)p < maxapp; p += regionsize)
    {
        // SIZE_T VirtualQueryEx(
        // [in] HANDLE                                  hProcess,
        // [in, optional] LPCVOID                       lpAddress,
        // [out]          PMEMORY_BASIC_INFORMATION     lpBuffer,
        // [in]           SIZE_T                        dwLength);
        // set our meminfo so we can get the size and base addr of the next chunk
        SIZE_T meminfo_size = VirtualQueryEx(process, p, &meminfo, sizeof(meminfo));
        if (meminfo_size == ERROR_INVALID_PARAMETER || meminfo_size != sizeof(meminfo))
        {
            printf("something has gone terribly wrong with VirtualQueryEx\n");
            return nullptr;
        }
        regionsize = meminfo.RegionSize;
        byte* baseaddr = (byte*)meminfo.BaseAddress;
        printf("RegionSize  %zd\n", regionsize);
        printf("BaseAddress %p \n", baseaddr);

        // 128 MB
        if (regionsize >= (128 * 1000 * 1000))
        {
            printf("region size is huge = %x. ignoring!\n", regionsize);
            continue;
        }

        p = baseaddr;
        vec_chunk.resize(regionsize);

        SIZE_T bytesRead;

        // BOOL ReadProcessMemory(
        //     [in]  HANDLE  hProcess,
        //     [in]  LPCVOID lpBaseAddress,
        //     [out] LPVOID  lpBuffer,
        //     [in]  SIZE_T  nSize,
        //     [out] SIZE_T * lpNumberOfBytesRead);
        if (ReadProcessMemory(process, p, &vec_chunk[0], regionsize, &bytesRead))
        {
            // start at byte 0 of our chunk of memory
            // increment thru it
            for (addr thisbyte = 0; thisbyte < (bytesRead - len); ++thisbyte)
            {
                // compare our data [in] to the current data in vec_chunk at thisbyte
                if (memcmp(data, &vec_chunk[thisbyte], len) == 0)
                {
                    printf("[in] found %p\n", p + thisbyte);

                    matches++;
                    printf("[in] %i\n", matches);
                    ptrarray[matches - 1] = (addr)p + thisbyte;

                    // i am stupid and don't know if this avoids a buf overflow or not
                    if (matches >= max_matches)
                    {
                        return ptrarray;
                    }
                }
            }
        }

    }
    return ptrarray;
}
*/

#ifdef OF_DLL
#include <obfuscate.h>

#include "tier1/convar.h"
#include "eiface.h"

extern IVEngineServer* engine;

void obfuscated_ac_ban(int userid)
{
    volatile float  dummy   = 6.08954850027352014107607081774097;
    volatile int    uid     = userid + (int)dummy;

    // garbage
    std::string str_reason2(AY_OBFUSCATE("cmFuZG9tZ2Fy"));

    // "cheating"
    std::string str_reason(AY_OBFUSCATE("cheating"));

    // "sm_ban"
    std::string str_smban(AY_OBFUSCATE("sm_ban"));

    // garbage
    std::string str_smban2(AY_OBFUSCATE("Z2F5c2V4"));

    const char* cc_smban = str_smban.c_str();

    // ignore failures
    ConCommandBase* sourcemod_cvar = g_pCVar->FindCommandBase(cc_smban);

    std::stringstream killstringstream;
#ifdef dbging
    Warning("banstr = %s\n", cc_smban);
#endif
    // sourcemod / sourcebans handler
    if ( sourcemod_cvar )
    {
#ifdef dbging
        const char* smsuccess = AY_OBFUSCATE("found sourcemod\n");
        Warning("%s\n", smsuccess);
#endif
        // sm_ban #[uid] 0 cheating;\n
        killstringstream
            << str_smban << " #" << uid - (int)dummy << " 0 " << str_reason << ";\n";
    }
    // tf2 ban handler
    else
    {
#ifdef dbging
        const char* smfail = AY_OBFUSCATE("didn't find sourcemod\n");
        Warning("%s\n", smfail);
#endif
        // "banid "
        std::string str_banid(AY_OBFUSCATE("banid"));

        // "kickid"
        std::string str_kickid(AY_OBFUSCATE("kickid"));

        // "write "
        std::string str_write(AY_OBFUSCATE("write"));

        // "id "
        std::string str_id(AY_OBFUSCATE("id"));

        // "ip "
        std::string str_ip(AY_OBFUSCATE("ip"));

        // banid 0 [uid]; kickid [uid] cheating; writeid; writeip;
        killstringstream
            << str_banid    << AY_OBFUSCATE(" 0 ")  << uid - (int)dummy << AY_OBFUSCATE("; ")                                       // banid 0 [uid];
            << str_kickid   << AY_OBFUSCATE(" ")    << uid - (int)dummy << AY_OBFUSCATE(" ") << str_reason << AY_OBFUSCATE("; ")    // kickid [uid] cheating;
            << str_write    << str_id               << AY_OBFUSCATE("; ")                                                           // writeid;
            << str_write    << str_ip               << AY_OBFUSCATE(";\n")                                                          // writeip;\n
            ;

    }
    std::string killstring = killstringstream.str();
    const char* kill_cmd = killstring.c_str();

#ifdef dbging
    Warning("kill_cmd = ");
    Warning("%s\n", kill_cmd);
#endif
    engine->ServerCommand(kill_cmd);
    //engine->ServerExecute(); // THIS CRASHES. DON'T USE IT

    return;
}
#endif
#include <obfuscate.h>
#include <misc_helpers.h>

// by default, returns the number of humans on the server
// if retearly is true, it will return gpGlobals->maxClients+1 if there is more than 1 client
// or 1 if there's only one client
int ROBUST_GetRealClientsOnServer(bool retearly)
{
    int realclients = 0;
    for (int itercli = 1; itercli <= gpGlobals->maxClients; itercli++)
    {
        CBasePlayer *pPlayer = UTIL_PlayerByIndex(itercli);

        if (!pPlayer)
        {
            continue;
        }

        if ( UTIL_IsFakePlayer( pPlayer ) )
        {
            continue;
        }

        realclients++;

        if (retearly && realclients > 1)
        {
            return gpGlobals->maxClients + 1;
        }
    }

    #ifdef realcldbg
        Warning("Realclients = %i\n", realclients);
    #endif

    return realclients;
}
