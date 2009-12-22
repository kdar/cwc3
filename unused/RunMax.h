#ifndef _RUNMAX_H
#define _RUNMAX_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>

//-------------------------------
class RunMax : public wxThread
{
  public:
    RunMax ();
    ~RunMax ();

    bool Execute (const int nSeconds, const wxString &sCommand);

  private:
    virtual void *Entry ();

    int m_nSeconds;
    wxString m_sCommand;
};

#endif
