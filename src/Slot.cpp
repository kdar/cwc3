#include "Slot.h"
#include "Client.h"
#include "Config.h"

//===============================
wxString Slot::ToString() const
{
  wxString sString = "";

  wxString sColor = CONFIG(IntStrMap, "Client/ColorMap")[nSlotId];
  wxString sTeamName = CONFIG(IntStrMap, "Client/TeamMap")[nTeamId];

  sString += wxString::Format("Slot:       %hd\n", nSlotId);
  sString += wxString::Format("Color:      %s\n",  sColor.c_str());
  sString += wxString::Format("Team:       %s (%hd)\n", sTeamName.c_str(), nTeamId);

  return sString;
}
