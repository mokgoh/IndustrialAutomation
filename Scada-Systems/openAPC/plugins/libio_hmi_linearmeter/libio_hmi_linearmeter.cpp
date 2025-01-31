/********************************************************************************************

These sources are distributed in the hope that they will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. All
the information given here, within the interface descriptions and within the specification
are subject to change without notice. Errors and omissions excepted.

These sources demonstrate the usage of the OpenAPC Plug-In programming interface. They can be
used freely according to the OpenAPC Dual License: As long as the sources and the resulting
applications/libraries/Plug-Ins are used together with the OpenAPC software, they are
licensed as freeware. When you use them outside the OpenAPC software they are licensed under
the terms of the GNU General Public License.

For more information please refer to http://www.openapc.com/oapc_license.php

*********************************************************************************************/



/********************************************************************************************

This control was inspired by:

// Name:        LinearMeter.cpp
// Purpose:     wxIndustrialControls Library
// Author:      Marco Cavallini <m.cavallini AT koansoftware.com>
// Modified by: 
// Copyright:   (C)2004-2006 Copyright by Koan s.a.s. - www.koansoftware.com
// Licence:     KWIC License http://www.koansoftware.com/kwic/kwic-license.htm

*********************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef ENV_WINDOWSCE
#include <sys/types.h>
#endif
#include <time.h>

#include <wx/wx.h>
#include <wx/dcbuffer.h>

#if defined (ENV_LINUX) || defined (ENV_QNX)
 #include <sys/io.h>
 #include <arpa/inet.h>
 #include <errno.h>
 #include <unistd.h>
 #include <fcntl.h>
#else
 #include <wx/msw/private.h>
 #define snprintf _snprintf
 #ifdef ENV_WINDOWSCE
  #include <winsock2.h>
 #endif
#endif

#ifdef ENV_LINUX
#include <termios.h>
#include <unistd.h>
#endif

#include "oapc_libio.h"



#define MAX_XML_SIZE   3500

#pragma pack(8)

struct hmiConfigData
{
   wxUint16 length,version;
   wxUint32 m_cLimitColour[2],m_cValueColour[2],m_cBorderColour[2],m_cTagsColour[2];
   wxUint32 m_nTagsNum;
};



struct instData
{
   struct hmiConfigData  hmiData;
   wxInt32               m_nScaledVal,m_nMax,m_nMin;
   bool                  m_bShowCurrent,m_bShowLimits,m_bDirOrizFlag;
   wxUint8               digi[8],prevDigi[8]; // we use an array for all possible digital IOs
   wxFloat64             num[8],prevNum[8];  // we use an array for all possible numerical IOs
};



/** the xml data that define the behaviour and HMI configuration panels within the main application */
static char xmlhmitempl[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<oapc-config>\
<dialogue>\
 <dualpanel>\
  <name>Colours</name>\
  <param>\
   <name>limitColour0</name>\
   <text>Limits</text>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>limitColour1</name>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>valueColour0</name>\
   <text>Value</text>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>valueColour1</name>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>borderColour0</name>\
   <text>Border</text>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>borderColour1</name>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>tagsColour0</name>\
   <text>Ticks</text>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>tagsColour1</name>\
   <type>colorbutton</type>\
   <default>%d</default>\
  </param>\
  <param>\
   <name>tagsNum</name>\
   <text>Number of Ticks</text>\
   <type>integer</type>\
   <default>%d</default>\
   <min>0</min>\
   <max>20</max>\
  </param>\
 </dualpanel>\
</dialogue>\
</oapc-config>\n";

static char                  libname[]="Linear Meter";
static char                  xmlhmidescr[MAX_XML_SIZE+1];

static struct hmiConfigData  saveConfig;

/* HMI element variabled */
#define MAX_TAGS 20



/**
This function has to return the name that is used to display this library within the main application.
The name returned here has to be short and descriptive. It is displayed within the popup menu of the flow
editor, therefore it should not be too long!
@param[out] data pointer to the beginning of an char-array that contains the name
@return the length of the name structure or 0 in case of an error
*/
OAPC_EXT_API char *oapc_get_name(void)
{
   return libname;
}



/**
This function returns a set of OAPC_HAS_... flags that describe the general capabilities of this
library. These flags can be taken from oapc_libio.h
@return or-concatenated flags that describe the libraries capabilities
*/
OAPC_EXT_API unsigned long oapc_get_capabilities(void)
{
   return OAPC_HAS_INPUTS|OAPC_HAS_OUTPUTS|
          OAPC_HAS_XML_CONFIGURATION|           // for the custom HMI configuration panel
          OAPC_HAS_STANDARD_FLOW_CONFIGURATION| // for the standard Flow configuration panel
          OAPC_HAS_LOG_TYPE_FLOATNUM|           // to let the main application log the change of data
          OAPC_ACCEPTS_PLAIN_CONFIGURATION|
          OAPC_HMICAT_DISPLAY|
          OAPC_USERPRIVI_HIDE;
}



/**
This function returns a set of OAPC_HMI_NO_... flags that modify the general configuration dialogue
within the HMI Editor. These flags decide which parts of that dialogue have NOT to be accessible and
can be taken from oapc_libio.h
@return or-concatenated flags that describe which user interface properties are not supported by this
        type of HMI object
*/
OAPC_EXT_API unsigned long oapc_get_no_ui_flags(void)
{
   return OAPC_HMI_NO_UI_RO|OAPC_HMI_NO_UI_TEXT;
}



/**
When the OAPC_HAS_INPUTS flag is set, the application calls this function to get the configuration
for the inputs. Here "input" is defined from the libraries point of view, means data that are sent from
the application to the library are input data
@return or-concatenated OAPC_..._IO...-flags that describe which inputs and data types are used or 0 in
        case of an error
*/
OAPC_EXT_API unsigned long oapc_get_input_flags(void)
{
   return  OAPC_DIGI_IO0|OAPC_NUM_IO7;
   // avoid it to submit the same IO number for different data types, that would cause displaying problems for the flow symbol
}



/**
When the OAPC_HAS_OUTPUTS flag is set, the application calls this function to get the configuration
for the outputs. Here "output" is defined from the libraries point of view, means data that are sent from
the library to the application are output data
@return or-concatenated OAPC_..._IO...-flags that describe which outputs and data types are used or 0 in case
        of an error
*/
OAPC_EXT_API unsigned long oapc_get_output_flags(void)
{
   return OAPC_DIGI_IO0|OAPC_NUM_IO7;
   // avoid it to submit the same IO number for different data types, that would cause displaying problems for the flow symbol
}



/**
When the OAPC_HAS_XML_CONFIGURATION capability flag was set this function is called to retrieve an XML
structure that describes which configuration information have to be displayed within the main application.
When there are no extended flow configuration possibilities but the flag was set in order to provide
extended HMI possibilities (please see function below) this function has to return NULL.
@return an char-array that contains the XML data; this char array has to be released only when the 
        library is unloaded!
*/
OAPC_EXT_API char *oapc_get_config_data(void* WXUNUSED(instanceData))
{
   // here we're using a standard configuration dialogue so no XML document is returned for the custom flow dialogue
   return NULL;
}



/**
This function has to return a XML structure that describes additional configuration possibilities beside the standard
HMI properties. Within that XML structure several panels can be configured that are displayed as additional tab panes
when the user is editing the configuration of an element within the HMI editor. This function is called only when the
OAPC_HAS_XML_CONFIGURATION flag is set. When there are no extended HMI configuration possibilities but the flag was
set in order to provide extended flow possibilities (please see function above) this function has to return NULL.
@return an char-array that contains the XML data; this char array has to be released only when the 
        library is unloaded!
*/
OAPC_EXT_API char *oapc_get_hmi_config_data(void* instanceData)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   snprintf(xmlhmidescr,MAX_XML_SIZE,xmlhmitempl,
            data->hmiData.m_cLimitColour[0],data->hmiData.m_cLimitColour[1],
            data->hmiData.m_cValueColour[0],data->hmiData.m_cValueColour[1],
            data->hmiData.m_cBorderColour[0],data->hmiData.m_cBorderColour[1],
            data->hmiData.m_cTagsColour[0],data->hmiData.m_cTagsColour[1],data->hmiData.m_nTagsNum);
   return xmlhmidescr;
}



