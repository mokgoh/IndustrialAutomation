/********************************************************************************************

These sources are distributed in the hope that they will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. All
the information given here, within the interface descriptions and within the specification
are subject to change without notice. Errors and omissions excepted.

These sources demonstrate the usage of the OpenAPC Plug-In programming interface. They can be
used freely according to the OpenAPC Dual License: As long as the sources and the resulting
applications/libraries/Plug-Ins are used together with the OpenAPC software, they are
licensed as Freeware. When you use them outside the OpenAPC software they are licensed under
the terms of the GNU General Public License.

For more information please refer to http://www.openapc.com/oapc_license.php

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

#ifndef ENV_WINDOWS
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#define closesocket close
#else
#include <winsock2.h>
#define MSG_NOSIGNAL 0
#endif

#include "oapc_libio.h"
#include "liboapc.h"

#define MAX_XML_SIZE       5000
#define MAX_FILENAME_SIZE   255
#define MAX_BUFSIZE        5000

struct libio_config
{
   unsigned short version,length;
   int            mScalex,mScaley,mScalez;
   int            uTransx,uTransy,uTransz;
};



struct instData
{
   struct libio_config   config;
   struct oapc_bin_head *m_bin;
   int                   m_callbackID;
};



/** the xml data that define the behaviour and user interface of the plugin within the main application */
static char xmltempl[]={"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n\
<oapc-config>\n\
 <flowimage>%s</flowimage>\n\
 <dialogue>\n\
  <general>\n\
   <param>\n\
    <name>scalex</name>\n\
    <text>Scale X</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000</max>\n\
   </param>\n\
   <param>\n\
    <name>scaley</name>\n\
    <text>Scale Y</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000</max>\n\
   </param>\n\
   <param>\n\
    <name>scalez</name>\n\
    <text>Scale Z</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000</max>\n\
   </param>\n\
   <param>\n\
    <name>transx</name>\n\
    <text>Translate X</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000000</max>\n\
    <unit>mm</unit>\n\
   </param>\n\
   <param>\n\
    <name>transy</name>\n\
    <text>Translate Y</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000000</max>\n\
    <unit>mm</unit>\n\
   </param>\n\
   <param>\n\
    <name>transz</name>\n\
    <text>Translate Z</text>\n\
    <type>float</type>\n\
    <default>%f</default>\n\
    <min>0.0001</min>\n\
    <max>1000000</max>\n\
    <unit>mm</unit>\n\
   </param>\n\
  </general>\n\
  <helppanel>\n\
   <in0>CTRL - Control data to be converted</in0>\n\
   <in1>SCX - Scale factor X direction</in1>\n\
   <in2>SCY - Scale factor Y direction</in2>\n\
   <in3>SCZ - Scale factor Z direction</in3>\n\
   <in4>TX - Translate in X direction</in4>\n\
   <in5>TY - Translate in Y direction</in5>\n\
   <in6>TZ - Translate in Z direction</in6>\n\
   <out0>CTRL - Converted control data</out0>\n\
  </helppanel>\n\
 </dialogue>\n\
 </oapc-config>\n"};
/** Base64-encoded PNG data of an image that is used as icon for the program flow diagram within the main application*/
static char                 flowImage[]="iVBORw0KGgoAAAANSUhEUgAAAGoAAAAyCAMAAACDIr4/AAAAA3NCSVQICAjb4U/gAAAAG1BMVEUEAgT8/vyEgoT//wAE/vz////EwsQAAACAgP/nmd0YAAAACXBIWXMAAA7EAAAOxAGVKw4bAAABjklEQVRYhe2Y27LDIAhFtbnQ///iolYFxahRM2fOdD9JQllsg31QqQeltVavbdteZmWDfd9nBYcXPraoUyFKnS7zVJg5K8hQxpbvyXY1LchRaMv3ZLuaFuQobCT0NDUQUGfsaWogoDTpaWYgodbriwposwZcge8mLHzAHhwdaRIK04D/LqyhhGpIE1FJj0AsQglVT+t05ToWSdW0vm9VVkOaQ/lIE2ypYENROaXJlRyUqwPA3Q2MP6MlgFZk1auuII4F7Ulq0L14owJKzqmiimLnBVHwbpwagrL+ddxAUpAH5BwYTsnxBcopoGhBdsoo2LhqPQsUBTq6urBF5SYBmPxj/oqhkBRd9QjcdHwFxxFiIKs5KMbKUCCg7FjoGyhbEhIU2cQMRdWJsrMBCSpJYK4GUKZUirpwNbCB9AM1fashV6j2CRz5VumwV89VdDaIKup5lI5TsRrlgj+Oyv5ul6E69Dzq9rn6of4xKl6WrEfR25i1KHYbsxbFbmMWo+htzGIUvUxZjSKXKQtRj+kDNYZTy4zekFkAAAAASUVORK5CYII=";
static char                 libname[]="Scale/Translate Control";
static char                 xmldescr[MAX_XML_SIZE+1];
static struct libio_config  save_config;
static lib_oapc_io_callback m_oapc_io_callback; // callback function that is used to inform the main function about changes at the IO ports



