
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"

#include "CodingParameter.h"
#include "SequenceStructure.h"

#include <math.h>

#define ROTREPORT(x,t)   {if(x) {::printf("\n%s\n",t); assert(0); return Err::m_nInvalidParameter;} }
#define SETREPORT(x,v,s) {if(x!=v) {x=v; ::printf("in layer %d: %s\n",m_uiLayerId,s);}}

// h264 namepace begin
H264AVC_NAMESPACE_BEGIN


ErrVal MotionVectorSearchParams::check()
{
  ROTREPORT( 4 < m_uiSearchMode,   "No Such Search Mode 0==Block,1==Spiral,2==Log,3==Fast, 4==NewFast" )
  ROTREPORT( 3 < m_uiFullPelDFunc, "No Such Search Func (Full Pel) 0==SAD,1==SSE,2==Hadamard,3==YUV-SAD" )
  ROTREPORT( 3 == m_uiFullPelDFunc && (m_uiSearchMode==2 || m_uiSearchMode==3), "Log and Fast search not possible in comb. with distortion measure 3" )
  ROTREPORT( 2 < m_uiSubPelDFunc,  "No Such Search Func (Sub Pel) 0==SAD,1==SSE,2==Hadamard" )
  ROTREPORT( 1 < m_uiDirectMode,  "Direct Mode Exceeds Supported Range 0==Temporal, 1==Spatial");

  if( m_uiELSearchRange == 0 )
  {
    m_uiELSearchRange = m_uiSearchRange;
    m_bELSearch       = false;
  }
  else
  {
    m_bELSearch       = true;
  }

  return Err::m_nOK;
}


ErrVal LoopFilterParams::check() const
{
  //V032 of FSL for extending the Idc value to 3 and 4 w.r.t. 0 and 2 in the enhanced layer
  //for disabling chroma deblocking in enhanced layer
  ROTREPORT( 6 < getFilterIdc(),        "Loop Filter Idc exceeds supported range 0..6");


  if( 69 != getAlphaOffset() )
  {
    ROTREPORT( 26 < getAlphaOffset(),       "Loop Filter Alpha Offset exceeds supported range -26..26");
    ROTREPORT( -26 > getAlphaOffset(),      "Loop Filter Alpha Offset exceeds supported range -26..26");
  }

  if( 69 != getBetaOffset() )
  {
    ROTREPORT( 26 < getBetaOffset(),        "Loop Filter Beta Offset exceeds supported range -26..26");
    ROTREPORT( -26 > getBetaOffset(),       "Loop Filter Beta Offset exceeds supported range -26..26");
  }

  return Err::m_nOK;
}


UInt LayerParameters::getNumberOfQualityLevelsCGSSNR() const
{
  UInt uiVectPos = 0;
  UInt uiQLs;
  for( uiQLs = 0; uiVectPos != 16; uiQLs++ )
  {
    uiVectPos += getMGSVect( uiQLs );
    ROTREPORT( uiQLs == 15 && uiVectPos != 16, "MGS vectors don't add up to 16" );
  }
  return uiQLs;
}

