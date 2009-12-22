#ifndef _IPINFO_H
#define _IPINFO_H

#include "Object.h"

//Forward declarations
class Database;

//-------------------------------
class IpInfo : public Object<IpInfo>
{
  public:
    IpInfo();
    
    wxString GetCountry(const wxString &sIp) const;
    
  private:
    u_long IpToLong(const wxString &sIp) const;
    
  private:
    shared_ptr<Database> m_pDb;
    shared_ptr<Database> m_pDb2;
};

#endif
