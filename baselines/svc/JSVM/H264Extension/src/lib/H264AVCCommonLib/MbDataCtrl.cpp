
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbData.h"
#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/Frame.h"

#include "H264AVCCommonLib/CFMO.h"
#include <math.h>

H264AVC_NAMESPACE_BEGIN


#define MEDIAN(a,b,c)  ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))


PosCalcParam::PosCalcParam( ResizeParameters& rcResizeParameters )
{
  m_bRSChangeFlag = rcResizeParameters.getRestrictedSpatialResolutionChangeFlag();
  m_iBaseMbAff    = ( rcResizeParameters.m_bRefLayerIsMbAffFrame  ? 1 : 0 );
  m_iCurrMbAff    = ( rcResizeParameters.m_bIsMbAffFrame          ? 1 : 0 );
  m_iBaseField    = ( rcResizeParameters.m_bRefLayerFieldPicFlag  ? 1 : 0 );
  m_iCurrField    = ( rcResizeParameters.m_bFieldPicFlag          ? 1 : 0 );
  m_iBaseBotField = ( rcResizeParameters.m_bRefLayerBotFieldFlag  ? 1 : 0 );
  m_iCurrBotField = ( rcResizeParameters.m_bBotFieldFlag          ? 1 : 0 );
  m_iRefW         = rcResizeParameters.m_iRefLayerFrmWidth;
  m_iRefH         = rcResizeParameters.m_iRefLayerFrmHeight  >> m_iBaseField;
  m_iScaledW      = rcResizeParameters.m_iScaledRefFrmWidth;
  m_iScaledH      = rcResizeParameters.m_iScaledRefFrmHeight >> m_iCurrField;
  m_iOffsetX      = rcResizeParameters.m_iLeftFrmOffset;
  m_iOffsetY      = rcResizeParameters.m_iTopFrmOffset       >> m_iCurrField;
  m_iShiftX       = ( rcResizeParameters.m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( m_iRefW ) );
  m_iShiftY       = ( rcResizeParameters.m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( m_iRefH ) );
  m_iScaleX       = ( ( (UInt)m_iRefW << m_iShiftX ) + ( m_iScaledW >> 1 ) ) / m_iScaledW;
  m_iScaleY       = ( ( (UInt)m_iRefH << m_iShiftY ) + ( m_iScaledH >> 1 ) ) / m_iScaledH;
  m_iAddX         = ( 1 << ( m_iShiftX - 1 ) ) - m_iOffsetX * m_iScaleX;
  m_iAddY         = ( 1 << ( m_iShiftY - 1 ) ) - m_iOffsetY * m_iScaleY;
}

MvScaleParam::MvScaleParam( ResizeParameters& rcResizeParameters, RefFrameList* pcRefFrameList0, RefFrameList* pcRefFrameList1 )
{
  m_bRSChangeFlag   = rcResizeParameters.getRestrictedSpatialResolutionChangeFlag();
  m_bCropChangeFlag = rcResizeParameters.getCroppingChangeFlag();
  m_iCurrMbAff      = ( rcResizeParameters.m_bIsMbAffFrame          ? 1 : 0 );
  m_iBaseField      = ( rcResizeParameters.m_bRefLayerFieldPicFlag  ? 1 : 0 );
  m_iCurrField      = ( rcResizeParameters.m_bFieldPicFlag          ? 1 : 0 );
  m_iRefW           = rcResizeParameters.m_iRefLayerFrmWidth;
  m_iRefH           = rcResizeParameters.m_iRefLayerFrmHeight;
  m_iScaledW        = rcResizeParameters.m_iScaledRefFrmWidth;
  m_iScaledH        = rcResizeParameters.m_iScaledRefFrmHeight;
  m_iOffsetX        = rcResizeParameters.m_iLeftFrmOffset;
  m_iOffsetY        = rcResizeParameters.m_iTopFrmOffset;
  m_iScaleX         = (Int)( ( ( (UInt64)m_iScaledW << 16 ) + ( m_iRefW >> 1 ) ) / m_iRefW );
  m_iScaleY         = (Int)( ( ( (UInt64)m_iScaledH << 16 ) + ( m_iRefH >> 1 ) ) / m_iRefH );

  if( m_bCropChangeFlag )
  {
    Int iCurrLO = rcResizeParameters.m_iLeftFrmOffset;
    Int iCurrTO = rcResizeParameters.m_iTopFrmOffset;
    Int iCurrRO = rcResizeParameters.m_iFrameWidth  - rcResizeParameters.m_iLeftFrmOffset - rcResizeParameters.m_iScaledRefFrmWidth;
    Int iCurrBO = rcResizeParameters.m_iFrameHeight - rcResizeParameters.m_iTopFrmOffset - rcResizeParameters.m_iScaledRefFrmHeight;

    for( Int iListIdx = 0; iListIdx < 2; iListIdx++ )
    {
      RefFrameList* pcRefList = ( iListIdx  ? pcRefFrameList1        : pcRefFrameList0 );
      m_aiNumActive[iListIdx] = ( pcRefList ? pcRefList->getActive() : 0 );

      for( Int iIdx = 0; iIdx < m_aiNumActive[iListIdx]; iIdx++ )
      {
        AOF( pcRefList->getEntry( (UInt)iIdx ) );
        const PictureParameters&  rcPP  = pcRefList->getEntry( (UInt)iIdx )->getPicParameters();
        Int iRefLO  = rcPP.m_iLeftFrmOffset;
        Int iRefTO  = rcPP.m_iTopFrmOffset;
        Int iRefRO  = rcResizeParameters.m_iFrameWidth  - rcPP.m_iLeftFrmOffset - rcPP.m_iScaledRefFrmWidth;
        Int iRefBO  = rcResizeParameters.m_iFrameHeight - rcPP.m_iTopFrmOffset - rcPP.m_iScaledRefFrmHeight;
        m_aaidOX[iListIdx][iIdx] = iCurrLO - iRefLO;
        m_aaidOY[iListIdx][iIdx] = iCurrTO - iRefTO;
        m_aaidSW[iListIdx][iIdx] = iCurrRO - iRefRO + m_aaidOX[iListIdx][iIdx];
        m_aaidSH[iListIdx][iIdx] = iCurrBO - iRefBO + m_aaidOY[iListIdx][iIdx];
      }
    }
  }
}


MotionUpsampling::MotionUpsampling( MbDataCtrl&       rcMbDataCtrlCurr,
                                    SliceHeader&      rcSliceHeaderCurr,
                                    ResizeParameters& rcResizeParameters,
                                    MbDataCtrl&       rcMbDataCtrlBase,
                                    RefFrameList*     pcRefFrameList0,
                                    RefFrameList*     pcRefFrameList1,
                                    Bool              bFieldMacroblocks,
                                    Bool              bCheckResidualPred,
                                    Int               iMvThreshold )
: m_bCheckResidualPred  ( bCheckResidualPred )
, m_bDirect8x8Inference ( rcSliceHeaderCurr.getSPS().getDirect8x8InferenceFlag() )
, m_iMvThreshold        ( iMvThreshold )
, m_bCurrFieldMb        ( bFieldMacroblocks )
, m_eSliceType          ( rcSliceHeaderCurr.getSliceType   () )
, m_iRefLayerDQId       ( rcSliceHeaderCurr.getRefLayerDQId() )
, m_iMaxListIdx         ( m_eSliceType == B_SLICE ? 2 : ( m_eSliceType == I_SLICE ? 0 : 1 ) )
, m_bSCoeffPred         ( rcSliceHeaderCurr.getSCoeffResidualPredFlag   () )
, m_bTCoeffPred         ( rcSliceHeaderCurr.getTCoeffLevelPredictionFlag() )
, m_rcMbDataCtrlCurr    ( rcMbDataCtrlCurr )
, m_rcResizeParameters  ( rcResizeParameters )
, m_cPosCalc            ( rcResizeParameters )
, m_cMvScale            ( rcResizeParameters, pcRefFrameList0, pcRefFrameList1 )
, m_rcMbDataCtrlBase    ( rcMbDataCtrlBase )
, m_iMbX0CropFrm        ( ( rcResizeParameters.m_iLeftFrmOffset + 15 ) / 16 )
, m_iMbY0CropFrm        ( ( rcResizeParameters.m_iTopFrmOffset + 15 ) / 16 )
, m_iMbX1CropFrm        ( ( rcResizeParameters.m_iLeftFrmOffset + rcResizeParameters.m_iScaledRefFrmWidth  ) / 16 )
, m_iMbY1CropFrm        ( ( rcResizeParameters.m_iTopFrmOffset + rcResizeParameters.m_iScaledRefFrmHeight ) / 16 )
{
}


MotionUpsampling::~MotionUpsampling()
{
}


ErrVal
MotionUpsampling::resample( Int iMbXCurr, Int iMbYCurr )
{
  RNOK( xInitMb         ( iMbXCurr, iMbYCurr ) );
  RNOK( xSetPartIdcArray() );

  if( m_bInCropWindow && ! m_bIntraBL )
  {
    for( Int iListIdx = 0; iListIdx < m_iMaxListIdx; iListIdx++ )
    {
      RNOK( xGetRefIdxAndInitialMvPred( ListIdx( iListIdx ) ) );
    }
    for( Int iB8x8Idx = 0; iB8x8Idx < 4; iB8x8Idx++ )
    {
      RNOK( xDeriveBlockModeAndUpdateMv( iB8x8Idx ) );
    }
    RNOK( xDeriveMbMode       () );
    RNOK( xDeriveFwdBwd       () );
    RNOK( xSetInterIntraIdc   () );
  }
  if( m_bInCropWindow )
  {
    RNOK( xSetResPredSafeFlag () );
  }

  RNOK( xSetPredMbData() );
  return Err::m_nOK;
}


ErrVal
MotionUpsampling::xInitMb( Int iMbXCurr, Int iMbYCurr )
{
  m_iMbXCurr              = iMbXCurr;
  m_iMbYCurr              = iMbYCurr;
  m_bInCropWindow         = xIsInCropWindow();
  m_bIntraBL              = false;
  m_eMbMode               = MODE_SKIP;
  m_uiFwdBwd              = 0;
  m_bResPredSafe          = m_bInCropWindow;
  m_aeBlkMode[0]          = BLK_8x8;
  m_aeBlkMode[1]          = BLK_8x8;
  m_aeBlkMode[2]          = BLK_8x8;
  m_aeBlkMode[3]          = BLK_8x8;
  m_aabBaseIntra[0][0]    = false;
  m_aabBaseIntra[0][1]    = false;
  m_aabBaseIntra[1][0]    = false;
  m_aabBaseIntra[1][1]    = false;
  m_aaaiRefIdx  [0][0][0] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [0][0][1] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [0][1][0] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [0][1][1] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [1][0][0] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [1][0][1] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [1][1][0] = BLOCK_NOT_PREDICTED;
  m_aaaiRefIdx  [1][1][1] = BLOCK_NOT_PREDICTED;

  //--- set field mode for SNR scalability ---
  if( m_bInCropWindow && ( m_bSCoeffPred || m_bTCoeffPred ) )
  {
    Int     iFieldPic     = ( m_rcResizeParameters.m_bFieldPicFlag ? 1 : 0 );
    Int     iBotField     = ( m_rcResizeParameters.m_bBotFieldFlag ? 1 : 0 );
    Int     iMbXBase      = m_iMbXCurr -   ( m_rcResizeParameters.m_iLeftFrmOffset >> 4 );
    Int     iMbYBase      = m_iMbYCurr - ( ( m_rcResizeParameters.m_iTopFrmOffset  >> 4 ) >> iFieldPic );
    Int     iMbStrideBase = ( m_rcResizeParameters.m_iRefLayerFrmWidth >> 4 ) << iFieldPic;
    Int     iMbOffsetBase = ( m_rcResizeParameters.m_iRefLayerFrmWidth >> 4 )  * iBotField;
    Int     iMbIdxBase    = iMbOffsetBase + iMbYBase * iMbStrideBase + iMbXBase;
    MbData& rcMbDataBase  = m_rcMbDataCtrlBase.getMbDataByIndex( (UInt)iMbIdxBase );
    Bool    bFieldMbFlag  = rcMbDataBase.getFieldFlag() || m_rcResizeParameters.m_bRefLayerFieldPicFlag;
    m_bCurrFieldMb        = bFieldMbFlag;
  }
  return Err::m_nOK;
}