ErrVal LayerParameters::check()
{
  ROTREPORT( getFrameWidthInSamples     () % 2,         "Frame Width must be a multiple of 2" );
  ROTREPORT( getFrameHeightInSamples    () % 2,         "Frame Height must be a multiple of 2" );
  ROTREPORT( getFrameHeightInSamples    () % 4
             && isInterlaced            (),             "Frame Height must be a multiple of 4 for interlace" );
  ROTREPORT( getInputFrameRate          () <
             getOutputFrameRate         (),             "Output frame rate must be less than or equal to input frame rate" );
  ROTREPORT( getEnable8x8Trafo          ()  > 1,        "The value for Enable8x8Transform is not supported" );
  ROTREPORT( getScalingMatricesPresent  ()  > 1,        "The value for ScalingMatricesPresent is not supported" );
  ROTREPORT( getBiPred8x8Disable        () > 1,         "The value for BiPredLT8x8Disable is not supported" );
  ROTREPORT( getMCBlks8x8Disable        () > 1,         "The value for MCBlocksLT8x8Disable is not supported" );
  ROTREPORT( getPicCodingType           () > 1,         "The value for DisableBSlices is not supported" );
  ROTREPORT( getMaxAbsDeltaQP           () > 7,         "MaxAbsDeltaQP not supported" );
  ROTREPORT( getChromaQPIndexOffset     () < -12 ||
             getChromaQPIndexOffset     () >  12,       "Unsupported value for CbQPIndexOffset" );
  ROTREPORT( get2ndChromaQPIndexOffset  () < -12 ||
             get2ndChromaQPIndexOffset  () >  12,       "Unsupported value for CrQPIndexOffset" );
  ROTREPORT( getConstrainedIntraPred    () > 1,         "Unsupported value for ConstrainedIntraPred" );

  //----- check inter-layer prediction modes -----
  ROTREPORT( getInterLayerPredictionMode() > 2,         "Unsupported value for InterLayerPred" );
  if( m_uiILPredMode     == MSYS_UINT_MAX )   m_uiILPredMode      = m_uiInterLayerPredictionMode;
  if( m_uiILPredMotion   == MSYS_UINT_MAX )   m_uiILPredMotion    = m_uiInterLayerPredictionMode;
  if( m_uiILPredResidual == MSYS_UINT_MAX )   m_uiILPredResidual  = m_uiInterLayerPredictionMode;
  ROTREPORT( m_uiILPredMode                > 2,         "Unsupported value for ILModePred" );
  ROTREPORT( m_uiILPredMotion              > 2,         "Unsupported value for ILMotionPred" );
  ROTREPORT( m_uiILPredResidual            > 2,         "Unsupported value for ILResidualPred" );
  m_uiInterLayerPredictionMode = ( m_uiInterLayerPredictionMode != 0 ||
                                   m_uiILPredMode               != 0 || 
                                   m_uiILPredMotion             != 0 ||
                                   m_uiILPredResidual           != 0  ? 1 : 0 );

  ROTREPORT( getBaseLayerId() != MSYS_UINT_MAX && getBaseLayerId() >= getDependencyId(), "BaseLayerId is not possible" );

  ROTREPORT( m_uiSliceMode != 0 && m_uiSliceMode != 1 && m_uiSliceMode != 2, "Invalid slice mode" );
  ROTREPORT( m_uiSliceMode != 0 && m_uiSliceArgument == 0, "Invalid slice argument" );

  UInt uiVectPos = 0;
  UInt ui;
  for( ui = 0; uiVectPos < 16; ui++ )
  {
    uiVectPos += getMGSVect( ui );
    ROTREPORT( uiVectPos > 16 || (ui == 15 && uiVectPos < 16), "Sum over elements of MGSVector does not equal 16." );
  }
  Bool bTrailingZeros = true;
  for( ui = 15; ui > 0; ui-- )
  {
    if( getMGSVect( ui ) != 0 )
      bTrailingZeros = false;
    ROTREPORT( !bTrailingZeros && getMGSVect( ui ) == 0, "Zeros inside of the MGSVector are not allowed (except for the first element and the end of the vector)." );
  }
  Bool bUseMGSVectors = getMGSVect( 0 ) != 16;
  ROTREPORT( ( getTCoeffLevelPredictionFlag() || getAVCAdaptiveRewriteFlag() ) && bUseMGSVectors, "MGS Vectors are not allowed with AVC rewriting enabled." );
  ROTREPORT( m_uiSliceMode == 2 && bUseMGSVectors, "SliceMode 2 not supported in connection with MGS Vectors" );

  if( bUseMGSVectors && m_uiMaxAbsDeltaQP )
  {
    m_uiMaxAbsDeltaQP = 0;
    printf("\nInfo: MaxDeltaQP was set to 0 for layer with MGSVectorMode\n\n");
  }

  if ((getBaseLayerId() == MSYS_UINT_MAX) && (getTCoeffLevelPredictionFlag() == true))
  {
    printf( "AvcRewriteFlag should be false for base layer, reset to 0\n" );
    m_bAVCRewriteFlag = 0;
  }

  if( m_dQpModeDecisionLP == -1.0 )
  {
    m_dQpModeDecisionLP = m_dBaseQpResidual;
  }

  if ( m_uiNumSliceGroupsMinus1 > 0 && m_uiSliceGroupMapType == 2 && ( m_uiSliceMode == 0 || m_uiSliceMode == 1 ) )
  {
    ROT( m_uiSliceMode == 1 && m_uiSliceArgument == 0 );
    ROTREPORT( isInterlaced(), "Slice groups and interlaced are not supported in the same profile" );
    UInt  uiMaxSliceSize  = ( m_uiSliceMode == 1 ? m_uiSliceArgument : MSYS_UINT_MAX );
    UInt  uiMapWidth      = getFrameWidthInMbs  ();
    UInt  uiMapHeight     = getFrameHeightInMbs ();
    UInt  uiMapSize       = uiMapHeight * uiMapWidth;
    UInt* pauiSGMap       = new UInt [ uiMapSize ];
    UInt* pauiROIMap      = new UInt [ uiMapSize ];
    UInt* pauiROIFirstMB  = new UInt [ uiMapSize ];
    UInt* pauiROILastMB   = new UInt [ uiMapSize ];
    UInt* pauiROIWidth    = new UInt [ uiMapSize ];
    UInt* pauiROIHeight   = new UInt [ uiMapSize ];
    ROF(  pauiSGMap && pauiROIMap && pauiROIFirstMB && pauiROILastMB && pauiROIWidth && pauiROIHeight );
    //----- set slice group Id's in map -----
    for( Int iIdx0 = 0; iIdx0 < (Int)uiMapSize; iIdx0++ )
    {
      pauiSGMap[iIdx0] = m_uiNumSliceGroupsMinus1;
    }
    for( Int iGroup0 = m_uiNumSliceGroupsMinus1 - 1; iGroup0 >= 0; iGroup0-- )
    {
      Int iY0 = m_uiTopLeft    [ iGroup0 ] / uiMapWidth;
      Int iX0 = m_uiTopLeft    [ iGroup0 ] % uiMapWidth;
      Int iY1 = m_uiBottomRight[ iGroup0 ] / uiMapWidth;
      Int iX1 = m_uiBottomRight[ iGroup0 ] % uiMapWidth;
      for( Int iY = iY0; iY <= iY1; iY++ )
      for( Int iX = iX0; iX <= iX1; iX++ )
      {
        pauiSGMap[ iY * uiMapWidth + iX ] = iGroup0;
      }
    }
    //----- set ROI map -----
    UInt uiROIId = MSYS_UINT_MAX;
    for( Int iGroup = 0; iGroup <= (Int)m_uiNumSliceGroupsMinus1; iGroup++ )
    {
      UInt uiNumMb = 0;
      for( UInt uiIdx = 0; uiIdx < uiMapSize; uiIdx++ )
      {
        if( pauiSGMap[uiIdx] == iGroup )
        {
          if( uiNumMb == 0 )
          {
            uiROIId++;
            pauiROIFirstMB[uiROIId] = uiIdx;
          }
          pauiROILastMB   [uiROIId] = uiIdx;
          pauiROIMap      [uiIdx]   = uiROIId;
          if( ++uiNumMb == uiMaxSliceSize )
          {
            uiNumMb = 0;
          }
        }
      }
    }
    UInt  uiNumROI      = uiROIId + 1;
    Bool  bRectangular  = true;
    Bool  bSameSize     = true;
    //----- check ROI map and set sizes -----
    for( uiROIId = 0; uiROIId < uiNumROI; uiROIId++ )
    {
      Int iY0 = pauiROIFirstMB[ uiROIId ] / uiMapWidth;
      Int iX0 = pauiROIFirstMB[ uiROIId ] % uiMapWidth;
      Int iY1 = pauiROILastMB [ uiROIId ] / uiMapWidth;
      Int iX1 = pauiROILastMB [ uiROIId ] % uiMapWidth;
      for( Int iY = 0; iY < (Int)uiMapHeight; iY++ )
      for( Int iX = 0; iX < (Int)uiMapWidth;  iX++ )
      {
        Bool bInside = ( iY >= iY0 && iY <= iY1 && iX >= iX0 && iX <= iX1 );
        Bool bMatch  = ( pauiROIMap[ iY * uiMapWidth + iX ] == uiROIId );
        if( ( bInside && ! bMatch ) || ( !bInside && bMatch ) )
        {
          bRectangular = false;
        }
      }
      pauiROIWidth [uiROIId] = iX1 - iX0 + 1;
      pauiROIHeight[uiROIId] = iY1 - iY0 + 1;
      if( uiROIId > 0 && bSameSize )
      {
        if( pauiROIWidth[uiROIId] != pauiROIWidth[0] || pauiROIHeight[uiROIId] != pauiROIHeight[0] )
        {
          bSameSize = false;
        }
      }
    }
    //----- set ROI parameters -----
    if( bRectangular )
    {
      m_bSliceDivisionFlag  = true;
      m_bGridFlag           = bSameSize;
      m_uiNumSliceMinus1    = uiNumROI - 1;
      // alloc arrays
      if (m_puiGridSliceWidthInMbsMinus1 != NULL)
        free(m_puiGridSliceWidthInMbsMinus1);
      m_puiGridSliceWidthInMbsMinus1 = (UInt*)malloc((m_uiNumSliceMinus1+1)*sizeof(UInt));
      if (m_puiGridSliceHeightInMbsMinus1 != NULL)
        free(m_puiGridSliceHeightInMbsMinus1);
      m_puiGridSliceHeightInMbsMinus1 = (UInt*)malloc((m_uiNumSliceMinus1+1)*sizeof(UInt));
      if (m_puiFirstMbInSlice != NULL)
        free(m_puiFirstMbInSlice);
      m_puiFirstMbInSlice = (UInt*)malloc((m_uiNumSliceMinus1+1)*sizeof(UInt));
      if (m_puiLastMbInSlice != NULL)
        free(m_puiLastMbInSlice);
      m_puiLastMbInSlice = (UInt*)malloc((m_uiNumSliceMinus1+1)*sizeof(UInt));
      if (m_puiSliceId != NULL)
        free(m_puiSliceId);
      m_puiSliceId = (UInt*)malloc(uiMapSize*sizeof(UInt));
      // set data and output some info
      UInt uiMBCount  = 0;
      printf("Layer%2d: %d IROI's with IROI grid flag = %d\n", m_uiLayerId, uiNumROI, m_bGridFlag );
      for( UInt uiSliceNum = 0; uiSliceNum < uiNumROI; uiSliceNum++ )
      {
        printf("\tROI%2d:  W =%3d,  H =%3d,  FirstMb = %d\n", uiSliceNum, pauiROIWidth[uiSliceNum], pauiROIHeight[uiSliceNum], pauiROIFirstMB[uiSliceNum] );
        m_puiGridSliceWidthInMbsMinus1  [uiSliceNum] = pauiROIWidth  [ uiSliceNum ] - 1;
        m_puiGridSliceHeightInMbsMinus1 [uiSliceNum] = pauiROIHeight [ uiSliceNum ] - 1;
        m_puiFirstMbInSlice             [uiSliceNum] = pauiROIFirstMB[ uiSliceNum ];
        m_puiLastMbInSlice              [uiSliceNum] = pauiROILastMB [ uiSliceNum ];
        UInt uiMBAddr   = m_puiFirstMbInSlice[uiSliceNum];
        for( UInt i = 0; i <= m_puiGridSliceHeightInMbsMinus1[uiSliceNum]; i++, uiMBAddr += uiMapWidth )
        {
          for( UInt j = 0; j <= m_puiGridSliceWidthInMbsMinus1[uiSliceNum]; j++, uiMBCount++)
          {
            m_puiSliceId[uiMBAddr+j] = uiSliceNum;
          }
        }
      }
      ROF( uiMBCount == uiMapSize );
      printf("\n\n");
    }
    //----- delete temporary arrays -----
    delete [] pauiSGMap;
    delete [] pauiROIMap;
    delete [] pauiROIFirstMB;
    delete [] pauiROILastMB;
    delete [] pauiROIWidth;
    delete [] pauiROIHeight;
  }

  //S051{
  ROTREPORT( getAnaSIP	()>0 && getEncSIP(),			"Unsupported SIP mode\n");
  //S051}

  return Err::m_nOK;
}


