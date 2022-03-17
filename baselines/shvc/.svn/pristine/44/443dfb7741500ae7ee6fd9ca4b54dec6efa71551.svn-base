#ifndef __TCOMUPSAMPLEFILTER__
#define __TCOMUPSAMPLEFILTER__

#include "TComPic.h"
#include "TComYuv.h"
#include "TComDataCU.h"

#define NTAPS_US_LUMA        8 ///< Number of taps for luma upsampling filter
#define NTAPS_US_CHROMA      4 ///< Number of taps for chroma upsampling filter
#define US_FILTER_PREC       6 ///< Log2 of sum of filter taps

class TComUpsampleFilter
{
private:
  static const Int m_lumaFixedFilter[16][NTAPS_US_LUMA];     ///< Luma filter taps for both 1.5x and 2x scalability
  static const Int m_chromaFixedFilter[16][NTAPS_US_CHROMA]; ///< Chroma filter taps for 1.5x scalability

  Int m_lumaFilter[16][NTAPS_US_LUMA];
  Int m_chromaFilter[16][NTAPS_US_CHROMA];

  static inline Int sumLumaHor( Pel* pel, Int* coeff )
  {
    return ( pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + pel[3]*coeff[3] + pel[4]*coeff[4] + pel[5]*coeff[5] + pel[6]*coeff[6] + pel[7]*coeff[7]);
  }

  static inline Int sumChromaHor( Pel* pel, Int* coeff )
  {
    return ( pel[0]*coeff[0] + pel[1]*coeff[1] + pel[2]*coeff[2] + pel[3]*coeff[3] );
  }

  static inline Int sumLumaVer( Pel* pel, Int* coeff, Int stride )
  {
    return ( pel[0]*coeff[0] + pel[stride]*coeff[1] + pel[2*stride]*coeff[2] + pel[3*stride]*coeff[3] + pel[4*stride]*coeff[4] + pel[5*stride]*coeff[5] + pel[6*stride]*coeff[6] + pel[7*stride]*coeff[7]);
  }

  static inline Int sumChromaVer( Pel* pel, Int* coeff, Int stride )
  {
    return ( pel[0]*coeff[0] + pel[stride]*coeff[1] + pel[2*stride]*coeff[2] + pel[3*stride]*coeff[3] );
  }

public:
  TComUpsampleFilter(void);
  ~TComUpsampleFilter(void);

#if O0215_PHASE_ALIGNMENT
#if O0194_JOINT_US_BITSHIFT
  Void upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, Bool phaseAlignFlag );
#else
  Void upsampleBasePic( UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window, Bool phaseAlignFlag );
#endif
#else
#if O0194_JOINT_US_BITSHIFT
#if REF_REGION_OFFSET
  Void upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window, const Window altRefWindow );
#else
  Void upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window );
#endif
#else
  Void upsampleBasePic( UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window );
#endif
#endif

};

#endif //__TCOMUPSAMPLEFILTER__