/**
When the OAPC_ACCEPTS_PLAIN_CONFIGURATION capability flag was set this function is called for every configurable parameter
to return the value that was set within the application.
@param[in] name the name of the parameter according to the unique identifier that was set within the <name></name> field of the
           XML config file
@param[in] value the value that was configured for this parameter; in case it is not a string but a number it has to be converted,
           the representation sent by the application is always a string
*/
OAPC_EXT_API void oapc_set_config_data(void* instanceData,const char *name,const char *value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (strcmp(name,"limitColour0")==0)       data->hmiData.m_cLimitColour[0]=atoi(value);
   else if (strcmp(name,"limitColour1")==0)  data->hmiData.m_cLimitColour[1]=atoi(value);
   else if (strcmp(name,"valueColour0")==0)  data->hmiData.m_cValueColour[0]=atoi(value);
   else if (strcmp(name,"valueColour1")==0)  data->hmiData.m_cValueColour[1]=atoi(value);
   else if (strcmp(name,"borderColour0")==0) data->hmiData.m_cBorderColour[0]=atoi(value);
   else if (strcmp(name,"borderColour1")==0) data->hmiData.m_cBorderColour[1]=atoi(value);
   else if (strcmp(name,"tagsColour0")==0)   data->hmiData.m_cTagsColour[0]=atoi(value);
   else if (strcmp(name,"tagsColour1")==0)   data->hmiData.m_cTagsColour[1]=atoi(value);
   else if (strcmp(name,"tagsNum")==0)       data->hmiData.m_nTagsNum=atoi(value);
}



