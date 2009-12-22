#include "SlotManager.h"
#include "Client.h"

shared_ptr<SlotManager> SlotManager::ms_pInstance = shared_ptr<SlotManager>();

//===============================
/*static*/ shared_ptr<SlotManager> SlotManager::Get()
{
  if (!ms_pInstance)
    ms_pInstance = shared_ptr<SlotManager>(new SlotManager());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<SlotManager> SlotManager::Set(shared_ptr<SlotManager> pInstance)
{
  shared_ptr<SlotManager> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
bool SlotManager::TableContains(const shared_ptr<Client> pClient) const
{
  if (!pClient) return false;
    
  for (int x = 0; x < m_slotTable.size(); x++) {
    shared_ptr<Client> pItem = m_slotTable[x]->pClient;
    if (pItem && pItem->IsSameAs(*pClient))
      return true;
  }

  return false;
}

//===============================
void SlotManager::RemoveClient(shared_ptr<Slot> &pSlot)
{
  if (pSlot) {
    pSlot->pClient.reset();
    pSlot->sDisplay = "Open";
  }
}

//===============================
void SlotManager::RemoveClient(const shared_ptr<Client> pClient)
{
  for (int x = 0; x < m_slotTable.size(); x++) {
    shared_ptr<Client> pItem = m_slotTable[x]->pClient;
    if (pItem && pItem->IsSameAs(*pClient)) {
      RemoveClient(m_slotTable[x]);
    }
  }
}

//===============================
shared_ptr<Slot> SlotManager::GetSlot(int nSlotId) const
{
  shared_ptr<Slot> pSlot;
  
  for (int x = 0; x < m_slotTable.size(); x++) {
    if (m_slotTable[x]->nSlotId == nSlotId)
      return m_slotTable[x];
  }

  return pSlot;
}

//===============================
shared_ptr<Slot> SlotManager::GetSlot(const shared_ptr<Client> pClient) const
{
  shared_ptr<Slot> pSlot;
  
  if (!pClient) return pSlot;

  for (int x = 0; x < m_slotTable.size(); x++) {
    shared_ptr<Client> pItem = m_slotTable[x]->pClient;
    if (pItem && pItem->IsSameAs(*pClient))
      return m_slotTable[x];
  }

  return pSlot;
}

//===============================
int SlotManager::GetValidClientCount() const
{
  int nCount = 0;

  for (int x = 0; x < m_slotTable.size(); x++) {
    if (m_slotTable[x]->pClient != 0)
      nCount++;
  }

  return nCount;
}

//===============================
int SlotManager::GetHereClientCount() const
{
  int nCount = 0;

  for (int x = 0; x < m_slotTable.size(); x++) {
    if (m_slotTable[x]->pClient != 0 && m_slotTable[x]->pClient->IsHere())
      nCount++;
  }

  return nCount;
}

//===============================
shared_ptr<Slot> SlotManager::GetOrCreateSlot(int nSlotId)
{
  shared_ptr<Slot> pEntry = GetSlot(nSlotId);
  //Only create a slot if the slot doesn't exist, and we
  //aren't passed the maximum number of slots.
  if (!pEntry && m_slotTable.size() < m_nMaxSlots) {
    shared_ptr<Slot> p(new Slot());
    p->nSlotId = nSlotId;
    p->nTeamId = 0;
    p->nKind = KIND_HUMAN;
    p->nSlotStatus = STATUS_OPEN;
    p->nLevel = LEVEL_NORMAL;
    p->nRace = 0;

    p->sDisplay = "Open";
    
    p->pClient = shared_ptr<Client>();

    m_slotTable.push_back(p);
    return p;
  }

  return pEntry;
}

//===============================
//Gets the versus string with the team marked for which pClient belongs to.
//E.g. 5*v4, means the client passed was on the first team with 5 people.
//Only valid clients that are currently playing (didn't leave) are counted.
//This works on the basis that the teamids are in order in the vector. They
//should be because that's how the wc3 game sends them.
//If bPriorToLeaving is set to true, then if pClient is not here anymore, the
//team he/she was on is incremented. This is so we can see the state of the game
//before that person left.
wxString SlotManager::_GetVersusString(const shared_ptr<Client> pClient, bool bPriorToLeaving)
{
  shared_ptr<Slot> pSlot = GetSlot(pClient);
  
  wxString s;
  int nLastTeam = -1;
  int nTeamCount = 0;
  for (int x = 0; x < m_slotTable.size(); x++) {
    shared_ptr<Slot> pSlot = m_slotTable[x];
    int nCurrentTeam = pSlot->nTeamId;
    if (nCurrentTeam != nLastTeam && nLastTeam != -1) {
      s += wxString::Format("%d", nTeamCount);
      if (pSlot && pSlot->nTeamId == nLastTeam)
        s += "*";
      s += "v";
      nLastTeam = nCurrentTeam;
      nTeamCount = 0;
    }    
    
    if (pSlot->pClient && pSlot->pClient->IsHere())
      nTeamCount++;
    else if (pClient && bPriorToLeaving && pSlot->pClient == pClient && !pClient->IsHere())
      nTeamCount++;
    
    nLastTeam = nCurrentTeam;
  }
  
  s += wxString::Format("%d", nTeamCount);
  if (pSlot && pSlot->nTeamId == nLastTeam)
    s += "*";
  
  return s;
}
