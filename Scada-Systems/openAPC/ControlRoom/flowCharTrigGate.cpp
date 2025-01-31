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
#include "flowCharTrigGate.h"
#include "oapc_libio.h"


flowCharTrigGate::flowCharTrigGate():flowLogic()
{
   this->data.type=flowObject::FLOW_TYPE_FLOW_CHARTGATE;
   this->data.stdIN=OAPC_CHAR_IO0|OAPC_DIGI_IO1|OAPC_DIGI_IO2|OAPC_DIGI_IO3|OAPC_DIGI_IO4|OAPC_DIGI_IO5|OAPC_DIGI_IO6|OAPC_DIGI_IO7;
   this->data.stdOUT=            OAPC_CHAR_IO1|OAPC_CHAR_IO2|OAPC_CHAR_IO3|OAPC_CHAR_IO4|OAPC_CHAR_IO5|OAPC_CHAR_IO6|OAPC_CHAR_IO7;
#ifdef ENV_PLAYER
   txt=_T("");
   m_txtValid=false;
#endif
}



flowCharTrigGate::~flowCharTrigGate()
{
}



wxString flowCharTrigGate::getDefaultName()
{
   return _T("Character Triggered Gate");
}



#ifdef ENV_EDITOR
wxInt32 flowCharTrigGate::saveDATA(wxFile *FHandle,char chunkName[4],bool isCompiled)
{
   return flowObject::saveDATA(FHandle,chunkName,isCompiled);
}
#else



wxUint64 flowCharTrigGate::getAssignedOutput(wxUint64 input)
{
   if ((m_txtValid) && (input!=OAPC_CHAR_IO0))
   {
      if (input==OAPC_DIGI_IO1) return OAPC_CHAR_IO1;
      if (input==OAPC_DIGI_IO2) return OAPC_CHAR_IO2;
      if (input==OAPC_DIGI_IO3) return OAPC_CHAR_IO3;
      if (input==OAPC_DIGI_IO4) return OAPC_CHAR_IO4;
      if (input==OAPC_DIGI_IO5) return OAPC_CHAR_IO5;
      if (input==OAPC_DIGI_IO6) return OAPC_CHAR_IO6;
      if (input==OAPC_DIGI_IO7) return OAPC_CHAR_IO7;
   }
   return 0;
}



wxByte flowCharTrigGate::setCharInput(FlowConnection *connection,wxString value,wxUint32 *flowThreadID,flowObject *WXUNUSED(object))
{
   if (!threadIDOK(flowThreadID,false)) return 0;
#ifdef ENV_DEBUGGER
   if (connection->targetInputNum!=0)
   {
      g_debugWin->setDebugInformation(this,DEBUG_STOP_COND_ILLEGAL_IO,_T(""));
      return 0;
   }
#else
   connection=connection;
#endif
   txt=value;
   m_txtValid=true;
   return 1;
}



wxString flowCharTrigGate::getCharOutput(FlowConnection *connection,wxInt32 *rcode,wxUint64 WXUNUSED(lastInput))
{
#ifdef ENV_DEBUGGER
   if (connection->sourceOutputNum==0)
   {
      wxASSERT(0);
      g_debugWin->setDebugInformation(this,DEBUG_STOP_COND_ILLEGAL_IO,_T(""));
      *rcode=OAPC_ERROR_NO_SUCH_IO;
      return _T("");
   }
#endif
   if ((!m_txtValid) || (digi[connection->sourceOutputNum]==0)) *rcode=OAPC_ERROR_NO_DATA_AVAILABLE;
   else *rcode=OAPC_OK;
   return txt;
}


#endif


wxInt32 flowCharTrigGate::loadDATA(wxFile *FHandle,wxUint32 chunkSize,wxUint32 IDOffset,bool isCompiled)
{
   return flowObject::loadDATA(FHandle,chunkSize,IDOffset,isCompiled);
}


