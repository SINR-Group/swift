
#if !defined(AFX_RATEDISTORTION_H__C367E6C2_6E98_4DCB_9E6D_C4F84B9EC0D6__INCLUDED_)
#define AFX_RATEDISTORTION_H__C367E6C2_6E98_4DCB_9E6D_C4F84B9EC0D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RateDistortionIf.h"

H264AVC_NAMESPACE_BEGIN

class CodingParameter;

class RateDistortion :
public RateDistortionIf
{
protected:
  RateDistortion();
  virtual ~RateDistortion();

public:
  virtual ErrVal  setMbQpLambda( MbDataAccess& rcMbDataAccess, UInt uiQp, Double dLambda );

  static  ErrVal create( RateDistortion *&rpcRateDistortion );
  virtual ErrVal destroy();

  Double  getCost( UInt uiBits, UInt uiDistortion );
  Double  getFCost( UInt uiBits, UInt uiDistortion );
  UInt    getMotionCostShift( Bool bSad) { return (bSad) ? m_uiCostFactorMotionSAD : m_uiCostFactorMotionSSE; }

  ErrVal  fixMacroblockQP( MbDataAccess& rcMbDataAccess );

protected:
  Double m_dCost;
  Double m_dSqrtCost;
  UInt m_uiCostFactorMotionSAD;
  UInt m_uiCostFactorMotionSSE;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_RATEDISTORTION_H__C367E6C2_6E98_4DCB_9E6D_C4F84B9EC0D6__INCLUDED_)