/**
This function delivers the data that are stored within the project file by the main application. It is
recommended to put two fields "version" and "length" into the data structure that is handed over here.
So when the data to be saved need to be changed it is easy to recognize which version of the data structure
is used, possible conversions can be done easier in oapc_set_loaded_data().<BR>
PLEASE NOTE: In order to keep the resulting project files compatible with all possible platforms the
             application is running at you have to store all values in network byteorder
@param[out] length the total effective length of the data that are returned by this function
@return the data that have to be saved
*/
OAPC_EXT_API char *oapc_get_save_data(void* instanceData,unsigned long *length)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   *length=sizeof(struct hmiConfigData);
   saveConfig.version          =htons(data->hmiData.version);
   saveConfig.length           =htons(data->hmiData.length);
   saveConfig.m_cLimitColour[0]=htonl(data->hmiData.m_cLimitColour[0]);   saveConfig.m_cLimitColour[1]=htonl(data->hmiData.m_cLimitColour[1]);
   saveConfig.m_cValueColour[0]=htonl(data->hmiData.m_cValueColour[0]);   saveConfig.m_cValueColour[1]=htonl(data->hmiData.m_cValueColour[1]);
   saveConfig.m_cBorderColour[0]=htonl(data->hmiData.m_cBorderColour[0]); saveConfig.m_cBorderColour[1]=htonl(data->hmiData.m_cBorderColour[1]);
   saveConfig.m_cTagsColour[0]=htonl(data->hmiData.m_cTagsColour[0]);     saveConfig.m_cTagsColour[1]=htonl(data->hmiData.m_cTagsColour[1]);
   saveConfig.m_nTagsNum=htonl(data->hmiData.m_nTagsNum);

   return (char*)&saveConfig;
}