UInt CodingParameter::getLogFactor( Double r0, Double r1 )
{
  Double dLog2Factor  = log10( r1 / r0 ) / log10( 2.0 );
  Double dRound       = floor( dLog2Factor + 0.5 );
  Double dEpsilon     = 0.0001;

  if( dLog2Factor-dEpsilon < dRound && dRound < dLog2Factor+dEpsilon )
  {
    return (UInt)(dRound);
  }
  return MSYS_UINT_MAX;
}



ErrVal CodingParameter::check()
{
  if( !getAVCmode() && getNumberOfLayers() && getLayerParameters(0).getLowComplexMbEnable() )
  {
    m_cMotionVectorSearchParams.setFullPelDFunc ( 0 );
    m_cMotionVectorSearchParams.setSubPelDFunc  ( 0 );
  }

  ROTS( m_cLoopFilterParams           .check() );
  ROTS( m_cInterLayerLoopFilterParams .check() );
  ROTS( m_cMotionVectorSearchParams   .check() );

  if( getAVCmode() )
  {
    Bool bStringNotOk = SequenceStructure::checkString( getSequenceFormatString() );

    //===== coder is operated in MVC mode =====
    ROTREPORT( getFrameWidth            () <= 0 ||
               getFrameWidth            ()  % 16,             "Frame Width  must be greater than 0 and a multiple of 16" );
    ROTREPORT( getFrameHeight           () <= 0 ||
               getFrameHeight           ()  % 16,             "Frame Height must be greater than 0 and a multiple of 16" );
    ROTREPORT( getMaximumFrameRate      () <= 0.0,            "Frame rate not supported" );
    ROTREPORT( getTotalFrames           () == 0,              "Total Number Of Frames must be greater than 0" );
    ROTREPORT( getSymbolMode            ()  > 1,              "Symbol mode not supported" );
    ROTREPORT( getEnable8x8Trafo        ()  > 1,              "The value for Enable8x8Transform is not supported" );
    ROTREPORT( getScalingMatricesPresent()  > 1,              "The value for ScalingMatricesPresent is not supported" );
    ROTREPORT( getBiPred8x8Disable      ()  > 1,              "The value for BiPredLT8x8Disable is not supported" );
    ROTREPORT( getMCBlks8x8Disable      ()  > 1,              "The value for MCBlocksLT8x8Disable is not supported" );
    ROTREPORT( getDPBSize               () == 0,              "DPBSize must be greater than 0" );
    ROTREPORT( getNumDPBRefFrames       () == 0 ||
               getNumDPBRefFrames       ()  > getDPBSize(),   "NumRefFrames must be greater than 0 and must not be greater than DPB size" );
    ROTREPORT( getLog2MaxFrameNum       ()  < 4 ||
               getLog2MaxFrameNum       ()  > 16,             "Log2MaxFrameNum must be in the range of [4..16]" );
    ROTREPORT( getLog2MaxPocLsb         ()  < 4 ||
               getLog2MaxPocLsb         ()  > 15,             "Log2MaxFrameNum must be in the range of [4..15]" );
    ROTREPORT( bStringNotOk,                              "Unvalid SequenceFormatString" );
    ROTREPORT( getMaxRefIdxActiveBL0    () <= 0 ||
               getMaxRefIdxActiveBL0    ()  > 15,             "Unvalid value for MaxRefIdxActiveBL0" );
    ROTREPORT( getMaxRefIdxActiveBL1    () <= 0 ||
               getMaxRefIdxActiveBL1    ()  > 15,             "Unvalid value for MaxRefIdxActiveBL1" );
    ROTREPORT( getMaxRefIdxActiveP      () <= 0 ||
               getMaxRefIdxActiveP      ()  > 15,             "Unvalid value for MaxRefIdxActiveP" );
    ROTREPORT( getConstrainedIntraPred  ()  > 1,              "Unvalid value for ConstrainedIntraPred" );

    return Err::m_nOK;
  }

  ROTREPORT( getNumberOfLayers() == 0, "Number of layer must be greate than 0" );
  ROTREPORT( getMaximumFrameRate() <= 0.0,              "Maximum frame rate not supported" );
  ROTREPORT( getMaximumDelay    ()  < 0.0,              "Maximum delay must be greater than or equal to 0" );
  ROTREPORT( getTotalFrames     () == 0,                "Total Number Of Frames must be greater than 0" );

  ROTREPORT( getGOPSize         ()  < 1  ||
             getGOPSize         ()  > 64,               "GOP Size not supported" );
  UInt uiDecStages = getLogFactor( 1.0, getGOPSize() );
  ROTREPORT( uiDecStages == MSYS_UINT_MAX,              "GOP Size must be a power of 2" );
  setDecompositionStages( uiDecStages );

  ROTREPORT( getIntraPeriod     ()  <
             getGOPSize         (),                     "Intra period must be greater or equal to GOP size" );
  if( getIntraPeriod() == MSYS_UINT_MAX )
  {
    setIntraPeriodLowPass( MSYS_UINT_MAX );
  }
  else
  {
    UInt uiIntraPeriod = getIntraPeriod() / getGOPSize() - 1;
    ROTREPORT( getIntraPeriod() % getGOPSize(),         "Intra period must be a power of 2 of GOP size (or -1)" );
    setIntraPeriodLowPass( uiIntraPeriod );
  }

  ROTREPORT( getNumRefFrames    ()  < 1  ||
             getNumRefFrames    ()  > 15,               "Number of reference frames not supported" );
  ROTREPORT( getBaseLayerMode   ()  > 2,                "Base layer mode not supported" );
  ROTREPORT( getNumberOfLayers  ()  > MAX_LAYERS,       "Number of layers not supported" );

  if( m_uiCIUFlag )
  {
    UInt uiILFIdc = m_cInterLayerLoopFilterParams.getFilterIdc();
    ROTREPORT( uiILFIdc != 1 && uiILFIdc != 2 && uiILFIdc != 5, "Inter-layer deblocking filter idc not supported for constrained intra upsampling" );
  }

  ROTREPORT( m_uiEncodeKeyPictures    > 2,          "Key picture mode not supported" );
  ROTREPORT( m_uiMGSKeyPictureControl > 2,          "Unsupported value for MGSControl" );
  ROTREPORT( m_uiMGSKeyPictureControl &&
            !m_uiCGSSNRRefinementFlag,              "MGSControl can only be specified in connection with CGSSNRRefinement=1" );

  // JVT-AD021 {
  ROTREPORT(  getMultiLayerLambda() > MULTILAYER_LAMBDA_C08 , "Multi-Layer lambda selection mode not supported" );
  // JVT-AD021 }

  Double  dMaxFrameDelay  = gMax( 0, m_dMaximumFrameRate * m_dMaximumDelay / 1000.0 );
  UInt    uiMaxFrameDelay = (UInt)floor( dMaxFrameDelay );
  Double  dMinUnrstrDelay = Double( ( 1 << m_uiDecompositionStages ) - 1 ) / m_dMaximumFrameRate * 1000.0;

  for( UInt uiLayer = 0; uiLayer < getNumberOfLayers(); uiLayer++ )
  {
    LayerParameters*  pcLayer               = &m_acLayerParameters[uiLayer];

	  RNOK( pcLayer->check() );

    UInt              uiBaseLayerId         = uiLayer && pcLayer->getBaseLayerId() != MSYS_UINT_MAX ? pcLayer->getBaseLayerId() : MSYS_UINT_MAX;
    LayerParameters*  pcBaseLayer           = uiBaseLayerId != MSYS_UINT_MAX ? &m_acLayerParameters[uiBaseLayerId] : 0;
    UInt              uiLogFactorInOutRate  = getLogFactor( pcLayer->getOutputFrameRate (), pcLayer->getInputFrameRate() );
    UInt              uiLogFactorMaxInRate  = getLogFactor( pcLayer->getInputFrameRate  (), getMaximumFrameRate       () );

    // heiko.schwarz@hhi.fhg.de: add some additional check for input/output frame rates
    ROTREPORT( pcLayer->getInputFrameRate() < pcLayer->getOutputFrameRate(),  "Input frame rate must not be less than output frame rate" );
    ROTREPORT( pcLayer->getInputFrameRate() > getMaximumFrameRate(),          "Input frame rate must not be greater than maximum frame rate" );
    ROTREPORT( getDecompositionStages() < uiLogFactorMaxInRate + uiLogFactorInOutRate, "Number of decomposition stages is too small for the specified output rate" );

    ROTREPORT( uiLogFactorInOutRate == MSYS_UINT_MAX,   "Input frame rate must be a power of 2 of output frame rate" );
    ROTREPORT( uiLogFactorMaxInRate == MSYS_UINT_MAX,   "Maximum frame rate must be a power of 2 of input frame rate" );

    if( pcLayer->getUseLongTerm() && m_dMaximumDelay < dMinUnrstrDelay )
    {
      fprintf( stderr, "\nWARNING: The usage of long term pictures for"
                       "\n         a delay of less than %.2lf ms might"
                       "\n         result in non-conforming DPB behaviour"
                       "\n         for temporal sub-streams\n\n", dMinUnrstrDelay );
    }

    pcLayer->setNotCodedStages      ( uiLogFactorInOutRate );
    pcLayer->setTemporalResolution  ( uiLogFactorMaxInRate );
    pcLayer->setDecompositionStages ( getDecompositionStages() - uiLogFactorMaxInRate );
    pcLayer->setFrameDelay          ( uiMaxFrameDelay );
    if( ( uiMaxFrameDelay >> ( uiLogFactorMaxInRate + uiLogFactorInOutRate ) ) == 0 || ( getDecompositionStages() - uiLogFactorMaxInRate - uiLogFactorInOutRate ) == 0 )
    {
      pcLayer->setPicCodingType( 1 );
    }
    if( pcLayer->getPicCodingType() == 1 )
    {
      pcLayer->setBiPred8x8Disable( 1 );
    }
    if( uiBaseLayerId != MSYS_UINT_MAX )
    {
      ROTREPORT( pcLayer->getPicCodingType() == 1 && pcBaseLayer->getPicCodingType() != 1, "DisableBSlices must be equal to 0 when it is equal to 0 for the base layer" );
    }

    Bool bMGSVectorUsed = pcLayer->getMGSVect( 0 ) != 16;
    if( bMGSVectorUsed )
    {
      ROTREPORT( !getCGSSNRRefinement(),    "MGS vectors are only supported in MGS." );
      ROTREPORT( !pcBaseLayer,              "MGS vectors are not allowed in the base layer." );
      ROTREPORT( !pcLayer->getILPredMode(), "MGS vectors cannot be used with ILModePred = 0." );
      pcLayer->setConstrainedIntraPred( 1 );
    }

    ROTREPORT( ! pcBaseLayer && pcLayer->getSliceSkip(), "Slice skip only supported in enhancement layers" );

    ROTREPORT( pcLayer->getUseLongTerm() && ! pcLayer->getMMCOEnable(),
      "UseLongTerm cannot be equal to 1 when MMCOEnable is equal to 0" );

    if( pcBaseLayer )
    {
      Bool bResolutionChange = pcLayer->getFrameWidthInSamples () != pcBaseLayer->getFrameWidthInSamples () ||
                               pcLayer->getFrameHeightInSamples() != pcBaseLayer->getFrameHeightInSamples();
      ROTREPORT( bResolutionChange && pcLayer->getMGSVect(0) != 16, "Base layer and current layer must have the same resolution when MGS vectors are used in the current layer." );
      ROTREPORT( pcLayer->getInputFrameRate() < pcBaseLayer->getInputFrameRate(), "Input frame rate less than base layer output frame rate" );
      UInt uiLogFactorRate = getLogFactor( pcBaseLayer->getInputFrameRate(), pcLayer->getInputFrameRate() );
      ROTREPORT( uiLogFactorRate == MSYS_UINT_MAX, "Input Frame rate must be a power of 2 from layer to layer" );

      if( bResolutionChange && m_uiCIUFlag )
      {
        ROTREPORT( pcLayer->getILPredMode() == 1, "ILModePred == 1 not allowed in connection with ConstrainedIntraUps == 1 and resolution changes" );
      }

      ROTREPORT( m_uiCGSSNRRefinementFlag && !bResolutionChange && pcLayer->getPicCodingType() != pcBaseLayer->getPicCodingType(),
        "DisableBSlices shall be the same in successive MGS layers" );

      ROTREPORT( m_uiCGSSNRRefinementFlag && !bResolutionChange && pcLayer->getUseLongTerm() != pcBaseLayer->getUseLongTerm(),
        "UseLongTerm shall be the same in successive MGS layers" );

      ROTREPORT( m_uiCGSSNRRefinementFlag && !bResolutionChange && pcLayer->getMMCOEnable() != pcBaseLayer->getMMCOEnable(),
        "MMCOEnable shall be the same in successive MGS layers" );

      ROTREPORT( m_uiCGSSNRRefinementFlag && !bResolutionChange && pcLayer->getMMCOBaseEnable() != pcBaseLayer->getMMCOBaseEnable(),
        "MMCOBaseEnable shall be the same in successive MGS layers" );

      ROTREPORT( m_uiCGSSNRRefinementFlag && !bResolutionChange && !pcLayer->getInterLayerPredictionMode(), "InterLayerPred must not be 0 in MGS enhancement layers" );

      if( pcLayer->getILPredMode() == 1 )
      {
        ROTREPORT( pcLayer->getLayerIntraPeriod () != pcBaseLayer->getLayerIntraPeriod(), "ILModePred must not be 1 when layers have different intra periods" );
        ROTREPORT( pcLayer->getIDRPeriod        () != pcBaseLayer->getIDRPeriod       (), "ILModePred must not be 1 when layers have different IDR periods" );
      }
      if( getCGSSNRRefinement() && !bResolutionChange )
      {
        ROTREPORT( pcLayer->getLayerIntraPeriod () != pcBaseLayer->getLayerIntraPeriod(), "intra periods must be the same in successive MGS layers" );
        ROTREPORT( pcLayer->getIDRPeriod        () != pcBaseLayer->getIDRPeriod       (), "IDR periods must be the same in successive MGS layers" );
      }

      ResizeParameters& rcRP = pcLayer->getResizeParameters();
      if( rcRP.m_iExtendedSpatialScalability == ESS_SEQ )
      {
        ROTREPORT( rcRP.m_iScaledRefFrmWidth  < rcRP.m_iRefLayerFrmWidth,  "Scaled frame width  less than base layer frame width" );
        ROTREPORT( rcRP.m_iScaledRefFrmHeight < rcRP.m_iRefLayerFrmHeight, "Scaled frame height less than base layer frame height" );
        Bool  bI    = pcLayer->isInterlaced();
        Int   iV    = ( bI ? 4 : 2 );
        Int   iL    = rcRP.m_iLeftFrmOffset;
        Int   iT    = rcRP.m_iTopFrmOffset;
        Int   iR    = rcRP.m_iFrameWidth  - rcRP.m_iScaledRefFrmWidth  - iL;
        Int   iB    = rcRP.m_iFrameHeight - rcRP.m_iScaledRefFrmHeight - iT;
        Bool  bHor  = ( iL %  2 != 0 || iR %  2 != 0 );
        Bool  bVer  = ( iT % iV != 0 || iB % iV != 0 );
        ROTREPORT( bHor,        "Cropping window must be horizonzally aligned on a 2 pixel grid" );
        ROTREPORT( bVer && !bI, "Cropping window must be vertically aligned on a 2 pixel grid" );
        ROTREPORT( bVer &&  bI, "Cropping window must be vertically aligned on a 4 pixel grid for interlaced configurations" );
      }
      else if( rcRP.m_iExtendedSpatialScalability == ESS_NONE )
      {
        ROTREPORT( pcLayer->getFrameWidthInSamples ()  < pcBaseLayer->getFrameWidthInSamples (), "Scaled frame width  less than base layer frame width" );
        ROTREPORT( pcLayer->getFrameHeightInSamples()  < pcBaseLayer->getFrameHeightInSamples(), "Scaled frame height less than base layer frame height" );
      }

      pcBaseLayer->setConstrainedIntraPred( 1 );

      if( pcLayer->getSliceSkip() )
      {
        pcLayer->setSliceSkipTLevelStart( pcLayer->getSliceSkip() - 1 );
        pcLayer->setConstrainedIntraPred( 1 );
      }

      if( pcLayer->getTCoeffLevelPredictionFlag() )
      {
        Bool bSpatial = pcLayer->getResizeParameters().getSpatialResolutionChangeFlag();
        ROTREPORT( bSpatial,              "AVCRewriteFlag cannot be equal to 1 for a spatial enhancement layer" );
        ROTREPORT( m_uiEncodeKeyPictures, "Key pictures are not supported in connection with AVCRewriteFlag" );
      }
    }
    if( pcLayer->getTCoeffLevelPredictionFlag() )
    {
      pcLayer->setConstrainedIntraPred( 1 );
      pcLayer->setILPredMode( pcLayer->getILPredMode() > 0 ? 2 : 0 );
    }
  }

  RNOK( xCheckAndSetProfiles() );

  return Err::m_nOK;
}


