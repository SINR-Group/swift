
#include "H264AVCEncoderLib.h"
#include "H264AVCEncoder.h"

#include "GOPEncoder.h"
#include "CreaterH264AVCEncoder.h"
#include "ControlMngH264AVCEncoder.h"
#include "H264AVCCommonLib/ParameterSetMng.h"

#include <math.h>

// JVT-W043 {
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"
// JVT-W043 }
// JVT-V068 HRD {
#include "Scheduler.h"
#include "H264AVCCommonLib/Hrd.h"
// JVT-V068 HRD }

H264AVC_NAMESPACE_BEGIN


H264AVCEncoder::H264AVCEncoder():
  m_pcParameterSetMng ( NULL ),
  m_pcPocCalculator   ( NULL ),
  m_pcNalUnitEncoder  ( NULL ),
  m_pcControlMng      ( NULL ),
  m_pcCodingParameter ( NULL ),
  m_bVeryFirstCall    ( true ),
  m_bScalableSeiMessage( false ),
  m_bInitDone         ( false ),
  m_bTraceEnable      ( false ),
  m_bWrteROISEI		  ( true ),
  m_loop_roi_sei	  ( 0 )
{
  ::memset( m_apcLayerEncoder, 0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_aaadFinalFramerate, 0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(Double) );
  ::memset( m_aaadSeqBits,        0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(Double) );
  //JVT-W052
  ::memset( m_uiNumofCGS,     0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_uicrcVal,       0x00, MAX_LAYERS*sizeof(UInt) );
  //JVT-W052
  //JVT-W051 {
  ::memset( m_uiProfileIdc, 0, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_uiLevelIdc, 0, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_bConstraint0Flag, 0, MAX_LAYERS*sizeof(Bool) );
  ::memset( m_bConstraint1Flag, 0, MAX_LAYERS*sizeof(Bool) );
  ::memset( m_bConstraint2Flag, 0, MAX_LAYERS*sizeof(Bool) );
  ::memset( m_bConstraint3Flag, 0, MAX_LAYERS*sizeof(Bool) );
  //JVT-W051 }
  for( UInt ui = 0; ui < MAX_LAYERS; ui++ )
  {
    for( UInt uj = 0; uj < MAX_TEMP_LEVELS; uj++ )
    {
      for( UInt uk = 0; uk < MAX_QUALITY_LEVELS; uk++ )
      {
        m_aaadLayerBitrateRep[ui][uj][uk] = 0;
        m_aaauiScalableLayerId[ui][uj][uk] = MSYS_UINT_MAX;  // BUG_FIX Shenqiu (06-04-08)
      }
    }
  }

}

H264AVCEncoder::~H264AVCEncoder()
{
}


