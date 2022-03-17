
#if !defined(AFX_INVERSETRANSFORM_H__B2D732EC_10EA_4C2F_9387_4456CFCA4439__INCLUDED_)
#define AFX_INVERSETRANSFORM_H__B2D732EC_10EA_4C2F_9387_4456CFCA4439__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/Quantizer.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"

H264AVC_NAMESPACE_BEGIN

class H264AVCCOMMONLIB_API CoeffLevelPred
{
protected:
	CoeffLevelPred();
	virtual ~CoeffLevelPred();

public:

  ErrVal ScaleCoeffLevels       ( TCoeff* piCoeff,
                                  TCoeff* piRefCoeff,
                                  UInt uiQp,
                                  UInt uiRefQp,
                                  UInt uiNumCoeff );

};

class H264AVCCOMMONLIB_API Transform :
public Quantizer
, CoeffLevelPred
{
protected:
	Transform();
	virtual ~Transform();

public:
  static ErrVal create( Transform*& rpcTransform );
  ErrVal destroy();
  Void setClipMode( Bool bEnableClip ) { m_bClip = bEnableClip; }

  Bool getClipMode()                   { return m_bClip; }

  // JVT-V035 functions for SVC to AVC rewrite
  ErrVal        predict4x4Blk             ( TCoeff* piCoeff, TCoeff* piRef, UInt uiRefQp, UInt& ruiAbsSum );
  ErrVal        predict8x8Blk             ( TCoeff* piCoeff, TCoeff* piRef, UInt uiRefQp, UInt& ruiAbsSum );
  ErrVal        predictChromaBlocks       ( UInt uiComp, TCoeff* piCoeff, TCoeff* piRef, UInt uiRefQp, UInt& ruiDcAbs, UInt& ruiAcAbs );
  ErrVal        predictScaledACCoeffs     ( UInt uiComp, TCoeff* piCoeff, TCoeff* piRef, UInt uiRefQp, const UChar* pucScale );
  ErrVal        predictScaledChromaCoeffs ( TCoeff* piCoeff, TCoeff* piRef, UInt uiRefCbQp, UInt uiRefCrQp, const UChar* pucScaleCb, const UChar* pucScaleCr );
  ErrVal        predictMb16x16            ( TCoeff* piCoeff, TCoeff* piRef, UInt uiRefQp, UInt& ruiDcAbs, UInt& ruiAcAbs );
  ErrVal        addPrediction4x4Blk       ( TCoeff* piCoeff, TCoeff* piRefCoeff, UInt uiDstQp, UInt uiRefQp, UInt &uiCoded  );
  ErrVal        addPrediction8x8Blk       ( TCoeff* piCoeff, TCoeff* piRefCoeff, UInt uiQp, UInt uiRefQp, Bool& bCoded  );
  ErrVal        addPredictionChromaBlocks ( TCoeff* piCoeff, TCoeff* piRef, UInt uiQp, UInt uiRefQp, Bool& bDCflag, Bool& bACflag );

  ErrVal        transform8x8Blk           ( YuvMbBuffer*       pcOrgData,
                                            YuvMbBuffer*       pcPelData,
                                            TCoeff*               piCoeff,
                                            const UChar*          pucScale,
                                            UInt&                 ruiAbsSum );
  ErrVal        transform4x4Blk           ( YuvMbBuffer*       pcOrgData,
                                            YuvMbBuffer*       pcPelData,
                                            TCoeff*               piCoeff,
                                            const UChar*          pucScale,
                                            UInt&                 ruiAbsSum );
  ErrVal        transformMb16x16          ( YuvMbBuffer* pcOrgData,
                                            YuvMbBuffer* pcPelData,
                                            TCoeff* piCoeff,
                                            const UChar* pucScale,
                                            UInt& ruiDcAbs,
                                            UInt& ruiAcAbs );
  ErrVal        transformChromaBlocks     ( XPel* pucOrg,
                                            XPel* pucRec,
                                            const CIdx            cCIdx,
                                            Int iStride,
                                            TCoeff* piCoeff,
                                            TCoeff* piQuantCoeff,
                                            const UChar* pucScale,
                                            UInt& ruiDcAbs,
                                            UInt& ruiAcAbs );

  ErrVal        invTransform8x8Blk        ( XPel* puc, Int iStride, TCoeff* piCoeff );
  ErrVal        invTransform4x4Blk        ( XPel* puc, Int iStride, TCoeff* piCoeff );
  ErrVal        invTransformChromaBlocks  ( XPel* puc, Int iStride, TCoeff* piCoeff );

  ErrVal        invTransform4x4Blk        ( Pel*  puc, Int iStride, TCoeff* piCoeff );
  ErrVal        invTransformChromaBlocks  ( Pel*  puc, Int iStride, TCoeff* piCoeff );

  ErrVal        invTransformDcCoeff       ( TCoeff* piCoeff, const Int iQpScale, const Int iQpPer );
  ErrVal        invTransformDcCoeff       ( TCoeff* piCoeff, Int iQpScale );
  Void          invTransformChromaDc      ( TCoeff* piCoeff );

  void         setStoreCoeffFlag(Bool flag)   { m_storeCoeffFlag = flag;}
  Bool         getStoreCoeffFlag()        {return  m_storeCoeffFlag;}
  ErrVal       transform4x4BlkCGS         ( YuvMbBuffer*         pcOrgData,
                                            YuvMbBuffer*         pcPelData,
                                            TCoeff*                 piCoeff,
                                            TCoeff*                 piCoeffBase,
                                            const UChar*            pucScale,
                                            UInt&                   ruiAbsSum );
  ErrVal       transform8x8BlkCGS           ( YuvMbBuffer*       pcOrgData,
                                            YuvMbBuffer*         pcPelData,
                                            TCoeff*                 piCoeff,
                                            TCoeff*                 piCoeffBase,
                                            const UChar*            pucScale,
                                            UInt&                   ruiAbsSum );

  ErrVal        transformChromaBlocksCGS  ( XPel* pucOrg,
                                            XPel* pucRec,
                                            const CIdx            cCIdx,
                                            Int iStride,
                                            TCoeff* piCoeff,
                                            TCoeff* piQuantCoeff,
                                            TCoeff* piCoeffBase,
                                            const UChar* pucScale,
                                            UInt& ruiDcAbs,
                                            UInt& ruiAcAbs );

private:
  Void xForTransform8x8Blk      ( XPel* pucOrg, XPel* pucRec, Int iStride, TCoeff* piPredCoeff );
  Void xForTransform4x4Blk      ( XPel* pucOrg, XPel* pucRec, Int iStride, TCoeff* piPredCoeff );

  Void xInvTransform4x4Blk      ( XPel* puc, Int iStride, TCoeff* piCoeff );
  Void xInvTransform4x4Blk      ( Pel*  puc, Int iStride, TCoeff* piCoeff );

  Void xInvTransform4x4BlkNoAc  ( XPel* puc, Int iStride, TCoeff* piCoeff );
  Void xInvTransform4x4BlkNoAc  ( Pel*  puc, Int iStride, TCoeff* piCoeff );

  Void xForTransformChromaDc    ( TCoeff* piCoeff );
  Void xForTransformLumaDc      ( TCoeff* piCoeff );

  Int  xRound                   ( Int i     )             { return ((i)+(1<<5))>>6; }
  Int  xClip                    ( Int iPel  )             { return ( m_bClip ? gClip( iPel ) : iPel); }

  Void xQuantDequantUniform8x8      ( TCoeff* piQCoeff,
                                      TCoeff* piCoeff,
                                      const QpParameter& rcQp,
                                      const UChar* pucScale,
                                      UInt& ruiAbsSum );
  Void xQuantDequantUniform4x4      ( TCoeff* piQCoeff,
                                      TCoeff* piCoeff,
                                      const QpParameter& rcQp,
                                      const UChar* pucScale,
                                      UInt& ruiAbsSum );
  Void xQuantDequantNonUniformLuma  ( TCoeff* piQCoeff,
                                      TCoeff* piCoeff,
                                      const QpParameter& rcQp,
                                      const UChar* pucScale,
                                      UInt& ruiDcAbs,
                                      UInt& ruiAcAbs );
  Void xQuantDequantNonUniformChroma( TCoeff* piQCoeff,
                                      TCoeff* piCoeff,
                                      const QpParameter& rcQp,
                                      const UChar* pucScale,
                                      UInt& ruiDcAbs,
                                      UInt& ruiAcAbs );

  Void x4x4DequantChroma  ( TCoeff*             piQCoeff,
                            TCoeff*             piCoeff,
                            const QpParameter&  rcQp,
                            const UChar*        pucScale );

protected:
  const SliceHeader*  m_pcSliceHeader;
  Bool                m_bClip;
  Bool                m_storeCoeffFlag;
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_INVERSETRANSFORM_H__B2D732EC_10EA_4C2F_9387_4456CFCA4439__INCLUDED_)