Bool
MotionUpsampling::xIsInCropWindow()
{
  Int iMbAff  = ( m_rcResizeParameters.m_bIsMbAffFrame           ? 1 : 0 );
  Int iField  = ( iMbAff || m_rcResizeParameters.m_bFieldPicFlag ? 1 : 0 );
  Int iMbY0   = ( m_iMbYCurr >> iMbAff ) << iField;
  Int iMbY1   = iMbY0 + iField;
  ROFRS( m_iMbXCurr >= m_iMbX0CropFrm && m_iMbXCurr < m_iMbX1CropFrm, false );
  ROFRS( iMbY0      >= m_iMbY0CropFrm && iMbY1      < m_iMbY1CropFrm, false );
  return true;
}


//===== this function implements subclause G.6.1 =====
ErrVal
MotionUpsampling::xGetRefLayerMb( Int   iXInsideCurrMb,
                                  Int   iYInsideCurrMb,
                                  Int&  riBaseMbIdx,
                                  Int&  riXInsideBaseMb,
                                  Int&  riYInsideBaseMb )
{
  //===== reset output values =====
  riBaseMbIdx           = MSYS_INT_MAX;
  riXInsideBaseMb       = MSYS_INT_MAX;
  riYInsideBaseMb       = MSYS_INT_MAX;
  //===== get top-left luma sample location of macroblock =====
  Int     iFieldMb      = ( m_bCurrFieldMb ? 1 : 0 );
  Int     iFldInFrame   = ( m_cPosCalc.m_iCurrMbAff && m_bCurrFieldMb ? 1 : 0 );
  Int     iMbPosX       = (   m_iMbXCurr << 4 );
  Int     iMbPosY       = ( ( m_iMbYCurr >> iFldInFrame ) << ( 4 + iFldInFrame ) ) + ( m_iMbYCurr & iFldInFrame );
  //===== get luma location in current picture =====
  Int     iCurrPosX     = iMbPosX +   iXInsideCurrMb;
  Int     iCurrPosY     = iMbPosY + ( iYInsideCurrMb << iFldInFrame );
  //===== get luma location in reference picture =====
  Int     iBasePosX     = (Int)( (UInt)( iCurrPosX * m_cPosCalc.m_iScaleX + m_cPosCalc.m_iAddX ) >> m_cPosCalc.m_iShiftX );
  Int     iBasePosY     = (Int)( (UInt)( iCurrPosY * m_cPosCalc.m_iScaleY + m_cPosCalc.m_iAddY ) >> m_cPosCalc.m_iShiftY );

  //===== clip position ====
  iBasePosX = gMin( iBasePosX, m_cPosCalc.m_iRefW - 1 );
  iBasePosY = gMin( iBasePosY, m_cPosCalc.m_iRefH - 1 );

  //===== get virtual MbData =====
  Int     iMbStride     = ( m_cPosCalc.m_iRefW >> 4 ) << m_cPosCalc.m_iBaseField;
  Int     iMbOffset     = ( m_cPosCalc.m_iRefW >> 4 )  * m_cPosCalc.m_iBaseBotField;
  Int     iBaseMbX      = ( iBasePosX >> 4 );
  Int     iBaseMbY      = ( iBasePosY >> 4 );
  Int     iBaseMbIdx    = iMbOffset + iBaseMbY * iMbStride + iBaseMbX;
  MbData& rcBaseMb      = m_rcMbDataCtrlBase.getMbDataByIndex( (UInt)iBaseMbIdx );

  //===== non-Mbaff to non-Mbaff resampling =====
  if( ! m_cPosCalc.m_iBaseMbAff && ! m_cPosCalc.m_iCurrMbAff )
  {
    riBaseMbIdx     = iBaseMbIdx;
    riXInsideBaseMb = ( iBasePosX & 15 );
    riYInsideBaseMb = ( iBasePosY & 15 );
    return Err::m_nOK;
  }

  //===== same frame/field type in base and current layer =====
  if( m_bCurrFieldMb == rcBaseMb.getFieldFlag() )
  {
    iBaseMbY        = ( ( iBaseMbY >> iFieldMb ) << iFieldMb ) + iFieldMb * ( m_iMbYCurr & 1 );
    riBaseMbIdx     = iMbOffset + iBaseMbY * iMbStride + iBaseMbX;
    riXInsideBaseMb = ( iBasePosX &   15 );
    riYInsideBaseMb = ( iBasePosY & ( 15 + 16 * iFieldMb ) ) >> iFieldMb;
    return Err::m_nOK;
  }

  //===== field-to-frame conversion (subclause G.6.1.1) =====
  if( ! m_bCurrFieldMb )
  {
    Int     iBaseTopMbY = ( iBaseMbY >> 1 ) << 1;
    Int     iBaseBotMbY = iBaseTopMbY + 1;
    Int     iTopMbIdx   = iMbOffset + iBaseTopMbY * iMbStride + iBaseMbX;
    Int     iBotMbIdx   = iMbOffset + iBaseBotMbY * iMbStride + iBaseMbX;
    MbData& rcBaseTopMb = m_rcMbDataCtrlBase.getMbDataByIndex( (UInt)iTopMbIdx );
    riBaseMbIdx         = ( rcBaseTopMb.isIntra() ? iBotMbIdx : iTopMbIdx );
    riXInsideBaseMb     = ( iBasePosX & 15 );
    riYInsideBaseMb     = ( ( ( iBasePosY >> 4 ) & 1 ) << 3 ) + ( ( ( iBasePosY & 15 ) >> 3 ) << 2 );
    return Err::m_nOK;
  }

  //===== frame-to-field conversion (subclause G.6.1.2) =====
  {
    riBaseMbIdx     = iBaseMbIdx;
    riXInsideBaseMb = ( iBasePosX & 15 );
    riYInsideBaseMb = ( iBasePosY & 15 );
    return Err::m_nOK;
  }
}


//===== this function implements subclause G.6.2 =====
ErrVal
MotionUpsampling::xGetRefLayerPartIdc( Int  iXInsideCurrMb,
                                       Int  iYInsideCurrMb,
                                       Int& riPartIdc )
{
  Int iBase8x8MbPartIdx     = 0;  // index of top-left 8x8 block that is covered by the macroblock partition
  Int iBase4x4SubMbPartIdx  = 0;  // index of top-left 4x4 block that is covered by the sub-macroblock partition (inside 8x8 block)
  Int iBaseMbIdx            = MSYS_INT_MAX;
  Int iXInsideBaseMb        = MSYS_INT_MAX;
  Int iYInsideBaseMb        = MSYS_INT_MAX;
  RNOK( xGetRefLayerMb( iXInsideCurrMb, iYInsideCurrMb, iBaseMbIdx, iXInsideBaseMb, iYInsideBaseMb ) );

  //===== check whether intra =====
  MbData& rcMbDataBase = m_rcMbDataCtrlBase.getMbDataByIndex( (UInt)iBaseMbIdx );
  if( rcMbDataBase.isIntra() )
  {
    riPartIdc = -1; // intra
    return Err::m_nOK;
  }

  Int     iB8x8IdxBase    = ( (   iYInsideBaseMb       >> 3 ) << 1 ) + (   iXInsideBaseMb       >> 3 );
  Int     iB4x4IdxBase    = ( ( ( iYInsideBaseMb & 7 ) >> 2 ) << 1 ) + ( ( iXInsideBaseMb & 7 ) >> 2 );
  MbMode  eMbModeBase     = rcMbDataBase.getMbMode ();
  BlkMode eBlkModeBase    = rcMbDataBase.getBlkMode( Par8x8( iB8x8IdxBase ) );
  Bool    bBSkipOrDirect  = ( eMbModeBase == MODE_SKIP && rcMbDataBase.getSliceType() == B_SLICE );
  ROT( rcMbDataBase.getSliceType() == NOT_SPECIFIED_SLICE ); // check whether slice type is set

  if( bBSkipOrDirect )
  {
    //===== determine macroblock and sub-macroblock partition index for B_Skip and B_Direct_16x16 =====
    if( m_iRefLayerDQId == 0 )
    {
      iBase8x8MbPartIdx     = iB8x8IdxBase;
      iBase4x4SubMbPartIdx  = iB4x4IdxBase;
    }
  }
  else
  {
    const UInt aauiNxNPartIdx[6][4] =
    {
      { 0, 0, 0, 0 }, // MODE_SKIP (P Slice)
      { 0, 0, 0, 0 }, // MODE_16x16     or    BLK_8x8
      { 0, 0, 2, 2 }, // MODE_16x8      or    BLK_8x4
      { 0, 1, 0, 1 }, // MODE_8x16      or    BLK_4x8
      { 0, 1, 2, 3 }, // MODE_8x8       or    BLK_4x4
      { 0, 1, 2, 3 }  // MODE_8x8ref0
    };

    //===== determine macroblock partition index =====
    iBase8x8MbPartIdx = aauiNxNPartIdx[ eMbModeBase ][ iB8x8IdxBase ];

    //===== determine sub-macroblock partition index =====
    if( eMbModeBase == MODE_8x8 || eMbModeBase == MODE_8x8ref0 )
    {
      if( eBlkModeBase == BLK_SKIP )
      {
        if( m_iRefLayerDQId == 0 )
        {
          iBase4x4SubMbPartIdx = iB4x4IdxBase;
        }
      }
      else
      {
        iBase4x4SubMbPartIdx = aauiNxNPartIdx[ eBlkModeBase - BLK_8x8 + 1 ][ iB4x4IdxBase ];
      }
    }
  }

  //===== set partition indication =====
  Int iBase4x4BlkX    = ( ( iBase8x8MbPartIdx  & 1 ) << 1 ) + ( iBase4x4SubMbPartIdx  & 1 );
  Int iBase4x4BlkY    = ( ( iBase8x8MbPartIdx >> 1 ) << 1 ) + ( iBase4x4SubMbPartIdx >> 1 );
  Int iBase4x4BlkIdx  = ( iBase4x4BlkY << 2 ) + iBase4x4BlkX;
  riPartIdc           = ( iBaseMbIdx   << 4 ) + iBase4x4BlkIdx;
  return Err::m_nOK;
}


