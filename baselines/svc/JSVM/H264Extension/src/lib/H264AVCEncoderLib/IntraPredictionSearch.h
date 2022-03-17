
#if !defined(AFX_INTRAPREDICTIONSEARCH_H__3CA21A2D_62BE_464E_8C46_C98B0E424BE4__INCLUDED_)
#define AFX_INTRAPREDICTIONSEARCH_H__3CA21A2D_62BE_464E_8C46_C98B0E424BE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/IntraPrediction.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"
#include "DistortionIf.h"


H264AVC_NAMESPACE_BEGIN


class IntraPredictionSearch :
public IntraPrediction
{
protected:
  IntraPredictionSearch();
  virtual ~IntraPredictionSearch();

public:
  static ErrVal create( IntraPredictionSearch*& rpcIntraPredictionSearch );
  ErrVal destroy();

  ErrVal predictSLumaMb       ( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode,               Bool& rbValid );
  ErrVal predictSLumaBlock    ( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode, LumaIdx cIdx, Bool &rbValid );
  ErrVal predictSChromaBlock  ( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode,               Bool &rbValid );
  ErrVal predictSLumaBlock8x8 ( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode, B8x8Idx cIdx, Bool &rbValid );
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_INTRAPREDICTIONSEARCH_H__3CA21A2D_62BE_464E_8C46_C98B0E424BE4__INCLUDED_)
