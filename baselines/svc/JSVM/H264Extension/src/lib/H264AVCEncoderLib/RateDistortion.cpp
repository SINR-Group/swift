
#include "H264AVCEncoderLib.h"
#include "CodingParameter.h"
#include "math.h"
#include "RateDistortion.h"

H264AVC_NAMESPACE_BEGIN


RateDistortion::RateDistortion():
  m_uiCostFactorMotionSAD( 0 ),
  m_uiCostFactorMotionSSE( 0 )
{
}

RateDistortion::~RateDistortion()
{
}

ErrVal RateDistortion::create( RateDistortion *&rpcRateDistortion )
{
  rpcRateDistortion = new RateDistortion;

  ROT( NULL == rpcRateDistortion );

  return Err::m_nOK;
}

Double RateDistortion::getCost( UInt uiBits, UInt uiDistortion )
{
  Double d = ((Double)uiDistortion + (Double)(Int)uiBits * m_dCost+.5);
  return (Double)(UInt)floor(d);
}

Double RateDistortion::getFCost( UInt uiBits, UInt uiDistortion )
{
  Double d = (((Double)uiDistortion) + ((Double)uiBits * m_dCost));
  return d;
}


ErrVal RateDistortion::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal RateDistortion::fixMacroblockQP( MbDataAccess& rcMbDataAccess )
{
  if( !( rcMbDataAccess.getMbData().getMbCbp() || rcMbDataAccess.getMbData().isIntra16x16() ) ) // has no coded texture
  {
    rcMbDataAccess.getMbData().setQp( rcMbDataAccess.getLastQp() );
  }
  return Err::m_nOK;
}


ErrVal
RateDistortion::setMbQpLambda( MbDataAccess& rcMbDataAccess, UInt uiQp, Double dLambda )
{
  rcMbDataAccess.getMbData().setQp( uiQp );

  m_dCost                 = dLambda;
  m_dSqrtCost             = sqrt( dLambda );
  m_uiCostFactorMotionSAD = (UInt)floor(65536.0 * sqrt( m_dCost ));
  m_uiCostFactorMotionSSE = (UInt)floor(65536.0 *       m_dCost  );

  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