ErrVal
CodingParameter::xCheckAndSetProfiles()
{
  for( UInt uiLayer = 0; uiLayer < m_uiNumberOfLayers; uiLayer++ )
  {
    RNOK( m_acLayerParameters[uiLayer].setAndCheckProfile( this ) );
  }
  return Err::m_nOK;
}

ErrVal
LayerParameters::setAndCheckProfile( CodingParameter* pcCodingParameter )
{
  ROF( pcCodingParameter );

  Bool  bSVCLayersPresent     = ( pcCodingParameter->getNumberOfLayers() > 1 );
  Bool  bScalBaselineRequired = false;
  Bool  bScalHighRequired     = false;
  for( UInt uiTestLayer = m_uiLayerId + 1; uiTestLayer < pcCodingParameter->getNumberOfLayers(); uiTestLayer++ )
  {
    if( pcCodingParameter->getLayerParameters( uiTestLayer ).getProfileIdc() == SCALABLE_BASELINE_PROFILE )
    {
      bScalBaselineRequired = true;
    }
    if( pcCodingParameter->getLayerParameters( uiTestLayer ).getProfileIdc() == SCALABLE_HIGH_PROFILE )
    {
      bScalHighRequired     = true;
    }
  }

  //===== BASE LAYER =====
  if ( m_uiLayerId == 0 )
  {
    ROTREPORT( m_uiProfileIdc != BASELINE_PROFILE &&
               m_uiProfileIdc != MAIN_PROFILE     &&
               m_uiProfileIdc != EXTENDED_PROFILE &&
               m_uiProfileIdc != HIGH_PROFILE     &&
               m_uiProfileIdc != 0, "Unsupported ProfileIdc in base layer" );
    //----- try to force compatibility when required -----
    Bool  bForceBaseline    = ( m_uiProfileIdc == BASELINE_PROFILE || bScalBaselineRequired );
    Bool  bForceMain        = ( m_uiProfileIdc == MAIN_PROFILE     || bScalBaselineRequired );
    Bool  bForceExtended    = ( m_uiProfileIdc == EXTENDED_PROFILE || bScalBaselineRequired );
    Bool  bForceHigh        = ( m_uiProfileIdc == HIGH_PROFILE     || bSVCLayersPresent     );
    if(   bForceBaseline )  RNOK( xForceBaselineProfile ( pcCodingParameter ) );
    if(   bForceMain     )  RNOK( xForceMainProfile     ( pcCodingParameter ) );
    if(   bForceExtended )  RNOK( xForceExtendedProfile ( pcCodingParameter ) );
    if(   bForceHigh     )  RNOK( xForceHighProfile     ( pcCodingParameter ) );
    //----- check compatibility -----
    Bool  bBaseline         = xIsBaselineProfile        ( pcCodingParameter );
    Bool  bMain             = xIsMainProfile            ( pcCodingParameter );
    Bool  bExtended         = xIsExtendedProfile        ( pcCodingParameter );
    Bool  bHigh             = xIsHighProfile            ( pcCodingParameter );
    Bool  bIntraOnly        = xIsIntraOnly              ( pcCodingParameter );
    ROT(  bForceBaseline  && !bBaseline );
    ROT(  bForceMain      && !bMain     );
    ROT(  bForceExtended  && !bExtended );
    ROT(  bForceHigh      && !bHigh     );
    //----- set parameters -----
    if( m_uiProfileIdc == 0)
    {
      m_uiProfileIdc        = ( bBaseline ? BASELINE_PROFILE : bMain ? MAIN_PROFILE : bExtended ? EXTENDED_PROFILE : bHigh ? HIGH_PROFILE : 0 );
      ROTREPORT( m_uiProfileIdc == 0, "No profile found for given configuration parameters" );
    }
    m_bIntraOnly            = bIntraOnly;
    m_bConstrainedSetFlag0  = bBaseline;
    m_bConstrainedSetFlag1  = bMain;
    m_bConstrainedSetFlag2  = bExtended;
    m_bConstrainedSetFlag3  = ( m_uiProfileIdc == HIGH_PROFILE && bIntraOnly );
    m_uiNumDependentDId     = 1;
    //----- update IDR period -----
    if( bIntraOnly )
    {
      m_iIDRPeriod          = pcCodingParameter->getGOPSize();
      m_iLayerIntraPeriod   = pcCodingParameter->getGOPSize();
    }
    return Err::m_nOK;
  }

  //===== ENHANCEMENT LAYER =====
  ROTREPORT( m_uiProfileIdc != SCALABLE_BASELINE_PROFILE &&
             m_uiProfileIdc != SCALABLE_HIGH_PROFILE     &&
             m_uiProfileIdc != 0, "Unsupported ProfileIdc in enhancement layer" );
  //----- try to force compatibility when required -----
  Bool  bForceScalBase    = ( m_uiProfileIdc == SCALABLE_BASELINE_PROFILE || bScalBaselineRequired );
  Bool  bForceScalHigh    = ( m_uiProfileIdc == SCALABLE_HIGH_PROFILE     || bScalHighRequired     );
  if(   bForceScalBase )  RNOK( xForceScalableBaselineProfile ( pcCodingParameter ) );
  if(   bForceScalHigh )  RNOK( xForceScalableHighProfile     ( pcCodingParameter ) );
  //----- check compatibility -----
  Bool  bScalBase         = xIsScalableBaselineProfile        ( pcCodingParameter );
  Bool  bScalHigh         = xIsScalableHighProfile            ( pcCodingParameter );
  Bool  bIntraOnly        = xIsIntraOnly                      ( pcCodingParameter );
  ROT(  bForceScalBase  && !bScalBase );
  ROT(  bForceScalHigh  && !bScalHigh );
  //----- set parameters -----
  if( m_uiProfileIdc == 0)
  {
    m_uiProfileIdc        = ( bScalBase ? SCALABLE_BASELINE_PROFILE : bScalHigh ? SCALABLE_HIGH_PROFILE : 0 );
    ROTREPORT( m_uiProfileIdc == 0, "No profile found for given configuration parameters" );
  }
  m_bIntraOnly            = bIntraOnly;
  m_bConstrainedSetFlag0  = bScalBase;
  m_bConstrainedSetFlag1  = bScalHigh;
  m_bConstrainedSetFlag2  = false;
  m_bConstrainedSetFlag3  = ( m_uiProfileIdc == SCALABLE_HIGH_PROFILE && bIntraOnly );
  m_uiNumDependentDId     = 1;
  if( m_uiBaseLayerId != MSYS_UINT_MAX )
  {
    if( pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).m_uiLayerCGSSNR == m_uiLayerCGSSNR )
    {
      m_uiNumDependentDId = 0;
    }
    m_uiNumDependentDId  += pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).m_uiNumDependentDId;
    ROTREPORT( m_uiNumDependentDId > 3, "At most 3 dependent dependency layers are supported in SVC" );
  }
  //----- update picture coding type -----
  if( m_uiBaseLayerId != MSYS_UINT_MAX )
  {
    if( m_uiQualityLevelCGSSNR && pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).getPicCodingType() == 1 && m_uiPicCodingType != 1 )
    {
      SETREPORT( m_uiPicCodingType, 1, "B slices disabled for MGS enhancement" );
    }
    if( m_uiPicCodingType == 0 && pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).getPicCodingType() != 0 )
    {
      m_uiPicCodingType = 2;
    }
  }
  //----- update IDR period -----
  if( bIntraOnly )
  {
    m_iIDRPeriod          = pcCodingParameter->getGOPSize();
    m_iLayerIntraPeriod   = pcCodingParameter->getGOPSize();
  }
  return Err::m_nOK;
}

