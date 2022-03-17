
#if !defined(AFX_MBSYMBOLREADIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_)
#define AFX_MBSYMBOLREADIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN


class MbSymbolReadIf
{
protected:
  MbSymbolReadIf() {}
	virtual ~MbSymbolReadIf() {}

public:
  virtual Bool    isMbSkipped ( MbDataAccess& rcMbDataAccess, UInt& uiNextSkippedVLC ) = 0;
  virtual Bool    isBLSkipped ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual Bool    isEndOfSlice()                               = 0;
  virtual ErrVal  blockModes  ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  mbMode      ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  resPredFlag ( MbDataAccess& rcMbDataAccess ) = 0;

  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx ) = 0;
  virtual ErrVal  mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx ) = 0;

  virtual ErrVal  cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop ) = 0;
  virtual ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;

  virtual ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx ) = 0;
  virtual ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  ) = 0;
  virtual ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  ) = 0;
  virtual ErrVal  motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  ) = 0;

  virtual ErrVal  residualBlock( MbDataAccess& rcMbDataAccess, LumaIdx   cIdx, ResidualMode eResidualMode, UInt& ruiMbExtCbp, UInt uiStart = 0, UInt uiStop = 16 ) = 0;
  virtual ErrVal  residualBlock( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, ResidualMode eResidualMode, UInt uiStart = 0, UInt uiStop = 16 ) = 0;

  virtual ErrVal  deltaQp             ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  intraPredModeLuma   ( MbDataAccess& rcMbDataAccess, LumaIdx cIdx ) = 0;
  virtual ErrVal  intraPredModeChroma ( MbDataAccess& rcMbDataAccess ) = 0;
	virtual ErrVal  fieldFlag           ( MbDataAccess& rcMbDataAccess ) = 0;
  virtual ErrVal  samplesPCM          ( MbDataAccess& rcMbDataAccess ) = 0;

  virtual ErrVal  startSlice          ( const SliceHeader& rcSliceHeader ) = 0;
  virtual ErrVal  finishSlice         ( ) = 0;

  virtual ErrVal  transformSize8x8Flag( MbDataAccess& rcMbDataAccess) = 0;
  virtual ErrVal  residualBlock8x8    ( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx, UInt uiStart = 0, UInt uiStop = 16 ) = 0;
  virtual ErrVal  intraPredModeLuma8x8( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx ) = 0;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBSYMBOLREADIF_H__E5736EAC_0841_4E22_A1F0_ACAD8A7E5490__INCLUDED_)