//===== this function implements subclause G.8.6.1.1 =====
ErrVal
MotionUpsampling::xSetPartIdcArray()
{
  ROFRS( m_bInCropWindow, Err::m_nOK );

  //===== determine all 16 initial partition indexes =====
  {
    m_bIntraBL  = true;
    for( Int iY = 0; iY < 4; iY++ )
    for( Int iX = 0; iX < 4; iX++ )
    {
      RNOK( xGetRefLayerPartIdc( ( iX << 2 ) + 1, ( iY << 2 ) + 1, m_aaiPartIdc[iX][iY] ) );
      m_bIntraBL  = ( m_bIntraBL  && ( m_aaiPartIdc[iX][iY] == -1 ) );
    }
    if( m_bIntraBL )
    {
      m_eMbMode  = INTRA_BL;
    }
  }
  ROTRS( m_bIntraBL,                  Err::m_nOK );
  ROTRS( m_cPosCalc.m_bRSChangeFlag,  Err::m_nOK );

  //===== replace values of "-1" on a 4x4 block basis =====
  {
    for( Int iYP = 0; iYP < 2; iYP++ )
    for( Int iXP = 0; iXP < 2; iXP++ )
    {
      Bool aabProcI4x4Blk[2][2] = { { false, false }, { false, false } };

      for( Int iYS = 0; iYS < 2; iYS++ )
      for( Int iXS = 0; iXS < 2; iXS++ )
      {
        Int iYC = ( iYP << 1 ) + iYS;
        Int iXC = ( iXP << 1 ) + iXS;

        if( m_aaiPartIdc[iXC][iYC] == -1 )
        {
          Int iYSInv  = 1 - iYS;
          Int iXSInv  = 1 - iXS;
          Int iYCInv  = ( iYP << 1 ) + iYSInv;
          Int iXCInv  = ( iXP << 1 ) + iXSInv;
          aabProcI4x4Blk[iXS][iYS] = true;

          if( ! aabProcI4x4Blk[iXSInv][iYS] && m_aaiPartIdc[iXCInv][iYC] != -1 )
          {
            m_aaiPartIdc[iXC][iYC] = m_aaiPartIdc[iXCInv][iYC];
          }
          else if( ! aabProcI4x4Blk[iXS][iYSInv] && m_aaiPartIdc[iXC][iYCInv] != -1 )
          {
            m_aaiPartIdc[iXC][iYC] = m_aaiPartIdc[iXC][iYCInv];
          }
          else if( ! aabProcI4x4Blk[iXSInv][iYSInv] && m_aaiPartIdc[iXCInv][iYCInv] != -1 )
          {
            m_aaiPartIdc[iXC][iYC] = m_aaiPartIdc[iXCInv][iYCInv];
          }
        }
      }
    }
  }

  //===== replace values of "-1" on an 8x8 block basis =====
  {
    Bool aabProcI8x8Blk[2][2] = { { false, false }, { false, false } };

    for( Int iYP = 0; iYP < 2; iYP++ )
    for( Int iXP = 0; iXP < 2; iXP++ )
    {
      Int iYPInv  = 1 - iYP;
      Int iXPInv  = 1 - iXP;
      Int iYO     = ( iYP << 1 );
      Int iXO     = ( iXP << 1 );
      Int iYOInv  = ( 2 - iYP );
      Int iXOInv  = ( 2 - iXP );

      if( m_aaiPartIdc[iXO][iYO] == -1 )
      {
        aabProcI8x8Blk[iXP][iYP] = true;

        if( ! aabProcI8x8Blk[iXPInv][iYP] && m_aaiPartIdc[iXOInv][iYO] != -1 )
        {
          m_aaiPartIdc[iXO  ][iYO  ] = m_aaiPartIdc[iXOInv][iYO  ];
          m_aaiPartIdc[iXO+1][iYO  ] = m_aaiPartIdc[iXOInv][iYO  ];
          m_aaiPartIdc[iXO  ][iYO+1] = m_aaiPartIdc[iXOInv][iYO+1];
          m_aaiPartIdc[iXO+1][iYO+1] = m_aaiPartIdc[iXOInv][iYO+1];
        }
        else if( ! aabProcI8x8Blk[iXP][iYPInv] && m_aaiPartIdc[iXO][iYOInv] != -1 )
        {
          m_aaiPartIdc[iXO  ][iYO  ] = m_aaiPartIdc[iXO  ][iYOInv];
          m_aaiPartIdc[iXO+1][iYO  ] = m_aaiPartIdc[iXO+1][iYOInv];
          m_aaiPartIdc[iXO  ][iYO+1] = m_aaiPartIdc[iXO  ][iYOInv];
          m_aaiPartIdc[iXO+1][iYO+1] = m_aaiPartIdc[iXO+1][iYOInv];
        }
        else if( ! aabProcI8x8Blk[iXPInv][iYPInv] && m_aaiPartIdc[iXOInv][iYOInv] != -1 )
        {
          m_aaiPartIdc[iXO  ][iYO  ] = m_aaiPartIdc[iXOInv][iYOInv];
          m_aaiPartIdc[iXO+1][iYO  ] = m_aaiPartIdc[iXOInv][iYOInv];
          m_aaiPartIdc[iXO  ][iYO+1] = m_aaiPartIdc[iXOInv][iYOInv];
          m_aaiPartIdc[iXO+1][iYO+1] = m_aaiPartIdc[iXOInv][iYOInv];
        }
      }
    }
  }

  return Err::m_nOK;
}


//===== this function implements the reference index and motion vector scaling of subclause G.8.6.1.2 =====
ErrVal
MotionUpsampling::xGetInitialBaseRefIdxAndMv( Int     i4x4BlkX,
                                              Int     i4x4BlkY,
                                              ListIdx eListIdx,
                                              Int     iPartIdc,
                                              Int&    riRefIdx,
                                              Mv&     rcMv )
{
  Int           iMbIdxBase        = iPartIdc >> 4;
  B4x4Idx       c4x4IdxBase       = iPartIdc & 15;
  MbData&       rcMbDataBase      = m_rcMbDataCtrlBase.getMbDataByIndex ( (UInt)iMbIdxBase );
  Int           iCurrFieldMb      = ( m_bCurrFieldMb                                                              ? 1 : 0 );
  Int           iBaseFieldMb      = ( m_rcResizeParameters.m_bRefLayerFieldPicFlag || rcMbDataBase.getFieldFlag() ? 1 : 0 );
  MbMotionData& rcMotionDataBase  = rcMbDataBase    .getMbMotionData  ( eListIdx    );
  Int           iRefIdxBase       = rcMotionDataBase.getRefIdx        ( c4x4IdxBase );
  const Mv&     rcMvBase          = rcMotionDataBase.getMv            ( c4x4IdxBase );

  if( iRefIdxBase < 1 )
  {
    riRefIdx  = BLOCK_NOT_PREDICTED;
    rcMv      = Mv::ZeroMv();
    return Err::m_nOK;
  }

  //===== set reference index and convert motion vector to frame motion vector =====
  riRefIdx  = ( ( ( iRefIdxBase - 1 ) << ( iCurrFieldMb - m_cMvScale.m_iCurrField ) ) >> ( iBaseFieldMb - m_cMvScale.m_iBaseField ) ) + 1;
  Int iMvX  = rcMvBase.getHor();
  Int iMvY  = rcMvBase.getVer() * ( 1 + iBaseFieldMb );

  //===== get motion vector scaling factors =====
  Bool  bCropChange = ( m_cMvScale.m_bCropChangeFlag && riRefIdx <= m_cMvScale.m_aiNumActive[eListIdx] );
  Int   idOX        = 0;
  Int   idOY        = 0;
  Int   idSW        = 0;
  Int   idSH        = 0;
  Int   iScaleX     = m_cMvScale.m_iScaleX;
  Int   iScaleY     = m_cMvScale.m_iScaleY;
  if( bCropChange )
  {
    idOX    = m_cMvScale.m_aaidOX[eListIdx][riRefIdx-1];
    idOY    = m_cMvScale.m_aaidOY[eListIdx][riRefIdx-1];
    idSW    = m_cMvScale.m_aaidSW[eListIdx][riRefIdx-1];
    idSH    = m_cMvScale.m_aaidSH[eListIdx][riRefIdx-1];
    iScaleX = (Int)( ( ( (Int64)( m_cMvScale.m_iScaledW + idSW ) << 16 ) + ( m_cMvScale.m_iRefW >> 1 ) ) / m_cMvScale.m_iRefW );
    iScaleY = (Int)( ( ( (Int64)( m_cMvScale.m_iScaledH + idSH ) << 16 ) + ( m_cMvScale.m_iRefH >> 1 ) ) / m_cMvScale.m_iRefH );
  }

  //===== get scaled motion vector components =====
  iMvX  = ( iMvX * iScaleX + 32768 ) >> 16;
  iMvY  = ( iMvY * iScaleY + 32768 ) >> 16;

  //===== add correction vector =====
  if( bCropChange )
  {
    Int iFldInFrame = ( m_cMvScale.m_iCurrMbAff && m_bCurrFieldMb ? 1 : 0 );
    Int iMbPosX     = (   m_iMbXCurr << 4 );
    Int iMbPosY     = ( ( m_iMbYCurr >> iFldInFrame ) << ( 4 + iFldInFrame ) ) + ( m_iMbYCurr & iFldInFrame );
    Int iXFrm       =   iMbPosX +   ( ( i4x4BlkX << 2 ) + 1 );
    Int iYFrm       = ( iMbPosY + ( ( ( i4x4BlkY << 2 ) + 1 ) << ( iCurrFieldMb - m_cMvScale.m_iCurrField ) ) ) << m_cMvScale.m_iCurrField;
    Int iX          = iXFrm - m_cMvScale.m_iOffsetX;
    Int iY          = iYFrm - m_cMvScale.m_iOffsetY;
    iScaleX         = (Int)( ( ( (Int64)( 4 * idSW ) << 16 ) + ( m_cMvScale.m_iScaledW >> 1 ) ) / m_cMvScale.m_iScaledW );
    iScaleY         = (Int)( ( ( (Int64)( 4 * idSH ) << 16 ) + ( m_cMvScale.m_iScaledH >> 1 ) ) / m_cMvScale.m_iScaledH );
    iMvX           += ( ( iX * iScaleX + 32768 ) >> 16 ) - 4 * idOX;
    iMvY           += ( ( iY * iScaleY + 32768 ) >> 16 ) - 4 * idOY;
  }

  //===== set mv predictor (scale to field vector when required) =====
  rcMv.set( (Short)iMvX, (Short)( iMvY / ( 1 + iCurrFieldMb ) ) );

  return Err::m_nOK;
}


//===== this function implements the function MinPositive() specified in subclause G.8.6.1.2 =====
Int
MotionUpsampling::xGetMinRefIdx( Int iRefIdxA, Int iRefIdxB )
{
  ROTRS( iRefIdxA < 1,  iRefIdxB );
  ROTRS( iRefIdxB < 1,  iRefIdxA );
  return gMin( iRefIdxA, iRefIdxB );
}


