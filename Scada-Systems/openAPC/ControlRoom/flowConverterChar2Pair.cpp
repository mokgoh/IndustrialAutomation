/******************************************************************************

This file is part of ControlRoom process control/HMI software.

ControlRoom is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

ControlRoom is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
ControlRoom. If not, see http://www.gnu.org/licenses/

*******************************************************************************

For different licensing and/or usage of the sources apart from GPL or any other
open source license, please contact us at https://openapc.com/contact.php

*******************************************************************************/

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/checkbox.h>

#ifndef __WXMSW__
#include <arpa/inet.h>
#endif

#ifdef ENV_DEBUGGER
#include "DebugWin.h"
#endif
#include "iff.h"
#include "globals.h"
#include "flowObject.h"
#include "flowConverter.h"
#include "flowConverter2Pair.h"
#include "flowConverterChar2Pair.h"
#include "oapc_libio.h"

#ifdef ENV_WINDOWS
#ifdef ENV_WINDOWSCE
#include "Winsock2.h"
#endif
#endif



flowConverterChar2Pair::flowConverterChar2Pair():flowConverter2Pair()
{
   this->data.type=flowObject::FLOW_TYPE_CONV_CHAR2PAIR;
   this->data.stdIN=OAPC_CHAR_IO0|OAPC_CHAR_IO1|OAPC_CHAR_IO2|OAPC_CHAR_IO3|OAPC_CHAR_IO4|OAPC_CHAR_IO5|OAPC_CHAR_IO6|OAPC_CHAR_IO7;
   this->data.stdOUT=OAPC_CHAR_IO0|OAPC_CHAR_IO1;
#ifdef ENV_PLAYER
#endif
#ifdef ENV_EDITOR

#endif
}



flowConverterChar2Pair::~flowConverterChar2Pair()
{
}



wxString flowConverterChar2Pair::getDefaultName()
{
   return _T("Convert Char2Pair");
}



#ifndef ENV_PLAYER



#else



wxUint64 flowConverterChar2Pair::getAssignedOutput(wxUint64 WXUNUSED(input))
{
   return OAPC_CHAR_IO0|OAPC_CHAR_IO1;
}



wxByte flowConverterChar2Pair::setCharInput(FlowConnection *connection,wxString value,wxUint32 *flowThreadID,flowObject *WXUNUSED(object))
{
   if (!threadIDOK(flowThreadID,false)) return 0;
   m_lastUsedInput=connection->targetInputNum;
   txt[connection->targetInputNum]=value;
   return 1;
}



wxString flowConverterChar2Pair::getCharOutput(FlowConnection *connection,wxInt32 *rcode,wxUint64 lastInput)
{
   if (connection->sourceOutputNum==0)
      return flowConverter2Pair::getCharOutput(connection,rcode,lastInput);
#ifdef ENV_DEBUGGER
   else if (connection->sourceOutputNum!=1)
   {
      wxASSERT(0);
      g_debugWin->setDebugInformation(this,DEBUG_STOP_COND_ILLEGAL_IO,_T(""));
      *rcode=OAPC_ERROR_NO_SUCH_IO;
      return _T("");
   }
#endif
   *rcode=OAPC_OK;
   return txt[m_lastUsedInput];
}



#endif



