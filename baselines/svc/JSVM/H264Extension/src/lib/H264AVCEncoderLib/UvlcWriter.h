
#if !defined(AFX_UVLCWRITER_H__EA98D347_89D5_4D2D_B6D5_FB3A374CD295__INCLUDED_)
#define AFX_UVLCWRITER_H__EA98D347_89D5_4D2D_B6D5_FB3A374CD295__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MbSymbolWriteIf.h"
#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"
#include "BitWriteBufferIf.h"



H264AVC_NAMESPACE_BEGIN

#define REFSYM_MB                                     384

class UvlcWriter :
public MbSymbolWriteIf
, public HeaderSymbolWriteIf

{
public://
//protected://JVT-X046
	UvlcWriter( Bool bTraceEnable = false );
	virtual ~UvlcWriter();

  ErrVal xWriteSigRunCode ( UInt uiSymbol, UInt uiTableIdx );
  ErrVal xWriteUnaryCode (UInt uiSymbol );
  ErrVal xWriteCodeCB1 (UInt uiSymbol );
  ErrVal xWriteCodeCB2 (UInt uiSymbol );
  ErrVal xWriteGolomb(UInt uiSymbol, UInt uiK);
  ErrVal xEncodeMonSeq ( UInt* auiSeq, UInt uiStartVal, UInt uiLen );

public:
  static ErrVal create( UvlcWriter*& rpcUvlcWriter, Bool bTraceEnable = true );
  ErrVal destroy();

  ErrVal init(  BitWriteBufferIf* pcBitWriteBufferIf );
  ErrVal uninit();

  HeaderSymbolWriteIf* getHeaderSymbolWriteIfNextSlice( Bool bStartNewBitstream ) { return xGetUvlcWriterNextSlice( bStartNewBitstream ); }
  MbSymbolWriteIf*     getSymbolWriteIfNextSlice()                                { return xGetUvlcWriterNextSlice( false ); }
  Void                 setTraceEnableBit( Bool bActive )                          { m_bTraceEnable = bActive; }

  ErrVal  startSlice( const SliceHeader& rcSliceHeader );
  ErrVal  getLastByte(UChar &uiLastByte, UInt &uiLastBitPos); //FIX_FRAG_CAVLC
  ErrVal  setFirstBits(UChar ucByte,UInt uiLastBitPos); //FIX_FRAG_CAVLC
  ErrVal  finishSlice();

  ErrVal  blockModes( MbDataAccess& rcMbDataAccess );
  ErrVal  mbMode     ( MbDataAccess& rcMbDataAccess );
  ErrVal  resPredFlag( MbDataAccess& rcMbDataAccess );
  ErrVal  fieldFlag  ( MbDataAccess& rcMbDataAccess );

  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx );
  ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx );

  ErrVal  cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop );

  ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );

  ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );

  ErrVal  residualBlock( MbDataAccess& rcMbDataAccess, LumaIdx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal  residualBlock( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal  transformSize8x8Flag( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop );
  ErrVal  residualBlock8x8    ( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 );

  ErrVal  deltaQp( MbDataAccess& rcMbDataAccess );
  ErrVal  intraPredModeLuma( MbDataAccess& rcMbDataAccess, LumaIdx cIdx );
  ErrVal  intraPredModeChroma( MbDataAccess& rcMbDataAccess );
  ErrVal  samplesPCM( MbDataAccess& rcMbDataAccess );
  ErrVal  skipFlag( MbDataAccess& rcMbDataAccess );
  ErrVal  BLSkipFlag( MbDataAccess& rcMbDataAccess );
  ErrVal  terminatingBit ( UInt uiIsLast ) { return Err::m_nOK;}

  ErrVal writeUvlc ( UInt uiCode,                const Char* pcTraceString );
  ErrVal writeSvlc ( Int  iCode,                 const Char* pcTraceString );
  ErrVal writeCode ( UInt uiCode, UInt uiLength, const Char* pcTraceString );
  ErrVal writeSCode( Int  iCode,  UInt uiLength, const Char* pcTraceString );
  ErrVal writeFlag ( Bool bFlag,                 const Char* pcTraceString );

  UInt getNumberOfWrittenBits();

private:
  UvlcWriter* xGetUvlcWriterNextSlice( Bool bStartNewBitstream );
protected:

  UInt xGetCoeffCost() { return m_uiCoeffCost; }
  Void xSetCoeffCost(UInt uiCost) { m_uiCoeffCost = uiCost; }

  ErrVal xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, LumaIdx cIdx, UInt uiCoeffCount, UInt uiTrailingOnes, UInt uiStart, UInt uiStop, Bool bDC );
  ErrVal xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, UInt uiCoeffCount, UInt uiTrailingOnes, UInt uiStart, UInt uiStop );

  ErrVal xWriteMvd( Mv cMv );

  ErrVal xWriteTrailingOnes4( UInt uiCoeffCount, UInt uiTrailingOnes );
  ErrVal xWriteTrailingOnes16( UInt uiLastCoeffCount, UInt uiCoeffCount, UInt uiTrailingOnes );
  ErrVal xWriteTotalRun4( UInt uiVlcPos, UInt uiTotalRun );
  ErrVal xWriteTotalRun8( UInt uiVlcPos, UInt uiTotalRun );
  ErrVal xWriteTotalRun16( UInt uiVlcPos, UInt uiTotalRun );
  ErrVal xWriteLevelVLC0( Int iLevel );
  ErrVal xWriteLevelVLCN( Int iLevel, UInt uiVlcLength );
  ErrVal xWriteRun( UInt uiVlcPos, UInt uiRun );
  ErrVal xWriteRunLevel( Int* aiLevelRun, UInt uiCoeffCnt, UInt uiTrailingOnes, UInt uiMaxCoeffs, UInt uiTotalRun, MbDataAccess &rcMbDataAccess, Bool bDefaultScanIdx );
  ErrVal xWriteRefFrame( Bool bWriteBit, UInt uiRefFrame );
  ErrVal xWriteMotionPredFlag( Bool bFlag );

  ErrVal xWriteUvlcCode( UInt uiVal);
  ErrVal xWriteSvlcCode( Int iVal);

	//JVT-X046 {
	UInt getBitCounter(void)					{return m_uiBitCounter;				}
	UInt getPosCounter(void)					{return m_uiPosCounter;				}

	UInt getCoeffCost(void)						{return m_uiCoeffCost;				}
	Bool getTraceEnable(void)					{return m_bTraceEnable;				}

	Bool getRunLengthCoding(void)			{return m_bRunLengthCoding;		}
	UInt getRun(void)									{return m_uiRun;							}

	BitWriteBufferIf* getBitWriteBufferIf(void){return m_pcBitWriteBufferIf;}
	void loadCabacWrite(MbSymbolWriteIf *pcMbSymbolWriteIf)	{}
	void loadUvlcWrite(MbSymbolWriteIf *pcMbSymbolWriteIf);
	UInt getBitsWritten(void) {  return m_pcBitWriteBufferIf->getBitsWritten(); }
	//JVT-X046 }

protected:
  UInt xConvertToUInt( Int iValue )  {  return ( iValue <= 0) ? -iValue<<1 : (iValue<<1)-1; }

private:
  __inline ErrVal xWriteCodeNT( UInt uiCode, UInt uiLength );
  __inline ErrVal xWriteCode  ( UInt uiCode, UInt uiLength );
  __inline ErrVal xWriteFlag  ( UInt uiCode );

protected:
  BitWriteBufferIf* m_pcBitWriteBufferIf;
  UInt m_uiBitCounter;
  UInt m_uiPosCounter;

  UInt m_uiCoeffCost;
  Bool m_bTraceEnable;

  Bool m_bRunLengthCoding;
  UInt m_uiRun;

  // new variables for switching bitstream inputs
  UvlcWriter       *m_pcNextUvlcWriter;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_UVLCWRITER_H__EA98D347_89D5_4D2D_B6D5_FB3A374CD295__INCLUDED_)
