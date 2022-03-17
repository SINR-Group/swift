
#if !defined(AFX_LOOPFILTER_H__1860BB4C_C677_487A_A81F_0BD39DA40284__INCLUDED_)
#define AFX_LOOPFILTER_H__1860BB4C_C677_487A_A81F_0BD39DA40284__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class ControlMngIf;
class YuvPicBuffer;
class Frame;

class ReconstructionBypass;

class H264AVCCOMMONLIB_API LoopFilter
{
  enum LFPass
  {
    FIRST_PASS  = 0,
    SECOND_PASS = 1,
    ONE_PASS    = 1,
    TWO_PASSES  = 2
  };
  enum Dir
  {
    VER = 0,
    HOR = 1
  };
  typedef struct
  {
    UChar ucAlpha;
    UChar aucClip[5];
  } AlphaClip;
  static const UChar      g_aucBetaTab  [52];
  static const AlphaClip  g_acAlphaClip [52];

protected:
	LoopFilter();
	virtual ~LoopFilter();

public:
  static ErrVal create  ( LoopFilter*&           rpcLoopFilter );
  ErrVal        destroy ();
  ErrVal        init    ( ControlMngIf*          pcControlMngIf,
                          ReconstructionBypass*  pcReconstructionBypass,
                          Bool                   bEncoder );
  ErrVal        uninit  ();

  ErrVal        process ( SliceHeader&              rcSH,
                          Frame*                    pcFrame,
                          Frame*                    pcResidual,
                          MbDataCtrl*               pcMbDataCtrl,
                          const DBFilterParameter*  pcInterLayerDBParameter,
                          Bool                      bSpatialScalabilityFlag,
                          const MbStatus*           apcMbStatus = 0 );

private:
  ErrVal        xFilterMb             ( const MbDataCtrl*         pcMbDataCtrl,
                                        MbDataAccess&             rcMbDataAccess,
                                        YuvPicBuffer*             pcYuvBuffer,
                                        YuvPicBuffer*             pcResidual,
                                        const DBFilterParameter*  pcInterLayerDBParameter,
                                        Bool                      bSpatialScalableFlag,
                                        LFPass                    eLFPass,
                                        const SliceHeader*        pcDBSliceHeader );
#if PROPOSED_DEBLOCKING_APRIL2010
  ErrVal        xRecalcCBP            ( MbDataAccess&             rcMbDataAccess );
#endif

  //===== determination of filter strength =====
  UInt          xGetHorFilterStrength ( const MbDataAccess&       rcMbDataAccess,
                                        LumaIdx                   cIdx,
                                        Int                       iFilterIdc,
                                        Bool                      bInterLayerFlag,
                                        Bool                      bSpatialScalableFlag,
                                        LFPass                    eLFPass );
  UInt          xGetVerFilterStrength ( const MbDataAccess&       rcMbDataAccess,
                                        LumaIdx                   cIdx,
                                        Int                       iFilterIdc,
                                        Bool                      bInterLayerFlag,
                                        Bool                      bSpatialScalableFlag,
                                        LFPass                    eLFPass );
  const MbData& xGetMbDataLeft        ( const MbDataAccess&       rcMbDataAccess,
                                        LumaIdx&                  rcIdx );
  const MbData& xGetMbDataAbove       ( const MbDataAccess&       rcMbDataAccess );
  Bool          xFilterInsideEdges    ( const MbDataAccess&       rcMbDataAccess,
                                        Int                       iFilterIdc,
                                        Bool                      bInterLayerFlag,
                                        LFPass                    eLFPass );
  Bool          xFilterLeftEdge       ( const MbDataAccess&       rcMbDataAccess,
                                        Int                       iFilterIdc,
                                        Bool                      bInterLayerFlag,
                                        LFPass                    eLFPass );
  Bool          xFilterTopEdge        ( const MbDataAccess&       rcMbDataAccess,
                                        Int                       iFilterIdc,
                                        Bool                      bInterLayerFlag,
                                        LFPass                    eLFPass );
  UChar         xCheckMvDataP         ( const MbData&             rcQMbData,
                                        const LumaIdx             cQIdx,
                                        const MbData&             rcPMbData,
                                        const LumaIdx             cPIdx,
                                        const Short               sHorMvThr,
                                        const Short               sVerMvThr  );
  UChar         xCheckMvDataB         ( const MbData&             rcQMbData,
                                        const LumaIdx             cQIdx,
                                        const MbData&             rcPMbData,
                                        const LumaIdx             cPIdx,
                                        const Short               sHorMvThr,
                                        const Short               sVerMvThr );

  //===== filtering =====
  ErrVal        xLumaHorFiltering     ( const MbDataAccess&       rcMbDataAccess,
                                        const DBFilterParameter&  rcDFP,
                                        YuvPicBuffer*             pcYuvBuffer );
  ErrVal        xLumaVerFiltering     ( const MbDataAccess&       rcMbDataAccess,
                                        const DBFilterParameter&  rcDFP,
                                        YuvPicBuffer*             pcYuvBuffer );
  ErrVal        xChromaHorFiltering   ( const MbDataAccess&       rcMbDataAccess,
                                        const DBFilterParameter&  rcDFP,
                                        YuvPicBuffer*             pcYuvBuffer );
  ErrVal        xChromaVerFiltering   ( const MbDataAccess&       rcMbDataAccess,
                                        const DBFilterParameter&  rcDFP,
                                        YuvPicBuffer*             pcYuvBuffer );
  Void          xFilter               ( XPel*                     pFlt,
                                        const Int&                iOffset,
                                        const Int&                iIndexA,
                                        const Int&                iIndexB,
                                        const UChar&              ucBs,
                                        const Bool&               bLum );


protected:
  ControlMngIf*           m_pcControlMngIf;
  ReconstructionBypass*   m_pcReconstructionBypass;
  UChar                   m_aaaucBs[2][4][4];
  UChar                   m_aucBsHorTop[4];
  UChar                   m_aucBsVerBot[4];
  Bool                    m_bVerMixedMode;
  Bool                    m_bHorMixedMode;
  Bool                    m_bAddEdge;
  Bool                    m_bEncoder;
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_LOOPFILTER_H__1860BB4C_C677_487A_A81F_0BD39DA40284__INCLUDED_)
