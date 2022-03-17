
#if !defined(AFX_MBCODER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_)
#define AFX_MBCODER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MbSymbolWriteIf.h"
#include "RateDistortionIf.h"
//JVT-X046 {
//#include "CabacWriter.h"
//#include "UvlcWriter.h"
//JVT-X046 }

H264AVC_NAMESPACE_BEGIN

class MbCoder
{
protected:
	MbCoder();
	virtual ~MbCoder();

public:
  static ErrVal create( MbCoder*& rpcMbCoder );
  ErrVal destroy();

  ErrVal initSlice( const SliceHeader& rcSH,
                    MbSymbolWriteIf* pcMbSymbolWriteIf,
                    RateDistortionIf* pcRateDistortionIf );


  ErrVal uninit();

  ErrVal  encode            ( MbDataAccess& rcMbDataAccess,
                              MbDataAccess* pcMbDataAccessBase,
                              Bool          bTerminateSlice ,
                              Bool          bSendTerminateSlice);
  UInt    getBitCount       ()  { return m_pcMbSymbolWriteIf->getNumberOfWrittenBits(); }

	//JVT-X046 {
  UInt getBitsWritten(void) { return m_pcMbSymbolWriteIf->getBitsWritten(); }

  Bool bSliceCodedDone;
  UInt m_uiSliceMode;
  UInt m_uiSliceArgument;
	//JVT-X046 }

protected:
  ErrVal xWriteIntraPredModes ( MbDataAccess& rcMbDataAccess );
  ErrVal xWriteMotionPredFlags    ( MbDataAccess& rcMbDataAccess,
                                    MbMode        eMbMode,
                                    ListIdx       eLstIdx );
  ErrVal xWriteReferenceFrames    ( MbDataAccess& rcMbDataAccess,
                                    MbMode        eMbMode,
                                    ListIdx       eLstIdx );
  ErrVal xWriteMotionVectors      ( MbDataAccess& rcMbDataAccess,
                                    MbMode        eMbMode,
                                    ListIdx       eLstIdx );


  //-- JVT-R091
  ErrVal xWriteTextureInfo    ( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase, const MbTransformCoeffs& rcMbTCoeff, Bool bTrafo8x8Flag, UInt uiStart, UInt uiStop, UInt uiMGSFragment );
	//--
  ErrVal xWriteBlockMv        ( MbDataAccess& rcMbDataAccess, B8x8Idx c8x8Idx, ListIdx eLstIdx );


  ErrVal xScanLumaIntra16x16  ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, Bool bAC, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanLumaBlock       ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, LumaIdx cIdx, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanChromaDc        ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanChromaAcU       ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanChromaAcV       ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanChromaBlocks    ( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiChromCbp, UInt uiStart = 0, UInt uiStop = 16 );

protected:
  MbSymbolWriteIf* m_pcMbSymbolWriteIf;
  RateDistortionIf* m_pcRateDistortionIf;

  BitWriteBuffer*  m_pcBitWriteBufferCabac;
  BitWriteBuffer*  m_pcBitWriteBufferUvlc;
  //JVT-X046 {
  MbSymbolWriteIf* m_pcCabacSymbolWriteIf;
  MbSymbolWriteIf* m_pcUvlcSymbolWriteIf;
  //JVT-X046 }

  Bool m_bInitDone;
  Bool  m_bCabac;
  Bool  m_bPrevIsSkipped;
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_MBCODER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_)
