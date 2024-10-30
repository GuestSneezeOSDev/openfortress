/*
#ifdef _WIN32
    #pragma once
#endif

#include <ac/ac_utils.h>

#include "cbase.h"

#include "badsigs.h"

// This is needed so that CAutoGameSystem knows that we're using it, apparently
CEvilSigs g_CEvilSigs;

#ifdef _WIN32
CEvilSig g_evilsigs[] =
{
    // minhook
    // midfunc sig because i can't be assed to make GetAddrOfData take wildcards yet
    {
        "\x8B\xD0\x85\xD2\x74\x6F\x8B\x45\xD8\x8A\x4D\xE4\x8B\x7D\xE0\x80\xE1\x01\x89\x02\x8B\x45\xDC\x89\x42\x04\x8A\x42\x14\x24\xF8\x89\x7A\x08\x0A\xC8\x8B",
        37,
    },
    // only fortress by lak3
    // maybe a vmprot func? dunno.
    {
        "\x55\x8B\xEC\x83\xEC\x2C\x53\x56\x57\x89\x55\xFC\x8B\xF9\x33\xDB\xE8",
        17,
    },
    // old version of lithium
    // random sig, bin is very thoroughly vmprotected
    {
        "\x50\x48\x0F\xBF\xC1\x40\x8A\xC5\x41\x54\x49\x0F\xBF\xC5\x52\x41\x0F\x47\xD1\x48\x98\x57",
        22,
    },
    // nacl
    // midfunc sig because no wildcards etc.
    {
        "\xC6\x45\xFC\x37\x33\xC0\x50\x50\x57\x50\x83\xEC\x0C\x8D\x85\xC0\x00\x00\x00\x8B\xCC\x89\x65\xDC\x8D\x55\x00\x51\x50\x52\xE8",
        31,
    },
    // test string
    //{
    //    "sv_cheats",
    //    9,
    //},
};
#endif

CEvilSig::CEvilSig(char* signature, size_t sigsize)
{
    m_pSignature = (addr*)signature;
    m_iSize     = sigsize;
}

#ifdef _WIN32
bool findEvil()
{
    DWORD thisproc = GetCurrentProcessId();
    // loop thru our evil sigs
    for (int i = 0; i < (sizeof(g_evilsigs) / sizeof(*g_evilsigs)); i++)
    {
        int matches = -1;
        addr* ptrarray = nullptr;
        ptrarray = FindThingInProcessMem(
            thisproc,
            g_evilsigs[i].m_pSignature,
            g_evilsigs[i].m_iSize,
            matches
        );

        for (int match = 0; match < matches; match++)
        {
            Warning("matched ptr %x\n", ptrarray[match]);
        }

        Warning("matches %i\n", matches);
    }
    return true;
}
#else
bool findEvil()
{
    return true;
}
#endif

// This is "" so that we don't have an id-able string
// -sappho
CEvilSigs::CEvilSigs() : CAutoGameSystem("  ")
{
}

void CEvilSigs::PostInit()
{
    // Warning("-> evilsigs2\n");
    findEvil();
}
*/