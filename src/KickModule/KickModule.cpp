#include "KickModule/KickModule.h"
#include "KickModule/EnsureKick.h"
#include "CommandHandler.h"
#include "ClientManager.h"
#include "SlotManager.h"
#include "Interface.h"
#include "NetEventDispatcher.h"
#include "Game.h"
#include "Info.h"
#include "Config.h"

//FIXME: In this module we only check for auto kick if we are hosting.
//but I think we should check if they have a high ping, not in the correct
//country, or check if they have multiple IP regardless. Then, we should only
//kick if we are hosting.

BEGIN_EVENT_TABLE(KickModule, wxEvtHandler)
  EVT_NET(NET_SlotTableInitial, KickModule::OnSlotTableInitial)
  EVT_NET(NET_ClientSlotKick, KickModule::OnClientSlotKick)
  EVT_NET(NET_InitiateMapUpload, KickModule::OnInitiateMapUpload)
END_EVENT_TABLE()

//===============================
KickModule::KickModule()
  : Module()
{
}

//===============================
KickModule::~KickModule()
{
}

//===============================
void KickModule::Initialize(shared_ptr<Game> pGame)
{
  if (!pGame) return;
    
  m_pGame = pGame;
    
  shared_ptr<CommandHandler> pCmdHandler = pGame->GetCommandHandler();
  
  //Register commands to the command handler.
  if (pCmdHandler) {    
    BEGIN_REGISTER_COMMANDS(pCmdHandler);
  
    REGISTER_COMMAND(KickCmd);
    REGISTER_COMMAND(FKickCmd);
    REGISTER_COMMAND(KickTeamCmd);
    REGISTER_COMMAND(KickAllCmd);
    REGISTER_COMMAND(KickIpPortCmd);
    
    END_REGISTER_COMMANDS();
  }
  
  //wxEvtHandler *pPrev = pGame->GetPreviousHandler();
  wxEvtHandler *pNext = pGame->GetNextHandler();
  pGame->SetNextHandler(this);
  SetNextHandler(pNext);
  
  InitConfig();
  
  EnsureKick::Get()->Run();
}

//===============================
void KickModule::Shutdown()
{
  EnsureKick::Get()->Delete(); 
}

//===============================
void KickModule::InitConfig()
{
  shared_ptr<Config> p = Config::Get();
  p->GetBool("AutoKick/KickOnInvalidCountry", true);
  p->GetBool("AutoKick/WarnOnInvalidCountry", false);
  p->GetBool("AutoKick/KickOnInvalidPing", true);
  p->GetBool("AutoKick/WarnOnInvalidPing", false);
  p->GetBool("AutoKick/KickOnMultipleIp", false);
  p->GetBool("AutoKick/WarnOnMultipleIp", false);
  p->GetBool("AutoKick/KickOnMapDownload", true);
  p->GetBool("AutoKick/AddKickOnMultipleIpKick", true);
  p->GetBool("AutoKick/AddKickOnSlotKick", false);
  p->GetBool("AutoKick/GiveReason", true);
  p->GetArrayString("AutoKick/Exceptions", "tehsemi,megadaffy,thedarth,klutchzilla,hotnix,jimmydeano,moll420ll305ll,eltee,Laffy_Taffy");
  p->GetArrayString("AutoKick/AllowedCountries", "US,CA");
  p->GetInt("AutoKick/MaxPing", 200); 
}

//===============================
/*static*/ void KickModule::Kick(const shared_ptr<Client> &pClient, char *szReason, KickFlagType flags)
{
  if (!pClient)
    return;
    
  wxString sMsg;
  if (szReason) {
    sMsg = wxString::Format("</B/24>Kicking<!B!24>: \"</B>%s<!B>\" [%s]", pClient->sName.c_str(), szReason);
  } else {
    sMsg = wxString::Format("</B/24>Kicking<!B!24>: \"</B>%s<!B>\"", pClient->sName.c_str());
  }
  Interface::Get()->Output(sMsg);
    
  EnsureKick::Get()->Add(pClient);

  /*TcpKill *pKill = NULL;
  if (flags & KICK_FORCE)
    pKill = new TcpKill(pClient->sIp);
  else
    pKill = new TcpKill(pClient->sIp, pClient->nPort);
  pKill->Run();*/

  //pClient->Kicked();
  //if (!Info::Get()->GetGameInfo().bStartedGame)
  //  SlotManager::Get()->RemoveClient(pClient);
  //Interface::Get()->RefreshClientAll();
}

