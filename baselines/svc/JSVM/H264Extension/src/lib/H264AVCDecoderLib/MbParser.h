
#if !defined(AFX_MBPARSER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_)
#define AFX_MBPARSER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class Transform;
class MbSymbolReadIf;

class MbParser
{
protected:
	MbParser();
	virtual ~MbParser();

public:
  static ErrVal create  ( MbParser*&      rpcMbParser );
  ErrVal        destroy ();
  ErrVal        init    ();
  ErrVal        uninit  ();

  ErrVal initSlice      ( MbSymbolReadIf* pcMbSymbolReadIf );
  ErrVal read           ( MbDataAccess&   rcMbDataAccess,
                          UInt            uiNumMbRead,
                          Bool&           rbEndOfSlice,
                          UInt&           ruiNextSkippedVLC );

protected:
  Bool   xCheckSkipSliceMb    ( MbDataAccess& rcMbDataAccess, UInt uiNumMbRead, Bool& rbEndOfSlice );
  ErrVal xSkipMb              ( MbDataAccess& rcMbDataAccess );
  ErrVal xReadIntraPredModes  ( MbDataAccess& rcMbDataAccess );

  ErrVal xReadTextureInfo     ( MbDataAccess& rcMbDataAccess, Bool bTrafo8x8Flag, Bool bBaseLayerAvailable, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xScanChromaBlocks    ( MbDataAccess& rcMbDataAccess, UInt uiChromCbp, UInt uiStart = 0, UInt uiStop = 16 );
  ErrVal xReadMotionVectors   ( MbDataAccess& rcMbDataAccess, MbMode  eMbMode, ListIdx eLstIdx );
  ErrVal xReadReferenceIndices( MbDataAccess& rcMbDataAccess, MbMode  eMbMode, ListIdx eLstIdx );
  ErrVal xReadMotionPredFlags ( MbDataAccess& rcMbDataAccess, MbMode  eMbMode, ListIdx eLstIdx );
  ErrVal xGet8x8BlockMv       ( MbDataAccess& rcMbDataAccess, B8x8Idx c8x8Idx, ListIdx eLstIdx );

protected:
  Bool            m_bInitDone;
  MbSymbolReadIf* m_pcMbSymbolReadIf;
  Bool            m_bPrevIsSkipped;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBPARSER_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_)
