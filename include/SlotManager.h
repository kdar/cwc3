#ifndef _SLOTMANAGER_H
#define _SLOTMANAGER_H

#include <wx/string.h>

#include "Slot.h"
#include "Object.h"

typedef shared_ptr<Slot> SlotPtr;
typedef vector<SlotPtr> SlotTable;

//-------------------------------
class SlotManager : public Object<SlotManager>
{
  public:
    SlotManager()
      : m_nMaxSlots(0)
    {}

    static shared_ptr<SlotManager> Get();
    static shared_ptr<SlotManager> Set(shared_ptr<SlotManager> pInstance);

    bool IsTableEmpty() const
    { return m_slotTable.empty(); }

    void Reset()
    { ClearTable(); }

    bool TableContains(const shared_ptr<Client> pClient) const;

    void RemoveClient(shared_ptr<Slot> &pSlot);
    void RemoveClient(const shared_ptr<Client> pClient);

    shared_ptr<Slot> GetSlot(int nSlotId) const;
    shared_ptr<Slot> GetSlot(const shared_ptr<Client> pClient) const;
    shared_ptr<Slot> GetOrCreateSlot(int nSlotId);
    
    wxString GetVersusString(const shared_ptr<Client> pClient = shared_ptr<Client>())
    { return _GetVersusString(pClient); }
    wxString GetPriorVersusString(const shared_ptr<Client> pClient)
    { return _GetVersusString(pClient, true); }

    SlotTable &GetSlotTable()
    { return m_slotTable; }
    
    const SlotTable &GetSlotTable() const
    { return m_slotTable; }

    int GetSlotCount() const
    { return m_slotTable.size(); }

    int GetValidClientCount() const;
    int GetHereClientCount() const;
    
    void SetMaxSlots(int nMax)
    { m_nMaxSlots = nMax; }
  
  protected:
    wxString _GetVersusString(const shared_ptr<Client> pClient = shared_ptr<Client>(), bool bPriorToLeaving = false);
    
    void ClearTable()
    { m_slotTable.clear(); }

  private:
    static shared_ptr<SlotManager> ms_pInstance;

    SlotTable m_slotTable;
    
    int m_nMaxSlots;
};

#endif
