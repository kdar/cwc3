#include "Client.h"

//===============================
wxString Client::ToString() const
{
  wxString sString = "";

  sString += wxString::Format("Name:       %s\n",  sName.c_str());
  sString += wxString::Format("Location:   %s:%d %s\n", sIp.c_str(), nPort, sCountry.c_str());
  sString += wxString::Format("Id:         %hd\n", nId);
  sString += wxString::Format("Ping:       %u\n",  ping.GetMilliseconds().GetValue());
  if (!sRatio.IsEmpty())
    sString += wxString::Format("Ratio:      %s\n",  sRatio.c_str());
    
  StrIntMap::const_iterator i = flags.find("core");
  int nFlags = i->second;

  wxString sFlags = "";
  if (CF_NEW & nFlags)
    sFlags += "New ";
  if (CF_OLD & nFlags)
    sFlags += "Old ";
  if (CF_LEFT & nFlags)
    sFlags += "Left ";
  //if (CF_KICKED & nFlags)
  //  sFlags += "Kicked ";
  if (CF_PINGING & nFlags)
    sFlags += "Pinging ";
  if (CF_PINGED & nFlags)
    sFlags += "Pinged ";
  if (CF_HOST & nFlags)
    sFlags += "Host ";
  if (CF_DISCONNECTED & nFlags)
    sFlags += "Disconnected ";
  if (CF_DOWNLOADING & nFlags)
    sFlags += "Downloading ";
  if (CF_LOADED & nFlags)
    sFlags += "Loaded ";

  sString += wxString::Format("Flags:      %s\n",  sFlags.c_str());
    
  for (StrStrMap::const_iterator i = extraInfo.begin(); i != extraInfo.end(); i++) {
    wxString sTmp = i->first + ":";
    //Capitalize the first letter. Why is there no ucfirst?
    sTmp = sTmp.Mid(0, 1).Upper() + sTmp.Mid(1);
    sString += wxString::Format("%-12s%s\n", sTmp.c_str(), i->second.c_str());
  }

  return sString;
}

//===============================
bool Client::IsSameAs(const Client &other) const
{
  bool bRet = false;
  /*if (!other.sIp.IsEmpty() && !sIp.IsEmpty()) {
   bRet = other.sIp.IsSameAs(sIp) && other.nPort == nPort;
  } else {
   //bRet = other.sName.IsSameAs(sName) && other.nId == nId;
  }*/

  //Ports can change, so we can't go by just the ip and port.
  //The ip and their id seem like a good key.
  //bRet = sIp.IsSameAs(other.sIp) && other.nId == nId;
  bRet = other.sName.IsSameAs(sName) && other.nId == nId;

  //bRet = other.sIp.IsSameAs(sIp) && other.sName.IsSameAs(sName);

  return bRet;
}

//===============================
bool Client::operator==(const Client &other) const
{
  return IsSameAs(other);
}

//===============================
bool Client::operator!=(const Client &other) const
{
  return !IsSameAs(other);
}

//===============================
bool Client::IsHere() const
{
  StrIntMap::const_iterator i = flags.find("core");
  int nFlags = i->second;
  return (nFlags & CF_OLD) && !(/*nFlags & CF_KICKED || */nFlags & CF_LEFT/* || nFlags & CF_DISCONNECTED*/);
}

//===============================
int Client::GetPing() const
{
  wxTimeSpan tsPing = ping.GetMilliseconds();
  wxLongLong lPing = tsPing.GetValue();
  
  int nPing = static_cast<int>(lPing.ToDouble());
  return nPing;
}
