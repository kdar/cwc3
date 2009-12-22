#include "ClientManager.h"

shared_ptr<ClientManager> ClientManager::ms_pInstance = shared_ptr<ClientManager>();

//===============================
/*static*/ shared_ptr<ClientManager> ClientManager::Get()
{
  if (!ms_pInstance)
    ms_pInstance = shared_ptr<ClientManager>(new ClientManager());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<ClientManager> ClientManager::Set(shared_ptr<ClientManager> pInstance)
{
  shared_ptr<ClientManager> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
shared_ptr<Client> ClientManager::AddClient(const wxString &sName)
{
  shared_ptr<Client> pNewClient(new Client());
  pNewClient->sName = sName;

  m_clientArray.push_back(pNewClient);

  return *(m_clientArray.end() - 1);
}

//===============================
shared_ptr<Client> ClientManager::AddClient(const wxString &sIp, int nPort)
{
  shared_ptr<Client> pNewClient(new Client());
  pNewClient->sIp = sIp;
  pNewClient->nPort = nPort;

  m_clientArray.push_back(pNewClient);

  return *(m_clientArray.end() - 1);
}

//===============================
void ClientManager::RemoveClientByIpPort(const wxString &sIp, int nPort)
{
  shared_ptr<Client> pClient = GetClientByIpPort(sIp, nPort);
  RemoveClient(pClient);
}

//===============================
void ClientManager::RemoveClientById(int nId)
{
  shared_ptr<Client> pClient = GetClientById(nId);
  RemoveClient(pClient);
}

//===============================
//A reference to a pointer is passed so we are able to set what the variable
//is pointing to, to null.
void ClientManager::RemoveClient(shared_ptr<Client> &pOldClient)
{
  if (!pOldClient) return;

  /*int nIndex = m_clientList.Index(pOldClient);
  if (nIndex != wxNOT_FOUND) {
    m_clientList.RemoveAt(nIndex);
  }*/

  //For some reason the above does not work. Not sure what's happening, but it never removes the
  //client from the list.
  for (int i = 0; i < m_clientArray.size(); i++) {
    shared_ptr<Client> p = m_clientArray[i];
    if (pOldClient->IsSameAs(*p)) {
      m_clientArray.erase(m_clientArray.begin() + i);
    }
  }

  pOldClient.reset();
}

//===============================
shared_ptr<Client> ClientManager::GetClientByIpPort(const wxString &sIp, int nPort) const
{
  shared_ptr<Client> pClient;
  wxDateTime max;
  
  if (sIp.IsEmpty() || nPort < 0) return pClient;

  for (ClientArray::const_iterator i = m_clientArray.begin(); i != m_clientArray.end(); i++) {
    if ((*i)->sIp.IsSameAs(sIp) && (*i)->nPort == nPort) {
      if (!max.IsValid() || (*i)->joined.IsLaterThan(max)) {
        max = (*i)->joined;
        pClient = *i;
      }
    }
  }

  return pClient;
}

//===============================
//This function is pretty special. The reason being an ID number
//is not particularly unique.. What can happen sometimes is a client leaves
//before the game is started and another client joins. Sometimes this program does
//not catch that the client left. Now when the new client joins he will gets the same
//ID as the client who left. This is generally why we use timestamps to see who is who.
//We MAY want to remove the old client if pRetClient != NULL && pClient->joined.IsLaterThan(max).
//If we do that we would have to print a disconnect message as well.
shared_ptr<Client> ClientManager::GetClientById(int nId) const
{
  shared_ptr<Client> pClient;
  wxDateTime max;

  for (ClientArray::const_iterator i = m_clientArray.begin(); i != m_clientArray.end(); i++) {
    if ((*i)->nId == nId) {
      if (!max.IsValid() || (*i)->joined.IsLaterThan(max)) {
        max = (*i)->joined;
        pClient = *i;
      }
    }
  }

  return pClient;
}

//===============================
shared_ptr<Client> ClientManager::GetClientByName(const wxString &sName) const
{
  shared_ptr<Client> pClient;
  wxDateTime max;
  
  if (sName.IsEmpty()) return pClient;

  for (ClientArray::const_iterator i = m_clientArray.begin(); i != m_clientArray.end(); i++) {
    if ((*i)->sName.IsSameAs(sName)) {
      if (!max.IsValid() || (*i)->joined.IsLaterThan(max)) {
        max = (*i)->joined;
        pClient = *i;
      }
    }
  }

  return pClient;
}

//===============================
shared_ptr<Client> ClientManager::GetClientByNameMatch(const wxString &sNameMatch) const
{
  shared_ptr<Client> pClient;
  wxDateTime max;
  
  if (sNameMatch.IsEmpty()) return pClient;

  for (ClientArray::const_iterator i = m_clientArray.begin(); i != m_clientArray.end(); i++) {
    if ((*i)->sName.Lower().Find(sNameMatch.Lower()) != -1) {
      if (!max.IsValid() || (*i)->joined.IsLaterThan(max)) {
        max = (*i)->joined;
        pClient = *i;
      }
    }
  }

  return pClient;
}

//===============================
void ClientManager::SetClientRatios(const wxString &sRatio)
{
  for (ClientArray::iterator i = m_clientArray.begin(); i != m_clientArray.end(); i++) {
    if (*i && (*i)->IsHere()) {
      (*i)->sRatio = sRatio; 
    }
  }
}

//===============================
void ClientManager::FillAdditionalInfo(shared_ptr<Client> pClient) const
{
  if (!pClient) return;
  if (pClient->sIp.Length() > 0) {    
    pClient->sCountry = m_ipInfo.GetCountry(pClient->sIp);
  }
}