ErrVal
H264AVCEncoder::create( H264AVCEncoder*& rpcH264AVCEncoder )
{
  rpcH264AVCEncoder = new H264AVCEncoder;

  ROT( NULL == rpcH264AVCEncoder );

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::destroy()
{
  delete this;
  return Err::m_nOK;
}


Frame*
H264AVCEncoder::getLowPassRec( UInt uiLayerId, UInt uiLowPassIndex )
{
  //===== shall not be called in non-CGS mode =====
  Frame* pcLPRec = 0;
  for( UInt uiBL = MSYS_UINT_MAX; ( uiBL = m_pcCodingParameter->getLayerParameters( uiLayerId ).getBaseLayerId() ) != MSYS_UINT_MAX; uiLayerId = uiBL )
  {
    if( m_apcLayerEncoder[uiBL]->getMGSLPRec( uiLowPassIndex ) && m_pcCodingParameter->getLayerParameters( uiBL ).getQualityLevelCGSSNR() == 0 )  //zhangxd_20101220
    {
      pcLPRec = m_apcLayerEncoder[uiBL]->getMGSLPRec( uiLowPassIndex );
	    break; //zhangxd_20101220
    }
  }
  return pcLPRec;
}


Frame*
H264AVCEncoder::getELRefPic( UInt uiLayerId, UInt uiTemporalId, UInt uiFrameIdInTId )
{
  Frame* pcELRefPic = 0;
  for( UInt uiELId = uiLayerId + 1; uiELId < m_pcCodingParameter->getNumberOfLayers(); uiELId++ )
  {
    //===== check whether higher layer can be used =====
    LayerParameters&  rcCL  = m_pcCodingParameter->getLayerParameters( uiLayerId  );
    LayerParameters&  rcEL  = m_pcCodingParameter->getLayerParameters( uiELId     );
    if( ! rcEL.getResizeParameters().getCroppingFlag() &&
        rcCL.getFrameWidthInSamples () == rcEL.getFrameWidthInSamples () &&
        rcCL.getFrameHeightInSamples() == rcEL.getFrameHeightInSamples() &&
        rcCL.getResizeParameters().m_iExtendedSpatialScalability == ESS_NONE )
    {
      pcELRefPic = m_apcLayerEncoder[uiELId]->getRefPic( uiTemporalId, uiFrameIdInTId );
    }
    else
    {
      break;
    }
  }
  return pcELRefPic;
}

UInt
H264AVCEncoder::getMaxSliceSize( UInt uiLayerId, UInt uiAUIndex )
{
  UInt uiMaxSliceSize = MSYS_UINT_MAX;
  for( UInt uiLId = uiLayerId; uiLId < m_pcCodingParameter->getNumberOfLayers(); uiLId++ )
  {
    ANOK( m_apcLayerEncoder[uiLId]->updateMaxSliceSize( uiAUIndex, uiMaxSliceSize ) );
  }
  return uiMaxSliceSize;
}

UInt
H264AVCEncoder::getPicCodingType( UInt uiBaseLayerId, UInt uiTemporalId, UInt uiFrmIdInTLayer )
{
  AOF( uiBaseLayerId < m_pcCodingParameter->getNumberOfLayers() );
  return m_apcLayerEncoder[ uiBaseLayerId ]->getPicCodingType( uiTemporalId, uiFrmIdInTLayer );
}

Bool    
H264AVCEncoder::hasMGSEnhancementLayer( UInt uiLayerId, UInt& ruiMaxLevelIdc )
{
  Bool bMGSEnhLayer = false;
  for( UInt ui = uiLayerId+1; ui < m_pcCodingParameter->getNumberOfLayers(); ui++ )
  {
    if( m_apcLayerEncoder[ui]->isMGSEnhancementLayer( ruiMaxLevelIdc ) )
      bMGSEnhLayer = true;
    else
      break;
  }
  return bMGSEnhLayer;
}

ErrVal
H264AVCEncoder::getBaseLayerLevelIdc( UInt uiBaseLayerId, UInt& uiLevelIdc, Bool& bBiPred8x8Disable, Bool& bMCBlks8x8Disable )
{
  RNOK( m_apcLayerEncoder[uiBaseLayerId]->getBaseLayerLevelIdc( uiLevelIdc, bBiPred8x8Disable, bMCBlks8x8Disable ) );
  return Err::m_nOK;
}

ErrVal
H264AVCEncoder::getBaseLayerStatus( UInt&   ruiBaseLayerId,
                                    UInt    uiLayerId,
                                    PicType ePicType,
                                    UInt		uiTemporalId )
{
  //===== check data availability =====
  if( ruiBaseLayerId < m_pcCodingParameter->getNumberOfLayers() )
  {
    Bool  bExists   = false;
    RNOK( m_apcLayerEncoder[ruiBaseLayerId]->getBaseLayerStatus( bExists, ePicType, uiTemporalId ) );
    ruiBaseLayerId  = ( bExists ? ruiBaseLayerId : MSYS_UINT_MAX );
    return Err::m_nOK;
  }
  ruiBaseLayerId    = MSYS_UINT_MAX;
  return Err::m_nOK;
}

ErrVal
H264AVCEncoder::getBaseLayerDataAvlb( Frame*&       pcFrame,
                                      Frame*&       pcResidual,
                                      MbDataCtrl*&  pcMbDataCtrl,
                                      UInt          uiBaseLayerId,
                                      Bool&         bBaseDataAvailable,
                                      PicType       ePicType,
                                      UInt					 uiTemporalId )
{
  ROF ( uiBaseLayerId < MAX_LAYERS );
  RNOK( m_apcLayerEncoder[uiBaseLayerId]->getBaseLayerDataAvlb( pcFrame,
                                                                pcResidual,
                                                                pcMbDataCtrl,
                                                                ePicType,
                                                                uiTemporalId ) );
  bBaseDataAvailable = pcFrame && pcResidual && pcMbDataCtrl;
  return Err::m_nOK;
}

// JVT-AD021 {
ErrVal
H264AVCEncoder::getBaseLayerQpPredData ( UInt uiBaseLayerId , Double & dQpPredData , Int & iPOC , UInt & uiFrameSizeInMB )
{
  if( uiBaseLayerId >= MAX_LAYERS )
    return( Err::m_nDataNotAvailable );
  
  RNOK( m_apcLayerEncoder[uiBaseLayerId]->getCurQpPredData( dQpPredData , iPOC , uiFrameSizeInMB ) );
  
  return Err::m_nOK;
}
// JVT-AD021 }

ErrVal
H264AVCEncoder::getBaseLayerData( SliceHeader&  rcELSH,
                                  Frame*&       pcFrame,
                                  Frame*&       pcResidual,
                                  MbDataCtrl*&  pcMbDataCtrl,
                                  Bool          bSpatialScalability,
                                  UInt          uiBaseLayerId,
                                  PicType       ePicType,
                                  UInt				  uiTemporalId )
{
  ROF ( uiBaseLayerId < MAX_LAYERS );
  RNOK( m_apcLayerEncoder[uiBaseLayerId]->getBaseLayerData( rcELSH,
                                                            pcFrame,
                                                            pcResidual,
                                                            pcMbDataCtrl,
                                                            bSpatialScalability,
                                                            ePicType,
                                                            uiTemporalId ) );

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::getBaseLayerResidual( Frame*&      pcResidual,
                                      UInt            uiBaseLayerId)
{
  pcResidual = m_apcLayerEncoder[uiBaseLayerId]->getBaseLayerResidual();
  return Err::m_nOK;
}


UInt
H264AVCEncoder::getNewBits( UInt uiBaseLayerId )
{
  ROFRS( uiBaseLayerId < MAX_LAYERS, 0 );
  return m_apcLayerEncoder[uiBaseLayerId]->getNewBits();
}


ErrVal
H264AVCEncoder::init( LayerEncoder*     apcLayerEncoder[MAX_LAYERS],
                      ParameterSetMng*  pcParameterSetMng,
                      PocCalculator*    pcPocCalculator,
                      NalUnitEncoder*   pcNalUnitEncoder,
                      ControlMngIf*     pcControlMng,
                      CodingParameter*  pcCodingParameter,
                      // JVT-V068 {
                      StatBuf<Scheduler*, MAX_SCALABLE_LAYERS>* apcScheduler
                      // JVT-V068 }
                      )
{
  ROT( NULL == apcLayerEncoder );
  ROT( NULL == pcParameterSetMng );
  ROT( NULL == pcPocCalculator );
  ROT( NULL == pcNalUnitEncoder );
  ROT( NULL == pcControlMng );
  ROT( NULL == pcCodingParameter );

  m_pcParameterSetMng = pcParameterSetMng;
  m_pcPocCalculator   = pcPocCalculator;
  m_pcNalUnitEncoder  = pcNalUnitEncoder;
  m_pcControlMng      = pcControlMng;
  m_pcCodingParameter = pcCodingParameter;

  // JVT-V068 {
  m_apcScheduler = apcScheduler;
  m_apcScheduler->setAll( 0 );
  // JVT-V068 }

  UInt uiLayer;
  for( uiLayer = 0; uiLayer < m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
  {
    ROT( NULL == apcLayerEncoder[uiLayer] );
    m_apcLayerEncoder[uiLayer] = apcLayerEncoder[uiLayer];
  }
  for( ; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    m_apcLayerEncoder[uiLayer] = 0;
  }

  m_cAccessUnitDataList.clear();

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::uninit()
{
  m_cUnWrittenSPS.clear();
  m_cUnWrittenPPS.clear();
  m_pcParameterSetMng           = NULL;
  m_pcPocCalculator             = NULL;
  m_pcNalUnitEncoder            = NULL;
  m_pcControlMng                = NULL;
  m_pcCodingParameter           = NULL;
  m_bInitDone                   = false;
  m_bVeryFirstCall              = true;
  m_bScalableSeiMessage         = true;
  m_bTraceEnable                = false;

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    m_apcLayerEncoder    [uiLayer] = NULL;
    m_acOrgPicBufferList[uiLayer]   .clear();
    m_acRecPicBufferList[uiLayer]   .clear();
  }

  m_cAccessUnitDataList.clear();

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::xWriteScalableSEI( ExtBinDataAccessor* pcExtBinDataAccessor )
{
  //===== create message =====
  SEI::ScalableSei* pcScalableSEI;
  RNOK(SEI::ScalableSei::create(pcScalableSEI) );


  //===== set message =====
  UInt j = 0; //JVT-S036 lsj
  UInt i = 0;
  UInt uiInputLayers = m_pcCodingParameter->getNumberOfLayers ();
  UInt uiLayerNum = 0;	//total scalable layer numbers
  for ( i = 0; i < uiInputLayers; i++ )	//calculate total scalable layer numbers
  {
//bug-fix suffix{{
    //Bool bH264AVCCompatible = ( i == 0 && m_pcCodingParameter->getBaseLayerMode() > 0 );
    //Bool bH264AVCCompatible = ( i == 0 );
//bug-fix suffix}}
    //Bool bSubSeq            = ( i == 0 && m_pcCodingParameter->getBaseLayerMode() > 1 );

    LayerParameters& rcLayer = m_pcCodingParameter->getLayerParameters ( i );
    UInt uiTotalTempLevel = rcLayer.getDecompositionStages () - rcLayer.getNotCodedStages    ();
// *LMH(20060203): Fix Bug due to underflow (Replace)
    //UInt uiMinTempLevel   = ( !bH264AVCCompatible ||bSubSeq ) ? 0: gMax( 0, uiTotalTempLevel - 1 );
    UInt uiMinTempLevel   = 0; //bugfix replace
    UInt uiActTempLevel   = uiTotalTempLevel - uiMinTempLevel + 1;
    uiLayerNum += uiActTempLevel * rcLayer.getNumberOfQualityLevelsCGSSNR();

    pcScalableSEI->setROINum ( i, rcLayer.getNumROI() );
    pcScalableSEI->setROIID  ( i, rcLayer.getROIID() );
    pcScalableSEI->setSGID  ( i, rcLayer.getSGID() );
    pcScalableSEI->setSLID  ( i, rcLayer.getSLID() );
  }

  UInt uiNumLayersMinus1 = uiLayerNum - 1;
  pcScalableSEI->setNumLayersMinus1 ( uiNumLayersMinus1 );

  UInt uiNumScalableLayer = 0;
  for ( UInt uiCurrLayer = 0; uiCurrLayer < uiInputLayers; uiCurrLayer++)
  {
    LayerParameters& rcLayer = m_pcCodingParameter->getLayerParameters ( uiCurrLayer );
    UInt uiTotalTempLevel = rcLayer.getDecompositionStages () - rcLayer.getNotCodedStages    () + 1;
    UInt uiTotalFGSLevel = rcLayer.getNumberOfQualityLevelsCGSSNR();
    UInt uiMinTempLevel     = 0; //bugfix replace

    for ( UInt uiCurrTempLevel = 0; uiCurrTempLevel < uiTotalTempLevel; uiCurrTempLevel++ )
    {
      for ( UInt uiCurrFGSLevel = 0; uiCurrFGSLevel < uiTotalFGSLevel; uiCurrFGSLevel++ )
      {
        if( uiCurrTempLevel >= uiMinTempLevel )
        {
          //Bool bSubPicLayerFlag = false;
          Bool bSubRegionLayerFlag = false;
          Bool bProfileLevelInfoPresentFlag = false;
          //Bool bInitParameterSetsInfoPresentFlag = false;		//may be changed  //JVT-S036 lsj//SEI changes update
          Bool bParameterSetsInfoPresentFlag = false;		//may be changed  //JVT-S036 lsj//SEI changes update
					if( uiNumScalableLayer == 0 )
          {//JVT-S036 lsj
            bSubRegionLayerFlag = true;
            bProfileLevelInfoPresentFlag = true;
            //bInitParameterSetsInfoPresentFlag = true;	//SEI changes update
            bParameterSetsInfoPresentFlag = true;	//SEI changes update
          }
          Bool bBitrateInfoPresentFlag = true;
          Bool bFrmRateInfoPresentFlag = true;//rcLayer.getInputFrameRate () > 0;
          Bool bFrmSizeInfoPresentFlag = true;
// BUG_FIX liuhui{
          Bool bLayerDependencyInfoPresentFlag = true;			//may be changed
// BUG_FIX liuhui}
          Bool bExactInterayerPredFlag = true;			//JVT-S036 lsj may be changed
          pcScalableSEI->setLayerId(uiNumScalableLayer, uiNumScalableLayer);

          UInt uiTempLevel = uiCurrTempLevel; //BUG_FIX_FT_01_2006
          UInt uiDependencyID = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getLayerCGSSNR() : uiCurrLayer);
          UInt uiQualityLevel = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getQualityLevelCGSSNR() + uiCurrFGSLevel : uiCurrFGSLevel);
          m_aaauiScalableLayerId[uiDependencyID][uiCurrTempLevel][uiQualityLevel] = uiNumScalableLayer;

          //UInt uiSimplePriorityId = 0;//SEI changes update
					UInt uiPriorityId = 0;//SEI changes update
          Bool bDiscardableFlag  = false;
          if( uiCurrFGSLevel > 0 || rcLayer.isDiscardable() ) // bugfix replace
            bDiscardableFlag = true;
          //pcScalableSEI->setSimplePriorityId(uiNumScalableLayer, uiSimplePriorityId);//SEI update
          pcScalableSEI->setPriorityId(uiNumScalableLayer, uiPriorityId);//SEI update
          pcScalableSEI->setDiscardableFlag(uiNumScalableLayer, bDiscardableFlag);
          pcScalableSEI->setTemporalId(uiNumScalableLayer, uiTempLevel);
          pcScalableSEI->setDependencyId(uiNumScalableLayer, uiDependencyID);
          pcScalableSEI->setQualityLevel(uiNumScalableLayer, uiQualityLevel);
   //JVT-S036 lsj end
          pcScalableSEI->setSubRegionLayerFlag(uiNumScalableLayer, bSubRegionLayerFlag);
          // JVT-S054 (REPLACE)
          //pcScalableSEI->setIroiSliceDivisionInfoPresentFlag(uiNumScalableLayer, bIroiSliceDivisionFlag); //JVT-S036 lsj
          pcScalableSEI->setIroiSliceDivisionInfoPresentFlag(uiNumScalableLayer, rcLayer.m_bSliceDivisionFlag);
          pcScalableSEI->setProfileLevelInfoPresentFlag(uiNumScalableLayer, bProfileLevelInfoPresentFlag);
          pcScalableSEI->setBitrateInfoPresentFlag(uiNumScalableLayer, bBitrateInfoPresentFlag);
          pcScalableSEI->setFrmRateInfoPresentFlag(uiNumScalableLayer, bFrmRateInfoPresentFlag);
          pcScalableSEI->setFrmSizeInfoPresentFlag(uiNumScalableLayer, bFrmSizeInfoPresentFlag);
          pcScalableSEI->setLayerDependencyInfoPresentFlag(uiNumScalableLayer, bLayerDependencyInfoPresentFlag);
          //pcScalableSEI->setInitParameterSetsInfoPresentFlag(uiNumScalableLayer, bInitParameterSetsInfoPresentFlag);//SEI changes update
          pcScalableSEI->setParameterSetsInfoPresentFlag(uiNumScalableLayer, bParameterSetsInfoPresentFlag);//SEI changes update

          pcScalableSEI->setExactInterlayerPredFlag(uiNumScalableLayer, bExactInterayerPredFlag);//JVT-S036 lsj
          //JVT-W046 {
          //pcScalableSEI->setAvcLayerConversionFlag(uiNumScalableLayer, 0); //default: 0, could be changed
					pcScalableSEI->setLayerConversionFlag(uiNumScalableLayer, 0); //default: 0, could be changed SEI changes update
          //JVT-W046 }
          //JVT-W047
          if(m_apcLayerEncoder[0]->m_bOutputFlag)
            pcScalableSEI->setLayerOutputFlag(uiNumScalableLayer,true);
          else
            pcScalableSEI->setLayerOutputFlag(uiNumScalableLayer,false);
          //JVT-W047

          if(pcScalableSEI->getProfileLevelInfoPresentFlag(uiNumScalableLayer))
          {
            UInt uilayerProfileIdc = 0;	//may be changed
            Bool bLayerConstraintSet0Flag = false;	//may be changed
//bug-fix suffix{{
//					  Bool bH264AVCCompatibleTmp  = m_pcCodingParameter->getBaseLayerMode() > 0 && uiCurrLayer == 0;
            Bool bH264AVCCompatibleTmp  = ( uiCurrLayer == 0 );
//bug-fix suffix}}
            Bool bLayerConstraintSet1Flag = ( bH264AVCCompatibleTmp ? 1 : 0 );	//may be changed
            Bool bLayerConstraintSet2Flag = false;	//may be changed
            Bool bLayerConstraintSet3Flag = false;	//may be changed
            UInt uiLayerLevelIdc = 0;		//may be changed
            //JVT-W051 {
            uilayerProfileIdc = m_uiProfileIdc[uiCurrLayer];
            uiLayerLevelIdc = m_uiLevelIdc[uiCurrLayer];
            bLayerConstraintSet0Flag = m_bConstraint0Flag[uiCurrLayer];
            bLayerConstraintSet1Flag = m_bConstraint1Flag[uiCurrLayer];
            bLayerConstraintSet2Flag = m_bConstraint2Flag[uiCurrLayer];
            bLayerConstraintSet3Flag = m_bConstraint3Flag[uiCurrLayer];
            UInt uilayerProfileLevelIdc = 0;
            uilayerProfileLevelIdc += uilayerProfileIdc;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
            uilayerProfileLevelIdc += bLayerConstraintSet0Flag;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
            uilayerProfileLevelIdc += bLayerConstraintSet1Flag;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
            uilayerProfileLevelIdc += bLayerConstraintSet2Flag;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
            uilayerProfileLevelIdc += bLayerConstraintSet3Flag;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 4;
            uilayerProfileLevelIdc = uilayerProfileLevelIdc << 8;
            uilayerProfileLevelIdc += uiLayerLevelIdc;
            //JVT-W051 }
            pcScalableSEI->setLayerProfileLevelIdc( uiNumScalableLayer, uilayerProfileLevelIdc );
          }
					//else
          //{//JVT-S036 lsj
          //  UInt bProfileLevelInfoSrcLayerIdDelta = 0;  //may be changed

          //  pcScalableSEI->setProfileLevelInfoSrcLayerIdDelta(uiNumScalableLayer, bProfileLevelInfoSrcLayerIdDelta);
          //}
          //SEI changes update }
          if(pcScalableSEI->getBitrateInfoPresentFlag(uiNumScalableLayer))
          {
            Double  dAvgBitrate                 = m_aaadLayerBitrateRep[uiDependencyID][uiCurrTempLevel][uiQualityLevel];
            Double  dMaxBitrateLayer          = 0.0;	//should be changed
            Double  dMaxBitrateDecodedPicture = 0.0;	//should be changed
            UInt    uiMaxBitrateCalcWindow    = 0;    //should be changed

            pcScalableSEI->setAvgBitrateBPS(uiNumScalableLayer, dAvgBitrate);
            pcScalableSEI->setMaxBitrateLayerBPS(uiNumScalableLayer, dMaxBitrateLayer);
            pcScalableSEI->setMaxBitrateDecodedPictureBPS(uiNumScalableLayer, dMaxBitrateDecodedPicture);
            pcScalableSEI->setMaxBitrateCalcWindow(uiNumScalableLayer, uiMaxBitrateCalcWindow);
          }

          if(pcScalableSEI->getFrmRateInfoPresentFlag(uiNumScalableLayer))
          {
            UInt uiConstantFrmRateIdc = 0;
            ROT( m_aaadFinalFramerate[uiDependencyID][uiCurrTempLevel][uiQualityLevel] == 0.0 );
            UInt uiAvgFrmRate = (UInt)floor( 256.0 * m_aaadFinalFramerate[uiDependencyID][uiCurrTempLevel][uiQualityLevel] + 0.5 );

            pcScalableSEI->setConstantFrmRateIdc(uiNumScalableLayer, uiConstantFrmRateIdc);
            pcScalableSEI->setAvgFrmRate(uiNumScalableLayer, uiAvgFrmRate);
          }
          //SEI changes update {
          //else
          //{//JVT-S036 lsj
          //  UInt  bFrmRateInfoSrcLayerIdDelta = 0;  //may be changed

          //  pcScalableSEI->setFrmRateInfoSrcLayerIdDelta(uiNumScalableLayer, bFrmRateInfoSrcLayerIdDelta);
          //}

          //if(pcScalableSEI->getFrmSizeInfoPresentFlag(uiNumScalableLayer))
					if(pcScalableSEI->getFrmSizeInfoPresentFlag(uiNumScalableLayer)||pcScalableSEI->getIroiSliceDivisionInfoPresentFlag(uiNumScalableLayer))
					//SEI changes update }
          {
            UInt uiFrmWidthInMbsMinus1  = rcLayer.getFrameWidthInMbs () - 1;
            UInt uiFrmHeightInMbsMinus1 = rcLayer.getFrameHeightInMbs() - 1;
            pcScalableSEI->setFrmWidthInMbsMinus1(uiNumScalableLayer, uiFrmWidthInMbsMinus1);
            pcScalableSEI->setFrmHeightInMbsMinus1(uiNumScalableLayer, uiFrmHeightInMbsMinus1);
          }
          //SEI changes update
					//else
          //{//JVT-S036 lsj
          //  UInt  bFrmSizeInfoSrcLayerIdDelta = 0;  //may be changed

          //  pcScalableSEI->setFrmSizeInfoSrcLayerIdDelta(uiNumScalableLayer, bFrmSizeInfoSrcLayerIdDelta);
          //}

          if(pcScalableSEI->getSubRegionLayerFlag(uiNumScalableLayer))
          {
            UInt uiBaseRegionLayerId = 0;
            Bool bDynamicRectFlag = false;

            pcScalableSEI->setBaseRegionLayerId(uiNumScalableLayer, uiBaseRegionLayerId);
            pcScalableSEI->setDynamicRectFlag(uiNumScalableLayer, bDynamicRectFlag);
            if(pcScalableSEI->getDynamicRectFlag(uiNumScalableLayer))
            {
              UInt uiHorizontalOffset = 0;
              UInt uiVerticalOffset = 0;
              UInt uiRegionWidth = 0;
              UInt uiRegionHeight = 0;
              pcScalableSEI->setHorizontalOffset(uiNumScalableLayer, uiHorizontalOffset);
              pcScalableSEI->setVerticalOffset(uiNumScalableLayer, uiVerticalOffset);
              pcScalableSEI->setRegionWidth(uiNumScalableLayer, uiRegionWidth);
              pcScalableSEI->setRegionHeight(uiNumScalableLayer, uiRegionHeight);
            }
          }
          //SEI changes update
					//else
          //{//JVT-S036 lsj
          //  UInt  bSubRegionInfoSrcLayerIdDelta = 0; //may be changed

          //  pcScalableSEI->setSubRegionInfoSrcLayerIdDelta(uiNumScalableLayer, bSubRegionInfoSrcLayerIdDelta);
          //}

        //JVT-S036 lsj start
          if( pcScalableSEI->getSubPicLayerFlag( uiNumScalableLayer ) )
          {
            UInt RoiId = 1;//should be changed
            pcScalableSEI->setRoiId( uiNumScalableLayer, RoiId );
          }
          if( pcScalableSEI->getIroiSliceDivisionInfoPresentFlag( uiNumScalableLayer ) )
          {
            //SEI changes update {
						//pcScalableSEI->setIroiSliceDivisionType( uiNumScalableLayer, rcLayer.m_uiSliceDivisionType );
            /*if (rcLayer.m_uiSliceDivisionType == 0)
            {
              pcScalableSEI->setGridSliceWidthInMbsMinus1( uiNumScalableLayer, rcLayer.m_puiGridSliceWidthInMbsMinus1[0] );
              pcScalableSEI->setGridSliceHeightInMbsMinus1( uiNumScalableLayer, rcLayer.m_puiGridSliceHeightInMbsMinus1[0] );
            }
            else if (rcLayer.m_uiSliceDivisionType == 1)
            {
              pcScalableSEI->setNumSliceMinus1( uiNumScalableLayer, rcLayer.m_uiNumSliceMinus1 );
              for ( j = 0; j <= rcLayer.m_uiNumSliceMinus1; j++ )
              {
                pcScalableSEI->setFirstMbInSlice( uiNumScalableLayer, j, rcLayer.m_puiFirstMbInSlice[j] );
                pcScalableSEI->setSliceWidthInMbsMinus1( uiNumScalableLayer, j, rcLayer.m_puiGridSliceWidthInMbsMinus1[j] );
                pcScalableSEI->setSliceHeightInMbsMinus1( uiNumScalableLayer, j, rcLayer.m_puiGridSliceHeightInMbsMinus1[j] );
              }
            }
            else if (rcLayer.m_uiSliceDivisionType == 2)
            {
              pcScalableSEI->setNumSliceMinus1( uiNumScalableLayer, rcLayer.m_uiNumSliceMinus1 );
              UInt uiFrameHeightInMb = pcScalableSEI->getFrmHeightInMbsMinus1( uiNumScalableLayer ) + 1;
              UInt uiFrameWidthInMb  = pcScalableSEI->getFrmWidthInMbsMinus1(uiNumScalableLayer ) + 1;
              UInt uiPicSizeInMbs = uiFrameHeightInMb * uiFrameWidthInMb;
              for ( j = 0; j < uiPicSizeInMbs; j++)
              {
                pcScalableSEI->setSliceId( uiNumScalableLayer, j, rcLayer.m_puiSliceId[j] );
              }
            }
          }*/
						pcScalableSEI->setIroiGridFlag( uiNumScalableLayer, rcLayer.m_bGridFlag );
						if (rcLayer.m_bGridFlag)
            {
              pcScalableSEI->setGridSliceWidthInMbsMinus1( uiNumScalableLayer, rcLayer.m_puiGridSliceWidthInMbsMinus1[0] );
              pcScalableSEI->setGridSliceHeightInMbsMinus1( uiNumScalableLayer, rcLayer.m_puiGridSliceHeightInMbsMinus1[0] );
            }
            else
            {
              pcScalableSEI->setNumSliceMinus1( uiNumScalableLayer, rcLayer.m_uiNumSliceMinus1 );
              for ( j = 0; j <= rcLayer.m_uiNumSliceMinus1; j++ )
              {
                pcScalableSEI->setFirstMbInSlice( uiNumScalableLayer, j, rcLayer.m_puiFirstMbInSlice[j] );
                pcScalableSEI->setSliceWidthInMbsMinus1( uiNumScalableLayer, j, rcLayer.m_puiGridSliceWidthInMbsMinus1[j] );
                pcScalableSEI->setSliceHeightInMbsMinus1( uiNumScalableLayer, j, rcLayer.m_puiGridSliceHeightInMbsMinus1[j] );
              }
            }
          }
        //JVT-S036 lsj end
        //SEI changes update }
          if(pcScalableSEI->getLayerDependencyInfoPresentFlag(uiNumScalableLayer))
          {
// BUG_FIX liuhui{
          {
            UInt uiDelta;
            UInt uiQL = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getQualityLevelCGSSNR() + uiCurrFGSLevel : uiCurrFGSLevel);
            UInt uiL = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getLayerCGSSNR() : uiCurrLayer);
            if( uiQL ) // FGS layer, Q != 0
            {
              uiDelta = uiNumScalableLayer - getScalableLayerId( uiL, uiCurrTempLevel, uiQL-1 );
              assert ( uiDelta > 0 );
              pcScalableSEI->setDirectlyDependentLayerIdDeltaMinus1( uiNumScalableLayer, 0, uiDelta-1 ); //JVT-S036 lsj
              pcScalableSEI->setNumDirectlyDependentLayers(uiNumScalableLayer, 1 );
              if( uiCurrTempLevel- uiMinTempLevel ) // T != 0
              {
                uiDelta = uiNumScalableLayer - getScalableLayerId( uiL, uiCurrTempLevel-1, uiQL );
                assert ( uiDelta > 0 );
                pcScalableSEI->setDirectlyDependentLayerIdDeltaMinus1( uiNumScalableLayer, 1, uiDelta-1 ); //JVT-S036 lsj
                pcScalableSEI->setNumDirectlyDependentLayers(uiNumScalableLayer, 2 );
              }
            }
            else if( ( uiCurrTempLevel- uiMinTempLevel ) ) // Q = 0, T != 0
            {
              uiDelta = uiNumScalableLayer - getScalableLayerId( uiL, uiCurrTempLevel-1, uiQL );
              assert ( uiDelta > 0 );
              pcScalableSEI->setDirectlyDependentLayerIdDeltaMinus1( uiNumScalableLayer, 0, uiDelta-1 ); //JVT-S036 lsj
              pcScalableSEI->setNumDirectlyDependentLayers( uiNumScalableLayer, 1 );
              if( uiL ) // D != 0, T != 0, Q = 0
              {
                UInt uiBaseLayerCGSSNRId = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getBaseLayerCGSSNR       () : rcLayer.getBaseLayerId() );
                UInt uiBaseQualityLevel  = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getBaseQualityLevelCGSSNR() : 0 );
  //bugfix delete
                {
                  if( MSYS_UINT_MAX != getScalableLayerId( uiBaseLayerCGSSNRId, uiCurrTempLevel, uiBaseQualityLevel ) )
                  {
                    uiDelta = uiNumScalableLayer - getScalableLayerId( uiBaseLayerCGSSNRId, uiCurrTempLevel, uiBaseQualityLevel );
                    assert ( uiDelta > 0 );
                    pcScalableSEI->setDirectlyDependentLayerIdDeltaMinus1( uiNumScalableLayer, 1, uiDelta-1 ); //JVT-S036 lsj
                    pcScalableSEI->setNumDirectlyDependentLayers( uiNumScalableLayer, 2 );
                  }
                }
              }
            }
            else if ( uiL ) // D != 0,T = 0, Q = 0
            {
              UInt uiBaseLayerCGSSNRId = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getBaseLayerCGSSNR       () : rcLayer.getBaseLayerId() );
              UInt uiBaseQualityLevel  = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getBaseQualityLevelCGSSNR() : 0 );
//bugfix delete
              uiDelta = uiNumScalableLayer - getScalableLayerId( uiBaseLayerCGSSNRId, uiCurrTempLevel, uiBaseQualityLevel );
              assert ( uiDelta > 0 );
              pcScalableSEI->setDirectlyDependentLayerIdDeltaMinus1( uiNumScalableLayer, 0, uiDelta-1 ); //JVT-S036 lsj
              pcScalableSEI->setNumDirectlyDependentLayers( uiNumScalableLayer, 1 );
            }
            else // base layer, no dependency layers
            {
              pcScalableSEI->setNumDirectlyDependentLayers( uiNumScalableLayer, 0 );
            }
          }

// BUG_FIX liuhui}
          }
          else
          {//JVT-S036 lsj
            UInt uiLayerDependencyInfoSrcLayerIdDelta = 0; //may be changed

            pcScalableSEI->setLayerDependencyInfoSrcLayerIdDelta( uiNumScalableLayer, uiLayerDependencyInfoSrcLayerIdDelta);
          }

          //if(pcScalableSEI->getInitParameterSetsInfoPresentFlag(uiNumScalableLayer))//SEI changes update
          if(pcScalableSEI->getParameterSetsInfoPresentFlag(uiNumScalableLayer))//SEI changes update
          {
            UInt uiNumInitSPSMinus1  = 0;	//should be changed
						UInt uiNumInitSSPSMinus1 = 0;	//should be changed SEI changes update
            UInt uiNumInitPPSMinus1  = 0;	//should be changed
            pcScalableSEI->setNumInitSeqParameterSetMinus1(uiNumScalableLayer, uiNumInitSPSMinus1);
						pcScalableSEI->setNumInitSubsetSeqParameterSetMinus1(uiNumScalableLayer, uiNumInitSSPSMinus1);//SEI changes update
            pcScalableSEI->setNumInitPicParameterSetMinus1(uiNumScalableLayer, uiNumInitPPSMinus1);
            for( j = 0; j <= pcScalableSEI->getNumInitSPSMinus1(uiNumScalableLayer); j++)
            {
              UInt uiDelta = 0; //should be changed
              pcScalableSEI->setInitSeqParameterSetIdDelta( uiNumScalableLayer, j, uiDelta );
            }
						//SEI changes update {
            for( j = 0; j <= pcScalableSEI->getNumInitSSPSMinus1(uiNumScalableLayer); j++)
            {
              UInt uiDelta = 0; //should be changed
              pcScalableSEI->setInitSubsetSeqParameterSetIdDelta( uiNumScalableLayer, j, uiDelta );
            }
						//SEI changes update }
            for( j = 0; j <= pcScalableSEI->getNumInitPPSMinus1(uiNumScalableLayer); j++)
            {
              UInt uiDelta = 0; //should be changed
              pcScalableSEI->setInitPicParameterSetIdDelta( uiNumScalableLayer, j, uiDelta );
            }
          }
          else
          {//JVT-S036 lsj
            UInt bInitParameterSetsInfoSrcLayerIdDelta = 0;  //may be changed

            pcScalableSEI->setInitParameterSetsInfoSrcLayerIdDelta( uiNumScalableLayer, bInitParameterSetsInfoSrcLayerIdDelta );
          }

          //JVT-W051 {
          //if ( pcScalableSEI->getBitstreamRestrictionFlag ( uiNumScalableLayer ) )//SEI changes update
					if ( pcScalableSEI->getBitstreamRestrictionInfoPresentFlag ( uiNumScalableLayer ) )//SEI changes update
          {
            UInt uiMaxDecFrameBuffering = m_pcCodingParameter->getDPBSize();//should be changed
            UInt uiNumReorderFrames = pcScalableSEI->getMaxDecFrameBuffering( uiNumScalableLayer );//should be changed
            pcScalableSEI->setMaxDecFrameBuffering( uiNumScalableLayer, uiMaxDecFrameBuffering );
            pcScalableSEI->setNumReorderFrames( uiNumScalableLayer, uiNumReorderFrames );
            // JVT-W064 { value of the following restriction parameters are set to the maximum in this example.
            pcScalableSEI->setMotionVectorsOverPicBoundariesFlag( uiNumScalableLayer, true );
            pcScalableSEI->setMaxBytesPerPicDenom( uiNumScalableLayer, 0 );
            pcScalableSEI->setLog2MaxMvLengthHorizontal( uiNumScalableLayer, 16 );
            pcScalableSEI->setLog2MaxMvLengthVertical( uiNumScalableLayer, 16 );
            // JVT-W064 }
          }
          //JVT-W051 }
          uiNumScalableLayer++;
        }
      }
    }

  }
  //JVT-W051 {
  //SEI changes update {
	//pcScalableSEI->setQualityLayerInfoPresentFlag(true);
  //if ( pcScalableSEI->getQualityLayerInfoPresentFlag() )
  //{
  //  UInt uiQlNumdIdMinus1 = m_pcCodingParameter->getNumberOfLayers ()-1;
  //  pcScalableSEI->setQlNumdIdMinus1( uiQlNumdIdMinus1 );
  //  for ( i=0; i <= pcScalableSEI->getQlNumdIdMinus1(); i++ )
  //  {
  //    UInt uiQlDependencyId = i;//could be changed
  //    pcScalableSEI->setQlDependencyId(i, uiQlDependencyId);
  //    pcScalableSEI->setQlNumMinus1(i, 0);

  //    for (j=0; j <= pcScalableSEI->getQlNumMinus1(i); j++)
  //    {
  //      UInt uiQlId = j;//could be changed
  //      pcScalableSEI->setQlId(i,j,uiQlId);
  //      UInt uilayerProfileLevelIdc = 0;
  //      {
  //        UInt uilayerProfileIdc = m_uiProfileIdc[uiQlDependencyId];
  //        UInt uiLayerLevelIdc = m_uiLevelIdc[uiQlDependencyId];
  //        Bool bLayerConstraintSet0Flag = m_bConstraint0Flag[uiQlDependencyId];
  //        Bool bLayerConstraintSet1Flag = m_bConstraint1Flag[uiQlDependencyId];
  //        Bool bLayerConstraintSet2Flag = m_bConstraint2Flag[uiQlDependencyId];
  //        Bool bLayerConstraintSet3Flag = m_bConstraint3Flag[uiQlDependencyId];
  //        uilayerProfileLevelIdc += uilayerProfileIdc;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
  //        uilayerProfileLevelIdc += bLayerConstraintSet0Flag;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
  //        uilayerProfileLevelIdc += bLayerConstraintSet1Flag;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
  //        uilayerProfileLevelIdc += bLayerConstraintSet2Flag;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 1;
  //        uilayerProfileLevelIdc += bLayerConstraintSet3Flag;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 4;
  //        uilayerProfileLevelIdc = uilayerProfileLevelIdc << 8;
  //        uilayerProfileLevelIdc += uiLayerLevelIdc;
  //      }
  //      pcScalableSEI->setQlProfileLevelIdx(i,j,uilayerProfileLevelIdc);


  //      pcScalableSEI->setQlAvgBitrate(i,j,(UInt)(m_adAvgBitrate[uiQlDependencyId]+0.5));
  //      pcScalableSEI->setQlMaxBitrate(i,j,(UInt)(m_aadMaxBitrate[uiQlDependencyId][uiQlId]+0.5));
  //    }
  //  }
  //}
  //JVT-W051 }

  //note: this flag will be true after inserting Priority layer information SEI by QualityLevelAssigner
  pcScalableSEI->setPriorityLayerInfoPresentFlag(false);
  //SEI changes update }
  //JVT-W053
  const char *ucTemp = "http://svc.com";
  strcpy( pcScalableSEI->getPriorityIdSetUri(), ucTemp);
  //JVT-W053

  UInt              uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  cSEIMessageList.push_back                       ( pcScalableSEI );
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );
  RNOK( m_apcLayerEncoder[0]->addParameterSetBits ( uiBits+4*8 ) );

  return Err::m_nOK;

}