//===== this function implements the first part of subclause G.8.6.1.2 =====
ErrVal
MotionUpsampling::xGetRefIdxAndInitialMvPred( ListIdx eListIdx )
{
  //===== get initial predictors for reference indices and motion vectors =====
  {
    for( Int i4x4BlkY = 0; i4x4BlkY < 4; i4x4BlkY++ )
    for( Int i4x4BlkX = 0; i4x4BlkX < 4; i4x4BlkX++ )
    {
      RNOK( xGetInitialBaseRefIdxAndMv( i4x4BlkX, i4x4BlkY, eListIdx, m_aaiPartIdc[i4x4BlkX][i4x4BlkY],
                                        m_aaiRefIdxTemp[i4x4BlkX][i4x4BlkY], m_aaacMv[eListIdx][i4x4BlkX][i4x4BlkY] ) );
    }
  }

  //===== set reference indices =====
  m_aaaiRefIdx[eListIdx][0][0] = m_aaiRefIdxTemp[0][0];
  m_aaaiRefIdx[eListIdx][0][1] = m_aaiRefIdxTemp[0][2];
  m_aaaiRefIdx[eListIdx][1][0] = m_aaiRefIdxTemp[2][0];
  m_aaaiRefIdx[eListIdx][1][1] = m_aaiRefIdxTemp[2][2];
  ROTRS( m_cMvScale.m_bRSChangeFlag, Err::m_nOK );

  //===== merge reference indices and modify motion vectors accordingly =====
  for( Int i8x8BlkY = 0; i8x8BlkY < 2; i8x8BlkY++ )
  for( Int i8x8BlkX = 0; i8x8BlkX < 2; i8x8BlkX++ )
  {
    //----- determine reference indices -----
    for( Int i4x4BlkY = 0; i4x4BlkY < 2; i4x4BlkY++ )
    for( Int i4x4BlkX = 0; i4x4BlkX < 2; i4x4BlkX++ )
    {
      Int iY  = ( i8x8BlkY << 1 ) + i4x4BlkY;
      Int iX  = ( i8x8BlkX << 1 ) + i4x4BlkX;
      m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY] = xGetMinRefIdx( m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY], m_aaiRefIdxTemp[iX][iY] );
    }

    //----- update motion vectors -----
    for( Int iYS = 0; iYS < 2; iYS++ )
    for( Int iXS = 0; iXS < 2; iXS++ )
    {
      Int iY = ( i8x8BlkY << 1 ) + iYS;
      Int iX = ( i8x8BlkX << 1 ) + iXS;

      if( m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY] != m_aaiRefIdxTemp[iX][iY] )
      {
        Int iYInv = ( i8x8BlkY << 1 ) + 1 - iYS;
        Int iXInv = ( i8x8BlkX << 1 ) + 1 - iXS;

        if( m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY] == m_aaiRefIdxTemp[iXInv][iY] )
        {
          m_aaacMv[eListIdx][iX][iY] = m_aaacMv[eListIdx][iXInv][iY];
        }
        else if( m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY] == m_aaiRefIdxTemp[iX][iYInv] )
        {
          m_aaacMv[eListIdx][iX][iY] = m_aaacMv[eListIdx][iX][iYInv];
        }
        else
        {
          ROF( m_aaaiRefIdx[eListIdx][i8x8BlkX][i8x8BlkY] == m_aaiRefIdxTemp[iXInv][iYInv] );
          m_aaacMv[eListIdx][iX][iY] = m_aaacMv[eListIdx][iXInv][iYInv];
        }
      }
    }
  }

  return Err::m_nOK;
}


//===== this function implements the function mvDiff specified in subclause G.8.6.1.2 =====
Int
MotionUpsampling::xMvDiff( const Mv& rcMvA, const Mv& rcMbB )
{
  return rcMvA.getAbsHorDiff( rcMbB ) + rcMvA.getAbsVerDiff( rcMbB );
}


//===== this function implements subclause G.8.6.1.2 =====
ErrVal
MotionUpsampling::xDeriveBlockModeAndUpdateMv( Int i8x8BlkIdx )
{
  Int   iAbsMvDiffThreshold = ( m_cMvScale.m_bRSChangeFlag ? 0 : 1 );
  Int   iXO                 = ( i8x8BlkIdx  & 1 ) << 1;
  Int   iYO                 = ( i8x8BlkIdx >> 1 ) << 1;
  Bool  bHorMatch           = true;
  Bool  bVerMatch           = true;
  Bool  b8x8Match           = true;

  //===== unify 8x8 blocks when direct_8x8_inference_flag is equal to 1 =====
  if( m_bDirect8x8Inference && ! m_cMvScale.m_bRSChangeFlag && m_eSliceType == B_SLICE )
  {
    Int iXC = ( iXO >> 1 ) * 3;
    Int iYC = ( iYO >> 1 ) * 3;
    for( Int iListIdx = 0; iListIdx < m_iMaxListIdx; iListIdx++ )
    {
      Mv  cTmpMv  = m_aaacMv[iListIdx][iXC][iYC];
      m_aaacMv[iListIdx][iXO  ][iYO  ]  = cTmpMv;
      m_aaacMv[iListIdx][iXO+1][iYO  ]  = cTmpMv;
      m_aaacMv[iListIdx][iXO  ][iYO+1]  = cTmpMv;
      m_aaacMv[iListIdx][iXO+1][iYO+1]  = cTmpMv;
    }
  }

  //===== derive partition size =====
  {
    for( Int iListIdx = 0; iListIdx < m_iMaxListIdx; iListIdx++ )
    {
      Bool  bHor1Match  = ( xMvDiff( m_aaacMv[iListIdx][iXO  ][iYO  ], m_aaacMv[iListIdx][iXO+1][iYO  ] ) <= iAbsMvDiffThreshold );
      Bool  bHor2Match  = ( xMvDiff( m_aaacMv[iListIdx][iXO  ][iYO+1], m_aaacMv[iListIdx][iXO+1][iYO+1] ) <= iAbsMvDiffThreshold );
      Bool  bVer1Match  = ( xMvDiff( m_aaacMv[iListIdx][iXO  ][iYO  ], m_aaacMv[iListIdx][iXO  ][iYO+1] ) <= iAbsMvDiffThreshold );
      Bool  bVer2Match  = ( xMvDiff( m_aaacMv[iListIdx][iXO+1][iYO  ], m_aaacMv[iListIdx][iXO+1][iYO+1] ) <= iAbsMvDiffThreshold );
      Bool  bDiagMatch  = ( xMvDiff( m_aaacMv[iListIdx][iXO  ][iYO  ], m_aaacMv[iListIdx][iXO+1][iYO+1] ) <= iAbsMvDiffThreshold );
      b8x8Match         = b8x8Match && bHor1Match && bVer1Match && bDiagMatch;
      bHorMatch         = bHorMatch && bHor1Match && bHor2Match;
      bVerMatch         = bVerMatch && bVer1Match && bVer2Match;
    }
    const BlkMode aiBlkMode[4]  = { BLK_4x4, BLK_8x4, BLK_4x8, BLK_8x8 };
    m_aeBlkMode[i8x8BlkIdx]     = aiBlkMode[ b8x8Match ? 3 : bHorMatch ? 1 : bVerMatch ? 2 : 0 ];
  }
  ROTRS( m_cMvScale.m_bRSChangeFlag,          Err::m_nOK );
  ROTRS( m_aeBlkMode[i8x8BlkIdx] == BLK_4x4,  Err::m_nOK );

  //===== combine motion vectors =====
  {
    for( Int iListIdx = 0; iListIdx < m_iMaxListIdx; iListIdx++ )
    {
      switch( m_aeBlkMode[i8x8BlkIdx] )
      {
      case BLK_8x8:
        {
          Mv  cNewMv  = ( m_aaacMv[iListIdx][iXO  ][iYO  ] + m_aaacMv[iListIdx][iXO+1][iYO  ] +
                          m_aaacMv[iListIdx][iXO  ][iYO+1] + m_aaacMv[iListIdx][iXO+1][iYO+1] + Mv(2,2) ) >> 2;
          m_aaacMv[iListIdx][iXO  ][iYO  ]  = cNewMv;
          m_aaacMv[iListIdx][iXO+1][iYO  ]  = cNewMv;
          m_aaacMv[iListIdx][iXO  ][iYO+1]  = cNewMv;
          m_aaacMv[iListIdx][iXO+1][iYO+1]  = cNewMv;
          break;
        }
      case BLK_8x4:
        {
          Mv  cNewMvA = ( m_aaacMv[iListIdx][iXO  ][iYO  ] + m_aaacMv[iListIdx][iXO+1][iYO  ] + Mv(1,1) ) >> 1;
          Mv  cNewMvB = ( m_aaacMv[iListIdx][iXO  ][iYO+1] + m_aaacMv[iListIdx][iXO+1][iYO+1] + Mv(1,1) ) >> 1;
          m_aaacMv[iListIdx][iXO  ][iYO  ]  = cNewMvA;
          m_aaacMv[iListIdx][iXO+1][iYO  ]  = cNewMvA;
          m_aaacMv[iListIdx][iXO  ][iYO+1]  = cNewMvB;
          m_aaacMv[iListIdx][iXO+1][iYO+1]  = cNewMvB;
          break;
        }
      case BLK_4x8:
        {
          Mv  cNewMvA = ( m_aaacMv[iListIdx][iXO  ][iYO  ] + m_aaacMv[iListIdx][iXO  ][iYO+1] + Mv(1,1) ) >> 1;
          Mv  cNewMvB = ( m_aaacMv[iListIdx][iXO+1][iYO  ] + m_aaacMv[iListIdx][iXO+1][iYO+1] + Mv(1,1) ) >> 1;
          m_aaacMv[iListIdx][iXO  ][iYO  ]  = cNewMvA;
          m_aaacMv[iListIdx][iXO+1][iYO  ]  = cNewMvB;
          m_aaacMv[iListIdx][iXO  ][iYO+1]  = cNewMvA;
          m_aaacMv[iListIdx][iXO+1][iYO+1]  = cNewMvB;
          break;
        }
      default:
        ROT(1);
      }
    }
  }

  return Err::m_nOK;
}


Bool
MotionUpsampling::x8x8BlocksHaveSameMotion( ListIdx eListIdx, Int i8x8IdxA, Int i8x8IdxB )
{
  Bool  bBlkASameMv   = true;
  Bool  bBlkBSameMv   = true;
  Int   aaiComp[4][4] = { {0,1,4,5}, {2,3,6,7}, {8,9,12,13}, {10,11,14,15} };

  for( Int i=0; i<3; i++ )
  {
    if( m_aaacMv[eListIdx][ aaiComp[i8x8IdxA][i] % 4 ][ aaiComp[i8x8IdxA][i] / 4 ] != m_aaacMv[eListIdx][ aaiComp[i8x8IdxA][i+1] % 4 ][ aaiComp[i8x8IdxA][i+1] / 4 ] )
    {
      bBlkASameMv = false;
    }
    if( m_aaacMv[eListIdx][ aaiComp[i8x8IdxB][i] % 4 ][ aaiComp[i8x8IdxB][i] / 4 ] != m_aaacMv[eListIdx][ aaiComp[i8x8IdxB][i+1] % 4 ][ aaiComp[i8x8IdxB][i+1] / 4 ] )
    {
      bBlkBSameMv = false;
    }
  }
  // check reference indices
  ROFRS( m_aaaiRefIdx[eListIdx][i8x8IdxA&1][i8x8IdxA>>1] == m_aaaiRefIdx[eListIdx][i8x8IdxB&1][i8x8IdxB>>1], false );
  // check motion vectors
  ROFRS( bBlkASameMv, false ); // inside block A
  ROFRS( bBlkBSameMv, false ); // inside block B
  ROFRS( m_aaacMv[eListIdx][(i8x8IdxA&1)<<1][(i8x8IdxA>>1)<<1] == m_aaacMv[eListIdx][(i8x8IdxB&1)<<1][(i8x8IdxB>>1)<<1], false );
  return true;
}