/**
This function has to return the name that is used to display this library within the main application.
The name returned here has to be short and descriptive
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
          OAPC_HAS_XML_CONFIGURATION|
          OAPC_ACCEPTS_PLAIN_CONFIGURATION|OAPC_ACCEPTS_IO_CALLBACK|
          OAPC_FLOWCAT_CALC;
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
   return OAPC_BIN_IO0|
		  OAPC_NUM_IO1|OAPC_NUM_IO2|OAPC_NUM_IO3|
          OAPC_NUM_IO4|OAPC_NUM_IO5|OAPC_NUM_IO6;
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
   return OAPC_BIN_IO0;
}



/**
When the OAPC_HAS_XML_CONFIGURATION capability flag was set this function is called to retrieve an XML
structure that describes which configuration information have to be displayed within the main application.
@param[out] data pointer to the beginning of an char-array that contains the XML data
@return the length of the XML structure or 0 in case of an error
*/
OAPC_EXT_API char *oapc_get_config_data(void* instanceData)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   sprintf(xmldescr,xmltempl,flowImage,data->config.mScalex/1000.0,data->config.mScaley/1000.0,data->config.mScalez/1000.0,
		                               data->config.uTransx/1000.0,data->config.uTransy/1000.0,data->config.uTransz/1000.0);
   assert(strlen(xmldescr)<MAX_XML_SIZE);
   return xmldescr;
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

   if (strcmp(name,"scalex")==0)      data->config.mScalex=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
   else if (strcmp(name,"scaley")==0) data->config.mScaley=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
   else if (strcmp(name,"scalez")==0) data->config.mScalez=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
   else if (strcmp(name,"transx")==0) data->config.uTransx=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
   else if (strcmp(name,"transy")==0) data->config.uTransy=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
   else if (strcmp(name,"transz")==0) data->config.uTransz=(int)OAPC_ROUND(oapc_util_atof(value)*1000.0,0);
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

   *length=sizeof(struct libio_config);
   save_config.version=htons(data->config.version);
   save_config.length =htons(data->config.length);
   save_config.mScalex=htonl(data->config.mScalex);
   save_config.mScaley=htonl(data->config.mScaley);
   save_config.mScalez=htonl(data->config.mScalez);
   save_config.uTransx=htonl(data->config.uTransx);
   save_config.uTransy=htonl(data->config.uTransy);
   save_config.uTransz=htonl(data->config.uTransz);

   return (char*)&save_config;
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

   if (length>sizeof(struct libio_config)) length=sizeof(struct libio_config);
   memcpy(&save_config,loadedData,length);
   data->config.version=ntohs(save_config.version);
   data->config.length =ntohs(save_config.length);
   data->config.mScalex=ntohl(save_config.mScalex);
   data->config.mScaley=ntohl(save_config.mScaley);
   data->config.mScalez=ntohl(save_config.mScalez);
   data->config.uTransx=ntohl(save_config.uTransx);
   data->config.uTransy=ntohl(save_config.uTransy);
   data->config.uTransz=ntohl(save_config.uTransz);
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
   memset(data,0,sizeof(struct instData));

   data->config.version=1;
   data->config.length=sizeof(struct libio_config);
   data->config.mScalex=1000;
   data->config.mScaley=1000;
   data->config.mScalez=1000;

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
When the capability flag OAPC_ACCEPTS_IO_CALLBACK is set, the main application no longer cyclically polls
the outputs of a Plug-In and the related parameter within the flow configuration dialogue is turned off.
Instead of this the main application hands over a function pointer to a callback and an ID. Whenever something
changes within the scope of this Plug-In that influences the output state of it, the Plug-In jumps into
that callback function to notify the main application about the new output state. The callback function
"oapc_io_callback" expects two parameters. The first one "outputs" expects the Or-concatenated flags of
the outputs that have changed and the second one "callbackID" expects the ID that is handed over here to
identify the Plug-In. For a typedef of the callback function oapc_io_callback() that is called by the Plug-In
please refer to oapc_libio.h.<BR><BR>
Here the main application hands over a pointer to the callback function and a unique callback ID. Both have
to be stored for later use
@param[in] oapc_io_callback the callback function that has to be called whenever something changes at the
           outputs of this Plug-In