//===============================
void KickModule::AddAutoKick(const wxString &sIp, const wxString &sReason)
{
  AutoKick ak;
  ak.sIp = sIp;
  ak.sReason = sReason;
  m_autoKickList.push_back(ak);
}

//===============================
//FIXME: It checks for multiple IP here, but it should probably do this somewhere else
//       and report it. We would still need to see if we should kick them here though.
void KickModule::CheckKick(const net_ip *pIp, const net_tcp *pTcp)
{
  //Get the client based on the ip/port.
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(pIp->ip_dst), ntohs(pTcp->th_dport));
  if (!pClient) {
    return;
  }

  ArrayString asExceptions = CONFIG(ArrayString, "AutoKick/Exceptions");
  ArrayString asAllowedCountries = CONFIG(ArrayString, "AutoKick/AllowedCountries");
  bool bAutoKickReason = CONFIG(bool, "AutoKick/GiveReason");
  bool bWarnOnInvalidPing = CONFIG(bool, "AutoKick/WarnOnInvalidPing");
  bool bWarnOnInvalidCountry = CONFIG(bool, "AutoKick/WarnOnInvalidCountry");
  bool bWarnOnMultipleIp = CONFIG(bool, "AutoKick/WarnOnMultipleIp");
  bool bKickOnMultipleIp = CONFIG(bool, "AutoKick/KickOnMultipleIp");
  bool bKickOnInvalidPing = CONFIG(bool, "AutoKick/KickOnInvalidPing");
  bool bKickOnInvalidCountry = CONFIG(bool, "AutoKick/KickOnInvalidCountry");
  bool bAddKickOnMultipleIpKick = CONFIG(bool, "AutoKick/AddKickOnMultipleIpKick");
  int nMaxPing = CONFIG(int, "AutoKick/MaxPing");
  
  shared_ptr<CommandHandler> pCommandHandler = m_pGame->GetCommandHandler();

  //--Check if this client is in the autokick exception list--
  //FIXME: Why do we do it like this? Why don't we just return from this function if it matches?...
  bool bAutoKickException = false;
  for (int x = 0; x < asExceptions.size(); x++) {
    if (asExceptions[x].IsSameAs(pClient->sName, false))
      bAutoKickException = true;
  }

  //--Check to see if this client is on the autokick list--
  for (u_int x = 0; x < m_autoKickList.size(); x++) {
    if (pClient->sIp.IsSameAs(m_autoKickList[x].sIp)) {
      //Their IP matches, so attempt to kick
      if (!bAutoKickException) {
        char *szReason = NULL;
        if (bAutoKickReason) {
          wxString sReason = m_autoKickList[x].sReason;
          if (sReason.IsEmpty())
            szReason = const_cast<char *>(sReason.c_str());
          else
            szReason = "Autokick list";
        }
        
        KickModule::Kick(pClient, szReason);

        //If we kicked them from the auto kick list, then no need to kick them again.
        return;
      }
    }
  }

  //--Check to see if the client's Ip matches someone already connected--
  //This could be done a lot better and cleaner without so much redundancy, but this will stay until it needs to be changed.

  //This first part simply checks to see if we have a multiple Ip and makes a list of all the names
  //that have the same Ip.
  bool bMultipleIp = false;
  const ClientArray &clientArray = ClientManager::Get()->GetClientArray();
  wxArrayString multipleIpNameList;
  for (int i = 0; i < clientArray.size(); i++) {
    shared_ptr<Client> p = clientArray[i];
    if (p->sIp.IsSameAs(pClient->sIp)) {
      //Add the client to the list, even the current client we're checking for
      multipleIpNameList.Add(p->sName);

      //This means if we found an Ip match and it's not the client we are currently checking, then
      //we have a multiple Ip issue. So set the boolean to true.
      if (!pClient->IsSameAs(*p))
        bMultipleIp = true;
    }
  }

  //Now we must concatenate each name entry into a string so we can print it properly.
  wxString sNames;
  for (u_int x = 0; x < multipleIpNameList.Count(); x++) {
    sNames += multipleIpNameList.Item(x);
    if (x + 1 < multipleIpNameList.Count())
      sNames += ", ";
  }

  //This just warns of the clients who have the same Ip
  if (bMultipleIp && bWarnOnMultipleIp) {
    Interface::Get()->Output(wxString::Format("</B>Multiple Ip<!B>: %s", sNames.c_str()), OUTPUT_WARNING);
  }

  //We go through the list again and we do the actual kicking here.
  if (bMultipleIp) {
    //We must save the IP because eventually the client will be deleted
    wxString sIp = pClient->sIp;
    for (int i = 0; i < clientArray.size(); i++) {
      shared_ptr<Client> p = clientArray[i];
      if (p->sIp.IsSameAs(sIp)) {
        //Auto kick them for having a multiple IP
        if (bKickOnMultipleIp && !bAutoKickException) {
          char *szReason = NULL;
          if (bAutoKickReason) {
            szReason = "Multiple Ip";
          }
          
          KickModule::Kick(p, szReason);

          //Add them to the autokick list
          if (bAddKickOnMultipleIpKick) {
            wxString sReason = "(auto)Multiple Ip: " + sNames;
            AddAutoKick(sIp, sReason);
          }

          //If we kicked them for multiple Ip, then no need to kick them again.
          return;
        }
      }
    }
  }

  //--Check if this client is from an allowed country--
  bool bCountryFound = false;
  for (int x = 0; x < asAllowedCountries.size(); x++) {
    if (asAllowedCountries[x].IsSameAs(pClient->sCountry, false))
      bCountryFound = true;
  }

  //The client isn't from an allowed country
  if (!bCountryFound) {
    if (bWarnOnInvalidCountry)
      Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\" is not from an allowable country.", pClient->sName.c_str()), OUTPUT_WARNING);

    //Auto kick them for a non-allowed country
    if (bKickOnInvalidCountry && !bAutoKickException) {
      char *szReason = NULL;
      if (bAutoKickReason) {
        szReason = "Invalid country";
      }
      
      KickModule::Kick(pClient, szReason);

      //If we kicked them for an invalid country, then no need to kick them again.
      return;
    }
  }

  //--Check to see if the client's ping is too high--
  if (pClient->ping.GetMilliseconds().GetValue() > nMaxPing) {
    if (bWarnOnInvalidPing)
      Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\"'s ping is too high.", pClient->sName.c_str()), OUTPUT_WARNING);

    //Auto kick them for a high ping
    if (bKickOnInvalidPing && !bAutoKickException) {
      char *szReason = NULL;
      if (bAutoKickReason) {
        szReason = "Invalid ping";
      }
      
      KickModule::Kick(pClient, szReason);

      //If we kicked them for a high ping, then no need to kick them again.
      return;
    }
  }
}

