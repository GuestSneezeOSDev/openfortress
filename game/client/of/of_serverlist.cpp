#include "cbase.h"


#include "filesystem.h"
#include "tf_gamerules.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <tier1/fmtstr.h>
#include <tier1/netadr.h>

#include <misc_helpers.h>
#include <sdkCURL.h>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include <misc_helpers.h>


class OFServerlist : public ISteamMatchmakingServerListResponse
{
public:
    std::map<servernetadr_t, gameserveritem_t> serverMap;

    OFServerlist()  {};
    ~OFServerlist() {};
    void UpdateServerlistInfo();

    int playersOnline = {};
    HServerListRequest m_hServersRequest = nullptr;
    virtual void ServerResponded(HServerListRequest hRequest, int iServer);
    virtual void ServerFailedToRespond(HServerListRequest hRequest, int iServer);
    virtual void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response);

};

OFServerlist gOFServerList;


void OFServerlist::UpdateServerlistInfo()
{
    ISteamMatchmakingServers* pMatchmaking = steamapicontext->SteamMatchmakingServers();
    if (!pMatchmaking || pMatchmaking->IsRefreshing(m_hServersRequest))
    {
        return;
    }
    // serverMap.clear();
    playersOnline = 0;
    MatchMakingKeyValuePair_t* pFilters;

    MatchMakingKeyValuePair_t filter = {};
    V_snprintf(filter.m_szKey,      sizeof(filter.m_szKey), "%s", "gamedir");
    V_snprintf(filter.m_szValue,    sizeof(filter.m_szValue), "%s", "open_fortress");

    pFilters = &filter;
    if (m_hServersRequest)
    {
        pMatchmaking->ReleaseRequest(m_hServersRequest);
        m_hServersRequest = nullptr;
    }
    m_hServersRequest = pMatchmaking->RequestInternetServerList(engine->GetAppID(), &pFilters, 1, this);
}


void OFServerlist::ServerResponded(HServerListRequest hRequest, int iServer)
{
    gameserveritem_t* pServerItem = steamapicontext->SteamMatchmakingServers()->GetServerDetails(hRequest, iServer);

    auto adr = pServerItem->m_NetAdr;

    auto it = serverMap.find(adr);
    if (it != serverMap.end())
    {
        it->second = *pServerItem;
    }
    else
    {
        serverMap.insert(std::pair(adr, *pServerItem));
    }
    auto realPeople = pServerItem->m_nPlayers - pServerItem->m_nBotPlayers;
    playersOnline += realPeople;
}



// BUGBUG: What if the user loses internet connection? Wouldn't this just remove all the servers?
void OFServerlist::ServerFailedToRespond(HServerListRequest hRequest, int iServer)
{
    Warning("->%i", iServer);
}

void OFServerlist::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
    Warning("->\n");
    for (auto& item : serverMap)
    {
        Warning("-> %s %s\n", item.first.GetQueryAddressString(), item.second.GetName() );
    }

    Warning("->Players in visible servers = %i - plus you!\n", playersOnline);
}



CON_COMMAND_F(doServerList, 0, 0)
{
    gOFServerList.UpdateServerlistInfo();
}