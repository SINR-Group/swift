
#if !defined(AFX_MBSYMBOLWRITEIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_)
#define AFX_MBSYMBOLWRITEIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BitWriteBuffer.h"


// h264 namepace begin
H264AVC_NAMESPACE_BEGIN

class BitWriteBufferIf;


class MbSymbolWriteIf
{
public://
//protected://JVT-X046
  MbSymbolWriteIf() {}
	virtual ~MbSymbolWriteIf() {}

public:

  virtual Void             setTraceEnableBit( Bool bActive ) = 0;
  virtual MbSymbolWriteIf* getSymbolWriteIfNextSlice() = 0;

  virtual ErrVal  blockModes          ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  mbMode              ( MbDataAccess& rcMbDataAccess /*, Bool bBLQRefFlag*/ ) = 0;
  virtual ErrVal  resPredFlag         ( MbDataAccess& rcMbDataAccess ) = 0;

  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx ) = 0;
  virtual ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx ) = 0;

  virtual ErrVal  cbp                 ( MbDataAccess& rcMbDataAccess, UInt uiStart = 0, UInt uiStop = 16 ) = 0;

  virtual ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;

  virtual ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;

  virtual ErrVal  residualBlock       ( MbDataAccess& rcMbDataAccess, LumaIdx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 ) = 0;
  virtual ErrVal  residualBlock       ( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 ) = 0;

  virtual ErrVal  transformSize8x8Flag( MbDataAccess& rcMbDataAccess, UInt uiStart = 0, UInt uiStop = 16 ) = 0;
  virtual ErrVal  residualBlock8x8    ( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 ) = 0;

  virtual ErrVal  deltaQp             ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  intraPredModeLuma   ( MbDataAccess& rcMbDataAccess, LumaIdx cIdx ) = 0;
  virtual ErrVal  intraPredModeChroma ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  samplesPCM          ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  skipFlag            ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  BLSkipFlag          ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  terminatingBit      ( UInt uiIsLast ) = 0;
  virtual UInt    getNumberOfWrittenBits() = 0;
  virtual ErrVal  fieldFlag           ( MbDataAccess& rcMbDataAccess ) = 0;

  virtual ErrVal  startSlice          ( const SliceHeader& rcSliceHeader ) = 0;
  virtual ErrVal  getLastByte         (UChar &uiLastByte, UInt &uiLastBitPos) = 0; //FIX_FRAG_CAVLC
  virtual ErrVal  setFirstBits(UChar ucByte,UInt uiLastBitPos) = 0; //FIX_FRAG_CAVLC
  virtual ErrVal  finishSlice         ( ) = 0;
	//JVT-X046 {
	virtual void loadCabacWrite( MbSymbolWriteIf* pcMbSymbolWriteIf ) = 0;
	virtual void loadUvlcWrite( MbSymbolWriteIf* pcMbSymbolWriteIf ) = 0;
    virtual UInt getBitsWritten(void) = 0;
  //JVT-X046 }
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBSYMBOLWRITEIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_)
