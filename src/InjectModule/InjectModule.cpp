#include <libnet.h>

#include "InjectModule/InjectModule.h"
#include "KickModule/EnsureKick.h"
#include "CommandHandler.h"
#include "Game.h"
#include "Info.h"
#include "Config.h"

//===============================
InjectModule::InjectModule()
  : Module()
{
}

//===============================
InjectModule::~InjectModule()
{
}

//===============================
void InjectModule::Initialize(shared_ptr<Game> pGame)
{
  if (!pGame) return;
    
  m_pGame = pGame;
    
  shared_ptr<CommandHandler> pCmdHandler = pGame->GetCommandHandler();
  
  //Register commands to the command handler.
  if (pCmdHandler) {    
    BEGIN_REGISTER_COMMANDS(pCmdHandler);
  
    REGISTER_COMMAND(InjectCmd);
    
    END_REGISTER_COMMANDS();
  }
}

//===============================
void InjectModule::Shutdown()
{
}

//-------------------------------
//Commands

//===============================
DEFINE_COMMAND(InjectCmd)
{
  /*if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide a client to kick.", OUTPUT_ERROR);
    return;
  }*/

  /*char szErrBuf[LIBNET_ERRBUF_SIZE];
  libnet_ptag_t ip;
  libnet_ptag_t tcp;

  wxString sSniffDevice = CONFIG(wxString, "Network/SniffDevice");
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  int nGamePort = CONFIG(int, "Network/GamePort");

  shared_ptr<RealmInfo> pRealmInfo = Info::Get()->GetRealmInfo();

  shared_ptr<NodeInfo> pNodeInfo = Info::Get()->GetNodeInfo(pRealmInfo->sIp, 6112);
  if (!pNodeInfo) {
    //Interface::Get()->Output(wxString::Format("Can't find %s:%d in net info map.", m_sIp.c_str(), m_nPort), OUTPUT_ERROR);
    return;
  }

  NetInfo &netInfo = Info::Get()->GetNetInfo();

  //This is pretty inefficient since I constantly init libnet and destroy it. It would be better
  //if we could init once and just keep changing the sequence number and destination ip.
  //for (u_int x = 0; x < m_nSeverity; x++) {
    u_int32_t seq_out = pNodeInfo->last_sentack;
    u_int32_t ack_out = pNodeInfo->last_sentseq;

    libnet_t *pLibnet = libnet_init(LIBNET_LINK, const_cast<char *>(sSniffDevice.c_str()), szErrBuf);
    libnet_seed_prand(pLibnet);

    tcp = libnet_build_tcp(nPort, nGamePort,
                         seq_out, 0, TH_RST, 0, 0, 0, LIBNET_TCP_H, NULL, 0, pLibnet, 0);

    u_long src_ip = libnet_name2addr4(pLibnet, const_cast<char *>(sIp.c_str()), LIBNET_DONT_RESOLVE);
    u_long dst_ip = libnet_name2addr4(pLibnet, const_cast<char *>(sGameHost.c_str()), LIBNET_DONT_RESOLVE);

    ip = libnet_build_ipv4(sizeof(net_ip) + sizeof(net_tcp), 0,
                          libnet_get_prand(LIBNET_PRu16), 0, 64, IPPROTO_TCP, 0,
                          src_ip, dst_ip, NULL, 0, pLibnet, 0);

    libnet_autobuild_ethernet(netInfo.gamehost_ether, ETHERTYPE_IP, pLibnet);

    libnet_write(pLibnet);
    libnet_destroy(pLibnet);*/
  //}
}
