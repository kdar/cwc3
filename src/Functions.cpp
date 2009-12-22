#include "Functions.h"

//===============================
//First argument is how many values you are sending to this function.
//The rest is a value for every argument.
//A new string is created on the heap and returned with the corresponding values.
shared_ptr<u_char> ValuesToString(u_int nCount, ...)
{
  if (nCount < 1) return shared_ptr<u_char>();

  va_list arguments;

  va_start(arguments, nCount);

  u_char *pString = new u_char[nCount];

  for (u_int x = 0; x < nCount; x++) {
    int nTmp = va_arg(arguments, int);
    pString[x] = static_cast<u_char>(nTmp);
  }

  va_end(arguments);

  return shared_ptr<u_char>(pString);
}

//===============================
//Simply converts the hex string into decimal and returns it.
u_int ConvertHexToDec(const wxString &sHex)
{
  const wxString sHexMap = "0123456789ABCDEF";

  if (sHex.IsEmpty()) return 0;

  u_int nDecimal = 0;
  for (int x = 0; x < sHex.Length(); x++) {
    int nDec = sHexMap.Find(sHex[x]);
    if (nDec != -1)
      //nDecimal += nDec * pow(16, sHex.Length() - x - 1);
      nDecimal += nDec * (1 << ((sHex.Length() - x - 1) << 2));
  }

  return nDecimal;
}

//===============================
int CreateCharArrayFromWxArrayString(const wxArrayString &wxarray, char **pArray)
{
  int nItems = wxarray.GetCount();
  for (int x = 0; x < nItems; x++) {
    pArray[x] = const_cast<char *>(wxarray[x].c_str());
  }
  return nItems;
}

//===============================
wxString Join(ArrayString::const_iterator begin, ArrayString::const_iterator end, const wxString &by)
{
  wxString s;
  for (ArrayString::const_iterator i = begin; i != end;) {
    s += *i;
    if (++i != end)
      s += by;
  }
  return s;
}

//===============================
wxString Join(const ArrayString &v, const wxString &by)
{
  return Join(v.begin(), v.end(), by);
}
