
#if !defined(AFX_CABACREADER_H__06F9800B_44E9_4FB9_9BBC_BF5E02AFBBB3__INCLUDED_)
#define AFX_CABACREADER_H__06F9800B_44E9_4FB9_9BBC_BF5E02AFBBB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MbSymbolReadIf.h"
#include "CabaDecoder.h"
#include "H264AVCCommonLib/CabacContextModel2DBuffer.h"
#include "H264AVCCommonLib/Quantizer.h"
#include "H264AVCCommonLib/ContextTables.h"

H264AVC_NAMESPACE_BEGIN


class CabacReader :
public MbSymbolReadIf
, private CabaDecoder
, public Quantizer

{
protected:
  CabacReader();
  virtual ~CabacReader();

public:
  static ErrVal create        ( CabacReader*& rpcCabacReader );
  ErrVal        destroy       ();

  ErrVal  startSlice          ( const SliceHeader& rcSliceHeader );
  ErrVal  finishSlice         ( ) { return Err::m_nOK; }

  ErrVal  init                ( BitReadBuffer* pcBitReadBuffer );
  ErrVal  uninit              ();

  Bool    isEndOfSlice        ();
  Bool    isMbSkipped         ( MbDataAccess& rcMbDataAccess, UInt& uiNextSkippedVLC );
  Bool    isBLSkipped         ( MbDataAccess& rcMbDataAccess );
  ErrVal  blockModes          ( MbDataAccess& rcMbDataAccess );
  ErrVal  mbMode              ( MbDataAccess& rcMbDataAccess );
  ErrVal  resPredFlag         ( MbDataAccess& rcMbDataAccess );

  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx );
  ErrVal  mvd                 ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx );
  ErrVal  cbp                 ( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop );
  ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  refFrame            ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );

  ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx );
  ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  );
  ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  );
  ErrVal  motionPredFlag      ( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  );

  ErrVal  residualBlock       ( MbDataAccess& rcMbDataAccess, LumaIdx   cIdx, ResidualMode eResidualMode, UInt& ruiMbExtCbp, UInt uiStart, UInt uiStop );
  ErrVal  residualBlock       ( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, ResidualMode eResidualMode, UInt uiStart, UInt uiStop );

  ErrVal  deltaQp             ( MbDataAccess& rcMbDataAccess );
  ErrVal  intraPredModeLuma   ( MbDataAccess& rcMbDataAccess, LumaIdx cIdx );
  ErrVal  intraPredModeChroma ( MbDataAccess& rcMbDataAccess );
	ErrVal  fieldFlag           ( MbDataAccess& rcMbDataAccess );
  ErrVal  samplesPCM          ( MbDataAccess& rcMbDataAccess );

  ErrVal  residualBlock8x8    ( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx, UInt uiStart, UInt uiStop );
  ErrVal  intraPredModeLuma8x8( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx );
  ErrVal  transformSize8x8Flag( MbDataAccess& rcMbDataAccess);

protected:

  ErrVal xGetMvd( MbDataAccess& rcMbDataAccess, Mv& rcMv, LumaIdx cIdx, ListIdx eLstIdx );

  ErrVal xInitContextModels( const SliceHeader& rcSliceHeader );
  ErrVal xGetMvdComponent( Short& rsMvdComp, UInt uiAbsSum, UInt uiCtx );
  ErrVal xRefFrame      ( MbDataAccess& rcMbDataAccess, UInt& ruiRefFrame, ListIdx eLstIdx, ParIdx8x8 eParIdx );
  ErrVal xMotionPredFlag( Bool& bFlag,       ListIdx eLstIdx );

  ErrVal xReadBCbp( MbDataAccess& rcMbDataAccess, Bool& rbCoded, ResidualMode eResidualMode, LumaIdx cIdx, UInt uiStart, UInt uiStop );
  ErrVal xReadBCbp( MbDataAccess& rcMbDataAccess, Bool& rbCoded, ResidualMode eResidualMode, ChromaIdx cIdx, UInt uiStart, UInt uiStop );
  ErrVal xReadCoeff( TCoeff*        piCoeff,
                     ResidualMode   eResidualMode,
                     const UChar*   pucScan,
                     Bool           bFrame,
                     UInt           uiStart,
                     UInt           uiStop );

protected:
  CabacContextModel2DBuffer m_cFieldFlagCCModel;
  CabacContextModel2DBuffer m_cFldMapCCModel;
  CabacContextModel2DBuffer m_cFldLastCCModel;
  CabacContextModel2DBuffer m_cBCbpCCModel;
  CabacContextModel2DBuffer m_cMapCCModel;
  CabacContextModel2DBuffer m_cLastCCModel;
  CabacContextModel2DBuffer m_cOneCCModel;
  CabacContextModel2DBuffer m_cAbsCCModel;
  CabacContextModel2DBuffer m_cChromaPredCCModel;
  CabacContextModel2DBuffer m_cBLSkipCCModel;
  CabacContextModel2DBuffer m_cMbTypeCCModel;
  CabacContextModel2DBuffer m_cBlockTypeCCModel;
  CabacContextModel2DBuffer m_cMvdCCModel;
  CabacContextModel2DBuffer m_cRefPicCCModel;
  CabacContextModel2DBuffer m_cMotPredFlagCCModel;
  CabacContextModel2DBuffer m_cResPredFlagCCModel;
  CabacContextModel2DBuffer m_cDeltaQpCCModel;
  CabacContextModel2DBuffer m_cIntraPredCCModel;
  CabacContextModel2DBuffer m_cCbpCCModel;
  CabacContextModel2DBuffer m_cTransSizeCCModel;

  UInt m_uiBitCounter;
  UInt m_uiPosCounter;
  UInt m_uiLastDQpNonZero;
};

H264AVC_NAMESPACE_END


#endif // !defined(AFX_CABACREADER_H__06F9800B_44E9_4FB9_9BBC_BF5E02AFBBB3__INCLUDED_)
