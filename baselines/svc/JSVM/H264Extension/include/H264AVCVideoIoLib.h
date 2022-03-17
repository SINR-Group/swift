
#ifndef __H264AVCVIDEOIOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCVIDEOIOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79


#include "H264AVCCommonIf.h"


#if defined( MSYS_WIN32 )
  #if defined( H264AVCVIDEOIOLIB_EXPORTS )
    #define H264AVCVIDEOIOLIB_API __declspec(dllexport)
  #else
    #if !defined( H264AVCVIDEOIOLIB_LIB )
      #define H264AVCVIDEOIOLIB_API __declspec(dllimport)
    #else
      #define H264AVCVIDEOIOLIB_API
    #endif
  #endif
#elif defined( MSYS_LINUX )
  #define H264AVCVIDEOIOLIB_API
#endif


#endif //__H264AVCVIDEOIOLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
