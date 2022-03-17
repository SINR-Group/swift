
#if !defined(AFX_MOTIONESTIMATIONQUARTERPEL_H__3CDE908E_D756_4E9A_8B86_C55174766C93__INCLUDED_)
#define AFX_MOTIONESTIMATIONQUARTERPEL_H__3CDE908E_D756_4E9A_8B86_C55174766C93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "MotionEstimation.h"
#define X1 24


H264AVC_NAMESPACE_BEGIN

class MotionEstimationQuarterPel :
public MotionEstimation
{
protected:
  MotionEstimationQuarterPel();
  virtual ~MotionEstimationQuarterPel();

public:
  static ErrVal create( MotionEstimation*& rpcMotionEstimation );

  ErrVal compensateBlock( YuvMbBuffer *pcRecPelData, UInt uiBlk, UInt uiMode, YuvMbBuffer *pcRefPelData2 = NULL );

  Void xSubPelSearch( YuvPicBuffer *pcPelData, Mv& rcMv, UInt& ruiSAD, UInt uiBlk, UInt uiMode );

protected:
  Void xCompensateBlocksHalf( XPel *pPelDes, YuvPicBuffer *pcRefPelData, Mv cMv, UInt uiMode, UInt uiYSize, UInt uiXSize );
  Void xGetSizeFromMode( UInt& ruiXSize, UInt& ruiYSize, UInt uiMode );
  Void xInitBuffer();

protected:
  UInt m_uiBestMode;
  XPel m_aXHPelSearch[17*X1*4];
  XPel m_aXQPelSearch[16*16*9];
  XPel *m_apXHPelSearch[9];
  XPel *m_apXQPelSearch[9];
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MOTIONESTIMATIONQUARTERPEL_H__3CDE908E_D756_4E9A_8B86_C55174766C93__INCLUDED_)
