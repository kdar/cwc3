#ifndef _CONFIG_H
#define _CONFIG_H

#include <wx/fileconf.h>
#include <wx/hashmap.h>

#include "Object.h"

#define CONFIG(type, str) (any_cast<type>(Config::Get()->GetValue(str)))

//-------------------------------
class Config : public Object<Config>
{
  public:
    Config(const wxString &sFileName = _T("cwc3.conf"))
        : m_pConfig(shared_ptr<wxFileConfig>()), m_sFileName(sFileName)
    { Load(sFileName); }

    static shared_ptr<Config> Get();
    static shared_ptr<Config> Set(shared_ptr<Config> pInstance);

    bool Load(const wxString &sFileName);
    bool Reload();
    
    any &operator [](const wxString &sKey)
    { return m_map[sKey]; }
      
    any &GetValue(const wxString &sKey)
    { return m_map[sKey]; }
    
    StrAnyMap &GetMap()
    { return m_map; }
    
    const StrAnyMap &GetMap() const
    { return m_map; }

    bool GetBool(const wxString &sPath, bool bDefault = false);
    int GetInt(const wxString &sPath, int nDefault = 0);
    wxString GetString(const wxString &sPath, const wxString &sDefault = wxEmptyString);

    ArrayInt GetArrayInt(const wxString &sPath, const wxString &sDefault = wxEmptyString);
    ArrayInt GetArrayIntHexToDec(const wxString &sPath, const wxString &sDefault = wxEmptyString);
    ArrayString GetArrayString(const wxString &sPath, const wxString &sDefault = wxEmptyString);
    IntStrMap GetMapByIndexing(const wxString &sPath, int nStartNumber, int nSize, const char *asDefault[] = static_cast<const char **>(NULL));
    StrStrMap GetMapByStrings(const wxString &sPath, int nSize = 0, 
                              const char *asDefaultKeys[] = static_cast<const char **>(NULL), 
                              const char *asDefaultValues[] = static_cast<const char **>(NULL));

  private:
    shared_ptr<wxFileConfig> m_pConfig;

    wxString m_sFileName;
    
    StrAnyMap m_map;

    static shared_ptr<Config> ms_pInstance;
};

#endif
