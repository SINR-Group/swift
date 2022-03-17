
#ifndef __H264AVCENCODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCENCODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79

#include "H264AVCCommonLib.h"

#if defined( MSYS_WIN32 )
  #if defined( H264AVCENCODERLIB_EXPORTS )
    #define H264AVCENCODERLIB_API __declspec(dllexport)
  #else
    #if !defined( H264AVCENCODERLIB_LIB )
      #define H264AVCENCODERLIB_API __declspec(dllimport)
    #else
      #define H264AVCENCODERLIB_API
    #endif
  #endif
#elif defined( MSYS_LINUX )
  #define H264AVCENCODERLIB_API
#endif

//#define RANDOM_MBAFF

#endif //__H264AVCENCODERLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
