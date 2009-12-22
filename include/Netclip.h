#ifndef _NETCLIP_H
#define _NETCLIP_H

#include <wx/string.h>

#include "Object.h"

//-------------------------------
class Netclip : public Object<Netclip>
{
  public:
    Netclip() {}
    
    static bool Init();
    
    static int Send(const wxString &sData);
    
    static int SendKeys(const wxString &sData)
    { 
      wxString s = "s ";
      s += sData;
      return Send(s);
    }
    
    static int SendVoice(const wxString &sData)
    {
      wxString s = "p ";
      s += sData;
      return Send(s);
    }
    
  private:
    static udp::endpoint m_receiver;
};

#endif
