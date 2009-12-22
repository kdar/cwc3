#ifndef _CLIENTMANAGER_H
#define _CLIENTMANAGER_H

#include <wx/defs.h>
#include <wx/string.h>

#include "IpInfo.h"
#include "Client.h"

#include "Object.h"

typedef shared_ptr<Client> ClientPtr;
typedef vector<ClientPtr> ClientArray;

//-------------------------------
class ClientManager : public Object<ClientManager>
{
  public:
    ClientManager()
    {}

    static shared_ptr<ClientManager> Get();
    static shared_ptr<ClientManager> Set(shared_ptr<ClientManager> pInstance);

    shared_ptr<Client> AddClient(const wxString &sName);
    shared_ptr<Client> AddClient(const wxString &sIp, int nPort);

    void RemoveClientByIpPort(const wxString &sIp, int nPort);
    void RemoveClientById(int nId);
    void RemoveClient(shared_ptr<Client> &pOldClient);

    shared_ptr<Client> GetClientByIpPort(const wxString &sIp, int nPort) const;
    shared_ptr<Client> GetClientById(int nId) const;
    shared_ptr<Client> GetClientByName(const wxString &sName) const;
    shared_ptr<Client> GetClientByNameMatch(const wxString &sNameMatch) const;
    
    void SetClientRatios(const wxString &sRatio);

    ClientArray &GetClientArray()
    { return m_clientArray; }

    const ClientArray &GetClientArray() const
    { return m_clientArray; }

    int GetClientCount() const
    { return m_clientArray.size(); }

    void Reset()
    { ClearClients(); }

    void FillAdditionalInfo(shared_ptr<Client> pClient) const;

  protected:
    void ClearClients()
    { m_clientArray.clear(); }

  private:
    static shared_ptr<ClientManager> ms_pInstance;

    ClientArray m_clientArray;

    IpInfo m_ipInfo;
};

#endif