ErrVal
MotionUpsampling::xDeriveMbMode()
{
  //===== summarize 8x8 blocks when possible =====
  Bool  bHorMatch   = true;
  Bool  bVerMatch   = true;
  for( Int iListIdx = 0; iListIdx < m_iMaxListIdx; iListIdx++ )
  {
    bHorMatch = bHorMatch && x8x8BlocksHaveSameMotion( ListIdx( iListIdx ), 0, 1 );
    bHorMatch = bHorMatch && x8x8BlocksHaveSameMotion( ListIdx( iListIdx ), 2, 3 );
    bVerMatch = bVerMatch && x8x8BlocksHaveSameMotion( ListIdx( iListIdx ), 0, 2 );
    bVerMatch = bVerMatch && x8x8BlocksHaveSameMotion( ListIdx( iListIdx ), 1, 3 );
  }
  const MbMode aiMbMode[4]  = { MODE_8x8, MODE_16x8, MODE_8x16, MODE_16x16 };
  m_eMbMode                 = aiMbMode[ ( bVerMatch ? 2 : 0 ) + ( bHorMatch ? 1 : 0 ) ];
  return Err::m_nOK;
}


ErrVal
MotionUpsampling::xDeriveFwdBwd()
{
  m_uiFwdBwd = 0;
  for( Int n = 3; n >= 0; n--)
  {
    m_uiFwdBwd <<= 4;
    m_uiFwdBwd  += ( m_aaaiRefIdx[0][n&1][n>>1] > 0 ? 1 : 0 );
    m_uiFwdBwd  += ( m_aaaiRefIdx[1][n&1][n>>1] > 0 ? 2 : 0 );
  }
  return Err::m_nOK;
}


ErrVal
MotionUpsampling::xSetInterIntraIdc()
{
  if( m_cMvScale.m_bRSChangeFlag )
  {
    m_aabBaseIntra[0][0]  = false;
    m_aabBaseIntra[0][1]  = false;
    m_aabBaseIntra[1][0]  = false;
    m_aabBaseIntra[1][1]  = false;
    return Err::m_nOK;
  }

  for( Int iY = 0; iY < 2; iY++ )
  for( Int iX = 0; iX < 2; iX++ )
  {
    Int iXPos     = iX * 15;
    Int iYPos     = iY * 15;
    Int iPartIdc  = -2;
    RNOK( xGetRefLayerPartIdc( iXPos, iYPos, iPartIdc ) );
    m_aabBaseIntra[iX][iY] = ( iPartIdc == -1 );
  }
  return Err::m_nOK;
}


ErrVal
MotionUpsampling::xSetResPredSafeFlag()
{
  ROFRS( m_bCheckResidualPred,        Err::m_nOK );
  ROTRS( m_cMvScale.m_bRSChangeFlag,  Err::m_nOK );

  for( Int iY = 0; iY < 4 && m_bResPredSafe; iY++ )
  for( Int iX = 0; iX < 4 && m_bResPredSafe; iX++ )
  {
    Int   iXPos0          = ( iX << 2 );
    Int   iYPos0          = ( iY << 2 );
    Bool  bSamePartition  = true;
    Bool  bSomeNoInter    = false;
    Int   aiPartIdc[4], k;
    for( k = 0; k < 4; k++ )
    {
      RNOK( xGetRefLayerPartIdc( iXPos0+3*(k&1), iYPos0+3*(k>>1), aiPartIdc[k] ) );
      bSamePartition  = ( bSamePartition && ( aiPartIdc[k] == aiPartIdc[0] ) );
      bSomeNoInter    = ( bSomeNoInter   || ( aiPartIdc[k] < 0 ) );
    }
    if( bSamePartition || bSomeNoInter )
    {
      m_bResPredSafe  = bSomeNoInter;
      continue;
    }

    //===== check differences =====
    for( Int iListIdx = 0; iListIdx < 2 && m_bResPredSafe; iListIdx++ )
    {
      Int aiRefIdx[4];
      Mv  acMv    [4];
      for( k = 0; k < 4; k++ )
      {
        Int           iMbIdxBase        = aiPartIdc[k] >> 4;
        B4x4Idx       c4x4IdxBase       = aiPartIdc[k] & 15;
        MbData&       rcMbDataBase      = m_rcMbDataCtrlBase.getMbDataByIndex ( (UInt)iMbIdxBase );
        MbMotionData& rcMotionDataBase  = rcMbDataBase      .getMbMotionData  ( ListIdx( iListIdx ) );
        aiRefIdx[k] = rcMotionDataBase.getRefIdx( c4x4IdxBase );
        acMv    [k] = rcMotionDataBase.getMv    ( c4x4IdxBase );
      }
      //----- check reference indices -----
      for( k = 0; k < 4 && m_bResPredSafe; k++ )
      {
        m_bResPredSafe = ( aiRefIdx[k] == aiRefIdx[0] );
      }
      //----- check motion vectors -----
      if( m_bResPredSafe )
      {
        Mv cMvAverage = ( acMv[0] + acMv[1] + acMv[2] + acMv[3] + Mv(2,2) ) >> 2;

        for( k = 0; k < 4 && m_bResPredSafe; k++ )
        {
          m_bResPredSafe = ( acMv[k].getAbsMvDiff( cMvAverage ) <= m_iMvThreshold );
        }
      }
    }
  }
  return Err::m_nOK;
}


ErrVal
MotionUpsampling::xSetPredMbData()
{
  //=== get MbData reference ===
  Int           iFieldPic     = ( m_rcResizeParameters.m_bFieldPicFlag ? 1 : 0 );
  Int           iBotField     = ( m_rcResizeParameters.m_bBotFieldFlag ? 1 : 0 );
  Int           iMbStride     = ( m_rcResizeParameters.m_iFrameWidth >> 4 ) << iFieldPic;
  Int           iMbOffset     = ( m_rcResizeParameters.m_iFrameWidth >> 4 )  * iBotField;
  Int           iMbIdx        = iMbOffset + m_iMbYCurr * iMbStride + m_iMbXCurr;
  MbData&       rcMbData      = m_rcMbDataCtrlCurr.getMbDataByIndex( (UInt)iMbIdx );
  MbMotionData* apcMotion[2]  = { &rcMbData.getMbMotionData( LIST_0 ), &rcMbData.getMbMotionData( LIST_1 ) };

  //=== reset MbDataStruct data data ===
  rcMbData.clear();

  //=== set motion data (ref idx & motion vectors ) ===
  if( ! m_bInCropWindow || m_bIntraBL )
  {
    for( Int iListIdx = 0; iListIdx < 2; iListIdx++ )
    {
      apcMotion[iListIdx]->clear( BLOCK_NOT_PREDICTED );
    }
  }
  else
  {
    Int    iListIdx = 0;
    for( ; iListIdx < m_iMaxListIdx; iListIdx++ )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        apcMotion[iListIdx]->setRefIdx( m_aaaiRefIdx[iListIdx][c8x8Idx.b8x8Index()&1][c8x8Idx.b8x8Index()>>1], c8x8Idx.b8x8() );
      }
      for( B4x4Idx c4x4Idx; c4x4Idx.isLegal(); c4x4Idx++ )
      {
        apcMotion[iListIdx]->setMv( m_aaacMv[iListIdx][c4x4Idx.x()][c4x4Idx.y()], c4x4Idx );
      }
    }
    for( ; iListIdx < 2; iListIdx++ )
    {
      apcMotion[iListIdx]->clear( BLOCK_NOT_PREDICTED );
    }
  }
  apcMotion[0]->setFieldMode( m_bCurrFieldMb );
  apcMotion[1]->setFieldMode( m_bCurrFieldMb );

  //=== set general Mb data ===
  rcMbData.setInCropWindowFlag( m_bInCropWindow );
  rcMbData.setFieldFlag       ( m_bCurrFieldMb );
  rcMbData.setSafeResPred     ( m_bResPredSafe );
  if( m_bInCropWindow )
  {
    rcMbData.setMbMode( m_eMbMode );
    rcMbData.setFwdBwd( (UShort)m_uiFwdBwd );
    for( Int iBlk = 0; iBlk < 4; iBlk++ )
    {
      rcMbData.setBlkMode   ( Par8x8( iBlk ), m_aeBlkMode[iBlk] );
      rcMbData.setBaseIntra ( iBlk&1, iBlk>>1, m_aabBaseIntra[iBlk&1][iBlk>>1] );
    }
  }

  //=== values for SNR scalability ===
  if( ( m_bSCoeffPred || m_bTCoeffPred ) && m_bInCropWindow )
  {
    Int     iMbXBase      = m_iMbXCurr -   ( m_rcResizeParameters.m_iLeftFrmOffset >> 4 );
    Int     iMbYBase      = m_iMbYCurr - ( ( m_rcResizeParameters.m_iTopFrmOffset  >> 4 ) >> iFieldPic );
    Int     iMbStrideBase = ( m_rcResizeParameters.m_iRefLayerFrmWidth >> 4 ) << iFieldPic;
    Int     iMbOffsetBase = ( m_rcResizeParameters.m_iRefLayerFrmWidth >> 4 )  * iBotField;
    Int     iMbIdxBase    = iMbOffsetBase + iMbYBase * iMbStrideBase + iMbXBase;
    MbData& rcMbDataBase  = m_rcMbDataCtrlBase.getMbDataByIndex( (UInt)iMbIdxBase );

    rcMbData.copyTCoeffs    ( rcMbDataBase );
    rcMbData.setBLSkipFlag  ( rcMbDataBase.getBLSkipFlag() );
    if( m_bIntraBL )
    {
      rcMbData.setMbMode    ( rcMbDataBase.getMbMode() );
    }
    if( m_bTCoeffPred )
    {
      rcMbData.copyIntraPred( rcMbDataBase );
    }
  }

  return Err::m_nOK;
}




MbDataCtrl::MbDataCtrl():
  m_pcMbTCoeffs       ( NULL ),
  m_pcMbData          ( NULL ),
  m_pcMbDataAccess    ( NULL ),
  m_pcSliceHeader     ( NULL ),
  m_ucLastMbQp        ( 0 ),
  m_ucLastMbQp4LF     ( 0 ),
  m_uiMbStride        ( 0 ),
  m_uiMbOffset        ( 0 ),
  m_iMbPerLine        ( 0 ),
  m_iMbPerColumn      ( 0 ),
  m_uiSize            ( 0 ),
  m_uiMbProcessed     ( 0 ),
  m_uiSliceId         ( 0 ),
	m_iColocatedOffset  ( 0 ),
  m_eProcessingState  ( PRE_PROCESS),
  m_pcMbDataCtrl0L1   ( NULL ),
  m_bUseTopField      ( false ),
  m_bPicCodedField    ( false ),
  m_bInitDone         ( false )
{
  m_apcMbMvdData    [LIST_0]  = NULL;
  m_apcMbMvdData    [LIST_1]  = NULL;
  m_apcMbMotionData [LIST_0]  = NULL;
  m_apcMbMotionData [LIST_1]  = NULL;
}

