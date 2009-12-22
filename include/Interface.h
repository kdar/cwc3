#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/event.h>

#include "Object.h"

//If you include this before boost/asio.hpp, we get stdscr errors..
//WHY?
#ifdef USE_INTERFACE_CDK
#include <cdk/cdk.h>
#endif

//Forward declarations
class CommandHandler;

//-------------------------------
enum OutputType
{
  OUTPUT_NORMAL = 0,
  OUTPUT_ERROR,
  OUTPUT_WARNING,
  OUTPUT_DEBUG
};

//-------------------------------
class Interface : public wxThread, public Object<Interface>
{
  public:
    Interface();

    static shared_ptr<Interface> Get();
    static shared_ptr<Interface> Set(shared_ptr<Interface> pInstance);

    bool Initialize();
    void Shutdown() {}
    bool TestShutdown();
    void OnExit();
    
    void OnInterval();

    void Output(const wxString &sOutput, OutputType type = OUTPUT_NORMAL);
    void OutputText(const wxString &sOutput, OutputType type = OUTPUT_NORMAL);
    
    wxString EscapeFormatting(const wxString &s);

    void RefreshScreen();
    void RefreshClientAll();
    void RefreshClientDisplay();
    void RefreshClientSlotDisplay();
    void RefreshClientInfo();
    void RefreshMapInfo();
    void RefreshGameInfo();

#ifdef USE_INTERFACE_CDK
    int CB_ChangeFocus(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput);
    int CB_OnClientDisplayChange(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput);
    int CB_OnResize();
#endif

  protected:
    void ActivateClientDisplay();
    void ActivateOutput();
    void ActivateCommandEntry();
    void HighlightFocused();

  private:
    virtual void *Entry();    

  private:
    static shared_ptr<Interface> ms_pInstance;

    volatile bool m_bShutdown;

    shared_ptr<CommandHandler> m_pCommandHandler;

#ifdef USE_INTERFACE_CDK
    CDKSCREEN *m_pCdkScreen;
    CDKSWINDOW *m_pOutput;
    CDKENTRY *m_pCommandEntry;
    WINDOW *m_pCursesWin;
    CDKLABEL *m_pClock;
    CDKLABEL *m_pMapName;
    CDKLABEL *m_pGameName;
    CDKSCROLL *m_pClientDisplay;
    CDKSCROLL *m_pClientSlotDisplay;
    CDKSWINDOW *m_pClientInfo;

    void *m_pCurrentFocus;
    
    bool m_bColorLoaded;
    int m_nColorLoadedTime;    
#endif
};

#endif
