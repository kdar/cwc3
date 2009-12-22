#ifndef _SLOT_H
#define _SLOT_H

#include <wx/string.h>

#include "Object.h"

class Client;

//-------------------------------
enum SlotTeamType
{
  TEAM_SENTINEL = 0,
  TEAM_SCOURGE = 1
};

//-------------------------------
enum SlotKindType
{
  KIND_HUMAN = 0,
  KIND_COMPUTER = 1
};

//-------------------------------
enum SlotLevelType
{
  LEVEL_EASY = 0,
  LEVEL_NORMAL = 1,
  LEVEL_INSANE = 2
};

//-------------------------------
enum SlotRaceType
{
  RACE_HUMAN = 4,
  RACE_UNDEAD = 8
};

//-------------------------------
enum SlotStatusType
{
  STATUS_OPEN = 0,
  STATUS_CLOSED = 1,
  STATUS_OCCUPIED = 2
};

//-------------------------------
class Slot : public Object<Slot>
{
  public:
    Slot() {}

    wxString ToString() const;

  public:
    int nSlotId;
    int nTeamId;
    int nKind;
    int nSlotStatus;
    int nLevel;
    int nRace;

    //What is displayed in that slot (the client's name or slot status)
    wxString sDisplay;

    //int nColorId;
    shared_ptr<Client> pClient;
    //Client *pClient;
};

#endif