MbDataCtrl::~MbDataCtrl()
{
  AOT( xDeleteData() );
  AOT( m_bInitDone );
}

ErrVal MbDataCtrl::xCreateData( UInt uiSize )
{
  uiSize++;

  ROT( NULL == ( m_pcMbTCoeffs         = new MbTransformCoeffs [ uiSize ] ) );
  ROT( NULL == ( m_apcMbMotionData[0]  = new MbMotionData      [ uiSize ] ) );
  ROT( NULL == ( m_apcMbMotionData[1]  = new MbMotionData      [ uiSize ] ) );
  ROT( NULL == ( m_apcMbMvdData[0]     = new MbMvData          [ uiSize ] ) );
  ROT( NULL == ( m_apcMbMvdData[1]     = new MbMvData          [ uiSize ] ) );
  ROT( NULL == ( m_pcMbData            = new MbData            [ uiSize ] ) );

  for( UInt uiIdx = 0; uiIdx < uiSize; uiIdx++ )
  {
    m_pcMbData[ uiIdx ].init( m_pcMbTCoeffs        + uiIdx,
                              m_apcMbMvdData   [0] + uiIdx,
                              m_apcMbMvdData   [1] + uiIdx,
                              m_apcMbMotionData[0] + uiIdx,
                              m_apcMbMotionData[1] + uiIdx );
  }

  // clear outside mb data
  m_pcMbData[uiSize-1].getMbTCoeffs().setAllCoeffCount( 0 );
  m_pcMbData[uiSize-1].initMbData( 0, 0, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, NOT_SPECIFIED_SLICE );

  return Err::m_nOK;
}

ErrVal MbDataCtrl::xDeleteData()
{
  H264AVC_DELETE_CLASS( m_pcMbDataAccess );

  H264AVC_DELETE( m_pcMbTCoeffs );
  H264AVC_DELETE( m_apcMbMvdData[1] );
  H264AVC_DELETE( m_apcMbMvdData[0] );
  H264AVC_DELETE( m_apcMbMotionData[1] );
  H264AVC_DELETE( m_apcMbMotionData[0] );
  H264AVC_DELETE( m_pcMbData );
  m_uiSize          = 0;
  return Err::m_nOK;
}

ErrVal MbDataCtrl::xResetData()
{
  UInt uiIdx;
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_pcMbData[ uiIdx ].reset();
  }
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_pcMbTCoeffs[ uiIdx ].clear();
  }
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_apcMbMvdData[0][ uiIdx ].clear();
  }
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_apcMbMvdData[1][ uiIdx ].clear();
  }
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_apcMbMotionData[0][ uiIdx ].reset();
  }
  for( uiIdx = 0; uiIdx < m_uiSize; uiIdx++ )
  {
    m_apcMbMotionData[1][ uiIdx ].reset();
  }
  return Err::m_nOK;
}


Bool MbDataCtrl::isPicDone( const SliceHeader& rcSH )
{
  return ( m_uiMbProcessed == rcSH.getSPS().getMbInFrame() || m_uiMbProcessed == rcSH.getMbInPic());
}

Bool MbDataCtrl::isFrameDone( const SliceHeader& rcSH )
{
  return ( m_uiMbProcessed == rcSH.getSPS().getMbInFrame());
}


ErrVal MbDataCtrl::init( const SequenceParameterSet& rcSPS )
{
  AOT_DBG( m_bInitDone );

  UInt uiSize = rcSPS.getMbInFrame();

  ROT( 0 == uiSize );
  if( m_uiSize == uiSize )
  {
    RNOK( xResetData() );
  }
  else
  {
    RNOK( xDeleteData() );
    RNOK( xCreateData( uiSize ) );
    m_uiSize = uiSize;
  }

  m_iMbPerLine = rcSPS.getFrameWidthInMbs();

  RNOK( m_cDBFPBuffer.init( uiSize + 1 ) );
  m_cDBFPBuffer.clear();
  m_bInitDone = true;

  return Err::m_nOK;
}

ErrVal
MbDataCtrl::copyMotion( MbDataCtrl& rcMbDataCtrl, PicType ePicType )
{
  UInt    uiStride    = ( ePicType == FRAME     ? m_iMbPerLine : m_iMbPerLine << 1 );
  UInt    uiMbOffset  = ( ePicType == BOT_FIELD ? m_iMbPerLine : 0 );
  UInt    uiNumLines  = m_uiSize / uiStride;
  MbData* pcMbDataDes = &( m_pcMbData             [ uiMbOffset ] );
  MbData* pcMbDataSrc = &( rcMbDataCtrl.m_pcMbData[ uiMbOffset ] );

  for( Int y = 0; y < (Int)uiNumLines; y++, pcMbDataDes += uiStride, pcMbDataSrc += uiStride )
  {
    for( Int x = 0; x < m_iMbPerLine; x++ )
    {
      RNOK( pcMbDataDes[x].copyMotion( pcMbDataSrc[x], m_uiSliceId ) );
    }
  }
  m_bPicCodedField = rcMbDataCtrl.m_bPicCodedField;
  return Err::m_nOK;
}



ErrVal
MbDataCtrl::upsampleMotion( SliceHeader*      pcSliceHeader,
                            ResizeParameters* pcResizeParameters,
                            MbDataCtrl*       pcBaseMbDataCtrl,
                            RefFrameList*     pcRefFrameList0,
                            RefFrameList*     pcRefFrameList1,
                            Bool              bFieldResampling,
                            Bool              bResidualPredCheck,
                            Int               iMvThreshold )
{
  ROF( pcSliceHeader );
  ROF( pcResizeParameters );
  ROF( pcBaseMbDataCtrl );

  MotionUpsampling  cMotionUpsampling( *this, *pcSliceHeader, *pcResizeParameters, *pcBaseMbDataCtrl,
                                       pcRefFrameList0, pcRefFrameList1, bFieldResampling, bResidualPredCheck, iMvThreshold );
  Int               iMbXMax = ( pcResizeParameters->m_iFrameWidth  >> 4 );
  Int               iMbYMax = ( pcResizeParameters->m_iFrameHeight >> 4 ) >> ( pcResizeParameters->m_bFieldPicFlag ? 1 : 0 );

  for( Int iMbY = 0; iMbY < iMbYMax; iMbY++ )
  for( Int iMbX = 0; iMbX < iMbXMax; iMbX++ )
  {
    RNOK( cMotionUpsampling.resample( iMbX, iMbY ) );
  }
  return Err::m_nOK;
}


ErrVal MbDataCtrl::uninit()
{
  m_ucLastMbQp      = 0;
  m_ucLastMbQp4LF   = 0;
  m_uiMbStride      = 0;
  m_uiMbOffset      = 0;
  m_iMbPerLine      = 0;
  m_iMbPerColumn    = 0;
  m_uiMbProcessed   = 0;
  m_uiSliceId       = 0;
  m_pcMbDataCtrl0L1 = 0;

  for( UInt n = 0; n < m_cDBFPBuffer.size(); n++ )
  {
    delete m_cDBFPBuffer.get( n );
    m_cDBFPBuffer.set( n, 0 );
  }
  RNOK( m_cDBFPBuffer.uninit() );

  m_bInitDone = false;
  return Err::m_nOK;
}


ErrVal MbDataCtrl::reset()
{
  m_ucLastMbQp      = 0;
  m_ucLastMbQp4LF   = 0;
  m_uiMbProcessed   = 0;
  m_uiSliceId       = 0;
  m_pcMbDataCtrl0L1 = 0;
  return Err::m_nOK;
}

//TMM {
ErrVal MbDataCtrl::initUsedField( SliceHeader& rcSH, RefFrameList& rcRefFrameList1 )
{
   if( /*!rcSH.getFieldPicFlag() &&*/
        rcRefFrameList1.getSize() !=0 &&
        rcRefFrameList1[1]->getPic(TOP_FIELD) != NULL &&
        rcRefFrameList1[1]->getPic(BOT_FIELD) != NULL )
   {
     Int iCurrPoc     = rcSH.getPoc();
     Int iTopDiffPoc  = iCurrPoc - rcRefFrameList1[1]->getPic(TOP_FIELD)->getPoc();
     Int iBotDiffPoc  = iCurrPoc - rcRefFrameList1[1]->getPic(BOT_FIELD)->getPoc();
     m_bUseTopField   = ( abs( iTopDiffPoc ) < abs( iBotDiffPoc ) );
   }

  return Err::m_nOK;
}
//TMM }


ErrVal
MbDataCtrl::initSliceLF( SliceHeader& rcSH, const MbStatus* apcMbStatus )
{
  RNOK( initSlice( rcSH, POST_PROCESS, false, NULL ) );

  UInt uiCurrDQId = ( rcSH.getDependencyId() << 4 ) + rcSH.getQualityId();
  for( UInt uiMbAddress = 0; uiMbAddress < rcSH.getMbInPic(); uiMbAddress++ )
  {
    UInt  uiMbX   = 0;
    UInt  uiMbY   = 0;
    UInt  uiMbIdx = rcSH.getMbIndexFromAddress(  uiMbAddress );
    rcSH.getMbPositionFromAddress( uiMbY, uiMbX, uiMbAddress );
    MbData&         rcMbData      = getMbData( uiMbX, uiMbY );
    const MbStatus& rcMbStatus    = apcMbStatus[ uiMbIdx ];
    UInt            uiMbCbpDQID0  = 0;
    UInt            uiMbCbpLevels = rcMbData.getMbExtCbp();
    UInt            uiSliceIdcLF  = rcMbData.getSliceId(); // use sliceId in encoder (no incomplete layer representations)
    if( apcMbStatus )
    {
      if( rcMbStatus.getLastCodedDQId() == 0 )
      {
        uiMbCbpDQID0  = rcMbStatus.getMbCbpDQId0();
      }
      uiSliceIdcLF    = rcMbStatus.getLastCodedSliceIdc();
    }
    else if( uiCurrDQId == 0 )
    {
      uiMbCbpDQID0    = uiMbCbpLevels;
    }
    rcMbData.setMbCbpDQId0    ( uiMbCbpDQID0  );
    rcMbData.setMbCbpLevelsLF ( uiMbCbpLevels );
    rcMbData.setSliceIdcLF    ( uiSliceIdcLF  );
  }
  return Err::m_nOK;
}