/**
This function receives the data that have been stored within the project file by the main application. It is
recommended to check if the returned data are really what the library expects. To do that two fields
"version" and "length" within the saved data structure should be checked.<BR>
PLEASE NOTE: In order to keep the resulting project files compatible with all possible platforms the
             application is running at you have to convert all the values that have been stored in network
             byteorder back to the local byteorder
@param[in] length the total effective length of the data that are handed over to this function
@param[in] data the configuration data that are loaded for this library
*/
OAPC_EXT_API void oapc_set_loaded_data(void* instanceData,unsigned long length,char *loadedData)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (length>sizeof(struct hmiConfigData)) length=sizeof(struct hmiConfigData);
   memcpy(&saveConfig,loadedData,length);

   data->hmiData.m_cLimitColour[0]=ntohl(saveConfig.m_cLimitColour[0]);   data->hmiData.m_cLimitColour[1]=ntohl(saveConfig.m_cLimitColour[1]);
   data->hmiData.m_cValueColour[0]=ntohl(saveConfig.m_cValueColour[0]);   data->hmiData.m_cValueColour[1]=ntohl(saveConfig.m_cValueColour[1]);
   data->hmiData.m_cBorderColour[0]=ntohl(saveConfig.m_cBorderColour[0]); data->hmiData.m_cBorderColour[1]=ntohl(saveConfig.m_cBorderColour[1]);
   data->hmiData.m_cTagsColour[0]=ntohl(saveConfig.m_cTagsColour[0]);     data->hmiData.m_cTagsColour[1]=ntohl(saveConfig.m_cTagsColour[1]);
   data->hmiData.m_nTagsNum=ntohl(saveConfig.m_nTagsNum);
}



/**
This function handles all internal data initialisation and has to allocate a memory area where all
data are stored into that are required to operate this Plug-In. This memory area can be used by the
Plug-In freely, it is handed over with every function call so that the Plug-In cann access its
values. The memory area itself is released by the main application, therefore it has to be allocated
using malloc().
@return pointer where the allocated and pre-initialized memory area starts
*/
OAPC_EXT_API void* oapc_create_instance2(unsigned long flags)
{
   flags=flags; // removing "unused" warning

   struct instData *data;

   data=(struct instData*)malloc(sizeof(struct instData));

   data->m_nScaledVal=0;
   data->m_nMax=100;
   data->m_nMin=0;
   data->m_bShowCurrent=true;
   data->m_bShowLimits=true;
   data->m_bDirOrizFlag=false;
   data->hmiData.length=sizeof(struct hmiConfigData);
   data->hmiData.version=1;
   data->hmiData.m_cLimitColour[0]=0x0000FF;  data->hmiData.m_cLimitColour[1]=0x0000FF;
   data->hmiData.m_cValueColour[0]=0xFFFFFF;  data->hmiData.m_cValueColour[1]=0xFFFFFF;
   data->hmiData.m_cBorderColour[0]=0x700000; data->hmiData.m_cBorderColour[1]=0x700000;
   data->hmiData.m_cTagsColour[0]=0xDDD0D0;   data->hmiData.m_cTagsColour[1]=0xDDD0D0;
   data->hmiData.m_nTagsNum=5;
   for (wxInt32 i=0; i<8; i++)
   {
      data->digi[i]=0;  data->prevDigi[i]=0;
      data->num[i]=0.0; data->prevNum[i]=0.0;
   }
   data->num[7]=30;
   data->prevNum[7]=30;

   return data;
}



/**
This function is called finally, it has to be used to release the instance data structure that was created
during the call of oapc_create_instance()
*/
OAPC_EXT_API void oapc_delete_instance(void* instanceData)
{
   if (instanceData) free(instanceData);
}



/**
When this function is called everything has to be initialized in order to perform the required operation
@return a return value/error code that informs the main application if the initialization was done successfully
        or not
*/
OAPC_EXT_API unsigned long oapc_init(void* instanceData)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   data->num[7]=0;
   data->prevNum[7]=0;
   return OAPC_OK;
}



/**
This function is called before the application unloads everything, it has to be used to deinitialize
everything and to release used resources.
*/
OAPC_EXT_API unsigned long oapc_exit(void* WXUNUSED(instanceData))
{
   return OAPC_OK;
}



/**
This function belongs to the implementation of the HMI element and draws parts of it
*/
static void DrawCurrent(struct instData *data,wxAutoBufferedPaintDC *dc,wxInt32 w, wxInt32 h)
{
	int tw,th;
	wxString s;

	//valore attuale
	s.Printf(_T("%.0f"),data->num[7]);
	dc->GetTextExtent(s, &tw, &th);
	dc->SetTextForeground(wxColour(data->hmiData.m_cValueColour[data->digi[0]]));
	dc->DrawText(s, w / 2 - tw / 2 , h / 2 - th / 2);
}



