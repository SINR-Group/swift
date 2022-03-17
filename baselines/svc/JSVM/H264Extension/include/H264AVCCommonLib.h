
#ifndef __H264AVCCOMMONLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCCOMMONLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79

#include "H264AVCCommonIf.h"


#if defined( MSYS_WIN32 )
  #if defined( H264AVCCOMMONLIB_EXPORTS )
    #define H264AVCCOMMONLIB_API __declspec(dllexport)
  #else
    #if !defined( H264AVCCOMMONLIB_LIB )
      #define H264AVCCOMMONLIB_API __declspec(dllimport)
    #else
      #define H264AVCCOMMONLIB_API
    #endif
  #endif
#elif defined( MSYS_LINUX )
  #define H264AVCCOMMONLIB_API
#endif


#define H264AVC_NAMESPACE_BEGIN      namespace h264 {
#define H264AVC_NAMESPACE_END        }



#include "H264AVCCommonLib/Macros.h"
#include "H264AVCCommonLib/GlobalFunctions.h"

#include "H264AVCCommonLib/CommonDefs.h"


#if defined( USE_NAMESPACE_H264AVC )
  using namespace h264;
#endif

#include "H264AVCCommonLib/CommonBuffers.h"
#include "H264AVCCommonLib/CommonTypes.h"
#include "H264AVCCommonLib/MbDataAccess.h"

#if defined( USE_NAMESPACE_H264AVC )
  using namespace h264;
#endif


#define H264AVCCommonLib_FIRST_VER      (0)
#define H264AVCCommonLib_SECOND_VER     (1)
#define H264AVCCommonLib_THIRD_VER      (0)
#define H264AVCCommonLib_FOURTH_VER     (0)


#endif //__H264AVCCOMMONLIB_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