UInt getMaxLevel( UInt uiLevel1, UInt uiLevel2 )
{
  if( ( uiLevel1 == 9 && uiLevel2 == 10 ) || ( uiLevel1 == 10 && uiLevel2 == 9 ) )
  {
    return 10;
  }
  return gMax( uiLevel1, uiLevel2 );
}

ErrVal
LayerParameters::updateWithLevel( CodingParameter* pcCodingParameter, UInt& ruiLevelIdc )
{
  if( m_uiLevelIdc )
  {
    switch( m_uiLevelIdc )
    {
    case  9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 20:
    case 21:
    case 22:
    case 30:
    case 31:
    case 32:
    case 40:
    case 41:
    case 42:
    case 50:
    case 51:
      break;
    default:
      ROTREPORT( true, "Unknown value for MinLevelIdc" );
      break;
    }
    ruiLevelIdc = getMaxLevel( ruiLevelIdc, m_uiLevelIdc );
  }
  if( m_uiBaseLayerId != MSYS_UINT_MAX )
  {
    UInt uiBaseLevel  = pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).getLevelIdc();
    ROF( uiBaseLevel );
    ruiLevelIdc       = getMaxLevel( ruiLevelIdc, uiBaseLevel );
  }
  switch( m_uiProfileIdc )
  {
  case MAIN_PROFILE:
  case EXTENDED_PROFILE:
  case HIGH_PROFILE:
  case SCALABLE_HIGH_PROFILE:
    if( ruiLevelIdc < 21 && isInterlaced() )
    {
      ruiLevelIdc = 21;
    }
    else if( ruiLevelIdc > 41 )
    {
      SETREPORT( m_uiMbAff, 0, "MbAff disabled for level compatibility" );
      SETREPORT( m_uiPAff,  0, "PAff disabled for level compatibility" );
    }
    break;
  case SCALABLE_BASELINE_PROFILE:
    if( ruiLevelIdc < 21 && ( m_uiEntropyCodingModeFlag || m_uiEnable8x8Trafo ) )
    {
      ruiLevelIdc = 21;
    }
    break;
  default:
    break;
  }
  m_uiLevelIdc = ruiLevelIdc;
  return Err::m_nOK;
}

