#ifndef _CLIENT_H
#define _CLIENT_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/datetime.h>

#include "Object.h"

#define INVALID_VALUE 0xFF
#define ID_HOST 1

//-------------------------------
enum ClientFlagType
{
  CF_NEW          = 0x00000001,
  CF_OLD          = 0x00000002,
  CF_LEFT         = 0x00000004,
  //CF_KICKED       = 0x00000008,
  CF_PINGING      = 0x00000010,
  CF_PINGED       = 0x00000020,
  CF_HOST         = 0x00000040,
  CF_DISCONNECTED = 0x00000080,
  CF_DOWNLOADING  = 0x00000100,
  CF_LOADED       = 0x00000200
};

//-------------------------------
class Client : public Object<Client>
{
  public:
    Client()
        : sName(_T("")), sIp(_T("")), sCountry(_T("")),
        nId(INVALID_VALUE), nPort(0), nLeftCount(0),
        joined(wxDateTime::UNow())
    { flags[_T("core")] = 0; }

    wxString ToString() const;

    bool IsSameAs(const Client &other) const;
    bool operator==(const Client &other) const;
    bool operator!=(const Client &other) const;
    
    bool IsHere() const;
    
    void Left()
    { flags[_T("core")] |= CF_LEFT; left = wxDateTime::UNow(); }
    
    //void Kicked()
    //{ flags["core"] |= CF_KICKED; left = wxDateTime::UNow(); }
    
    void Disconnected()
    { flags[_T("core")] |= CF_DISCONNECTED; left = wxDateTime::UNow(); }
      
    int GetPing() const;

  public:
    wxString sName;
    wxString sIp;
    wxString sCountry;

    int nId;

    int nPort;
    
    //This is a map of strings mapping integers, because other modules
    //may need to have flags of their own for the client. The flags
    //that the core of cwc3 uses is flags["core"]
    StrIntMap flags;

    wxDateTime joined;
    wxDateTime left;
    wxTimeSpan ping;
    
    //The amount of time it took this client to load.
    wxTimeSpan loadTime;
    
    //Complicated, look at OnClientEnd in Game.cpp
    int nLeftCount;
    
    //The ratio right before the client left. This is of the form 4*v5 where the
    //aterisk indicates which team this client was on.
    wxString sRatio;
    
    //Extra information that modules and other things can assign to this client.
    //This is purely for information and not for functionality.
    StrStrMap extraInfo;
};

#endif