//===============================
void KickModule::OnSlotTableInitial(NetEvent &e)
{
  e.Skip();
  
  if (Info::Get()->GetGameInfo().bHosted) {
    //wxString sRefresherName = CONFIG(wxString, "AutoRefresh/Name");
    IntStrMap refresherNameMap = CONFIG(IntStrMap, "AutoRefresh/NameMap");
    
    shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
    if (pClient) {
      //Don't attempt to kick if it is the auto refresher
      for (IntStrMap::iterator i = refresherNameMap.begin(); i != refresherNameMap.end(); i++) {
        if (i->second.IsSameAs(pClient->sName, false))
          return; 
      }
      
      //Checks to see if the client should be auto kicked. If they should then they will.
      CheckKick(e.m_pIp, e.m_pTcp); 
  	}
  }
}

//===============================
void KickModule::OnClientSlotKick(NetEvent &e)
{
  e.Skip();
  
  bool bAddKickOnSlotKick = CONFIG(bool, "AutoKick/AddKickOnSlotKick");

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
  if (!pClient) {
    return;
  }

  if (!pClient->sName.IsEmpty()) {
    if (bAddKickOnSlotKick) {
      wxString sReason = "Slot Kicked";
      AddAutoKick(pClient->sIp, sReason);
    }
  }
}

