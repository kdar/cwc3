#ifndef _INFO_H
#define _INFO_H

#include <wx/datetime.h>

#include "Inet.h"
#include "Object.h"

class NodeInfo;

typedef shared_ptr<NodeInfo> NodeInfoPtr;
typedef map<wxString, NodeInfoPtr> NodeInfoMap;

//-------------------------------
//Contains information pertaining to the game
class GameInfo : public Object<GameInfo>
{
  public:
    GameInfo()
    { Reset(); }

    void Reset();

  public:
    //The start and end time of the game.
    wxDateTime startTime;
    wxDateTime endTime;

    //The start and end time of loading the game.
    wxDateTime startLoadTime;
    wxDateTime endLoadTime;

    //How long the game has gone for.
    wxTimeSpan gameDuration;

    //Map name contains the full name of the map.
    wxString sMapName;
    //Short map name contains the name, and not the extension of the map.
    wxString sShortMapName;
    
    //The game name.
    wxString sGameName;
    
    //Our name
    wxString sMyName;
    //The host's name
    wxString sHostName;
    
    //Is this a private game?
    bool bPrivate;

    //Are we hosting this game? Or did we join one.
    bool bHosted;
    
    //This is how many games we have hosted since wc3 was started.
    //We need this for the auto refresher. Clients use it to connect
    //to games (it's almost like authentication).
    int nHostedCount;
    
    //Order matters, this contains how many people are here on each team.
    IntIntMap herePerTeamCount;
    
    //Different booleans for the state of a game.
    bool bCreatingGame;
    bool bCreatedGame;
    bool bStartingGame;
    bool bStartedGame;
    bool bLoadedGame;
    bool bExitedGame;
    bool bEndedGame;
};

//-------------------------------
//Contains netork info that needs to be known
class NetInfo : public Object<NetInfo>
{
  public:
    NetInfo()
      : dataLink(0), bInitialized(false)
    {}

  public:
    int dataLink;
    u_int8_t gamehost_ether[ETHER_ADDR_LEN];

    bool bInitialized;
};

//-------------------------------
//Contains network info about a specific node(ip:port)
class NodeInfo : public Object<NodeInfo>
{
  public:
    NodeInfo()
      : last_sentack(0), last_recvack(0), last_sentseq(0),
        last_recvseq(0), last_sentwin(0), last_recvwin(0),
        bPinged(false), bReceivedRst(false), bSentRst(false)
    {}
    
  public:
    u_int32_t last_sentack;
    u_int32_t last_recvack;
    u_int32_t last_sentseq;
    u_int32_t last_recvseq;
    u_short last_sentwin;
    u_short last_recvwin;

    wxDateTime sent;
    wxTimeSpan ping;
    bool bPinged;

    wxDateTime lastResponse;
    
    //When our game sends a TH_RST packet to this node, we mark it.
    //Helps us in determining if a client left.
    bool bReceivedRst;
    //Sadly, the above doesn't work all the time. Here, we use this
    //to assume a client left if bSentRst is set, and a OnClientEnd was
    //sent.
    bool bSentRst;
};

//-------------------------------
//Contains info about the realm we are on.
class RealmInfo : public Object<RealmInfo>
{
  public:
    RealmInfo() 
      : bInitialized(false)
    {}
    
    void Reset()
    { sRealmName = _T(""); bInitialized = false; }
    
  public:
    //The name of this realm.
    wxString sRealmName;

    //The ip of this realm.
    wxString sIp;
    
    bool bInitialized;
};

//-------------------------------
//Centrialized info class that is a gateway to different info structures.
class Info : public Object<Info>
{
  public:
    Info() {}
    
    static shared_ptr<Info> Get();    
    static shared_ptr<Info> Set(shared_ptr<Info> pInstance);
    
    shared_ptr<NodeInfo> GetNodeInfo(const wxString &sIp, int nPort);
    void ClearNodeInfoMap();
    void RemoveNodeInfoEntry(const wxString &sIp, int nPort);
    
    NodeInfoMap &GetNodeInfoMap()
    { return m_nodeInfoMap; }
    
    const NodeInfoMap &GetNodeInfoMap() const
    { return m_nodeInfoMap; }

    GameInfo &GetGameInfo()
    { return m_gameInfo; }

    const GameInfo &GetGameInfo() const
    { return m_gameInfo; }

    NetInfo &GetNetInfo()
    { return m_netInfo; }

    const NetInfo &GetNetInfo() const
    { return m_netInfo; }
    
    RealmInfo &GetRealmInfo()
    { return m_realmInfo; }

    const RealmInfo &GetRealmInfo() const
    { return m_realmInfo; }
    
  private:
    static shared_ptr<Info> ms_pInstance;
    
    NodeInfoMap m_nodeInfoMap;
    GameInfo m_gameInfo;
    NetInfo m_netInfo;
    RealmInfo m_realmInfo;
};

#endif
