
#ifndef __H264AVCDECODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCDECODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79


#include "H264AVCCommonLib.h"

#if defined( USE_NAMESPACE_H264AVC )
  using namespace h264;
#endif

#if defined( MSYS_WIN32 )
  #if defined( H264AVCDECODERLIB_EXPORTS )
    #define H264AVCDECODERLIB_API __declspec(dllexport)
  #else
    #if !defined( H264AVCDECODERLIB_LIB )
      #define H264AVCDECODERLIB_API __declspec(dllimport)
    #else
      #define H264AVCDECODERLIB_API
    #endif
  #endif
#elif defined( MSYS_LINUX )
  #define H264AVCDECODERLIB_API
#endif


#endif //__H264AVCDECODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