Bool
LayerParameters::xIsBaselineProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId                    == 0,                       false );
  ROFRS( m_uiBaseLayerId                == MSYS_UINT_MAX,           false );
  ROFRS( m_uiPicCodingType              == 1,                       false );
  ROFRS( m_uiMbAff                      == 0,                       false );
  ROFRS( m_uiPAff                       == 0,                       false );
  ROFRS( m_uiScalingMatricesPresent     == 0,                       false );
  ROFRS( pcCodingParameter->m_uiIPMode  == 0,                       false );
  ROFRS( pcCodingParameter->m_uiIPMode  == 0,                       false );
  ROFRS( m_uiEntropyCodingModeFlag      == 0,                       false );
  ROFRS( m_uiNumSliceGroupsMinus1       <= 7,                       false );
  ROFRS( m_uiEnable8x8Trafo             == 0,                       false );
  ROFRS( m_i2ndChromaQPIndexOffset      == m_iChromaQPIndexOffset,  false );
  return true;
}
ErrVal
LayerParameters::xForceBaselineProfile( CodingParameter* pcCodingParameter )
{
  ROT( m_uiLayerId );
  ROF( m_uiBaseLayerId == MSYS_UINT_MAX );
  ROTREPORT( m_uiNumSliceGroupsMinus1     > 7,  "NumSliceGrpMns1 must be less than 8 in Baseline profile compatibility mode" );
  SETREPORT( m_uiPicCodingType,             1,  "B slices disabled for Baseline profile compatibility" );
  SETREPORT( m_uiMbAff,                     0,  "MbAff disabled for Baseline profile compatibility" );
  SETREPORT( m_uiPAff,                      0,  "PAff disabled for Baseline profile compatibility" );
  SETREPORT( m_uiScalingMatricesPresent,    0,  "Scaling matrices disabled for Baseline profile compatibility" );
  SETREPORT( pcCodingParameter->m_uiIPMode, 0,  "Weighted prediction disabled for Baseline profile compatibility" );
  SETREPORT( pcCodingParameter->m_uiIPMode, 0,  "Weighted prediction disabled for Baseline profile compatibility" );
  SETREPORT( m_uiEntropyCodingModeFlag,     0,  "CABAC disabled for Baseline profile compatibility" );
  SETREPORT( m_uiEnable8x8Trafo,            0,  "8x8 transform disabled for Baseline profile compatibility" );
  SETREPORT( m_i2ndChromaQPIndexOffset,
             m_iChromaQPIndexOffset,            "2nd chroma QP index offset ignored for Baseline profile compatibility" );
  return Err::m_nOK;
}