// JVT-S080 LMI {
ErrVal
H264AVCEncoder::xWriteScalableSEILayersNotPresent( ExtBinDataAccessor* pcExtBinDataAccessor, UInt uiNumLayers, UInt* uiLayerId)
{
  UInt i, uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent;
  RNOK(SEI::ScalableSeiLayersNotPresent::create(pcScalableSeiLayersNotPresent) );
  pcScalableSeiLayersNotPresent->setNumLayers( uiNumLayers );
  for (i=0; i < uiNumLayers; i++)
  pcScalableSeiLayersNotPresent->setLayerId( i, uiLayerId[i] );
  cSEIMessageList.push_back                       ( pcScalableSeiLayersNotPresent );
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );
  RNOK( m_apcLayerEncoder[0] ->addParameterSetBits ( uiBits+4*8 ) );

  return Err::m_nOK;
}

ErrVal
H264AVCEncoder::xWriteScalableSEIDependencyChange( ExtBinDataAccessor* pcExtBinDataAccessor, UInt uiNumLayers, UInt* uiLayerId, Bool* pbLayerDependencyInfoPresentFlag,
                          UInt* uiNumDirectDependentLayers, UInt** puiDirectDependentLayerIdDeltaMinus1, UInt* puiLayerDependencyInfoSrcLayerIdDeltaMinus1)
{
  UInt uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  SEI::ScalableSeiDependencyChange* pcScalableSeiDependencyChange;
  RNOK(SEI::ScalableSeiDependencyChange::create(pcScalableSeiDependencyChange) );
  pcScalableSeiDependencyChange->setNumLayersMinus1(uiNumLayers-1);
    UInt uiLayer, uiDirectLayer;

  for( uiLayer = 0; uiLayer < uiNumLayers; uiLayer++ )
  {
    pcScalableSeiDependencyChange->setDependencyId( uiLayer, uiLayerId[uiLayer]);
    pcScalableSeiDependencyChange->setLayerDependencyInfoPresentFlag( uiLayer, pbLayerDependencyInfoPresentFlag[uiLayer] );
    if ( pcScalableSeiDependencyChange->getLayerDependencyInfoPresentFlag( uiLayer ) )
    {
          pcScalableSeiDependencyChange->setNumDirectDependentLayers( uiLayer, uiNumDirectDependentLayers[uiLayer] );
      for ( uiDirectLayer = 0; uiDirectLayer < pcScalableSeiDependencyChange->getNumDirectDependentLayers( uiLayer ); uiDirectLayer++)
              pcScalableSeiDependencyChange->setDirectDependentLayerIdDeltaMinus1( uiLayer, uiDirectLayer,  puiDirectDependentLayerIdDeltaMinus1[uiLayer][uiDirectLayer] );
    }
    else
            pcScalableSeiDependencyChange->setLayerDependencyInfoSrcLayerIdDeltaMinus1( uiLayer, puiLayerDependencyInfoSrcLayerIdDeltaMinus1[uiLayer] );
  }


  cSEIMessageList.push_back                       ( pcScalableSeiDependencyChange );
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );
  RNOK( m_apcLayerEncoder[0] ->addParameterSetBits ( uiBits+4*8 ) );

  return Err::m_nOK;
}
//  JVT-S080 LMI }

