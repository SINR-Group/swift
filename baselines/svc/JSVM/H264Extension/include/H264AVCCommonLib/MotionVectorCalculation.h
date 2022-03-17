
#if !defined(AFX_MOTIONVECTORCALCULATION_H__9F13FBF7_8AD0_49DA_985B_08EE7CE0F231__INCLUDED_)
#define AFX_MOTIONVECTORCALCULATION_H__9F13FBF7_8AD0_49DA_985B_08EE7CE0F231__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN


class H264AVCCOMMONLIB_API MotionVectorCalculation
{
protected:
  MotionVectorCalculation();
  virtual ~MotionVectorCalculation();

public:
  static ErrVal create  ( MotionVectorCalculation*& rpcMotionVectorCalculation );
  ErrVal        destroy ();

  ErrVal initSlice( const SliceHeader& rcSH );
  ErrVal uninit();

  ErrVal  calcMvMb    ( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase );
  ErrVal  calcMvSubMb ( B8x8Idx c8x8Idx, MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase );

protected:
  Void xCalc16x16( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase );
  Void xCalc16x8( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase );
  Void xCalc8x16( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase );
  Void xCalc8x8( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase, Bool bFaultTolerant );
  Void xCalc8x8( B8x8Idx c8x8Idx, MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase, Bool bFaultTolerant );

protected:
  UInt m_uiMaxBw;
  Bool m_bSpatialDirectMode;
  Mv3D m_cMvA;
  Mv3D m_cMvB;
  Mv3D m_cMvC;
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_MOTIONVECTORCALCULATION_H__9F13FBF7_8AD0_49DA_985B_08EE7CE0F231__INCLUDED_)