/**
This function belongs to the implementation of the HMI element and draws parts of it
*/
static void DrawLimits(struct instData *data,wxAutoBufferedPaintDC *dc,wxWindow *canvas,wxInt32 w, wxInt32 h)
{
	int tw,th;
	wxString s;

	dc->SetFont(canvas->GetFont());
	dc->SetTextForeground(wxColour(data->hmiData.m_cLimitColour[data->digi[0]]));

	if(data->m_bDirOrizFlag)
	{
		//valore minimo
		s.Printf(_T("%d"), data->m_nMin);	
		dc->GetTextExtent(s, &tw, &th);
		dc->DrawText(s, 5, h / 2 - th / 2);

		//valore massimo
		s.Printf(_T("%d"), data->m_nMax);
		dc->GetTextExtent(s, &tw, &th);
		dc->DrawText(s,w - tw - 5, h / 2 - th / 2);
	}
	else
	{
		//valore minimo
		s.Printf(_T("%d"), data->m_nMin);	
		dc->GetTextExtent(s, &tw, &th);
		dc->DrawText(s, w / 2 - tw / 2, h - th - 5);

		//valore massimo
		s.Printf(_T("%d"), data->m_nMax);
		dc->GetTextExtent(s, &tw, &th);
		dc->DrawText(s, w / 2 - tw / 2, 5);
	}
}



/**
This function belongs to the implementation of the HMI element and draws parts of it
*/
void DrawTags(struct instData *data,wxAutoBufferedPaintDC *dc,wxInt32 w, wxInt32 h)
{
   int       tw,th;
   wxFloat32 tagPos,tagDist;
   wxInt32   tagEnd,tagVal,tagValStep;
   wxString  text ;

   if(data->m_bDirOrizFlag)
   {
      tagDist=((wxFloat32)w)/(data->hmiData.m_nTagsNum+1);
      tagEnd=w;
   }
   else
   {
      tagDist=((wxFloat32)h)/(data->hmiData.m_nTagsNum+1);
      tagEnd=h;
   }
   tagValStep=(data->m_nMax-data->m_nMin)/(data->hmiData.m_nTagsNum+1);
   tagVal=tagValStep+data->m_nMin;

   dc->SetPen(wxColour(data->hmiData.m_cTagsColour[data->digi[0]]));
   dc->SetTextForeground(wxColour(data->hmiData.m_cTagsColour[data->digi[0]]));

   tagPos=tagDist;
   while (tagPos+0.5<tagEnd)
   {
	  text.Printf(_T("%d"),tagVal);
		
      if(data->m_bDirOrizFlag)
      {
	     dc->DrawLine(tagPos + 1, h - 2 , tagPos + 1, h - 10);
		 dc->GetTextExtent(text, &tw, &th);
		 dc->DrawText(text, tagPos + 1 - (tw / 2 ), h - 10 - th);
      }
	  else
      {
	     dc->DrawLine(w - 2, h - tagPos , w - 10 , h - tagPos);
		 dc->GetTextExtent(text, &tw, &th);
		 dc->DrawText(text, w - 10 - tw, h - tagPos - (th / 2) );
      }
      tagPos+=tagDist;
      tagVal+=tagValStep;
   }
}