ErrVal MbDataCtrl::initSlice( SliceHeader&    rcSH,
                              ProcessingState eProcessingState,
                              Bool            bDecoder,
                              MbDataCtrl*     pcMbDataCtrl )
{
  AOF_DBG( m_bInitDone );

  m_eProcessingState  = eProcessingState;
  m_pcMbDataCtrl0L1   = NULL;
	m_iColocatedOffset  = 0;
  m_bPicCodedField    = rcSH.getFieldPicFlag();

  if( rcSH.isBSlice() )
  {
    if( pcMbDataCtrl )
    {
      m_pcMbDataCtrl0L1 = pcMbDataCtrl;
    }
    if( rcSH.isH264AVCCompatible() && rcSH.isBSlice() && m_pcMbDataCtrl0L1 )
    {
      const RefFrameList* pcRefFrameList  = rcSH.getRefFrameList( rcSH.getPicType(), LIST_1 );
      ROF( pcRefFrameList );
      const Frame*        pcPic0L1        = pcRefFrameList->getEntry( 0 );
      ROF( pcPic0L1 );
      const Frame*        pcFrame0L1      = pcPic0L1->getFrame();
      ROF( pcFrame0L1 );

      Int iCurrPoc      = rcSH.getPoc();
      Int iTopDiffPoc   = iCurrPoc - pcFrame0L1->getTopFieldPoc();
      Int iBotDiffPoc   = iCurrPoc - pcFrame0L1->getBotFieldPoc();
      m_bUseTopField    = ( abs( iTopDiffPoc ) < abs( iBotDiffPoc ) );

      if( FRAME != rcSH.getPicType() )
      {
        if( pcPic0L1->getPicType() != rcSH.getPicType() && m_pcMbDataCtrl0L1->isPicCodedField() )
        {
          m_iColocatedOffset = m_iMbPerLine;
        }
      }
    }
  }

  if( PARSE_PROCESS == m_eProcessingState || ENCODE_PROCESS == m_eProcessingState )
  {
    m_uiSliceId++;

    DBFilterParameter* pcDBF   = new DBFilterParameter( rcSH.getDeblockingFilterParameter() );
    if( m_cDBFPBuffer.get( m_uiSliceId ) )
    {
      delete m_cDBFPBuffer.get( m_uiSliceId );
      m_cDBFPBuffer  .set( m_uiSliceId, 0 );
    }
    m_cDBFPBuffer  .set( m_uiSliceId, pcDBF );
  }
  m_pcSliceHeader = &rcSH;


  Int iMbPerColumn  = rcSH.getSPS().getFrameHeightInMbs ();
  m_iMbPerLine      = rcSH.getSPS().getFrameWidthInMbs  ();
  m_uiMbOffset      = rcSH.getBottomFieldFlag() ? 1 * m_iMbPerLine : 0;
  m_uiMbStride      = rcSH.getFieldPicFlag   () ? 2 * m_iMbPerLine : m_iMbPerLine;
  m_iMbPerColumn    = rcSH.getFieldPicFlag   () ?  iMbPerColumn>>1 : iMbPerColumn;
  m_ucLastMbQp      = rcSH.getSliceQp();

  H264AVC_DELETE_CLASS( m_pcMbDataAccess );
  return Err::m_nOK;
}


const MbData& MbDataCtrl::xGetColMbData( UInt uiIndex )
{
  return (( m_pcMbDataCtrl0L1 == NULL ) ? xGetOutMbData() : m_pcMbDataCtrl0L1->getMbData( uiIndex ));
}

const MbData& MbDataCtrl::xGetRefMbData( UInt uiSliceId,
                                         Int uiCurrSliceID,
                                         Int iMbY,
                                         Int iMbX,
                                         Bool bLoopFilter )
{
  // check whether ref mb is inside
  ROTRS( iMbX < 0,               xGetOutMbData() );
  ROTRS( iMbY < 0,               xGetOutMbData() );
  ROTRS( iMbX >= m_iMbPerLine,   xGetOutMbData() );
  ROTRS( iMbY >= m_iMbPerColumn, xGetOutMbData() );

  // get the ref mb data
  const MbData& rcMbData = getMbData( iMbY * m_uiMbStride + iMbX + m_uiMbOffset );

  ROTRS( ! m_pcSliceHeader->isTrueSlice() && m_pcSliceHeader->getTCoeffLevelPredictionFlag(), rcMbData ); // special case
  ROTRS( !bLoopFilter && uiCurrSliceID != getSliceGroupIDofMb( m_pcSliceHeader->getMapUnitFromPosition( UInt(iMbY), UInt(iMbX) ) ), xGetOutMbData() ); // different slice group
  // test slice id
  return (( rcMbData.getSliceId() == uiSliceId || bLoopFilter ) ? rcMbData : xGetOutMbData() );
}

ErrVal MbDataCtrl::initMb( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, const Bool bFieldFlag, const Int iForceQp )
{
  UInt     uiCurrIdx    = uiMbY        * m_uiMbStride + uiMbX + m_uiMbOffset;
  MbData&  rcMbDataCurr = m_pcMbData[ uiCurrIdx ];
  rcMbDataCurr.setFieldFlag( bFieldFlag );

  return initMb( rpcMbDataAccess, uiMbY, uiMbX, iForceQp );
}

ErrVal MbDataCtrl::initMb( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, const Int iForceQp )
{
  ROF( m_bInitDone );

  AOT_DBG( uiMbY * m_uiMbStride + uiMbX + m_uiMbOffset >= m_uiSize );

  Bool     bLf          = (m_eProcessingState == POST_PROCESS);
  Bool     bMbAff       = m_pcSliceHeader->isMbaffFrame();
  Bool     bTopMb       = ((bMbAff && (uiMbY % 2)) ? false : true);
  UInt     uiMbYComp    = ( bMbAff ? ( bTopMb ? uiMbY+1 : uiMbY-1 ) : uiMbY );
  UInt     uiCurrIdx    = uiMbY * m_uiMbStride + uiMbX + m_uiMbOffset;
  UInt     uiCompIdx    = uiMbYComp    * m_uiMbStride + uiMbX + m_uiMbOffset;
  ROT( uiCompIdx >= m_uiSize );
  ROT( uiCurrIdx >= m_uiSize );
  MbData&  rcMbDataCurr = m_pcMbData[ uiCurrIdx ];
	MbData&  rcMbDataComp = m_pcMbData[ uiCompIdx ];


    //----- get co-located MbIndex -----
  UInt     uiIdxColTop;
  UInt     uiIdxColBot;
  if( ! m_pcSliceHeader->getFieldPicFlag() )
  {
    UInt  uiMbYColTop = 2 * ( uiMbY / 2 );
    uiIdxColTop       = uiMbYColTop * m_uiMbStride + uiMbX + m_uiMbOffset;
    uiIdxColBot       = uiIdxColTop + m_uiMbStride;
    if( uiIdxColBot >= m_pcSliceHeader->getMbInPic() )
    {
      uiIdxColBot = uiIdxColTop;
    }
  }
  else if( ! m_pcSliceHeader->getBottomFieldFlag() )
  {
    uiIdxColTop       = uiCurrIdx   + m_iColocatedOffset;
    uiIdxColBot       = uiIdxColTop - m_iColocatedOffset + m_iMbPerLine;
  }
  else
  {
    uiIdxColBot       = uiCurrIdx   - m_iColocatedOffset;
    uiIdxColTop       = uiIdxColBot + m_iColocatedOffset - m_iMbPerLine;
  }

  if( m_pcMbDataAccess )
  {
    m_ucLastMbQp    = m_pcMbDataAccess->getMbData().getQp();
    m_ucLastMbQp4LF = m_pcMbDataAccess->getMbData().getQp4LF();
  }

  UInt uiSliceId    = rcMbDataCurr.getSliceId();
  UInt uiMbAddr     = m_pcSliceHeader->getMbAddressFromPosition ( uiMbY, uiMbX );
  UInt uiMapUnit    = m_pcSliceHeader->getMapUnitFromPosition   ( uiMbY, uiMbX );

  if( PARSE_PROCESS == m_eProcessingState || ENCODE_PROCESS == m_eProcessingState)
  {
    if( 0 == uiSliceId )
    {
      uiSliceId = m_uiSliceId;
      rcMbDataCurr.getMbTCoeffs().clear();
      rcMbDataCurr.initMbData( m_ucLastMbQp, m_ucLastMbQp4LF, uiSliceId, uiMbAddr, uiMapUnit, m_pcSliceHeader->getSliceType() );
      rcMbDataCurr.clear();
      m_uiMbProcessed++;
    }
    else
    {
      //allready assigned;
      if( ENCODE_PROCESS != m_eProcessingState )
      {
       AF();
      }
      else
      {
        if( iForceQp != -1 )
        {
          m_ucLastMbQp = iForceQp;
        }
      }
    }
  }

  const Bool bColocatedField = ( m_pcMbDataCtrl0L1 == NULL ) ? true : m_pcMbDataCtrl0L1->isPicCodedField();

  Int icurrSliceGroupID = getSliceGroupIDofMb( uiMapUnit );

  m_pcMbDataAccess = new (m_pcMbDataAccess) MbDataAccess( rcMbDataCurr,                                      // current
                                                        	rcMbDataComp,                                      // complementary
                                                          xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY,   uiMbX-1, bLf ), // left
                                                          xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-1, uiMbX  , bLf ), // above
                                                          xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-1, uiMbX-1, bLf ), // above left
																													((bMbAff && (uiMbY % 2 == 1)) ? xGetOutMbData() : xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-1, uiMbX+1, bLf )), // above right
																													xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-2, uiMbX  , bLf ), // above above
																													xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-2, uiMbX-1, bLf ), // above above left
																													xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY-2, uiMbX+1, bLf ), // above above right
																													xGetRefMbData( uiSliceId, icurrSliceGroupID, uiMbY+1, uiMbX-1, bLf ), // below left
                                                          xGetOutMbData(),                                   // unvalid
																													xGetColMbData( uiIdxColTop ),
																													xGetColMbData( uiIdxColBot ),
                                                         *m_pcSliceHeader,
                                                          uiMbX,
                                                          uiMbY,
																													bTopMb,
																													m_bUseTopField,
                                                          bColocatedField,// TMM_INTERLACE
                                                          m_ucLastMbQp, m_ucLastMbQp4LF );

  ROT( NULL == m_pcMbDataAccess );

  rpcMbDataAccess = m_pcMbDataAccess;

  return Err::m_nOK;
}






ControlData::ControlData()
: m_pcMbDataCtrl         ( 0   )
, m_pcMbDataCtrl0L1      ( 0   )
, m_pcSliceHeader        ( 0   )
, m_pcSliceHeaderBot     ( 0   )
, m_dLambda              ( 0   )
, m_pcBaseLayerRec       ( 0   )
, m_pcBaseLayerSbb       ( 0   )
, m_pcBaseLayerCtrl      ( 0   )
, m_pcBaseLayerCtrlField ( 0   )
, m_uiUseBLMotion        ( 0   )
, m_dScalingFactor       ( 1.0 )
, m_bSpatialScalability  ( false)
{
}

ControlData::~ControlData()
{
}

Void
ControlData::clear()
{
  m_pcMbDataCtrl0L1 = 0;
  m_pcBaseLayerRec       = 0;
  m_pcBaseLayerSbb       = 0;
  m_pcBaseLayerCtrl      = 0;
  m_pcBaseLayerCtrlField = 0;
  m_uiUseBLMotion        = 0;
  m_dScalingFactor       = 1.0;
}