//===============================
void KickModule::OnInitiateMapUpload(NetEvent &e)
{
  e.Skip();
  
  bool bKickOnMapDownload = CONFIG(bool, "AutoKick/KickOnMapDownload");  
  ArrayString asExceptions = CONFIG(ArrayString, "AutoKick/Exceptions");
  
  if (bKickOnMapDownload) {
    shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
    if (!pClient) {
      return;
    }
  
    //If they aren't in the table, return
    if (!SlotManager::Get()->TableContains(pClient))
      return;
    
    //Check if they are on the exception list.
    bool bKickException = false;
    for (int x = 0; x < asExceptions.size(); x++) {
      if (asExceptions[x].IsSameAs(pClient->sName, false)) {
        bKickException = true;
        break;
      }
    }
  
    if (!bKickException)
      KickModule::Kick(pClient, "Map download");
  }
}

//-------------------------------
//Commands

//===============================
DEFINE_COMMAND(KickCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide a client to kick.", OUTPUT_ERROR);
    return;
  }

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(asCmd[1]);
  if (pClient) {
    KickModule::Kick(pClient, static_cast<char*>(pExtra));
  } else {
    Interface::Get()->Output(wxString::Format("Couldn\'t find a client that matches \"%s\"", asCmd[1].c_str()), OUTPUT_ERROR);
  }
}

//===============================
DEFINE_COMMAND(FKickCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide a client to kick.", OUTPUT_ERROR);
    return;
  }

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(asCmd[1]);
  if (pClient) {
    KickModule::Kick(pClient, static_cast<char*>(pExtra), KICK_FORCE);
  } else {
    Interface::Get()->Output(wxString::Format("Couldn\'t find a client that matches \"%s\"", asCmd[1].c_str()), OUTPUT_ERROR);
  }
}

//===============================
DEFINE_COMMAND(KickTeamCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide a team id to kick.", OUTPUT_ERROR);
    return;
  }

  u_long lTeamId;
  asCmd[1].ToULong(&lTeamId);
  int nTeamId = static_cast<int>(lTeamId);

  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    shared_ptr<Client> pClient = table[x]->pClient;
    if (pClient && table[x]->nTeamId == nTeamId) {
      KickModule::Kick(pClient, static_cast<char*>(pExtra));
    }
  }
}

//===============================
DEFINE_COMMAND(KickAllCmd)
{
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    shared_ptr<Client> pClient = table[x]->pClient;
    if (pClient && pClient->nId != ID_HOST) {
      KickModule::Kick(pClient, static_cast<char*>(pExtra));
    }
  }
}

//===============================
DEFINE_COMMAND(KickIpPortCmd)
{
  if (asCmd.size() != 3) {
    Interface::Get()->Output("You must provide an IP and a port to kick.", OUTPUT_ERROR);
    return;
  }

  //Get the IP and port from the command
  wxString sIp = asCmd[1];
  wxString sPort = asCmd[2];
  u_long lPort = 0;
  sPort.ToULong(&lPort);
  int nPort = static_cast<int>(lPort);

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(sIp, nPort);
  if (pClient) {
    KickModule::Kick(pClient, static_cast<char*>(pExtra));
  } else {
    Interface::Get()->Output(wxString::Format("Couldn\'t find a client that matches \"%s:%d\"", sIp.c_str(), nPort), OUTPUT_ERROR);
  }
}
