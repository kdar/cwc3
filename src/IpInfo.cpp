#include <wx/tokenzr.h>

#include "IpInfo.h"
#include "Config.h"
#include "Database.h"

//===============================
IpInfo::IpInfo()
{
  wxString sFile = CONFIG(wxString, "Ip2Country/DbFile");
  m_pDb = shared_ptr<Database>(new Database(":memory:"));  
  
  m_pDb->Exec("PRAGMA temp_store=2");
  //Load our database into memory.
  m_pDb->Exec(wxString::Format("ATTACH \"%s\" as local", sFile.c_str()).c_str());
}

//===============================
//Queries taken from: http://ip-to-country.webhosting.info/node/view/384
//Possible alternative to a db: http://weirdsilence.net/software/ip2c/
wxString IpInfo::GetCountry(const wxString &sIp) const
{
  wxString sCountry = "UNKNOWN";
  
  u_long lIp = IpToLong(sIp);
  //Bad select:
  //wxString sSelect = wxString::Format("SELECT ctry FROM ip2country WHERE %lu between ipfrom and ipto", lIp);
  //Better select:
  //Note that this always returns a result, even when it shouldn't.
  wxString sSelect = wxString::Format("SELECT ctry FROM ip2country WHERE ipfrom <= %lu ORDER BY ipfrom DESC LIMIT 1", lIp);
  //Another select:
  //wxString sSelect = wxString::Format("SELECT ctry FROM (SELECT * FROM ip2country WHERE ipfrom <= %lu ORDER BY ipfrom DESC LIMIT 1) AS 'a' WHERE ipto >= %lu", lIp, lIp);
  
  m_pDb->Exec(sSelect.c_str());  
  
  /*if (sql.vcol.size() > 0) {
    std::cout << "Headings" << std::endl;
    copy(sql.vcol.begin(),sql.vcol.end(),std::ostream_iterator<std::string>(std::cout,"\t")); 
    std::cout << std::endl << std::endl;
    std::cout << "Data" << std::endl;
    copy(sql.vdata.begin(),sql.vdata.end(),std::ostream_iterator<std::string>(std::cout,"\t")); 
    std::cout << std::endl;
  }*/
  
  if (m_pDb->vdata.size() > 0) {
    sCountry = m_pDb->vdata[0];
  }
  
  return sCountry;
}

//===============================
//b4, b3, b2, b1 = ip.split('.')
//long( b4 ) << 24 ) | ( long( b3 ) << 16 ) | ( long( b2 ) << 8 ) | long( b1 )
u_long IpInfo::IpToLong(const wxString &sIp) const
{
  u_long lIp = 0;
  
  wxStringTokenizer tkz(sIp, wxT("."));
  while (tkz.HasMoreTokens()) {
    wxString sToken = tkz.GetNextToken();
  
    u_long lSection;
    sToken.ToULong(&lSection);
    
    lIp += lSection;
    if (tkz.HasMoreTokens())
      lIp *= 256;
  }
  
  return lIp;
}