@param[in] callbackID a unique ID that identifies this Plug-In and that has to be used when the function
           oapc_io_callback is called
*/
OAPC_EXT_API void oapc_set_io_callback(void* instanceData,lib_oapc_io_callback oapc_io_callback,unsigned long callbackID)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   m_oapc_io_callback=oapc_io_callback;
   data->m_callbackID=callbackID;
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
   struct instData           *data;

   data=(struct instData*)instanceData;
   if (input==1) data->config.mScalex=(int)OAPC_ROUND(value*1000.0,0);
   else if (input==2) data->config.mScaley=(int)OAPC_ROUND(value*1000.0,0);
   else if (input==3) data->config.mScalez=(int)OAPC_ROUND(value*1000.0,0);
   else if (input==4) data->config.uTransx=(int)OAPC_ROUND(value*1000.0,0);
   else if (input==5) data->config.uTransy=(int)OAPC_ROUND(value*1000.0,0);
   else if (input==6) data->config.uTransz=(int)OAPC_ROUND(value*1000.0,0);
   else return OAPC_ERROR_NO_SUCH_IO;
   return OAPC_OK;
}



OAPC_EXT_API unsigned long  oapc_set_bin_value(void* instanceData,unsigned long input,struct oapc_bin_head *value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (data->m_bin) return OAPC_ERROR_STILL_IN_PROGRESS;
   if (input==0)
   {
	  struct oapc_bin_struct_ctrl *ctrl;
	  int                          i;
     unsigned short               numCoords;
     unsigned int                 validityFlags;

	  if ((value->type!=OAPC_BIN_TYPE_STRUCT) || (value->subType!=OAPC_BIN_SUBTYPE_STRUCT_CTRL)) return OAPC_ERROR_INVALID_INPUT;
   	  // in case a former compression failed and we still have uncompressed data this has to be accepted too

      data->m_bin=(struct oapc_bin_head*)malloc(value->sizeData+sizeof(struct oapc_bin_head));
      if (!data->m_bin) return OAPC_ERROR_NO_MEMORY;
      memcpy(data->m_bin,value,value->sizeData+sizeof(struct oapc_bin_head));
      ctrl=(struct oapc_bin_struct_ctrl*)&data->m_bin->data;
      numCoords=ntohs(ctrl->numCoords);
      validityFlags=ntohl(ctrl->validityFlags);

      for (i=0; i<numCoords; i++)
      {
         ctrl->coord[i].x=(int)htonl((int)OAPC_ROUND(((int)ntohl(ctrl->coord[i].x))*(data->config.mScalex/1000.0),0)+data->config.uTransx);
         ctrl->coord[i].y=(int)htonl((int)OAPC_ROUND(((int)ntohl(ctrl->coord[i].y))*(data->config.mScaley/1000.0),0)+data->config.uTransy);
         if (validityFlags & TOOLPARAM_VALIDFLAG_Z)
         {
            ctrl->coord[i].z=(int)htonl((int)OAPC_ROUND(((int)ntohl(ctrl->coord[i].z))*(data->config.mScalez/1000.0),0)+data->config.uTransz);
         }
      }
      m_oapc_io_callback(OAPC_BIN_IO0,data->m_callbackID);
   }
   else return OAPC_ERROR_NO_SUCH_IO;
   return OAPC_OK;
}



/**
This function is called by the main application as soon as the plug-in triggers it via the callback function.
It has to return the data that are available for that input or - in case there are none available -
the error code OAPC_ERROR_NO_DATA_AVAILABLE to notify the main application, that there is nothing new.
@param[in] output specifies the output where the data are fetched from, here not the OAPC_BIN_IO...-flag is used
           but the plain, 0-based output number
@param[out] value if there are new data available, they have to be stored within that variable, if there are
           no new data, the returned data are ignored by the main application
@return an error code OAPC_ERROR_... in case of an error, OAPC_ERROR_NO_DATA_AVAILABLE in case no new data are
           available or OAPC_OK in case the value could be set
*/
OAPC_EXT_API unsigned long  oapc_get_bin_value(void *instanceData,unsigned long /*output*/,struct oapc_bin_head **value)
{
   struct instData *data;

   data=(struct instData*)instanceData;

   if (!data->m_bin) return OAPC_ERROR_NO_DATA_AVAILABLE;
   else *value=data->m_bin;
   return OAPC_OK;
}



OAPC_EXT_API void oapc_release_bin_data(void *instanceData,unsigned long /*output*/)
{
   struct instData *data;

   data=(struct instData*)instanceData;
   	
   free(data->m_bin);
	data->m_bin=NULL;
}