ErrVal
H264AVCEncoder::xWriteSubPicSEI ( ExtBinDataAccessor* pcExtBinDataAccessor )
{
  SEI::SubPicSei* pcSubPicSEI;
  RNOK( SEI::SubPicSei::create( pcSubPicSEI ) );

  //===== set message =====
  UInt uiScalableLayerId = 0;	//should be changed
  pcSubPicSEI->setDependencyId( uiScalableLayerId );

  //===== write message =====
  UInt              uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  cSEIMessageList.push_back                       ( pcSubPicSEI );
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::xWriteSubPicSEI ( ExtBinDataAccessor* pcExtBinDataAccessor, UInt layer_id )
{
  SEI::SubPicSei* pcSubPicSEI;
  RNOK( SEI::SubPicSei::create( pcSubPicSEI ) );

  //===== set message =====
  pcSubPicSEI->setDependencyId( layer_id );

  //===== write message =====
  UInt              uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  cSEIMessageList.push_back                       ( pcSubPicSEI );
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );

  return Err::m_nOK;
}

// Scalable SEI for ROI ICU/ETRI
ErrVal
H264AVCEncoder::xWriteMotionSEI( ExtBinDataAccessor* pcExtBinDataAccessor, UInt sg_id )
{
  //===== create message =====
  SEI::MotionSEI* pcMotionSEI;
  RNOK( SEI::MotionSEI::create( pcMotionSEI) );

  pcMotionSEI->setSliceGroupId(sg_id);


  //===== write message =====
  UInt              uiBits = 0;
  SEI::MessageList  cSEIMessageList;
  cSEIMessageList.push_back                       ( pcMotionSEI);
  RNOK( m_pcNalUnitEncoder  ->initNalUnit         ( pcExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder  ->write               ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder  ->closeNalUnit        ( uiBits ) );

  return Err::m_nOK;
}

ErrVal
H264AVCEncoder::writeParameterSets( ExtBinDataAccessor* pcExtBinDataAccessor,
                                    SequenceParameterSet*& rpcAVCSPS,
                                    Bool &rbMoreSets )
{
  if( m_bVeryFirstCall )
  {
    m_bVeryFirstCall = false;

    if( ! m_bScalableSeiMessage )
    {
      RNOK( xInitParameterSets() );
    }
    else
    {
      RNOK( xWriteScalableSEI( pcExtBinDataAccessor ) );
    }

    LayerParameters& rcLayer = m_pcCodingParameter->getLayerParameters ( 0 );
    if (0 < rcLayer.getNumROI())
      m_bWrteROISEI = true;
    else
      m_bWrteROISEI = false;
    m_loop_roi_sei=0;

    return Err::m_nOK;
  }
  else
    m_bScalableSeiMessage = true;

  UInt uiNumLayer = m_pcCodingParameter->getNumberOfLayers();

  if(m_bWrteROISEI)
  {
  LayerParameters& rcLayer = m_pcCodingParameter->getLayerParameters ( m_loop_roi_sei/2 );
  {
    if(((m_loop_roi_sei+1)/2) >= uiNumLayer )
    {
    m_bWrteROISEI = false;
    }

    if(m_loop_roi_sei%2)
    {
    RNOK( xWriteMotionSEI( pcExtBinDataAccessor,rcLayer.getSGID()[0] ) );    m_loop_roi_sei++; return Err::m_nOK;
    }
    else
    {
      RNOK( xWriteSubPicSEI( pcExtBinDataAccessor, rcLayer.getSLID()[0] ) );    m_loop_roi_sei++; return Err::m_nOK;
    }
    }
  }

  UInt uiBits;

  if( ! m_cUnWrittenSPS.empty() )
  {
    RNOK( m_pcNalUnitEncoder->initNalUnit( pcExtBinDataAccessor ) );
    SequenceParameterSet& rcSPS = *m_cUnWrittenSPS.front();

    if( rcSPS.getMbAdaptiveFrameFieldFlag() )
    {
      rcSPS.setFrameMbsOnlyFlag( false );
    }

    RNOK( m_pcNalUnitEncoder->write( rcSPS ) );
    RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );

    if( m_pcCodingParameter->getNumberOfLayers() )
    {
      UInt  uiLayer = rcSPS.getDependencyId();
      UInt  uiSize  = pcExtBinDataAccessor->size();
      RNOK( m_apcLayerEncoder[uiLayer]->addParameterSetBits( 8*(uiSize+4) ) );
    }

// JVT-V068 {
    if ( (m_pcCodingParameter->getEnableNalHRD() || m_pcCodingParameter->getEnableVclHRD()) &&
         rcSPS.getProfileIdc() != SCALABLE_BASELINE_PROFILE &&
         rcSPS.getProfileIdc() != SCALABLE_HIGH_PROFILE )
    {
      rpcAVCSPS = &rcSPS;
    }
// JVT-V068 }

    m_cUnWrittenSPS.pop_front();
  }
  else
  {
    if( ! m_cUnWrittenPPS.empty() )
    {
      RNOK( m_pcNalUnitEncoder->initNalUnit( pcExtBinDataAccessor ) );
      PictureParameterSet& rcPPS = *m_cUnWrittenPPS.front();
      RNOK( m_pcNalUnitEncoder->write( rcPPS ) )
      RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );

      if( m_pcCodingParameter->getNumberOfLayers() )
      {
        UInt  uiSPSId = rcPPS.getSeqParameterSetId();
        SequenceParameterSet* pcSPS;
        RNOK( m_pcParameterSetMng->get( pcSPS, uiSPSId, rcPPS.referencesSubsetSPS() ) );

        UInt  uiLayer = pcSPS->getDependencyId();
        UInt  uiSize  = pcExtBinDataAccessor->size();
        RNOK( m_apcLayerEncoder[uiLayer]->addParameterSetBits( 8*(uiSize+4) ) );
      }

      m_cUnWrittenPPS.pop_front();
    }
    else
    {
      AF();
      rbMoreSets = false;
      return Err::m_nERR;
    }
  }

  rbMoreSets = !(m_cUnWrittenSPS.empty() && m_cUnWrittenPPS.empty());
  return Err::m_nOK;
}