Bool
LayerParameters::xIsMainProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId                    == 0,                       false );
  ROFRS( m_uiBaseLayerId                == MSYS_UINT_MAX,           false );
  ROFRS( m_uiScalingMatricesPresent     == 0,                       false );
  ROFRS( m_uiNumSliceGroupsMinus1       == 0,                       false );
  ROFRS( m_uiUseRedundantSlice          == 0,                       false );
  ROFRS( m_uiUseRedundantKeySlice       == 0,                       false );
  ROFRS( m_uiEnable8x8Trafo             == 0,                       false );
  ROFRS( m_i2ndChromaQPIndexOffset      == m_iChromaQPIndexOffset,  false );
  return true;
}
ErrVal
LayerParameters::xForceMainProfile( CodingParameter* pcCodingParameter )
{
  ROT( m_uiLayerId );
  ROF( m_uiBaseLayerId == MSYS_UINT_MAX );
  SETREPORT( m_uiScalingMatricesPresent,  0,  "Scaling matrices disabled for Main profile compatibility" );
  SETREPORT( m_uiNumSliceGroupsMinus1,    0,  "NumSliceGrpMns1 set to 0 for Main profile compatibility" );
  SETREPORT( m_uiUseRedundantSlice,       0,  "Redundant slices disabled for Main profile compatibility" );
  SETREPORT( m_uiUseRedundantKeySlice,    0,  "Redundant slices disabled for Main profile compatibility" );
  SETREPORT( m_uiEnable8x8Trafo,          0,  "8x8 transform disabled for Main profile compatibility" );
  SETREPORT( m_i2ndChromaQPIndexOffset,
             m_iChromaQPIndexOffset,          "2nd chroma QP index offset ignored for Main profile compatibility" );
  return Err::m_nOK;
}

Bool
LayerParameters::xIsExtendedProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId                    == 0,                       false );
  ROFRS( m_uiBaseLayerId                == MSYS_UINT_MAX,           false );
  ROFRS( m_uiScalingMatricesPresent     == 0,                       false );
  ROFRS( m_uiEntropyCodingModeFlag      == 0,                       false );
  ROFRS( m_uiNumSliceGroupsMinus1       <= 7,                       false );
  ROFRS( m_uiEnable8x8Trafo             == 0,                       false );
  ROFRS( m_i2ndChromaQPIndexOffset      == m_iChromaQPIndexOffset,  false );
  return true;
}
ErrVal
LayerParameters::xForceExtendedProfile( CodingParameter* pcCodingParameter )
{
  ROT( m_uiLayerId );
  ROF( m_uiBaseLayerId == MSYS_UINT_MAX );
  ROTREPORT( m_uiNumSliceGroupsMinus1   > 7,  "NumSliceGrpMns1 must be less than 8 in Extended profile compatibility mode" );
  SETREPORT( m_uiScalingMatricesPresent,  0,  "Scaling matrices disabled for Extended profile compatibility" );
  SETREPORT( m_uiEntropyCodingModeFlag,   0,  "CABAC disabled for Extended profile compatibility" );
  SETREPORT( m_uiEnable8x8Trafo,          0,  "8x8 transform disabled for Extended profile compatibility" );
  SETREPORT( m_i2ndChromaQPIndexOffset,
             m_iChromaQPIndexOffset,          "2nd chroma QP index offset ignored for Extended profile compatibility" );
  return Err::m_nOK;
}

Bool
LayerParameters::xIsHighProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId                    == 0,               false );
  ROFRS( m_uiBaseLayerId                == MSYS_UINT_MAX,   false );
  ROFRS( m_uiNumSliceGroupsMinus1       == 0,               false );
  ROFRS( m_uiUseRedundantSlice          == 0,               false );
  ROFRS( m_uiUseRedundantKeySlice       == 0,               false );
  return true;
}
ErrVal
LayerParameters::xForceHighProfile( CodingParameter* pcCodingParameter )
{
  ROT( m_uiLayerId );
  ROF( m_uiBaseLayerId == MSYS_UINT_MAX );
  SETREPORT( m_uiNumSliceGroupsMinus1,    0, "NumSliceGrpMns1 set to 0 for High profile compatibility" );
  SETREPORT( m_uiUseRedundantSlice,       0, "Redundant slices disabled for High profile compatibility" );
  SETREPORT( m_uiUseRedundantKeySlice,    0, "Redundant slices disabled for High profile compatibility" );
  return Err::m_nOK;
}

