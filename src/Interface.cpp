#include <stdio.h>
#include <wx/tokenzr.h>
#include <wx/arrstr.h>

#include "Interface.h"
#include "Config.h"
#include "Functions.h"
#include "ClientManager.h"
#include "SlotManager.h"
#include "Info.h"
#include "CommandHandler.h"

shared_ptr<Interface> Interface::ms_pInstance = shared_ptr<Interface>();

//-------------------------------
//Callbacks
#ifdef USE_INTERFACE_CDK
static int CB_ChangeFocus(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput);
static int CB_Null(EObjectType cdktype GCC_UNUSED, void *pObject GCC_UNUSED, void *pClientData GCC_UNUSED, chtype cInput GCC_UNUSED);
static int CB_OnClientDisplayChange(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput);
static int CB_OnResize(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput);
#endif
//-------------------------------

//===============================
Interface::Interface()
  : wxThread(wxTHREAD_DETACHED),
    m_bShutdown(false)
{
#ifdef USE_INTERFACE_CDK
  m_bColorLoaded = false;
  m_nColorLoadedTime = -1;
  m_pCurrentFocus = 0;
#endif
  m_pCommandHandler = shared_ptr<CommandHandler>(new CommandHandler(CHT_CONSOLE));
}

//===============================
/*static*/ shared_ptr<Interface> Interface::Get()
{
  if (!ms_pInstance)
    //We must not delete the memory allocated directly, as this is
    //a detatched thread and wxwidgets will take care of it.
    ms_pInstance = shared_ptr<Interface>(new Interface(), NullDeleter());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<Interface> Interface::Set(shared_ptr<Interface> pInstance)
{
  shared_ptr<Interface> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
bool Interface::Initialize()
{
  if (Create() != wxTHREAD_NO_ERROR) {
    OutputText("Could not create Interface thread.", OUTPUT_ERROR);
    return false;
  }

#ifdef USE_INTERFACE_CDK
  m_pCursesWin = initscr();
  m_pCdkScreen = initCDKScreen(m_pCursesWin);
  initCDKColor();

  char *prompt = "</8>#> ";
  m_pCommandEntry = newCDKEntry(m_pCdkScreen, LEFT, BOTTOM,
                                0, prompt, A_BOLD | COLOR_PAIR(8), COLOR_PAIR(8) | ' ',
                                vMIXED, 0, 1, 512, TRUE, FALSE);

  char *aszClockLabel[1] = {"00:00:00"};
  m_pClock = newCDKLabel(m_pCdkScreen, 1, TOP, aszClockLabel, 1, FALSE, FALSE);

  char *aszMapLabel[1] = {"Waiting for map info query..."};
  m_pMapName = newCDKLabel(m_pCdkScreen, 14, TOP, aszMapLabel, 1, FALSE, FALSE);

  char *aszGameLabel[1] = {"Waiting for game creation..."};
  m_pGameName = newCDKLabel(m_pCdkScreen, m_pMapName->boxWidth + 20, TOP, aszGameLabel, 1, FALSE, FALSE);

  char *aszTwelveBlanks[12] = {" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " "};
  m_pClientDisplay = newCDKScroll(m_pCdkScreen, 4, 1, 0, 14, 25,
                                  "",
                                  aszTwelveBlanks, 12,
                                  FALSE, A_BOLD/*A_REVERSE|COLOR_PAIR(32)*/, TRUE, FALSE);
  RefreshClientDisplay();

  m_pClientSlotDisplay = newCDKScroll(m_pCdkScreen, LEFT, 1, 0, 14, 4,
                                      "",
                                      aszTwelveBlanks, 12,
                                      FALSE, 0, TRUE, FALSE);
  RefreshClientSlotDisplay();

  m_pClientInfo = newCDKSwindow(m_pCdkScreen, 4 + m_pClientDisplay->boxWidth,
                                1, 13, COLS - (4 + m_pClientDisplay->boxWidth), NULL, 100, TRUE, FALSE);

  m_pOutput = newCDKSwindow(m_pCdkScreen, LEFT, m_pClientDisplay->boxHeight + BorderOf(m_pClientDisplay),
                            LINES - m_pClientDisplay->boxHeight - m_pCommandEntry->boxHeight - (2 * BorderOf(m_pCommandEntry)),
                            0, NULL, 1000, TRUE, FALSE);

  refreshCDKScreen(m_pCdkScreen);

  //Binds
  bindCDKObject(vENTRY, m_pCommandEntry, KEY_TAB, ::CB_ChangeFocus, 0);
  bindCDKObject(vSCROLL, m_pClientDisplay, KEY_TAB, ::CB_ChangeFocus, 0);
  bindCDKObject(vSCROLL, m_pClientDisplay, KEY_ENTER, ::CB_Null, 0);
  bindCDKObject(vSWINDOW, m_pOutput, KEY_TAB, ::CB_ChangeFocus, 0);
  bindCDKObject(vSWINDOW, m_pOutput, KEY_ENTER, ::CB_Null, 0);

  bindCDKObject(vENTRY, m_pCommandEntry, KEY_RESIZE, ::CB_OnResize, 0);
  bindCDKObject(vSCROLL, m_pClientDisplay, KEY_RESIZE, ::CB_OnResize, 0);
  bindCDKObject(vSWINDOW, m_pOutput, KEY_RESIZE, ::CB_OnResize, 0);

  //Pre/Post process
  setCDKScrollPostProcess(m_pClientDisplay, ::CB_OnClientDisplayChange, 0);

  //Call the resize function since it has the final say in position/size.
  CB_OnResize();
#endif

  return true;
}

//===============================
bool Interface::TestShutdown()
{
  m_bShutdown = TestDestroy();  
  return m_bShutdown;
}

//===============================
void Interface::OnExit()
{
#ifdef USE_INTERFACE_CDK
  //Normally here I would delete all the memory allocated to create
  //each widget by invoking destroyCDK[Widget](). Unfortunately this
  //function will sometimes crash with a call to wtouchln(). The reason
  //being that not only does it free the memory, but it also attempts to
  //delete it from the ncurses screen. We don't really care if we delete it
  //from the ncurses screen because we're exiting anyways. Even though
  //it bugs me deeply with my coding ethics, the OS will take care of the
  //memory for us. Sigh.
  endCDK();
#endif
}

//===============================
//This function does some updating of CDK/Ncurses based on an interval time.
//This is called from a loop in the main thread.
void Interface::OnInterval()
{
#ifdef USE_INTERFACE_CDK
  //Update the clock if the game is started.
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  if (gameInfo.bStartedGame && !gameInfo.bExitedGame && !gameInfo.bEndedGame && !gameInfo.bLoadedGame) {
    char *aszLabel[1] = {"00:00:00"};
    setCDKLabel(m_pClock, aszLabel, 1, ObjOf(m_pClock)->box);
    drawCDKLabel(m_pClock, ObjOf(m_pClock)->box);
  } else if (gameInfo.bLoadedGame && !gameInfo.bExitedGame && !gameInfo.bEndedGame) {
    if (gameInfo.startTime.IsValid()) {
      wxTimeSpan currentDuration = wxDateTime::Now().Subtract(gameInfo.startTime);

      char *aszLabel[1] = {const_cast<char *>(currentDuration.Format("%H:%M:%S").c_str())};
      setCDKLabel(m_pClock, aszLabel, 1, ObjOf(m_pClock)->box);
      drawCDKLabel(m_pClock, ObjOf(m_pClock)->box);
    }
  }
  
  //If we created a game but have not loaded yet, then we want the color effect to 
  //be on and the time reset.
  if (gameInfo.bCreatedGame && !gameInfo.bLoadedGame) {
    m_bColorLoaded = true;
    m_nColorLoadedTime = -1;
  //If the game has loaded, and the color effect is on and the color time is reset, then
  //set the color time. Then we can check if the correct time has elapsed below.
  } else if (gameInfo.bLoadedGame && m_bColorLoaded && m_nColorLoadedTime == -1) {
    m_nColorLoadedTime =  wxDateTime::Now().GetTicks();
  //If the game has loaded, and the color effect is on (and the color time is SET), then 
  //check if we should disable this effect.
  } else if (gameInfo.bLoadedGame && m_bColorLoaded) {
    int nTimePassed = wxDateTime::Now().GetTicks();
    if (nTimePassed - m_nColorLoadedTime > 10) {
      m_bColorLoaded = false;
      RefreshClientDisplay();
    }
  }
  //RefreshClientAll();
#endif
}

//===============================
void *Interface::Entry()
{
#ifdef USE_INTERFACE_TEXT
  while (!TestShutdown()) {
    Sleep(1);
  }
#endif

#ifdef USE_INTERFACE_CDK
  while (!TestShutdown()) {
    //Probably could be done a better way...
    if (!m_pCurrentFocus || m_pCurrentFocus == m_pCommandEntry)
      ActivateCommandEntry();
    else if (m_pCurrentFocus == m_pOutput)
      ActivateOutput();
    else if (m_pCurrentFocus == m_pClientDisplay)
      ActivateClientDisplay();
      
    if (m_bShutdown)
      break;
  }
#endif

  return 0;
}

//===============================
void Interface::Output(const wxString &sOutput, OutputType type)
{
#ifdef USE_INTERFACE_TEXT
  OutputText(sOutput, type);
#endif

#ifdef USE_INTERFACE_CDK
  wxString sFinalOutput;

  if (type == OUTPUT_ERROR) {
    sFinalOutput = wxString::Format("</B/16>Error<!B!16>: %s", sOutput.c_str());
  } else if (type == OUTPUT_WARNING) {
    sFinalOutput = wxString::Format("</B/32>Warning<!B!32>: %s", sOutput.c_str());
  } else if (type == OUTPUT_DEBUG) {
    if (DEBUG == 1)
      sFinalOutput = wxString::Format("</B>DEBUG<!B>: %s", sOutput.c_str());
  } else {
    sFinalOutput = wxString::Format("%s", sOutput.c_str());
  }

  addCDKSwindow(m_pOutput, const_cast<char *>(sFinalOutput.c_str()), BOTTOM);
#endif
}

//===============================
void Interface::OutputText(const wxString &sOutput, OutputType type)
{
  if (type == OUTPUT_ERROR) {
    fprintf(stderr, "Error: %s\n", sOutput.c_str());
  } else if (type == OUTPUT_WARNING) {
    fprintf(stdout, "Warning: %s\n", sOutput.c_str());
  } else if (type == OUTPUT_DEBUG) {
    if (DEBUG == 1)
      fprintf(stdout, "DEBUG: %s\n", sOutput.c_str());
  } else {
    fprintf(stdout, "%s\n", sOutput.c_str());
  }
}

//===============================
wxString Interface::EscapeFormatting(const wxString &s)
{
  wxString sNew = s;

#ifdef USE_INTERFACE_CDK
  //To escape the formatting for cdk, we must find every occurence of
  //'<', and replace it with "<</N>"
  sNew.Replace("<", "<</N>");
#endif

  return sNew;
}

//===============================
void Interface::RefreshScreen()
{
#ifdef USE_INTERFACE_CDK
  refreshCDKScreen(m_pCdkScreen);
  CB_OnResize();
#endif
}

//===============================
void Interface::RefreshClientAll()
{
  RefreshClientDisplay();
  RefreshClientSlotDisplay();
  RefreshClientInfo();
}

//===============================
void Interface::RefreshClientDisplay()
{
#ifdef USE_INTERFACE_CDK
  int nItems = SlotManager::Get()->GetSlotCount();
  char *asListItems[MAX_ITEMS] = {0};
  wxArrayString asTransferItems;
  
  wxString sLoadedFormat = CONFIG(wxString, "Interface/Client/LoadedFormat");

  //The reason we use wxArrayString is the idea of scope. If I don't use this, then I have
  //to use the 'new' operator of C++ to create new memory allocations to a multidimensional
  //char array. This is a pain because I also have to delete the memory afterwards. Instead, I
  //utilize the wxArrayString and add wxString objects to it. I then fill the
  //asListItems array with memory addresses that point to the strings kept in asTransferItems.
  //When this function exits, the memory will be automatically deleted by the object.

  //Fill in the clients.
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    wxString sItem = "";    
    shared_ptr<Client> pClient = table[x]->pClient;    
    
    if (pClient && table[x]->nSlotStatus & STATUS_OCCUPIED) {
      if (!pClient->IsHere())
        sItem += "</R>";
      else if (pClient->flags["core"] & CF_LOADED && m_bColorLoaded)
        sItem += sLoadedFormat;
    }

    sItem += table[x]->sDisplay;

    //Soley used to hold the memory address of our color + item.
    asTransferItems.Add(sItem);
    asListItems[x] = const_cast<char *>(asTransferItems.Last().c_str());
  }
  
  int nSize = table.size();
  //Allows us to "reset" the client display by writing MAX_ITEMS when the table is empty.
  if (!nSize)
    nSize = MAX_ITEMS;
  setCDKScrollItems(m_pClientDisplay, asListItems, nSize, FALSE);
  drawCDKScroll(m_pClientDisplay, TRUE);
#endif
}

//===============================
void Interface::RefreshClientSlotDisplay()
{
#ifdef USE_INTERFACE_CDK
  IntStrMap hshDefaultClientSlotTextMap = CONFIG(IntStrMap, "Interface/Client/Slot/TextMap");
  IntStrMap hshDefaultClientSlotFormatMap = CONFIG(IntStrMap, "Interface/Client/Slot/FormatMap");

  int nItems = SlotManager::Get()->GetSlotCount();
  char *asListItems[MAX_ITEMS];
  wxArrayString asTransferItems;
  
  //Only way I can clear the slots out. For some reason assigning 0 doesn't work right.
  //If you set it to 0, it will just not show the first character.
  for (int x = 0; x < MAX_ITEMS; x++)
    asListItems[x] = "  ";
  
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    //Client *pClient = table.Item(x).pClient;
    wxString sItem = hshDefaultClientSlotFormatMap[table[x]->nSlotId];
    sItem += hshDefaultClientSlotTextMap[table[x]->nSlotId];

    //Soley used to hold the memory address of our color + item.
    asTransferItems.Add(sItem);

    //The array CDK will use;
    asListItems[x] = const_cast<char *>(asTransferItems.Last().c_str());
  }

  setCDKScrollItems(m_pClientSlotDisplay, asListItems, MAX_ITEMS, FALSE);
  drawCDKScroll(m_pClientSlotDisplay, TRUE);
#endif
}

//===============================
void Interface::RefreshClientInfo()
{
#ifdef USE_INTERFACE_CDK
  cleanCDKSwindow(m_pClientInfo);

  int nIndex = getCDKScrollCurrent(m_pClientDisplay);

  char *asListItems[MAX_ITEMS];
  getCDKScrollItems(m_pClientDisplay, asListItems);
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByName(asListItems[nIndex]);
  if (pClient) {
    wxString sClientString = pClient->ToString();
    shared_ptr<Slot> pSlot = SlotManager::Get()->GetSlot(pClient);
    if (pSlot)
      sClientString += pSlot->ToString();
    wxStringTokenizer tokenizer(sClientString, "\n", wxTOKEN_STRTOK);
    while (tokenizer.HasMoreTokens()) {
      wxString sToken = tokenizer.GetNextToken();
      addCDKSwindow(m_pClientInfo, const_cast<char *>(sToken.c_str()), BOTTOM);
    }
  }
#endif
}

//===============================
void Interface::RefreshMapInfo()
{
#ifdef USE_INTERFACE_CDK
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  char *aszLabel[1] = {const_cast<char *>(gameInfo.sMapName.c_str())};
  setCDKLabel(m_pMapName, aszLabel, 1, FALSE);
  RefreshScreen();
#endif
}

//===============================
void Interface::RefreshGameInfo()
{
#ifdef USE_INTERFACE_CDK
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  char *aszLabel[1] = {const_cast<char *>(gameInfo.sGameName.c_str())};
  setCDKLabel(m_pGameName, aszLabel, 1, FALSE);
  RefreshScreen();
#endif
}

#ifdef USE_INTERFACE_CDK
//===============================
int Interface::CB_ChangeFocus(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput)
{
  if (m_pCurrentFocus == m_pCommandEntry) {
    ActivateClientDisplay();
  } else if (m_pCurrentFocus == m_pClientDisplay) {
    ActivateOutput();
  } else {
    ActivateCommandEntry();
  }

  return 0;
}
#endif

#ifdef USE_INTERFACE_CDK
//===============================
int Interface::CB_OnClientDisplayChange(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput)
{
  RefreshClientInfo();
}
#endif

#ifdef USE_INTERFACE_CDK
//===============================
int Interface::CB_OnResize()
{
  int parentWidth = getmaxx(m_pCdkScreen->window);
  int parentHeight = getmaxy(m_pCdkScreen->window);

  m_pClientInfo->boxWidth = parentWidth - (4 + m_pClientDisplay->boxWidth);
  wresize(m_pClientInfo->win, m_pClientInfo->boxHeight, m_pClientInfo->boxWidth);

  m_pOutput->boxWidth = parentWidth;
  m_pOutput->boxHeight = parentHeight - m_pClientDisplay->boxHeight - m_pCommandEntry->boxHeight - (2 * BorderOf(m_pCommandEntry)) + 1;
  wresize(m_pOutput->win, m_pOutput->boxHeight, m_pOutput->boxWidth);

  m_pCommandEntry->boxWidth = parentWidth;
  m_pCommandEntry->boxHeight = 3;
  m_pCommandEntry->fieldWidth = parentWidth - m_pCommandEntry->labelLen - (2 * BorderOf(m_pCommandEntry));
  wresize(m_pCommandEntry->win, m_pCommandEntry->boxHeight, m_pCommandEntry->boxWidth);
  wresize(m_pCommandEntry->fieldWin, 1, m_pCommandEntry->fieldWidth);
  moveCDKEntry(m_pCommandEntry, LEFT, parentHeight - m_pCommandEntry->boxHeight, FALSE, FALSE);

  m_pMapName->boxWidth = parentWidth - 14;
  wresize(m_pMapName->win, m_pMapName->boxHeight, m_pMapName->boxWidth);

  //---Redraw widgets and screen---
  eraseCDKSwindow(m_pOutput);
  drawCDKSwindow(m_pOutput, ObjOf(m_pOutput)->box);

  eraseCDKSwindow(m_pClientInfo);
  drawCDKSwindow(m_pClientInfo, ObjOf(m_pClientInfo)->box);

  eraseCDKEntry(m_pCommandEntry);
  drawCDKEntry(m_pCommandEntry, ObjOf(m_pCommandEntry)->box);

  eraseCDKLabel(m_pMapName);
  drawCDKLabel(m_pMapName, ObjOf(m_pMapName)->box);

  eraseCDKScreen(m_pCdkScreen);
  drawCDKScreen(m_pCdkScreen);

  /*touchwin(m_pCursesWin);
  touchwin(m_pOutput->win);
  touchwin(m_pCommandEntry->win);
  touchwin(m_pClientInfo->win);
  touchwin(m_pClientDisplay->win);
  //wrefresh(m_pCursesWin);

  redrawwin(m_pCursesWin);
  redrawwin(m_pOutput->win);
  redrawwin(m_pCommandEntry->win);
  redrawwin(m_pClientInfo->win);
  redrawwin(m_pClientDisplay->win);*/
}
#endif

//===============================
void Interface::ActivateClientDisplay()
{
#ifdef USE_INTERFACE_CDK
  m_pCurrentFocus = (void *)m_pClientDisplay;
  HighlightFocused();

  int nSelection = activateCDKScroll(m_pClientDisplay, 0);

  drawCDKScroll(m_pClientDisplay, TRUE);
#endif
}

//===============================
void Interface::ActivateOutput()
{
#ifdef USE_INTERFACE_CDK
  m_pCurrentFocus = (void *)m_pOutput;
  HighlightFocused();

  activateCDKSwindow(m_pOutput, 0);
#endif
}

//===============================
void Interface::ActivateCommandEntry()
{
#ifdef USE_INTERFACE_CDK
  m_pCurrentFocus = (void *)m_pCommandEntry;
  HighlightFocused();

  wxString sCommand = activateCDKEntry(m_pCommandEntry, 0);

  Output(wxString::Format("#> %s", sCommand.c_str()));
  m_pCommandHandler->Process(sCommand);

  cleanCDKEntry(m_pCommandEntry);
  eraseCDKEntry(m_pCommandEntry);
  drawCDKEntry(m_pCommandEntry, TRUE);
#endif
}

//===============================
void Interface::HighlightFocused()
{
#ifdef USE_INTERFACE_CDK
  chtype cHighlight = COLOR_PAIR(32) | A_BOLD;
  chtype cNoHighlight = COLOR_PAIR(8);
  setCDKScrollBoxAttribute(m_pClientDisplay, m_pCurrentFocus == m_pClientDisplay ? cHighlight : cNoHighlight);
  drawCDKScroll(m_pClientDisplay, TRUE);

  setCDKSwindowBoxAttribute(m_pOutput, m_pCurrentFocus == m_pOutput ? cHighlight : cNoHighlight);
  drawCDKSwindow(m_pOutput, TRUE);

  setCDKEntryBoxAttribute(m_pCommandEntry, m_pCurrentFocus == m_pCommandEntry ? cHighlight : cNoHighlight);
  drawCDKEntry(m_pCommandEntry, TRUE);
#endif
}

//*******************************
//Callbacks
//*******************************

#ifdef USE_INTERFACE_CDK
//===============================
int CB_ChangeFocus(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput)
{
  return Interface::Get()->CB_ChangeFocus(cdktype, pObject, pClientData, cInput);
}

//===============================
int CB_Null(EObjectType cdktype GCC_UNUSED, void *pObject GCC_UNUSED, void *pClientData GCC_UNUSED, chtype cInput GCC_UNUSED)
{
  return 1;
}

//===============================
int CB_OnClientDisplayChange(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput)
{
  return Interface::Get()->CB_OnClientDisplayChange(cdktype, pObject, pClientData, cInput);
}

//===============================
int CB_OnResize(EObjectType cdktype, void *pObject, void *pClientData, chtype cInput)
{
  return Interface::Get()->CB_OnResize();
}
#endif
