#ifndef _AUTOREFRESH_H
#define _AUTOREFRESH_H

#include <wx/defs.h>
#include <wx/thread.h>

#include "Object.h"

//-------------------------------
class AutoRefresh : public wxThread, public Object<AutoRefresh>
{
  public:
    AutoRefresh()
      : wxThread(wxTHREAD_DETACHED),
        m_bRun(false)
    { Create(); }
    
    void SetHostedCount(int nCount)
    { m_nHostedCount = nCount; }
    
    //I originally tried to start and stop the thread, but that turned
    //out not to work so well. So we just use a boolean to start and stop
    //the auto refreshing.
    void Start()
    { m_bRun = true; }
    void Stop()
    { m_bRun = false; }

  private:
    virtual void *Entry();
    
    int m_nHostedCount;
    
    volatile bool m_bRun;
};

#endif
