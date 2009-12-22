#ifndef _LIB_H
#define _LIB_H

#include <wx/string.h>

#include <vector>
#include <map>
#include <string>
#include <iterator>
#include <iostream>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/utility.hpp>
#include <boost/any.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/asio.hpp>
#include <boost/typeof/typeof.hpp>

using namespace std;
using namespace boost;

using asio::ip::tcp;
using asio::ip::udp;

//-------------------------------
//Used when creating a smart pointer to an object
//on the stack. We do not want to delete it since the
//C++ will do it when it's out of scope automatically.
struct NullDeleter
{
  void operator()(void const *) const
  {
  }
};

//For convenience
//typedef tokenizer< char_zeparator<char> > Strtok;
//typedef char_separator<char> delimeter;
typedef vector<int> ArrayInt;
typedef vector<wxString> ArrayString;
typedef map<int, wxString> IntStrMap;
typedef map<wxString, int> StrIntMap;
typedef map<wxString, any> StrAnyMap;
typedef map<wxString, wxString> StrStrMap;
typedef map<int, int> IntIntMap;

#endif