/**
This function is called by the application whenever the element has to be repainted. That may happen
because a new value was set that has to be displayed or because the element was hidden and has to be
redrawn now. So the Plug-In does not take care about the states and conditions under which a repaint
is necessary, this function is called automatically.
@param[in] dc the wxWidgets drawing context structure that has to be used for painting the HMI element
@param[in] canvas the wxWindow element that was created by the main application in order to allocate
           a private area of defined size for the Plug-In to draw within
*/
OAPC_EXT_API void oapc_paint(void* instanceData,wxAutoBufferedPaintDC *dc,wxPanel *canvas)
{
   wxInt32   w,h,yPoint, rectHeight;
   wxFloat32 coeff;
   struct instData *data;

   data=(struct instData*)instanceData;

   canvas->GetClientSize(&w,&h);

   if(data->m_bDirOrizFlag) coeff = (w - 3) / (double)(data->m_nMax -data->m_nMin);
   else coeff = (h - 2) / (double)(data->m_nMax -data->m_nMin);

   data->m_nScaledVal = ceil((data->num[7]-data->m_nMin) * coeff);

   dc->SetPen(wxColour(data->hmiData.m_cBorderColour[data->digi[0]]));
   dc->SetBrush(wxBrush(canvas->GetBackgroundColour()));
   dc->DrawRectangle(0, 0, w, h);

   dc->SetBrush(wxBrush(canvas->GetForegroundColour()));

   if (data->m_bDirOrizFlag) dc->DrawRectangle(1, 1,data->m_nScaledVal, h - 2);
   else
   {
	   yPoint=h-data->m_nScaledVal ;
      if (data->m_nScaledVal == 0 ) rectHeight =data->m_nScaledVal ;
      else
      {
	     if (data->num[7]==data->m_nMax)
         {
		    rectHeight =data->m_nScaledVal;
			yPoint -= 1 ;
         }
         else rectHeight =data->m_nScaledVal - 1 ;
      }
      dc->DrawRectangle(1, yPoint, w - 2, rectHeight);
   }
   if (data->m_bShowCurrent) DrawCurrent(data,dc,w,h);	//valore attuale
   if (data->m_bShowLimits) DrawLimits(data,dc,canvas,w,h);	//valore minimo e massimo
   if (data->hmiData.m_nTagsNum>0) DrawTags(data,dc,w,h);
}



/**
Here the default size of the element has to be returned; this size is used when it is created
newly
@param[out] x the default size in x-direction (width)
@param[out] y the default size in y-direction (height)
*/
OAPC_EXT_API void oapc_get_defsize(wxFloat32 *x,wxFloat32 *y)
{
   *x=80;
   *y=200;
}



/**
This function specifies the minimum size that is allowed for this HMI element, no scaling
operation of the main application will set smaller sizes to the related canvas than specified by
this function
@param[out] x the minimum size in x-direction (width)
@param[out] y the minimum size in y-direction (height)
*/
OAPC_EXT_API void oapc_get_minsize(void*,wxFloat32 *x,wxFloat32 *y)
{
   *x=20;
   *y=60;
}



/**
This function specifies the maximum size that is allowed for this HMI element, no scaling
operation of the main application will set bigger sizes to the related canvas than specified by
this function
@param[out] x the maximum size in x-direction (width)
@param[out] y the maximum size in y-direction (height)
*/
OAPC_EXT_API void oapc_get_maxsize(void*,wxFloat32 *x,wxFloat32 *y)
{
   *x=2000;
   *y=6000;
}



/**
This function is relevant only to HMI controls that provide or handle numeric values, it specifies
the allowed range the numeric value can handle.
@param[out] minValue the minimum allowed value that can be handled by this control, this value can't
            be smaller than -2100000000
@param[out] maxValue the maximum allowed value that can be handled by this control, this value can't
            be bigger than 2100000000
*/
OAPC_EXT_API void oapc_set_numminmax(void* instanceData,wxFloat32 minValue,wxFloat32 maxValue)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   data->m_nMin=minValue;
   data->m_nMax=maxValue;
}



/**
This function is relevant only to HMI controls that provide or handle numeric values, it specifies
the allowed range the numeric value can handle.
@param[out] minValue the minimum allowed value that can be handled by this control, this value can't
            be smaller than -2100000000
@param[out] maxValue the maximum allowed value that can be handled by this control, this value can't
            be bigger than 2100000000
*/
OAPC_EXT_API void oapc_get_numminmax(void* WXUNUSED(instanceData),wxFloat32 *minValue,wxFloat32 *maxValue)
{
   *minValue=-2100000000;
   *maxValue=2100000000;
}



