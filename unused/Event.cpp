#include "Event.h"

//===============================
//This constructor is used to define an actual event.
Event::Event (EventFunction fn, u_char *pMatch, u_int nMatchSize)
  : m_fn(fn), m_pMatch(pMatch), m_nMatchSize(nMatchSize),
  	m_type (evt_Actual)
{
}

//===============================
//This constructor is used to define a virtual event.
//Virtual events don't contain any patterns to match.
Event::Event (EventFunction fn, void *pData)
	m_fn(fn), m_type (evt_Virtual)
{
}

//===============================
Event::~Event ()
{
  //if (m_pMatch) delete []m_pMatch;
}
