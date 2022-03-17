
#if !defined(AFX_RATEDISTORTIONIF_H__A4FCCF6E_E3BC_46A7_ACB8_7AE8CFE756E1__INCLUDED_)
#define AFX_RATEDISTORTIONIF_H__A4FCCF6E_E3BC_46A7_ACB8_7AE8CFE756E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN


class RateDistortionIf
{
protected:
  RateDistortionIf() {}
  virtual ~RateDistortionIf() {}

public:
  virtual ErrVal  setMbQpLambda( MbDataAccess& rcMbDataAccess, UInt uiQp, Double dLambda ) = 0;

  virtual Double  getCost( UInt uiBits, UInt uiDistortion ) = 0;
  virtual Double  getFCost( UInt uiBits, UInt uiDistortion ) = 0;
  virtual UInt    getMotionCostShift( Bool bSad ) = 0;

  virtual ErrVal  fixMacroblockQP( MbDataAccess& rcMbDataAccess ) = 0;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_RATEDISTORTIONIF_H__A4FCCF6E_E3BC_46A7_ACB8_7AE8CFE756E1__INCLUDED_)
