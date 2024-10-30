#pragma once

#include <memy/memytools.h>


class CHooks : public CAutoGameSystem
{
public:
                        CHooks();

    void                PostInit() override;

    static bool         GetAllHooks();

    static void         setofi();

    static void         setinsecure();
    // static bool         DoBT( bool doReturnAddrs = false );
};


class CFuncList
{
public:
    CFuncList(char* siggy, size_t siggy_size);
    bool CheckHook(modbin* mbin);
private:
    char* m_pSignature;
    size_t m_iSize;
};