ErrVal
H264AVCEncoder::process( ExtBinDataAccessorList&  rcExtBinDataAccessorList,
                         PicBuffer*               apcOriginalPicBuffer    [MAX_LAYERS],
                         PicBuffer*               apcReconstructPicBuffer [MAX_LAYERS],
                         PicBufferList*           apcPicBufferOutputList,
                         PicBufferList*           apcPicBufferUnusedList )
{
  UInt  uiHighestLayer  = m_pcCodingParameter->getNumberOfLayers() - 1;
  UInt  uiTargetSize    = ( 1 << m_pcCodingParameter->getLayerParameters(uiHighestLayer).getDecompositionStages() ) + ( m_apcLayerEncoder[uiHighestLayer]->firstGOPCoded() ? 0 : 1 );

  //===== fill lists =====
  for( UInt uiLayer = 0; uiLayer <= uiHighestLayer; uiLayer++ )
  {
    if( apcOriginalPicBuffer[ uiLayer ] )
    {
      ROF( apcReconstructPicBuffer[ uiLayer ] );
      m_acOrgPicBufferList  [ uiLayer ].push_back( apcOriginalPicBuffer   [ uiLayer ] );
      m_acRecPicBufferList  [ uiLayer ].push_back( apcReconstructPicBuffer[ uiLayer ] );
    }
    else if( apcReconstructPicBuffer[ uiLayer ] )
    {
      apcPicBufferUnusedList[ uiLayer ].push_back( apcReconstructPicBuffer[ uiLayer ] );
    }
  }

  //===== encoding of GOP =====
  ROT( m_acOrgPicBufferList[uiHighestLayer].size() >  uiTargetSize );
  if ( m_acOrgPicBufferList[uiHighestLayer].size() == uiTargetSize )
  {
    RNOK( xProcessGOP( apcPicBufferOutputList, apcPicBufferUnusedList ) );
  }

  //===== update data accessor list =====
  m_cAccessUnitDataList.emptyNALULists( rcExtBinDataAccessorList );

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::finish( ExtBinDataAccessorList&  rcExtBinDataAccessorList,
                        PicBufferList*           apcPicBufferOutputList,
                        PicBufferList*           apcPicBufferUnusedList,
                        UInt&                    ruiNumCodedFrames,
                        Double&                  rdHighestLayerOutputRate )
{
  //===== encode GOP =====
  RNOK( xProcessGOP( apcPicBufferOutputList, apcPicBufferUnusedList ) );

  //===== update data accessor list =====
  m_cAccessUnitDataList.emptyNALULists( rcExtBinDataAccessorList );

  //===== finish encoding =====
  if( m_pcCodingParameter->getCGSSNRRefinement() == 1 )
  {
    for( UInt uiLayer = 1; uiLayer <= m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
    {
      if( uiLayer == m_pcCodingParameter->getNumberOfLayers() || m_apcLayerEncoder[uiLayer]->getQualityLevelCGSSNR() == 0 ) //last layer is highest quality layer
      {
        RNOK( m_apcLayerEncoder[uiLayer-1]->finish( ruiNumCodedFrames, rdHighestLayerOutputRate, m_aaadFinalFramerate, m_aaadSeqBits ) );
      }
    }
  }
  else
  {
    for( UInt uiLayer = 0; uiLayer < m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
    {
      RNOK( m_apcLayerEncoder[uiLayer]->finish( ruiNumCodedFrames, rdHighestLayerOutputRate, m_aaadFinalFramerate, m_aaadSeqBits ) );
    }
  }
  printf("\n");

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::xProcessGOP( PicBufferList* apcPicBufferOutputList,
                             PicBufferList* apcPicBufferUnusedList )
{
  UInt uiAUIndex, uiLayer;

  //===== init GOP =====
  for( uiLayer = 0; uiLayer <  m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
  {
    m_apcLayerEncoder[uiLayer]->setLayerCGSSNR           ( m_pcCodingParameter->getLayerParameters( uiLayer ).getLayerCGSSNR           () );
    m_apcLayerEncoder[uiLayer]->setQualityLevelCGSSNR    ( m_pcCodingParameter->getLayerParameters( uiLayer ).getQualityLevelCGSSNR    () );
    m_apcLayerEncoder[uiLayer]->setBaseLayerCGSSNR       ( m_pcCodingParameter->getLayerParameters( uiLayer ).getBaseLayerCGSSNR       () );
    m_apcLayerEncoder[uiLayer]->setBaseQualityLevelCGSSNR( m_pcCodingParameter->getLayerParameters( uiLayer ).getBaseQualityLevelCGSSNR() );

    RNOK( m_apcLayerEncoder[uiLayer]->initGOP( m_cAccessUnitDataList.getAccessUnitData( MSYS_UINT_MAX ), m_acOrgPicBufferList[uiLayer] ) );
  }

  //JVT-W052
  if(m_pcCodingParameter->getIntegrityCheckSEIEnable() && m_pcCodingParameter->getCGSSNRRefinement())
  {
    for( uiLayer = 0; uiLayer < m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
    {
      m_uiNumofCGS[m_apcLayerEncoder[uiLayer]->getLayerCGSSNR()] = m_pcCodingParameter->getLayerParameters( uiLayer ).getQualityLevelCGSSNR() + m_pcCodingParameter->getLayerParameters( uiLayer ).getNumberOfQualityLevelsCGSSNR() - 1;
      m_uicrcVal  [m_apcLayerEncoder[uiLayer]->getLayerCGSSNR()] = 0xffff;
    }
  }
  //JVT-W052

  //===== loop over access units in GOP, and layers inside access units =====
  for( uiAUIndex = 0; uiAUIndex <= 64; uiAUIndex++ )
  {																																									//JVT-W052
    if(m_pcCodingParameter->getIntegrityCheckSEIEnable() && m_pcCodingParameter->getCGSSNRRefinement())//JVT-W052
    {
      RNOK( SEI::IntegrityCheckSEI::create( m_pcIntegrityCheckSEI ) );							//JVT-W052
    }
    else
    {
      m_pcIntegrityCheckSEI = 0;
    }
    for( uiLayer   = 0; uiLayer   <  m_pcCodingParameter->getNumberOfLayers(); uiLayer  ++ )
    {
      if( m_pcCodingParameter->getNonRequiredEnable() )
      {
        if( uiLayer == m_pcCodingParameter->getNumberOfLayers() - 1 )   m_apcLayerEncoder[ uiLayer ]->setNonRequiredWrite( 2 );
        else                                                            m_apcLayerEncoder[ uiLayer ]->setNonRequiredWrite( 1 );
      }
      m_bIsFirstGOP = !m_apcLayerEncoder[uiLayer]->firstGOPCoded();
      RNOK( m_apcLayerEncoder[uiLayer]->process( uiAUIndex,
                                                m_cAccessUnitDataList.getAccessUnitData( uiAUIndex ),
                                                m_acOrgPicBufferList  [uiLayer],
                                                m_acRecPicBufferList  [uiLayer],
                                                apcPicBufferUnusedList[uiLayer],
                                                m_pcParameterSetMng ) );
      //JVT-W051 {
      if ( m_bIsFirstGOP )
      {
        m_uiProfileIdc[uiLayer] = m_apcLayerEncoder[uiLayer]->m_uiProfileIdc;
        m_uiLevelIdc[uiLayer] = m_apcLayerEncoder[uiLayer]->m_uiLevelIdc;
        m_bConstraint0Flag[uiLayer] = m_apcLayerEncoder[uiLayer]->m_bConstraint0Flag;
        m_bConstraint1Flag[uiLayer] = m_apcLayerEncoder[uiLayer]->m_bConstraint1Flag;
        m_bConstraint2Flag[uiLayer] = m_apcLayerEncoder[uiLayer]->m_bConstraint2Flag;
        m_bConstraint3Flag[uiLayer] = m_apcLayerEncoder[uiLayer]->m_bConstraint3Flag;
      }
      //JVT-W051 }
    }
    if( m_pcIntegrityCheckSEI )
    {
      delete m_pcIntegrityCheckSEI;
      m_pcIntegrityCheckSEI = 0;
    }
  }//JVT-W052

  //===== update pic buffer lists =====
  for( uiLayer = 0; uiLayer < m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
  {
    //----- set output list -----
    apcPicBufferOutputList[ uiLayer ] += m_acRecPicBufferList[ uiLayer ];
    //----- update unused list -----
    apcPicBufferUnusedList[ uiLayer ] += m_acOrgPicBufferList[ uiLayer ];
    apcPicBufferUnusedList[ uiLayer ] += m_acRecPicBufferList[ uiLayer ];
    //----- reset lists -----
    m_acOrgPicBufferList  [ uiLayer ].clear();
    m_acRecPicBufferList  [ uiLayer ].clear();
  }

  return Err::m_nOK;
}


ErrVal
H264AVCEncoder::xInitParameterSets()
{
  Bool bOutput    = true;
  UInt uiSPSId    = 0;
  UInt uiPPSId    = 0;
  UInt uiAllocMbX = 0;
  UInt uiAllocMbY = 0;
  UInt uiIndex;

  //===== determine values for Poc calculation =====
  UInt uiRequiredPocBits = gMax( 4, 1 + (Int)ceil( log10( 1.0 + ( 1 << m_pcCodingParameter->getDecompositionStages() ) ) / log10( 2.0 ) ) );

  //===== loop over layers =====
  for( uiIndex = 0; uiIndex < m_pcCodingParameter->getNumberOfLayers(); uiIndex++ )
  {
    //===== get configuration parameters =====
    LayerParameters&  rcLayerParameters   = m_pcCodingParameter->getLayerParameters( uiIndex );
    UInt              uiMbY               = rcLayerParameters.getFrameHeightInMbs();
    UInt              uiMbX               = rcLayerParameters.getFrameWidthInMbs ();
    UInt              uiCropLeft          = 0;
    UInt              uiCropTop           = 0;
    UInt              uiCropRight         = rcLayerParameters.getHorPadding() / 2;                                            // chroma_format_idc is always equal to 1
    UInt              uiCropBottom        = rcLayerParameters.getVerPadding() / ( rcLayerParameters.isInterlaced() ? 4 : 2 ); // chroma_format_idc is always equal to 1
    UInt              uiOutFreq           = (UInt)ceil( rcLayerParameters.getOutputFrameRate() );
    UInt              uiMvRange           = m_pcCodingParameter->getMotionVectorSearchParams().getSearchRange();
    UInt              uiKeyPicMode        = m_pcCodingParameter->getEncodeKeyPictures ();
    Bool              bMGS                = ( m_pcCodingParameter->getCGSSNRRefinement() != 0 );
    Bool              bKeyPictures        = ( uiKeyPicMode == 2 || ( bMGS && uiKeyPicMode == 1 ) );
    UInt              uiPaffOffset        = ( rcLayerParameters.getPAff() ? 1 : 0 );
    UInt              uiRefBasePicOffset  = ( bKeyPictures ? 1 + uiPaffOffset : 0 );
    UInt              uiDecStages         = rcLayerParameters.getDecompositionStages() - rcLayerParameters.getNotCodedStages();
    UInt              uiVirtDecStages     = gMax( 1, uiDecStages + uiPaffOffset );
    UInt              uiNumRefPic         = ( ( 1 << uiVirtDecStages ) >> 1 ) + uiVirtDecStages + uiRefBasePicOffset;
    UInt              uiDPBSize           = uiNumRefPic;
    uiAllocMbX                            = gMax( uiAllocMbX, uiMbX );
    uiAllocMbY                            = gMax( uiAllocMbY, uiMbY );
    if( rcLayerParameters.getUseLongTerm() )
    {
      ROF( rcLayerParameters.getMMCOEnable() );
      ROT( rcLayerParameters.isIntraOnly  () );
      uiNumRefPic = uiDecStages + 1 + uiPaffOffset + ( bKeyPictures ? 2 : 0 );
      uiDPBSize   = uiNumRefPic;
    }
    else if( !rcLayerParameters.getMMCOEnable() )
    {
      uiNumRefPic = ( 1 << uiVirtDecStages ) + uiRefBasePicOffset;
      uiDPBSize   = uiNumRefPic;
    }
    if( rcLayerParameters.isIntraOnly() )
    {
      uiNumRefPic = 0;
      uiDPBSize   = 1;
    }
    if( uiNumRefPic > 16 || uiDPBSize > 16 )
    {
      fprintf( stderr, "\n\nSpecified coding structure cannot be supported (DPB size exceeded)\n" );
      if( ! rcLayerParameters.getMMCOEnable() )
      {
        fprintf( stderr, "\t-> try to enable MMCO commands\n" );
      }
      else if( ! rcLayerParameters.getUseLongTerm() )
      {
        fprintf( stderr, "\t-> try to use long-term pictures\n" );
      }
      ROT(1);
    }

    //----- determine reference layer macroblocks (that are counted for level constraints) -----
    UInt uiRefLayerMbs = 0;
    { //>>>>> subclause G.10.2.1 >>>>>
      LayerParameters&  rcCurrLayer   = m_pcCodingParameter->getLayerParameters( uiIndex );
      UInt              uiLayerCount  = 0;
      UInt              uiBaseDId     = rcCurrLayer.getBaseLayerCGSSNR();             // for lowest MGS fragment
      UInt              uiBaseQId     = rcCurrLayer.getBaseQualityLevelCGSSNR();      // for lowest MGS fragment
      UInt              uiNumMGSFrag  = rcCurrLayer.getNumberOfQualityLevelsCGSSNR(); // number of MGS fragments in "layer"
      while( (--uiNumMGSFrag) > 0 ) // count MGS fragments (set level_idc for highest MGS fragment)
      {
        if( (++uiLayerCount) > 1 )
        {
          uiRefLayerMbs += rcCurrLayer.getFrameHeightInMbs() * rcCurrLayer.getFrameWidthInMbs();
        }
      }
      for( UInt uiRefLayerId = uiIndex - 1; uiBaseDId != MSYS_UINT_MAX; uiRefLayerId-- )
      {
        LayerParameters&  rcRefLayer    = m_pcCodingParameter->getLayerParameters( uiRefLayerId );
        UInt              uiRefDId      = rcRefLayer.getLayerCGSSNR();
        UInt              uiRefQId      = rcRefLayer.getQualityLevelCGSSNR();
        UInt              uiRefMGSFrag  = rcRefLayer.getNumberOfQualityLevelsCGSSNR();
        if( uiRefDId == uiBaseDId && uiRefQId <= uiBaseQId && uiRefQId + uiRefMGSFrag > uiBaseQId )
        {
          uiNumMGSFrag = uiBaseQId - uiRefQId + 2;
          while( (--uiNumMGSFrag) > 0 )
          {
            if( (++uiLayerCount) > 1 )
            {
              uiRefLayerMbs += rcRefLayer.getFrameHeightInMbs() * rcRefLayer.getFrameWidthInMbs();
            }
          }
          uiBaseDId = rcRefLayer.getBaseLayerCGSSNR();
          uiBaseQId = rcRefLayer.getBaseQualityLevelCGSSNR();
        }
      }
    } //<<<<< subclause G.10.2.1 <<<<<
    //----- get level idc -----
    UInt uiLevelIdc = SequenceParameterSet::getLevelIdc( uiMbY, uiMbX, uiOutFreq, uiMvRange, uiDPBSize, uiRefLayerMbs );
    RNOK( rcLayerParameters.updateWithLevel( m_pcCodingParameter, uiLevelIdc ) );
    if  ( uiLevelIdc == MSYS_UINT_MAX )
    {
      fprintf( stderr, "\n\nNo level found for specified configuration parameters\n" );
      ROT(1);
    }

    //===== create parameter sets, set Id's, and store =====
    SequenceParameterSet* pcSPS;
    PictureParameterSet*  pcPPS;

    RNOK( SequenceParameterSet::create( pcSPS ) );
    RNOK( PictureParameterSet ::create( pcPPS ) );
    pcPPS->setPicParameterSetId( uiPPSId++ );
    pcPPS->setSeqParameterSetId( uiSPSId   );
    pcSPS->setSeqParameterSetId( uiSPSId );
    if( uiIndex )
    {
      uiSPSId++;
      pcSPS->setNalUnitType( NAL_UNIT_SUBSET_SPS );
    }
    else
    {
      pcSPS->setNalUnitType( NAL_UNIT_SPS );
    }
    RNOK( m_pcParameterSetMng->store( pcPPS ) );
    RNOK( m_pcParameterSetMng->store( pcSPS ) );
    UInt  uiDId   = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayerParameters.getLayerCGSSNR()        : uiIndex );
    UInt  uiQId   = ( m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayerParameters.getQualityLevelCGSSNR() : 0 );
    UInt  uiDQId  = ( uiDId << 4 ) + uiQId;
    UInt  uiNumQ  = rcLayerParameters.getNumberOfQualityLevelsCGSSNR();
    for(  UInt ui = 0; ui < uiNumQ; ui++ )
    {
      m_pcParameterSetMng->setActiveSPS( pcSPS->getSeqParameterSetId(), uiDQId + ui );
    }

    //===== set sequence parameter set parameters =====
    pcSPS->setAVCHeaderRewriteFlag                ( true );
    pcSPS->setDependencyId                        ( rcLayerParameters.getDependencyId() );
    pcSPS->setProfileIdc                          ( (Profile)rcLayerParameters.getProfileIdc() );
    pcSPS->setConstrainedSet0Flag                 ( rcLayerParameters.getConstrainedSet0Flag() );
    pcSPS->setConstrainedSet1Flag                 ( rcLayerParameters.getConstrainedSet1Flag() );
    pcSPS->setConstrainedSet2Flag                 ( rcLayerParameters.getConstrainedSet2Flag() );
    pcSPS->setConstrainedSet3Flag                 ( rcLayerParameters.getConstrainedSet3Flag() );
    pcSPS->setConvertedLevelIdc                   ( uiLevelIdc );
    pcSPS->setSeqScalingMatrixPresentFlag         ( false );
    pcSPS->setLog2MaxFrameNum                     ( MAX_FRAME_NUM_LOG2 );
    pcSPS->setLog2MaxPicOrderCntLsb               ( gMin( 15, uiRequiredPocBits + 2 ) );  // HS: decoder robustness -> value increased by 2
    pcSPS->setNumRefFrames                        ( uiNumRefPic );
    pcSPS->setGapsInFrameNumValueAllowedFlag      ( true );
    pcSPS->setFrameWidthInMbs                     ( uiMbX );
    pcSPS->setFrameHeightInMbs                    ( uiMbY );
    pcSPS->setDirect8x8InferenceFlag              ( true );
    pcSPS->setFrameCropLeftOffset                 ( uiCropLeft );
    pcSPS->setFrameCropRightOffset                ( uiCropRight );
    pcSPS->setFrameCropTopOffset                  ( uiCropTop );
    pcSPS->setFrameCropBottomOffset               ( uiCropBottom );

    if( uiIndex == 0 )
    {  // base layer AVC rewrite flag should always be false
      pcSPS->setAVCRewriteFlag(false);
      pcSPS->setAVCAdaptiveRewriteFlag(false);
    }
    else
    {
      Bool  bAVCRewrite       = rcLayerParameters.getTCoeffLevelPredictionFlag();
      Bool  bAdaptiveRewrite  = rcLayerParameters.getAVCAdaptiveRewriteFlag   ();
      if( bAVCRewrite )
      {
        for( UInt uiMGSEL = uiIndex + 1; uiMGSEL < m_pcCodingParameter->getNumberOfLayers() && !bAdaptiveRewrite; uiMGSEL++ )
        {
          if( m_pcCodingParameter->getLayerParameters( uiMGSEL ).getQualityLevelCGSSNR() == 0 )
          {
            break;
          }
          if( ! m_pcCodingParameter->getLayerParameters( uiMGSEL ).getTCoeffLevelPredictionFlag() )
          {
            bAdaptiveRewrite = true;
          }
        }
      }
      pcSPS->setAVCRewriteFlag          ( bAVCRewrite );
      pcSPS->setAVCAdaptiveRewriteFlag  ( bAdaptiveRewrite );
    }

    pcSPS->setFrameMbsOnlyFlag										( ! rcLayerParameters.isInterlaced() );
    pcSPS->setMbAdaptiveFrameFieldFlag            ( (rcLayerParameters.getMbAff() ? true : false ) );

    if( pcSPS->getMbAdaptiveFrameFieldFlag() && uiMbY % 2)
    {
      printf(" MBAFF ignored ");
      pcSPS->setMbAdaptiveFrameFieldFlag( false ); //not allowed
    }

    pcSPS->setResizeParameters                    ( rcLayerParameters.getResizeParameters() );

    for(  UInt ui = 0; ui < 16; ui++ )
    {
      pcSPS->setMGSVect( ui, rcLayerParameters.getMGSVect( ui ) );
    }

    // JVT-V068 HRD {
    pcSPS->setVUI( pcSPS );
    xInitLayerInfoForHrd( pcSPS, uiIndex );
    // JVT-V068 HRD }
    //SSPS {
		if( uiIndex ) // sub-SPS
		{
      pcSPS->setAVCHeaderRewriteFlag( false );
			pcSPS->setSVCVUIParametersPresentFlag( ( m_pcCodingParameter->getEnableVUI()!=0 ) );
			pcSPS->setAdditionalExtension2Flag( false);
      pcSPS->setInterlayerDeblockingPresent( ! m_pcCodingParameter->getInterLayerLoopFilterParams().isDefault() );
		}
		else
		{
			pcSPS->setSVCVUIParametersPresentFlag( false );
			pcSPS->setAdditionalExtension2Flag( false);
		}
	  //SSPS }
    if( rcLayerParameters.getQualityLevelCGSSNR() ) // MGS enhancement
    {
      UInt                  uiCurrDId   = rcLayerParameters.getLayerCGSSNR        ();
      UInt                  uiCurrQId   = rcLayerParameters.getQualityLevelCGSSNR ();
      UInt                  uiCurrDQId  = ( uiCurrDId << 4 ) + uiCurrQId;
      SequenceParameterSet* pcQ0SPS = 0;
      RNOK( m_pcParameterSetMng->getActiveSPS   (  pcQ0SPS, uiCurrDQId - 1 ) );
      RNOK( pcSPS->copySPSDataForMGSEnhancement ( *pcQ0SPS, uiCurrQId ) ); //zhangxd_20101220
    }


    //===== set picture parameter set parameters =====
    pcPPS->setReferencesSubsetSPS                   ( uiIndex > 0 );
    pcPPS->setNalUnitType                           ( NAL_UNIT_PPS );
    pcPPS->setDependencyId                          ( rcLayerParameters.getDependencyId() );
    pcPPS->setEntropyCodingModeFlag                 ( rcLayerParameters.getEntropyCodingModeFlag() );
    pcPPS->setPicOrderPresentFlag                   ( true );
    pcPPS->setNumRefIdxActive                       ( LIST_0, m_pcCodingParameter->getNumRefFrames() );
    pcPPS->setNumRefIdxActive                       ( LIST_1, m_pcCodingParameter->getNumRefFrames() );
    // heiko.schwarz@hhi.fhg.de: ensures that the PPS QP will be in the valid range (specified QP can be outside that range to force smaller/higher lambdas)
    //pcPPSHP->setPicInitQp                             ( (Int)rcLayerParameters.getBaseQpResidual() );
    pcPPS->setPicInitQp                             ( gMin( 51, gMax( 0, (Int)rcLayerParameters.getBaseQpResidual() ) ) );
    // JVT-W043 {
    if ( bRateControlEnable && !pcJSVMParams->m_uiLayerId )
    {
      pcPPS->setPicInitQp                           ( gMin( 51, gMax( 0, (Int)pcGenericRC->m_pcJSVMParams->SetInitialQP ) ) );
    }
    // JVT-W043 }
    pcPPS->setChromaQpIndexOffset                   ( rcLayerParameters.getChromaQPIndexOffset() );
    pcPPS->setDeblockingFilterParametersPresentFlag ( ! m_pcCodingParameter->getLoopFilterParams().isDefault() );
    pcPPS->setConstrainedIntraPredFlag              ( rcLayerParameters.getConstrainedIntraPred() > 0 );
    pcPPS->setRedundantPicCntPresentFlag            ( rcLayerParameters.getUseRedundantSliceFlag() ); // JVT-Q054 Red. Picture
    //JVT-W049 {
    pcPPS->setRedundantKeyPicCntPresentFlag         ( rcLayerParameters.getUseRedundantKeySliceFlag() );
    if( m_pcCodingParameter->getEnableRedundantKeyPic() == true )
    {
      pcPPS->setEnableRedundantKeyPicCntPresentFlag ( true );
      if( rcLayerParameters.getDependencyId() != 0 )
      {
        pcPPS->setRedundantPicCntPresentFlag        ( true );
        pcPPS->setRedundantKeyPicCntPresentFlag     ( true );
      }
    }
    //JVT-W049 }
    pcPPS->setTransform8x8ModeFlag                  ( rcLayerParameters.getEnable8x8Trafo() > 0 );
    pcPPS->setPicScalingMatrixPresentFlag           ( rcLayerParameters.getScalingMatricesPresent() > 0 );
    if( pcPPS->getPicScalingMatrixPresentFlag() )
    {
      pcPPS->getPicScalingMatrix().init             ( rcLayerParameters.getScalMatrixBuffer() );
    }
    pcPPS->set2ndChromaQpIndexOffset                ( rcLayerParameters.get2ndChromaQPIndexOffset() );

    pcPPS->setWeightedPredFlag                      ( WEIGHTED_PRED_FLAG );
    pcPPS->setWeightedBiPredIdc                     ( WEIGHTED_BIPRED_IDC );
//TMM_WP
    pcPPS->setWeightedPredFlag                      ( m_pcCodingParameter->getIPMode() != 0 );
    pcPPS->setWeightedBiPredIdc                     ( m_pcCodingParameter->getBMode() );
//TMM_WP

    //--ICU/ETRI FMO Implementation : FMO stuff start
    pcPPS->setNumSliceGroupsMinus1                  ( rcLayerParameters.getNumSliceGroupsMinus1() );
    pcPPS->setSliceGroupMapType                     ( rcLayerParameters.getSliceGroupMapType() );
    pcPPS->setArrayRunLengthMinus1					        ( rcLayerParameters.getArrayRunLengthMinus1() );
    pcPPS->setArrayTopLeft								          ( rcLayerParameters.getArrayTopLeft() );
    pcPPS->setArrayBottomRight							        ( rcLayerParameters.getArrayBottomRight() );
    pcPPS->setSliceGroupChangeDirection_flag		    ( rcLayerParameters.getSliceGroupChangeDirection_flag() );
    pcPPS->setSliceGroupChangeRateMinus1			      ( rcLayerParameters.getSliceGroupChangeRateMinus1() );
    pcPPS->setNumSliceGroupMapUnitsMinus1			      ( rcLayerParameters.getNumSliceGroupMapUnitsMinus1() );
    pcPPS->setArraySliceGroupId						          ( rcLayerParameters.getArraySliceGroupId() );
    //--ICU/ETRI FMO Implementation : FMO stuff end

    //===== initialization using parameter sets =====
    pcSPS->setAllocFrameMbsX( uiAllocMbX );
    pcSPS->setAllocFrameMbsY( uiAllocMbY );
    RNOK( m_pcControlMng->initParameterSets( *pcSPS, *pcPPS ) );
  }

  //---- some output ----
  if( bOutput )
  {
    printf("\n");
    printf("\nprofile & level info:" );
    printf("\n=====================" );
    for( UInt uiLayer = 0; uiLayer < m_pcCodingParameter->getNumberOfLayers(); uiLayer++ )
    {
      LayerParameters&  rcL   = m_pcCodingParameter->getLayerParameters( uiLayer );
      UInt              uiDQ  = ( rcL.getLayerCGSSNR() << 4 ) + rcL.getQualityLevelCGSSNR();
      UInt              uiNX  = rcL.getNumberOfQualityLevelsCGSSNR();
      UInt              uiLV  = rcL.getLevelIdc();
      for( UInt uiN = 0; uiN < uiNX; uiN++ )
      {
        printf( "\nDQ=%3d:  ", uiDQ + uiN );
        switch( rcL.getProfileIdc() )
        {
        case BASELINE_PROFILE:          ::printf( "Baseline" );           break;
        case MAIN_PROFILE:              ::printf( "Main" );               break;
        case EXTENDED_PROFILE:          ::printf( "Extended" );           break;
        case HIGH_PROFILE:              ::printf( "High" );               break;
        case SCALABLE_BASELINE_PROFILE: ::printf( "Scalable Baseline" );  break;
        case SCALABLE_HIGH_PROFILE:
          if( rcL.isIntraOnly() )       ::printf( "Scalable High Intra" );
          else                          ::printf( "Scalable High" );      break;
        default:
          ROT(1);
        }
        printf( " @ Level " );
        if( uiLV == 9 )                 ::printf( "1b" );
        else if( uiLV % 10 == 0 )       ::printf( "%d", uiLV / 10 );
        else                            ::printf( "%d.%d", uiLV / 10, uiLV % 10 );
      }
    }
    printf("\n\n\n");
  }

  //===== set unwritten parameter lists =====
  RNOK( m_pcParameterSetMng->setParamterSetList( m_cUnWrittenSPS, m_cUnWrittenPPS ) );

  return Err::m_nOK;
}


// JVT-V068 HRD {
ErrVal H264AVCEncoder::xInitLayerInfoForHrd( SequenceParameterSet* pcSPS, UInt uiLayer )
{
  Scheduler* pcScheduler = NULL;

  VUI* pcVui = pcSPS->getVUI();
  LayerParameters& rcLayer = m_pcCodingParameter->getLayerParameters(uiLayer);

  UInt uiTotalTemporalLevel = rcLayer.getDecompositionStages()- rcLayer.getNotCodedStages    () + 1;
  UInt uiTotalFGSLevel = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? pcSPS->getNumberOfQualityLevelsCGSSNR() : 1);
  UInt uiDependencyId = m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getLayerCGSSNR() : uiLayer;

  pcVui->setVuiParametersPresentFlag( ( m_pcCodingParameter->getEnableVUI()!=0) );

  if (pcVui->getVuiParametersPresentFlag())
  {
    pcVui->setProfileIdc(pcSPS->getProfileIdc());
    pcVui->setLevelIdc(pcSPS->getLevelIdc());

    pcVui->init( uiTotalTemporalLevel, uiTotalFGSLevel );

    // initiate Nal HRD pamameters
    UInt uiCpbBrVclFactor = 1250;
    UInt uiCpbBrNalFactor = 1500;
    if( pcVui->getProfileIdc() == BASELINE_PROFILE || pcVui->getProfileIdc() == EXTENDED_PROFILE || pcVui->getProfileIdc() == MAIN_PROFILE )
    {
      uiCpbBrVclFactor = 1000;
      uiCpbBrNalFactor = 1200;
    }
    else if( pcVui->getProfileIdc() == HIGH_10_PROFILE )
    {
      uiCpbBrVclFactor = 3000;
      uiCpbBrNalFactor = 3600;
    }
    else if( pcVui->getProfileIdc() == HIGH_422_PROFILE || pcVui->getProfileIdc() == HIGH_444_PROFILE || pcVui->getProfileIdc() == CAVLC_444_PROFILE )
    {
      uiCpbBrVclFactor = 4000;
      uiCpbBrNalFactor = 4800;
    }

    // handle 'output frequency' case
    Float fOutFreq = (Float)rcLayer.getOutputFrameRate() / ( 1 << (uiTotalTemporalLevel - 1) );
    UInt uiTimeScale = UInt(fOutFreq * 2000000 + 0.5); // integer frame rate (24, 30, 50, 60, etc)
    UInt uiNumUnits = 1000000;

    for (UInt uiTemporalId = 0; uiTemporalId < uiTotalTemporalLevel; uiTemporalId++)
    for (UInt uiFGSLevel = 0; uiFGSLevel < uiTotalFGSLevel; uiFGSLevel++)
    {
      UInt uiQualityLevel = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getQualityLevelCGSSNR() + uiFGSLevel : uiFGSLevel);
      //UInt uiQualityLevel = (m_pcCodingParameter->getCGSSNRRefinement() == 1 ? rcLayer.getQualityLevelCGSSNR() : uiFGSLevel);
      UInt uiIndex = (uiDependencyId<<7)+(uiTemporalId<<4)+uiQualityLevel;
      UInt uiHrdIdx = uiTemporalId * uiTotalFGSLevel + uiFGSLevel;

      pcVui->getLayerInfo(uiHrdIdx).setDependencyID(uiDependencyId);
      pcVui->getLayerInfo(uiHrdIdx).setTemporalId(uiTemporalId);
      pcVui->getLayerInfo(uiHrdIdx).setQualityLevel(uiQualityLevel);
      pcVui->getTimingInfo(uiHrdIdx).setTimingInfoPresentFlag((m_pcCodingParameter->getEnableVUITimingInfo()!=0));
      if (pcVui->getTimingInfo(uiHrdIdx).getTimingInfoPresentFlag() )
      {
        // write TimingInfo
        pcVui->getTimingInfo(uiHrdIdx).setNumUnitsInTick ( uiNumUnits );
        pcVui->getTimingInfo(uiHrdIdx).setTimeScale      ( uiTimeScale << uiTemporalId );
        pcVui->getTimingInfo(uiHrdIdx).setFixedFrameRateFlag ( true );  // fixed frame rate hard coded
      }
      pcVui->getNalHrd(uiHrdIdx).setHrdParametersPresentFlag(m_pcCodingParameter->getEnableNalHRD());
      pcVui->getVclHrd(uiHrdIdx).setHrdParametersPresentFlag(m_pcCodingParameter->getEnableVclHRD());
      pcVui->getLowDelayHrdFlag(uiHrdIdx) = false;        // we hard coded the low delay mode.
      pcVui->getPicStructPresentFlag(uiHrdIdx) = false;   // we hard coded the pic_struct_present_flag.

      if ( pcVui->getNalHrd(uiHrdIdx).getHrdParametersPresentFlag() )
        pcVui->InitHrd( uiHrdIdx, HRD::NAL_HRD,  pcSPS->getMaxBitRate() * uiCpbBrNalFactor, pcSPS->getMaxCPBSize() * uiCpbBrNalFactor );

      if ( pcVui->getVclHrd(uiHrdIdx).getHrdParametersPresentFlag() )
        pcVui->InitHrd( uiHrdIdx, HRD::VCL_HRD, pcSPS->getMaxBitRate() * uiCpbBrVclFactor, pcSPS->getMaxCPBSize() * uiCpbBrVclFactor );

      if ( m_pcCodingParameter->getEnableNalHRD() || m_pcCodingParameter->getEnableVclHRD() )
      {
        Scheduler::create(pcScheduler);
        pcScheduler->init(m_pcCodingParameter, uiLayer);
        RNOK( pcScheduler->initBuffer (pcVui, uiHrdIdx));
        (*m_apcScheduler)[uiIndex] = pcScheduler;
      }

      if( uiTemporalId == uiTotalTemporalLevel - 1 && uiFGSLevel == uiTotalFGSLevel - 1 )
      {
        pcVui->setDefaultIdx( uiHrdIdx );
      }
    }
  }

  return Err::m_nOK;
}

ErrVal H264AVCEncoder::writeAVCCompatibleHRDSEI(ExtBinDataAccessor* pcExtBinDataAccessor, SequenceParameterSet& rcSPS )
{
  VUI* pcVUI = rcSPS.getVUI();
  if (pcVUI->getVuiParametersPresentFlag() && pcVUI->getNumTemporalLevels() > 1)
  {
    UInt uiBitsSei = 0;
    SEI::MessageList cSEIMessageList;
    SEI::AVCCompatibleHRD* pcAVCCompHRD;
    RNOK( SEI::AVCCompatibleHRD::create(pcAVCCompHRD, pcVUI) );
    cSEIMessageList.push_back(pcAVCCompHRD);
    RNOK( m_pcNalUnitEncoder->initNalUnit( pcExtBinDataAccessor ) );
    RNOK( m_pcNalUnitEncoder->write( cSEIMessageList ) );
    RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBitsSei ) );
  }
  return Err::m_nOK;
}
// JVT-V068 HRD }




#ifdef SHARP_AVC_REWRITE_OUTPUT

RewriteEncoder::RewriteEncoder()
: m_bInitialized              ( false )
, m_bPictureInProgress        ( false )
, m_bSliceInProgress          ( false )
, m_uiBinDataSize             ( 0 )
, m_pcBitWriteBuffer          ( 0 )
, m_pcBitCounter              ( 0 )
, m_pcNalUnitEncoder          ( 0 )
, m_pcUvlcWriter              ( 0 )
, m_pcUvlcTester              ( 0 )
, m_pcCabacWriter             ( 0 )
, m_pcMotionVectorCalculation ( 0 )
, m_pcMbCoder                 ( 0 )
, m_pcRateDistortion          ( 0 )
, m_pcMbDataCtrl              ( 0 )
, m_pcBinData                 ( 0 )
, m_pcBinDataAccessor         ( 0 )
, m_pcSliceHeader             ( 0 )
, m_pcMbSymbolWriteIf         ( 0 )
, m_bTraceEnable              ( true )
{
}


RewriteEncoder::~RewriteEncoder()
{
}


ErrVal
RewriteEncoder::create( RewriteEncoder*& rpcRewriteEncoder )
{
  rpcRewriteEncoder = new RewriteEncoder;
  ROF ( rpcRewriteEncoder );
  RNOK( rpcRewriteEncoder->xCreate() );
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::destroy()
{
  ROT ( m_bInitialized );
  RNOK( m_pcBitWriteBuffer          ->destroy() );
  RNOK( m_pcBitCounter              ->destroy() );
  RNOK( m_pcNalUnitEncoder          ->destroy() );
  RNOK( m_pcUvlcWriter              ->destroy() );
  RNOK( m_pcUvlcTester              ->destroy() );
  RNOK( m_pcCabacWriter             ->destroy() );
  RNOK( m_pcMotionVectorCalculation ->destroy() );
  RNOK( m_pcMbCoder                 ->destroy() );
  RNOK( m_pcRateDistortion          ->destroy() );

  delete this;
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::init()
{
  ROT ( m_bInitialized );

  INIT_ETRACE;

  RNOK( m_pcBitWriteBuffer  ->init() );
  RNOK( m_pcBitCounter      ->init() );
  RNOK( m_pcNalUnitEncoder  ->init( m_pcBitWriteBuffer, m_pcUvlcWriter, m_pcUvlcTester ) );
  RNOK( m_pcUvlcWriter      ->init( m_pcBitWriteBuffer ) );
  RNOK( m_pcUvlcTester      ->init( m_pcBitCounter ) );
  RNOK( m_pcCabacWriter     ->init( m_pcBitWriteBuffer ) );
  m_bInitialized        = true;
  m_bPictureInProgress  = false;
  m_bSliceInProgress    = false;
  m_uiBinDataSize       = 0;
  m_pcMbDataCtrl        = 0;
  m_pcBinData           = 0;
  m_pcBinDataAccessor   = 0;
  m_pcSliceHeader       = 0;
  m_pcMbSymbolWriteIf   = 0;
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::uninit()
{
  ROT ( m_bPictureInProgress );
  ROF ( m_bInitialized );

  RNOK( m_pcBitWriteBuffer          ->uninit() );
  RNOK( m_pcBitCounter              ->uninit() );
  RNOK( m_pcNalUnitEncoder          ->uninit() );
  RNOK( m_pcUvlcWriter              ->uninit() );
  RNOK( m_pcUvlcTester              ->uninit() );
  RNOK( m_pcCabacWriter             ->uninit() );
  RNOK( m_pcMotionVectorCalculation ->uninit() );
  RNOK( m_pcMbCoder                 ->uninit() );

  if( m_pcMbDataCtrl )
  {
    RNOK(  m_pcMbDataCtrl           ->uninit() );
    delete m_pcMbDataCtrl;
  }
  m_cRewrittenParameterSets         .clear  ();

  UNINIT_ETRACE;

  m_bInitialized = false;

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::startPicture( const SequenceParameterSet& rcSPS )
{
  ROF( m_bInitialized );
  ROT( m_bPictureInProgress );

  m_bPictureInProgress  = true;
  UInt uiBinDataSize    = rcSPS.getFrameWidthInMbs() * rcSPS.getFrameHeightInMbs() * 768;
  if(  uiBinDataSize == m_uiBinDataSize )
  {
    RNOK( m_pcMbDataCtrl->resetData () );
    RNOK( m_pcMbDataCtrl->reset     () );
    return Err::m_nOK;
  }

  if( m_pcMbDataCtrl )
  {
    RNOK(  m_pcMbDataCtrl->uninit() );
    delete m_pcMbDataCtrl;
  }
  m_uiBinDataSize = uiBinDataSize;
  m_pcMbDataCtrl  = new MbDataCtrl;
  ROF ( m_pcMbDataCtrl );
  RNOK( m_pcMbDataCtrl->init( rcSPS ) );
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::finishPicture( BinDataList& rcBinDataList )
{
  ROF( m_bInitialized );
  ROF( m_bPictureInProgress );
  ROT( m_bSliceInProgress );

  m_bPictureInProgress  = false;
  rcBinDataList        += m_cBinDataList;
  m_cBinDataList.clear();
  ETRACE_NEWPIC;
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::rewriteMb( MbDataAccess& rcMbDataAccessSource, Bool bSendEOS )
{
  ROF( m_bInitialized );
  ROF( m_bPictureInProgress );
  ROT( m_bSliceInProgress == rcMbDataAccessSource.isFirstMbInSlice() );

  //===== start slice =====
  if( rcMbDataAccessSource.isFirstMbInSlice() )
  {
    RNOK( xStartSlice( rcMbDataAccessSource ) );
  }

  //===== process macroblock =====
  MbDataAccess*     pcMbDataAccessRewrite = 0;
  RNOK( xInitMb  (  pcMbDataAccessRewrite, rcMbDataAccessSource ) );
  RNOK( xAdjustMb( *pcMbDataAccessRewrite, rcMbDataAccessSource.getSH().isH264AVCCompatible() ) );
  RNOK( xEncodeMb( *pcMbDataAccessRewrite, rcMbDataAccessSource.isLastMbInSlice(), bSendEOS ) );

  //===== finish slice =====
  if( rcMbDataAccessSource.isLastMbInSlice() )
  {
    RNOK( xFinishSlice() );
  }

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xCreate()
{
  RNOK( BitWriteBuffer          ::create( m_pcBitWriteBuffer          ) );
  RNOK( BitCounter              ::create( m_pcBitCounter              ) );
  RNOK( NalUnitEncoder          ::create( m_pcNalUnitEncoder          ) );
  RNOK( UvlcWriter              ::create( m_pcUvlcWriter              ) );
  RNOK( UvlcWriter              ::create( m_pcUvlcTester              ) );
  RNOK( CabacWriter             ::create( m_pcCabacWriter             ) );
  RNOK( MotionVectorCalculation ::create( m_pcMotionVectorCalculation ) );
  RNOK( MbCoder                 ::create( m_pcMbCoder                 ) );
  RNOK( RateDistortion          ::create( m_pcRateDistortion          ) );
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xStartSlice( MbDataAccess& rcMbDataAccessSource )
{
  //===== create copy of given slice header =====
  m_pcSliceHeader = new SliceHeader( rcMbDataAccessSource.getSH() );
  ROF( m_pcSliceHeader );

  //===== modify parameters for rewrite slice header =====
  m_pcSliceHeader->setNalUnitType                   ( m_pcSliceHeader->getIdrFlag() ? NAL_UNIT_CODED_SLICE_IDR : NAL_UNIT_CODED_SLICE );
  m_pcSliceHeader->setDependencyId                  ( 0 );
  m_pcSliceHeader->setQualityId                     ( 0 );
  m_pcSliceHeader->setNoInterLayerPredFlag          ( true );
  m_pcSliceHeader->setRefLayerDQId                  ( MSYS_UINT_MAX );
  m_pcSliceHeader->setAdaptiveBaseModeFlag          ( false );
  m_pcSliceHeader->setAdaptiveMotionPredictionFlag  ( false );
  m_pcSliceHeader->setAdaptiveResidualPredictionFlag( false );
  m_pcSliceHeader->setDefaultBaseModeFlag           ( false );
  m_pcSliceHeader->setDefaultMotionPredictionFlag   ( false );
  m_pcSliceHeader->setDefaultResidualPredictionFlag ( false );
  m_pcSliceHeader->setScanIdxStart                  ( 0 );
  m_pcSliceHeader->setScanIdxStop                   ( 16 );
  m_pcSliceHeader->setSliceHeaderQp                 ( rcMbDataAccessSource.getMbData().getQp4LF() );
  m_pcSliceHeader->setSliceSkipFlag                 ( false );

  //===== set entropy coder, write parameter, init NAL unit, write slice header, and init slice =====
  m_pcMbSymbolWriteIf = ( m_pcSliceHeader->getPPS().getEntropyCodingModeFlag() ? (MbSymbolWriteIf*)m_pcCabacWriter : (MbSymbolWriteIf*)m_pcUvlcWriter );
  RNOK( xRewriteSPS     ( m_pcSliceHeader->getSPS() ) );
  RNOK( xRewritePPS     ( m_pcSliceHeader->getPPS() ) );
  RNOK( xInitNALUnit    () );
  RNOK( m_pcSliceHeader             ->write     ( *m_pcUvlcWriter   ) );
  RNOK( m_pcMbSymbolWriteIf         ->startSlice( *m_pcSliceHeader  ) );
  RNOK( m_pcMotionVectorCalculation ->initSlice ( *m_pcSliceHeader  ) );
  RNOK( m_pcMbCoder                 ->initSlice ( *m_pcSliceHeader, m_pcMbSymbolWriteIf, m_pcRateDistortion ) );
  RNOK( m_pcMbDataCtrl              ->initSlice ( *m_pcSliceHeader, ENCODE_PROCESS, false, NULL ) );

  m_bSliceInProgress  = true;
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xFinishSlice()
{
  RNOK( xCloseNALUnit() );
  delete m_pcSliceHeader;
  m_pcSliceHeader     = 0;
  m_pcMbSymbolWriteIf = 0;
  m_bSliceInProgress  = false;
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xInitNALUnit()
{
  UChar*  pucBuffer   = new UChar [ m_uiBinDataSize ];
  m_pcBinData         = new BinData;
  m_pcBinDataAccessor = new BinDataAccessor;
  ROF( pucBuffer );
  ROF( m_pcBinData );
  ROF( m_pcBinDataAccessor );

  m_pcBinData->set( pucBuffer, m_uiBinDataSize );
  m_pcBinData->setMemAccessor( *m_pcBinDataAccessor );

  RNOK( m_pcNalUnitEncoder->initNalUnit( m_pcBinDataAccessor ) );
  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xCloseNALUnit()
{
  UInt uiBits = 0;
  RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );

  m_pcBinData->set( m_pcBinData->data(), m_pcBinDataAccessor->size() ); // set correct size
  m_cBinDataList.push_back( m_pcBinData );
  delete m_pcBinDataAccessor;

  m_pcBinData         = 0;
  m_pcBinDataAccessor = 0;
  return Err::m_nOK;
}


Bool
RewriteEncoder::xIsRewritten( const Void* pParameterSet )
{
  MyList<const Void*>::iterator iIter = m_cRewrittenParameterSets.begin();
  MyList<const Void*>::iterator iEnd  = m_cRewrittenParameterSets.end  ();
  while( iIter != iEnd )
  {
    if( (*iIter) == pParameterSet )
    {
      return true;
    }
    iIter++;
  }
  return false;
}


ErrVal
RewriteEncoder::xRewriteSPS( const SequenceParameterSet& rcSPS )
{
  ROTRS( xIsRewritten( &rcSPS ), Err::m_nOK );

  //===== store nal_unit_type and profile_idc =====
  NalUnitType eNalUnitType  = rcSPS.getNalUnitType();
  Profile     eProfile      = rcSPS.getProfileIdc ();

  //===== modify nal_unit_type and profile_idc =====
  const_cast<SequenceParameterSet&>(rcSPS).setNalUnitType( NAL_UNIT_SPS );
  const_cast<SequenceParameterSet&>(rcSPS).setProfileIdc ( HIGH_PROFILE );

  //===== write SPS =====
  RNOK( xInitNALUnit  () );
  RNOK( m_pcNalUnitEncoder->write( rcSPS ) );
  RNOK( xCloseNALUnit () );
  m_cRewrittenParameterSets.push_back( &rcSPS );

  //===== restore original nal_unit_type and profile_idc =====
  const_cast<SequenceParameterSet&>(rcSPS).setNalUnitType( eNalUnitType );
  const_cast<SequenceParameterSet&>(rcSPS).setProfileIdc ( eProfile );

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xRewritePPS( const PictureParameterSet& rcPPS )
{
  ROTRS( xIsRewritten( &rcPPS ), Err::m_nOK );

  //===== write PPS =====
  RNOK( xInitNALUnit  () );
  RNOK( m_pcNalUnitEncoder->write( rcPPS ) );
  RNOK( xCloseNALUnit () );
  m_cRewrittenParameterSets.push_back( &rcPPS );

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xInitMb( MbDataAccess*& rpcMbDataAccessRewrite, MbDataAccess& rcMbDataAccessSource )
{
  //===== create MbDataAccess for rewriting =====
  rpcMbDataAccessRewrite = 0;
  UInt  uiMbY            = rcMbDataAccessSource.getMbY();
  UInt  uiMbX            = rcMbDataAccessSource.getMbX();
  RNOK( m_pcMbDataCtrl->initMb( rpcMbDataAccessRewrite, uiMbY, uiMbX ) );

  //===== copy macroblock data =====
  if( ! rcMbDataAccessSource.getMbData().isPCM() )
  {
    rcMbDataAccessSource   .getMbTCoeffs().switchLevelCoeffData ();
  }
  rpcMbDataAccessRewrite->getMbData   ().copyFrom             ( rcMbDataAccessSource.getMbData   () );
  rpcMbDataAccessRewrite->getMbTCoeffs().copyFrom             ( rcMbDataAccessSource.getMbTCoeffs() );
  rpcMbDataAccessRewrite->getMbData   ().copyMotion           ( rcMbDataAccessSource.getMbData   () );
  if( ! rcMbDataAccessSource.getMbData().isPCM() )
  {
    rcMbDataAccessSource   .getMbTCoeffs().switchLevelCoeffData ();
  }

  //===== set correct rewrite QP =====
  rpcMbDataAccessRewrite->getMbData().setQp( rcMbDataAccessSource.getMbData().getQp4LF() );

  //===== set field mode =====
  rpcMbDataAccessRewrite->setFieldMode( rcMbDataAccessSource.getMbData().getFieldFlag() );

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xAdjustMb( MbDataAccess& rcMbDataAccessRewrite, Bool bBaseLayer )
{
  //===== clear motion data for later usage =====
  if( rcMbDataAccessRewrite.getMbData().isIntra() )
  {
    rcMbDataAccessRewrite.getMbData().getMbMotionData( LIST_0 ).clear( BLOCK_NOT_PREDICTED );
    rcMbDataAccessRewrite.getMbData().getMbMvdData   ( LIST_0 ).clear();
    rcMbDataAccessRewrite.getMbData().getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
    rcMbDataAccessRewrite.getMbData().getMbMvdData   ( LIST_1 ).clear();
  }
  else if( rcMbDataAccessRewrite.getMbData().isSkiped() )
  {
    rcMbDataAccessRewrite.getMbData().getMbMvdData   ( LIST_0 ).clear();
    rcMbDataAccessRewrite.getMbData().getMbMvdData   ( LIST_1 ).clear();
  }
  UInt uiFwdBwd = 0;
  if( rcMbDataAccessRewrite.getSH().isBSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += ( rcMbDataAccessRewrite.getMbData().getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) ) > 0 ? 1 : 0 );
      uiFwdBwd += ( rcMbDataAccessRewrite.getMbData().getMbMotionData( LIST_1 ).getRefIdx( Par8x8(n) ) > 0 ? 2 : 0 );
    }
  }
  else if( rcMbDataAccessRewrite.getSH().isPSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += ( rcMbDataAccessRewrite.getMbData().getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) ) > 0 ? 1 : 0 );
    }
  }
  rcMbDataAccessRewrite.getMbData().setFwdBwd( uiFwdBwd );

  //===== clear 8x8 transform flag when required =====
  if( ! rcMbDataAccessRewrite.getMbData().isIntra4x4() && ( rcMbDataAccessRewrite.getMbData().getMbCbp() & 0x0F ) == 0 )
  {
    rcMbDataAccessRewrite.getMbData().setTransformSize8x8( false );
  }

  //===== correct MbMode =====
  rcMbDataAccessRewrite.getMbData().setBLSkipFlag      ( false );
  rcMbDataAccessRewrite.getMbData().setResidualPredFlag( false );
  rcMbDataAccessRewrite.getMbData().setBCBPAll         ( 0 );
  if( rcMbDataAccessRewrite.getMbData().getMbMode() == MODE_SKIP )
  {
    if( ! bBaseLayer || ! rcMbDataAccessRewrite.getSH().isBSlice() )
    {
      rcMbDataAccessRewrite.getMbData().setMbMode( MODE_16x16 );
    }
    else
    {
      Bool bAll8x8 = true;
      rcMbDataAccessRewrite.getMbData().setMbMode( MODE_8x8 );
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( rcMbDataAccessRewrite.getSH().getSPS().getDirect8x8InferenceFlag() )
        {
          rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_8x8 );
        }
        else
        {
          rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_4x4 );
          //--- check whether 4x4 blocks can be combined to an 8x8 block (further improvements of this summarization process are possible) ---
          Bool b8x8Ok = true;
          for( UInt uiList = 0; uiList < 2; uiList++ )
          {
            ParIdx8x8     eParIdx8x8  = c8x8Idx.b8x8();
            MbMotionData& rcMotData   = rcMbDataAccessRewrite.getMbData().getMbMotionData( ListIdx( uiList ) );
            if( rcMotData.getRefIdx( eParIdx8x8 ) > 0 )
            {
              if( rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_1 ) ||
                  rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_2 ) ||
                  rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_3 )    )
              {
                b8x8Ok = false;
              }
            }
          }
          if( b8x8Ok )
          {
            rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_8x8 );
          }
          else
          {
            bAll8x8 = false;
          }
        }
      }
      //--- check whether 8x8 blocks can be combined into 16x16 macroblock (further improvements of this summarization process are possible) ---
      if( bAll8x8 )
      {
        Bool b16x16Ok = true;
        for( UInt uiList = 0; uiList < 2 && b16x16Ok; uiList++ )
        {
          MbMotionData& rcMotData = rcMbDataAccessRewrite.getMbData().getMbMotionData( ListIdx( uiList ) );
          b16x16Ok  = b16x16Ok && ( rcMotData.getRefIdx( PART_8x8_0 ) == rcMotData.getRefIdx( PART_8x8_1 ) );
          b16x16Ok  = b16x16Ok && ( rcMotData.getRefIdx( PART_8x8_0 ) == rcMotData.getRefIdx( PART_8x8_2 ) );
          b16x16Ok  = b16x16Ok && ( rcMotData.getRefIdx( PART_8x8_0 ) == rcMotData.getRefIdx( PART_8x8_3 ) );
          if( b16x16Ok && rcMotData.getRefIdx( PART_8x8_0 ) > 0 )
          {
            b16x16Ok = b16x16Ok && ( rcMotData.getMv( PART_8x8_0 ) == rcMotData.getMv( PART_8x8_1 ) );
            b16x16Ok = b16x16Ok && ( rcMotData.getMv( PART_8x8_0 ) == rcMotData.getMv( PART_8x8_2 ) );
            b16x16Ok = b16x16Ok && ( rcMotData.getMv( PART_8x8_0 ) == rcMotData.getMv( PART_8x8_3 ) );
          }
        }
        if( b16x16Ok )
        {
          rcMbDataAccessRewrite.getMbData().setMbMode( MODE_16x16 );
        }
      }
    }
  }
  else if( rcMbDataAccessRewrite.getMbData().getMbMode() == MODE_8x8 ||
           rcMbDataAccessRewrite.getMbData().getMbMode() == MODE_8x8ref0 )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      if( rcMbDataAccessRewrite.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) == BLK_SKIP )
      {
        if( ! bBaseLayer || rcMbDataAccessRewrite.getSH().getSPS().getDirect8x8InferenceFlag() )
        {
          rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_8x8 );
        }
        else
        {
          rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_4x4 );
          //--- check whether 4x4 blocks can be combined to an 8x8 block (further improvements of this summarization process are possible) ---
          Bool b8x8Ok = true;
          for( UInt uiList = 0; uiList < 2; uiList++ )
          {
            ParIdx8x8     eParIdx8x8  = c8x8Idx.b8x8();
            MbMotionData& rcMotData   = rcMbDataAccessRewrite.getMbData().getMbMotionData( ListIdx( uiList ) );
            if( rcMotData.getRefIdx( eParIdx8x8 ) > 0 )
            {
              if( rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_1 ) ||
                  rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_2 ) ||
                  rcMotData.getMv( eParIdx8x8, SPART_4x4_0 ) != rcMotData.getMv( eParIdx8x8, SPART_4x4_3 )    )
              {
                b8x8Ok = false;
              }
            }
          }
          if( b8x8Ok )
          {
            rcMbDataAccessRewrite.getMbData().setBlkMode( c8x8Idx.b8x8Index(), BLK_8x8 );
          }
        }
      }
    }
  }

  //===== recompute motion vector differences =====
  if( !rcMbDataAccessRewrite.getMbData().isIntra() )
  {
    UInt          uiList;
    Mv            cMv;
    MbMotionData  cMbMotionData[2];

    //----- save motion data and clear mvd's -----
    for( uiList = 0; uiList < 2; uiList++ )
    {
      ListIdx eListIdx = ( uiList == 0 ? LIST_0 : LIST_1 );
      cMbMotionData[uiList].copyFrom( rcMbDataAccessRewrite.getMbMotionData( eListIdx ) );
      rcMbDataAccessRewrite.getMbMvdData   ( eListIdx ).clear();
      rcMbDataAccessRewrite.getMbMotionData( eListIdx ).setMotPredFlag( false );
    }

    //----- re-assign mvd's (this is a sub-optimal implementation) -----
    for( uiList = 0; uiList < 2; uiList++ )
    {
      ListIdx eListIdx = ( uiList == 0 ? LIST_0 : LIST_1 );

      //----- recompute mvd's -----
      for( S4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        RNOK( m_pcMotionVectorCalculation->calcMvMb( rcMbDataAccessRewrite, 0 ) );
        cMv  = cMbMotionData[uiList].getMv( cIdx );
        cMv -= rcMbDataAccessRewrite.getMbMotionData( eListIdx ).getMv( cIdx );
        rcMbDataAccessRewrite.getMbMvdData( eListIdx ).setMv( cMv, cIdx );
      }

      //----- set mvd's for all 4x4 blocks (for CABAC context models) -----
      switch( rcMbDataAccessRewrite.getMbData().getMbMode() )
      {
      case MODE_16x16:
        cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( );
        rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv );
        break;
      case MODE_16x8:
        cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( PART_16x8_0 );
        rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, PART_16x8_0 );
        cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( PART_16x8_1 );
        rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, PART_16x8_1 );
        break;
      case MODE_8x16:
        cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( PART_8x16_0 );
        rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, PART_8x16_0 );
        cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( PART_8x16_1 );
        rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, PART_8x16_1 );
        break;
      case MODE_8x8:
      case MODE_8x8ref0:
        {
          for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
          {
            ParIdx8x8 eParIdx = c8x8Idx.b8x8();

            switch( rcMbDataAccessRewrite.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) )
            {
            case BLK_8x8:
              cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( eParIdx );
              rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, eParIdx );
              break;
            case BLK_8x4:
              cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( eParIdx, SPART_8x4_0 );
              rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, eParIdx, SPART_8x4_0 );
              cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( eParIdx, SPART_8x4_1 );
              rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, eParIdx, SPART_8x4_1 );
              break;
            case BLK_4x8:
              cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( eParIdx, SPART_4x8_0 );
              rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, eParIdx, SPART_4x8_0 );
              cMv = rcMbDataAccessRewrite.getMbMvdData( eListIdx ).getMv( eParIdx, SPART_4x8_1 );
              rcMbDataAccessRewrite.getMbMvdData(eListIdx).setAllMv( cMv, eParIdx, SPART_4x8_1 );
              break;
            case BLK_4x4:
              break;
            default:
              RERR();
            }
          }
        }
        break;
      default:
        RERR();
      }
    }

#if 1 // not required
    //>>>>> SANITY CHECK >>>>>
    RNOK( m_pcMotionVectorCalculation->calcMvMb( rcMbDataAccessRewrite, 0 ) );
    for( uiList = 0; uiList < 2; uiList++ )
    {
      for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        ListIdx eListIdx = ( uiList == 0 ? LIST_0 : LIST_1 );
        cMv              = cMbMotionData[uiList].getMv( cIdx );
        cMv             -= rcMbDataAccessRewrite.getMbMotionData( eListIdx ).getMv( cIdx );
        if( cMv.getHor() || cMv.getVer() )
        {
          RERR();
        }
      }
    }
    //<<<<< SANITY CHECK <<<<<
#endif
  }

  return Err::m_nOK;
}


ErrVal
RewriteEncoder::xEncodeMb( MbDataAccess& rcMbDataAccessRewrite, Bool bLastMbInSlice, Bool bSendEOS )
{
  //===== encode =====
  RNOK( m_pcMbCoder->encode( rcMbDataAccessRewrite, NULL, bLastMbInSlice, bSendEOS ) );
  return Err::m_nOK;
}


#endif



H264AVC_NAMESPACE_END
