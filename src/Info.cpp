#include "Info.h"

shared_ptr<Info> Info::ms_pInstance = shared_ptr<Info>();

//===============================
/*static*/ shared_ptr<Info> Info::Get()
{ 
  if (!ms_pInstance) 
    ms_pInstance = shared_ptr<Info>(new Info());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<Info> Info::Set(shared_ptr<Info> pInstance)
{
  shared_ptr<Info> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
void GameInfo::Reset()
{
  sMapName = "";
  sGameName = "";
  bHosted = false;
  bPrivate = false;
  nHostedCount = 0;
  
  herePerTeamCount.clear();
  
  //Set all states to false.
  bCreatingGame = false;
  bCreatedGame  = false;
  bStartingGame = false;
  bStartedGame  = false;
  bLoadedGame   = false;
  bExitedGame   = false;
  bEndedGame    = false;
}

//===============================
shared_ptr<NodeInfo> Info::GetNodeInfo(const wxString &sIp, int nPort)
{
  wxString sKey = wxString::Format("%s:%d", sIp.c_str(), nPort);
  if (m_nodeInfoMap.find(sKey) == m_nodeInfoMap.end())
    return shared_ptr<NodeInfo>();

  return m_nodeInfoMap[sKey];
}

//===============================
void Info::ClearNodeInfoMap()
{
  m_nodeInfoMap.clear();
}

//===============================
void Info::RemoveNodeInfoEntry(const wxString &sIp, int nPort)
{
  wxString sKey = wxString::Format("%s:%d", sIp.c_str(), nPort);
  m_nodeInfoMap.erase(sKey);
}
