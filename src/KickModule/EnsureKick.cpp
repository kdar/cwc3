#include "KickModule/EnsureKick.h"
#include "Client.h"
#include "Info.h"

shared_ptr<EnsureKick> EnsureKick::ms_pInstance = shared_ptr<EnsureKick>();

//===============================
/*static*/ shared_ptr<EnsureKick> EnsureKick::Get()
{
  if (!ms_pInstance)
    ms_pInstance = shared_ptr<EnsureKick>(new EnsureKick(), NullDeleter());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<EnsureKick> EnsureKick::Set(shared_ptr<EnsureKick> pInstance)
{
  shared_ptr<EnsureKick> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
void EnsureKick::Add(shared_ptr<Client> pClient)
{
  wxCriticalSectionLocker locker(m_cs);
  
  m_list.push_back(pClient);
}

//===============================
void *EnsureKick::Entry()
{
  while (!TestDestroy()) {
    Sleep(200);
    
    wxCriticalSectionLocker locker(m_cs);
    
    shared_ptr<Info> pInfo = Info::Get();    
    if (pInfo && (pInfo->GetGameInfo().bExitedGame || pInfo->GetGameInfo().bEndedGame)) {
      m_list.clear();
      Sleep(1000);
    }
    
    for (KickList::iterator i = m_list.begin(); i != m_list.end(); i++) {
      if (*i) {
        if ((*i)->IsHere()) {
          m_tcpKill.Execute((*i)->sIp, (*i)->nPort);
        } else {
          Info::Get()->RemoveNodeInfoEntry((*i)->sIp, (*i)->nPort);
          m_list.erase(i);
          //We can't continue unless we adjust the iterator. But I'll just break
          //and continue later.
          break;
        } 
      }
    }
  }
  
  return 0;
}