ErrVal
ControlData::init( SliceHeader*  pcSliceHeader,
                   MbDataCtrl*   pcMbDataCtrl,
                   Double        dLambda )
{
  ROF( pcSliceHeader );
  ROF( pcMbDataCtrl  );

  m_pcSliceHeader = pcSliceHeader;
  m_pcMbDataCtrl  = pcMbDataCtrl;
  m_dLambda       = dLambda;

  m_pcMbDataCtrl0L1       = 0;
  m_pcBaseLayerRec        = 0;
  m_pcBaseLayerSbb        = 0;
  m_pcBaseLayerCtrl       = 0;
  m_pcBaseLayerCtrlField  = 0;
  m_uiUseBLMotion         = 0;

  return Err::m_nOK;
}

ErrVal
ControlData::init( SliceHeader*  pcSliceHeader )
{
  ROF( pcSliceHeader );
  ROF( m_pcMbDataCtrl  );

  m_pcSliceHeader         = pcSliceHeader;

  m_pcMbDataCtrl0L1       = 0;
  m_pcBaseLayerRec        = 0;
  m_pcBaseLayerSbb        = 0;
  m_pcBaseLayerCtrl       = 0;
  m_pcBaseLayerCtrlField  = 0;
  m_uiUseBLMotion         = 0;

  return Err::m_nOK;
}

const Int MbDataCtrl::getSliceGroupIDofMb(Int mb)
{
  Int iRefSliceID ;
  if(m_pcSliceHeader->getFMO() != NULL)
	iRefSliceID =m_pcSliceHeader->getFMO()->getSliceGroupId(mb );
  else
	iRefSliceID =-1;

  return iRefSliceID ;
}


ErrVal
MbDataCtrl::getBoundaryMask( Int iMbY, Int iMbX, Bool& rbIntra, UInt& ruiMask, UInt uiCurrentSliceID ) const
{
  ruiMask         = 0;
  UInt uiCurrIdx  = iMbY * m_uiMbStride + iMbX + m_uiMbOffset;
  AOT( uiCurrIdx >= m_uiSize );

  rbIntra  = m_pcMbData[uiCurrIdx].isIntraInSlice( uiCurrentSliceID );
  ruiMask |= ( rbIntra ? 0x100 : 0 );

  Bool bLeftAvailable   = ( iMbX > 0 );
  Bool bTopAvailable    = ( iMbY > 0 );
  Bool bRightAvailable  = ( iMbX < m_iMbPerLine-1 );
  Bool bBottomAvailable = ( iMbY < m_iMbPerColumn-1 );

  if( bTopAvailable )
  {
    {
      Int iIndex = uiCurrIdx - m_uiMbStride;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x01 : 0 );
    }
    if( bLeftAvailable )
    {
      Int iIndex = uiCurrIdx - m_uiMbStride - 1;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x80 : 0 );
    }
    if( bRightAvailable )
    {
      Int iIndex = uiCurrIdx - m_uiMbStride + 1;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x02 : 0 );
    }
  }
  if( bBottomAvailable )
  {
    {
      Int iIndex = uiCurrIdx + m_uiMbStride;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x10 : 0 );
    }
    if( bLeftAvailable )
    {
      Int iIndex = uiCurrIdx  + m_uiMbStride - 1;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x20 : 0 );
    }
    if( bRightAvailable )
    {
      Int iIndex = uiCurrIdx + m_uiMbStride + 1;
      ruiMask   |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x08 : 0 );
    }
  }
  if( bLeftAvailable )
  {
    Int iIndex   = uiCurrIdx - 1;
    ruiMask     |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x40 : 0 );
  }
  if( bRightAvailable )
  {
    Int iIndex   = uiCurrIdx + 1;
    ruiMask     |= ( m_pcMbData[iIndex].isIntraInSlice( uiCurrentSliceID ) ? 0x04 : 0 );
  }
  return Err::m_nOK;
}


ErrVal
MbDataCtrl::getBoundaryMask_MbAff( Int iMbY, Int iMbX, Bool& rbIntra, UInt& ruiMask, UInt uiCurrentSliceID ) const
{
  ROF( iMbY >= 0 && iMbY < m_iMbPerColumn && iMbX >= 0 && iMbX < m_iMbPerLine );

  Bool  bAvailableTopLeft     = false; //0x001
  Bool  bAvailableTop         = false; //0x002
  Bool  bAvailableTopRight    = false; //0x004
  Bool  bAvailableLeftTop     = false; //0x008
  Bool  bAvailableLeftBot     = false; //0x010
  Bool  bAvailableCurrTop     = false; //0x020
  Bool  bAvailableCurrBot     = false; //0x040
  Bool  bAvailableRightTop    = false; //0x080
  Bool  bAvailableRightBot    = false; //0x100
  Bool  bAvailableBotLeft     = false; //0x200
  Bool  bAvailableBot         = false; //0x400
  Bool  bAvailableBotRight    = false; //0x800

  Int   iMbY0                 = ( iMbY >> 1 ) << 1;
  Int   iMbFieldOffset        = ( iMbY - iMbY0 ) * (Int)m_uiMbStride;

  Bool  bMbPairAvailableTop   = ( iMbY0 > 0 );
  Bool  bMbPairAvailableBot   = ( iMbY0 < m_iMbPerColumn - 2 );
  Bool  bMbPairAvailableLeft  = ( iMbX  > 0 );
  Bool  bMbPairAvailableRight = ( iMbX  < m_iMbPerLine   - 1 );

  Int   iMbPairTIdxCurr       = iMbY0 * (Int)m_uiMbStride + iMbX + (Int)m_uiMbOffset;
  Int   iMbPairTIdxTop        = iMbPairTIdxCurr - (Int)( m_uiMbStride << 1 );
  Int   iMbPairTIdxBot        = iMbPairTIdxCurr + (Int)( m_uiMbStride << 1 );

  //===== current macroblock pair =====
  {
    Int iMbIdxCurrTop   = iMbPairTIdxCurr + ( m_pcMbData[iMbPairTIdxCurr].getFieldFlag() ? iMbFieldOffset : 0            );
    Int iMbIdxCurrBot   = iMbPairTIdxCurr + ( m_pcMbData[iMbPairTIdxCurr].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
    bAvailableCurrTop   = m_pcMbData[iMbIdxCurrTop].isIntraInSlice( uiCurrentSliceID );
    bAvailableCurrBot   = m_pcMbData[iMbIdxCurrBot].isIntraInSlice( uiCurrentSliceID );
  }

  //===== reset =====
  ruiMask = 0;
  rbIntra = ( bAvailableCurrTop && bAvailableCurrBot );

  //===== left macroblock pair =====
  if( bMbPairAvailableLeft )
  {
    Int iMbIdxLeftTop   = iMbPairTIdxCurr - 1 + ( m_pcMbData[iMbPairTIdxCurr - 1].getFieldFlag() ? iMbFieldOffset : 0            );
    Int iMbIdxLeftBot   = iMbPairTIdxCurr - 1 + ( m_pcMbData[iMbPairTIdxCurr - 1].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
    bAvailableLeftTop   = m_pcMbData[iMbIdxLeftTop].isIntraInSlice( uiCurrentSliceID );
    bAvailableLeftBot   = m_pcMbData[iMbIdxLeftBot].isIntraInSlice( uiCurrentSliceID );
  }

  //===== right macroblock pair =====
  if( bMbPairAvailableRight )
  {
    Int iMbIdxRightTop  = iMbPairTIdxCurr + 1 + ( m_pcMbData[iMbPairTIdxCurr + 1].getFieldFlag() ? iMbFieldOffset : 0            );
    Int iMbIdxRightBot  = iMbPairTIdxCurr + 1 + ( m_pcMbData[iMbPairTIdxCurr + 1].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
    bAvailableRightTop  = m_pcMbData[iMbIdxRightTop].isIntraInSlice( uiCurrentSliceID );
    bAvailableRightBot  = m_pcMbData[iMbIdxRightBot].isIntraInSlice( uiCurrentSliceID );
  }

  if( bMbPairAvailableTop )
  {
    //===== top macroblock pair =====
    {
      Int iMbIdxTop = iMbPairTIdxTop + ( m_pcMbData[iMbPairTIdxTop].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
      bAvailableTop = m_pcMbData[iMbIdxTop].isIntraInSlice( uiCurrentSliceID );
    }

    //===== top-left macroblock pair =====
    if( bMbPairAvailableLeft )
    {
      Int iMbIdxTopLeft = iMbPairTIdxTop - 1 + ( m_pcMbData[iMbPairTIdxTop - 1].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
      bAvailableTopLeft = m_pcMbData[iMbIdxTopLeft].isIntraInSlice( uiCurrentSliceID );
    }

    //===== top-right macroblock pair =====
    if( bMbPairAvailableRight )
    {
      Int iMbIdxTopRight = iMbPairTIdxTop + 1 + ( m_pcMbData[iMbPairTIdxTop + 1].getFieldFlag() ? iMbFieldOffset : m_uiMbStride );
      bAvailableTopRight = m_pcMbData[iMbIdxTopRight].isIntraInSlice( uiCurrentSliceID );
    }
  }

  if( bMbPairAvailableBot )
  {
    //===== bottom macroblock pair =====
    {
      Int iMbIdxBot = iMbPairTIdxBot + ( m_pcMbData[iMbPairTIdxBot].getFieldFlag() ? iMbFieldOffset : 0 );
      bAvailableBot = m_pcMbData[iMbIdxBot].isIntraInSlice( uiCurrentSliceID );
    }

    //===== bottom-left macroblock pair =====
    if( bMbPairAvailableLeft )
    {
      Int iMbIdxBotLeft = iMbPairTIdxBot - 1 + ( m_pcMbData[iMbPairTIdxBot - 1].getFieldFlag() ? iMbFieldOffset : 0 );
      bAvailableBotLeft = m_pcMbData[iMbIdxBotLeft].isIntraInSlice( uiCurrentSliceID );
    }

    //===== bottom-right macroblock pair =====
    if( bMbPairAvailableRight )
    {
      Int iMbIdxBotRight = iMbPairTIdxBot + 1 + ( m_pcMbData[iMbPairTIdxBot + 1].getFieldFlag() ? iMbFieldOffset : 0 );
      bAvailableBotRight = m_pcMbData[iMbIdxBotRight].isIntraInSlice( uiCurrentSliceID );
    }
  }

  //===== set mask =====
  ruiMask |= ( bAvailableTopLeft  ? 0x001 : 0 );
  ruiMask |= ( bAvailableTop      ? 0x002 : 0 );
  ruiMask |= ( bAvailableTopRight ? 0x004 : 0 );
  ruiMask |= ( bAvailableLeftTop  ? 0x008 : 0 );
  ruiMask |= ( bAvailableLeftBot  ? 0x010 : 0 );
  ruiMask |= ( bAvailableCurrTop  ? 0x020 : 0 );
  ruiMask |= ( bAvailableCurrBot  ? 0x040 : 0 );
  ruiMask |= ( bAvailableRightTop ? 0x080 : 0 );
  ruiMask |= ( bAvailableRightBot ? 0x100 : 0 );
  ruiMask |= ( bAvailableBotLeft  ? 0x200 : 0 );
  ruiMask |= ( bAvailableBot      ? 0x400 : 0 );
  ruiMask |= ( bAvailableBotRight ? 0x800 : 0 );

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END

