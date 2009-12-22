#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <stdarg.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/string.h>

#include "Lib.h"

shared_ptr<u_char> ValuesToString(u_int nCount, ...);

u_int ConvertHexToDec(const wxString &sHex);

int CreateCharArrayFromWxArrayString(const wxArrayString &wxarray, char **pArray);

wxString Join(ArrayString::const_iterator begin, ArrayString::const_iterator end, const wxString &by = " ");
wxString Join(const ArrayString &v, const wxString &by = " ");

#endif