Bool
LayerParameters::xIsIntraOnly( CodingParameter* pcCodingParameter )
{
  ROTRS( m_uiUseLongTerm,                                                       false );
  ROFRS( m_uiDecompositionStages        == m_uiNotCodedStages,                  false );
  ROFRS( pcCodingParameter->m_uiGOPSize == pcCodingParameter->m_uiIntraPeriod ||
         pcCodingParameter->m_uiGOPSize == (UInt)m_iIDRPeriod                 ||
         pcCodingParameter->m_uiGOPSize == (UInt)m_iLayerIntraPeriod,           false );
  ROTRS( m_uiBaseLayerId                == MSYS_UINT_MAX,                       true  );
  return pcCodingParameter->getLayerParameters( m_uiBaseLayerId ).isIntraOnly();
}


Bool
LayerParameters::xHasRestrictedESS( CodingParameter* pcCodingParameter )
{
  ROTRS( m_uiBaseLayerId                            == MSYS_UINT_MAX, true  );
  ROFRS( m_cResizeParameters.m_iLeftFrmOffset % 16  == 0,             false );
  ROFRS( m_cResizeParameters.m_iTopFrmOffset  % 16  == 0,             false );
  Bool   bHor11 = (     m_cResizeParameters.m_iScaledRefFrmWidth  ==     m_cResizeParameters.m_iRefLayerFrmWidth  );
  Bool   bHor23 = ( 2 * m_cResizeParameters.m_iScaledRefFrmWidth  == 3 * m_cResizeParameters.m_iRefLayerFrmWidth  );
  Bool   bHor12 = (     m_cResizeParameters.m_iScaledRefFrmWidth  == 2 * m_cResizeParameters.m_iRefLayerFrmWidth  );
  Bool   bVer11 = (     m_cResizeParameters.m_iScaledRefFrmHeight ==     m_cResizeParameters.m_iRefLayerFrmHeight );
  Bool   bVer23 = ( 2 * m_cResizeParameters.m_iScaledRefFrmHeight == 3 * m_cResizeParameters.m_iRefLayerFrmHeight );
  Bool   bVer12 = (     m_cResizeParameters.m_iScaledRefFrmHeight == 2 * m_cResizeParameters.m_iRefLayerFrmHeight );
  UInt   uiHor  = ( bHor11 ? 11 : bHor23 ? 23 : bHor12 ? 12 : (UInt)'H' );
  UInt   uiVer  = ( bVer11 ? 11 : bVer23 ? 23 : bVer12 ? 12 : (UInt)'V' );
  ROFRS( uiHor == uiVer,                                              false );
  return true;
}

Bool
LayerParameters::xIsScalableBaselineProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId               > 0,                       false );
  ROFRS( m_uiMbAff                == 0,                       false );
  ROFRS( m_uiPAff                 == 0,                       false );
  ROFRS( m_uiNumSliceGroupsMinus1 <= 7,                       false );
  if   ( m_uiNumSliceGroupsMinus1 )
  {
    ROFRS( m_uiSliceGroupMapType  == 2,                       false );
  }
  ROFRS( m_uiBiPred8x8Disable     == 1,                       false );
  ROTRS( m_uiBaseLayerId          == MSYS_UINT_MAX,           true  );

  ROFRS( xHasRestrictedESS( pcCodingParameter ),              false );
  LayerParameters& rcBase = pcCodingParameter->getLayerParameters( m_uiBaseLayerId );
  if( m_uiBaseLayerId == 0 )
  {
    ROFRS( rcBase.m_bConstrainedSetFlag0,                     false );
    ROFRS( rcBase.m_bConstrainedSetFlag1,                     false );
    ROFRS( rcBase.m_bConstrainedSetFlag2,                     false );
    ROFRS( rcBase.m_uiProfileIdc == BASELINE_PROFILE ||
           rcBase.m_uiProfileIdc == MAIN_PROFILE     ||
           rcBase.m_uiProfileIdc == EXTENDED_PROFILE,         false );
    return true;
  }

  ROTRS( rcBase.m_uiProfileIdc == SCALABLE_BASELINE_PROFILE,  true  );
  ROTRS( rcBase.m_bConstrainedSetFlag0,                       true  );
  return false;
}

ErrVal
LayerParameters::xForceScalableBaselineProfile( CodingParameter* pcCodingParameter )
{
  ROF( m_uiLayerId );
  ROTREPORT( m_uiNumSliceGroupsMinus1     > 7,          "NumSliceGrpMns1 must be less than 8 in Scalable Baseline profile compatibility mode" );
  if( m_uiNumSliceGroupsMinus1 )
  {
    ROTREPORT( m_uiSliceGroupMapType     != 2,          "SliceGroupMapType must be equal to 2 in Scalable Baseline profile compatibility mode" );
  }
  SETREPORT( m_uiMbAff,                     0,          "MbAff disabled for Scalable Baseline profile compatibility" );
  SETREPORT( m_uiPAff,                      0,          "PAff disabled for Scalable Baseline profile compatibility" );
  SETREPORT( m_uiBiPred8x8Disable,          1,          "BiPred for blocks smaller than 8x8 disabled for Scalable Baseline profile compatibility" );
  ROTRS( m_uiBaseLayerId == MSYS_UINT_MAX,  Err::m_nOK  );
  ROTREPORT( ! xHasRestrictedESS( pcCodingParameter ),  "Spatial scalability parameters must be restricted for Scalable Baseline profile compatibility mode" );
  return Err::m_nOK;
}

Bool
LayerParameters::xIsScalableHighProfile( CodingParameter* pcCodingParameter )
{
  ROFRS( m_uiLayerId               > 0,                       false );
  ROFRS( m_uiNumSliceGroupsMinus1 == 0,                       false );
  ROFRS( m_uiUseRedundantSlice    == 0,                       false );
  ROFRS( m_uiUseRedundantKeySlice == 0,                       false );
  ROTRS( m_uiBaseLayerId          == MSYS_UINT_MAX,           true  );

  LayerParameters& rcBase = pcCodingParameter->getLayerParameters( m_uiBaseLayerId );
  if( m_uiBaseLayerId == 0 )
  {
    ROTRS( rcBase.m_uiProfileIdc == HIGH_PROFILE,             true  );
    ROFRS( rcBase.m_bConstrainedSetFlag1,                     false );
    ROFRS( rcBase.m_uiProfileIdc == BASELINE_PROFILE ||
           rcBase.m_uiProfileIdc == MAIN_PROFILE     ||
           rcBase.m_uiProfileIdc == EXTENDED_PROFILE,         false );
    return true;
  }

  ROTRS( rcBase.m_uiProfileIdc == SCALABLE_HIGH_PROFILE,      true  );
  ROTRS( rcBase.m_bConstrainedSetFlag1,                       true  );
  return false;
}
ErrVal
LayerParameters::xForceScalableHighProfile( CodingParameter* pcCodingParameter )
{
  ROF( m_uiLayerId );
  SETREPORT( m_uiNumSliceGroupsMinus1,    0, "NumSliceGrpMns1 set to 0 for Scalable High profile compatibility" );
  SETREPORT( m_uiUseRedundantSlice,       0, "Redundant slices disabled for Scalable High profile compatibility" );
  SETREPORT( m_uiUseRedundantKeySlice,    0, "Redundant slices disabled for Scalable High profile compatibility" );
  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
