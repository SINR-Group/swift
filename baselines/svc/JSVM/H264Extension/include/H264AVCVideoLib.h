
#ifndef __H264AVCVIDEOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCVIDEOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
//#define _ATL_APARTMENT_THREADED
#define _ATL_FREE_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>
#include <atlctl.h>


#include "MSysLib.h"
#include "CommonCodecLib.h"
#include "H264AVCEncoderLib.h"
#include "H264AVCDecoderLib.h"

#if defined( MSYS_WIN32 )
  #if defined( H264AVCVIDEOLIB_EXPORTS )
    #define H264AVCVIDEOLIB_API __declspec(dllexport)
  #else
    #if !defined( H264AVCVIDEOLIB_LIB )
      #define H264AVCVIDEOLIB_API __declspec(dllimport)
    #else
      #define H264AVCVIDEOLIB_API
    #endif
  #endif
#elif defined( MSYS_LINUX )
  #define H264AVCVIDEOLIB_API
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

//begin of WTL header file section
//remember to use namespace WTL
#include <atlapp.h>
#include <atlctrls.h>
#include <atlctrlx.h>
//end of WTL header file section


#include "MSysLib.h"
#include "MediaPacketLib.h"
#include "CommonCodecLib.h"

#include <string>
#include <sstream>


#endif //__H264AVCVIDEOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