/**
The foreground and background colour of an element are handled by the standard HMI properties dialogue.
This function is called once during first setup of this control and has to return the default colour
values for it. Afterwards these values are stored by the main application, the Plug-In does not to
take care about them. Within the painting-function (please see above) you can get these colours by
calling canvas->GetBackgroundColour() and canvas->GetForegroundColour()
@param[out] background the default background colour
@param[out] foreground the default foreground colour
*/
OAPC_EXT_API void oapc_get_colours(wxUint32 *background,wxUint32 *foreground)
{
   *foreground=0xFF8080;
   *background=0x700000;
}



/**
This function is called by the main application when the library provides an digital input (marked
using the digital input flags OAPC_DIGI_IO...), a connection was edited to this input and a data
flow reaches the input.
@param[in] input specifies the input where the data are send to, here not the OAPC_DIGI_IO...-flag is used
           but the plain, 0-based input number
@param[in] value specifies the value (0 or 1) that is set to that input
@return an error code OAPC_ERROR_... in case of an error or OAPC_OK in case the value could be set
*/
OAPC_EXT_API unsigned long  oapc_set_digi_value(void* instanceData,unsigned long input,unsigned char value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (input!=0) return OAPC_ERROR_NO_SUCH_IO; // check for valid IO
   data->digi[input]=value;
   return OAPC_OK;
}



/**
This function is called by the main application periodically in order to poll the state of the related
output. It has to return the data that are available for that input or - in case there are none available -
the error code OAPC_ERROR_NO_DATA_AVAILABLE to notify the main application, that there is nothing new.
@param[in] output specifies the output where the data are fetched from, here not the OAPC_DIGI_IO...-flag is used
           but the plain, 0-based output number
@param[out] value if there are new data available, they are stored within that variable, if there are no new
           data, the returned data are ignored by the main application
@return an error code OAPC_ERROR_... in case of an error, OAPC_ERROR_NO_DATA_AVAILABLE in case no new data are
           available or OAPC_OK in case the value could be set
*/
OAPC_EXT_API unsigned long  oapc_get_digi_value(void* instanceData,unsigned long output,unsigned char *value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (output!=0) return OAPC_ERROR_NO_SUCH_IO; // check for valid IO
   if (data->prevDigi[output]!=data->digi[output])
   {
      data->prevDigi[output]=data->digi[output];
      *value=data->digi[output];
      return OAPC_OK;
   }
   return OAPC_ERROR_NO_DATA_AVAILABLE;
}



/**
This function is called by the main application when the library provides an numerical input (marked
using the digital input flags OAPC_NUM_IO...), a connection was edited to this input and a data
flow reaches the input.
@param[in] input specifies the input where the data are send to, here not the OAPC_NUM_IO...-flag is used
           but the plain, 0-based input number
@param[in] value specifies the numerical floating-point value that is set to that input
@return an error code OAPC_ERROR_... in case of an error or OAPC_OK in case the value could be set
*/
OAPC_EXT_API unsigned long  oapc_set_num_value(void* instanceData,unsigned long input,double value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (input!=7) return OAPC_ERROR_NO_SUCH_IO;
   data->num[7]=value;
   return OAPC_OK;
}



/**
This function is called by the main application periodically in order to poll the state of the related
output. It has to return the data that are available for that input or - in case there are none available -
the error code OAPC_ERROR_NO_DATA_AVAILABLE to notify the main application, that there is nothing new.
@param[in] output specifies the output where the data are fetched from, here not the OAPC_NUM_IO...-flag is used
           but the plain, 0-based output number
@param[out] value if there are new data available, they are stored within that variable, if there are no new
           data, the returned data are ignored by the main application
@return an error code OAPC_ERROR_... in case of an error, OAPC_ERROR_NO_DATA_AVAILABLE in case no new data are
           available or OAPC_OK in case the value could be set
*/
OAPC_EXT_API unsigned long  oapc_get_num_value(void* instanceData,unsigned long output,double *value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if ((output!=6) && (output!=7)) return OAPC_ERROR_NO_SUCH_IO;
   *value=data->num[7];
   return OAPC_OK;
}



