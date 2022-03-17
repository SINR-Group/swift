
#include "H264AVCCommonLib.h"

#include "H264AVCCommonLib/SliceHeader.h"
#include "H264AVCCommonLib/PictureParameterSet.h"
#include "H264AVCCommonLib/Frame.h"



H264AVC_NAMESPACE_BEGIN


SliceHeader::SliceHeader()
: m_eErrorConcealMode   ( EC_NONE )
, m_bTrueSlice          ( true )
, m_uiNumMbsInSlice     ( 0 )
, m_uiLastMbInSlice     ( 0 )
, m_iTopFieldPoc        ( 0 )
, m_iBotFieldPoc        ( 0 )
, m_bSCoeffResidualPred ( false )
//>>> remove
, m_uiLayerCGSSNR                     ( 0 )
, m_uiQualityLevelCGSSNR              ( 0 )
, m_uiBaseLayerCGSSNR                 ( MSYS_UINT_MAX )
, m_uiBaseQualityLevelCGSSNR          ( 0 )
, m_uiBaseLayerId                     ( MSYS_UINT_MAX )
//<<< remove
, m_iLongTermFrameIdx   ( -1 )
, m_bReconstructionLayer( false )
, m_bAdaptiveILPred( false )
, m_pcBaseSliceHeader ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
  ::memset( m_aapcRefFrameList, 0x00, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    ::memset( m_aauiNumRefIdxActiveUpdate[ui] , 0x00, 2*sizeof(UInt));
  }
}

SliceHeader::SliceHeader( const NalUnitHeader& rcNalUnitHeader )
: SliceHeaderSyntax     ( rcNalUnitHeader )
, m_eErrorConcealMode   ( EC_NONE )
, m_bTrueSlice          ( true )
, m_uiNumMbsInSlice     ( 0 )
, m_uiLastMbInSlice     ( 0 )
, m_iTopFieldPoc        ( 0 )
, m_iBotFieldPoc        ( 0 )
, m_bSCoeffResidualPred ( false )
//>>> remove
, m_uiLayerCGSSNR                     ( 0 )
, m_uiQualityLevelCGSSNR              ( 0 )
, m_uiBaseLayerCGSSNR                 ( MSYS_UINT_MAX )
, m_uiBaseQualityLevelCGSSNR          ( 0 )
, m_uiBaseLayerId                     ( MSYS_UINT_MAX )
//<<< remove
, m_iLongTermFrameIdx   ( -1 )
, m_bReconstructionLayer( false )
, m_bAdaptiveILPred( false )
, m_pcBaseSliceHeader ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
  ::memset( m_aapcRefFrameList, 0x00, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    ::memset( m_aauiNumRefIdxActiveUpdate[ui] , 0x00, 2*sizeof(UInt));
  }
}

SliceHeader::SliceHeader( const PrefixHeader& rcPrefixHeader )
: SliceHeaderSyntax     ( rcPrefixHeader )
, m_eErrorConcealMode   ( EC_NONE )
, m_bTrueSlice          ( true )
, m_uiNumMbsInSlice     ( 0 )
, m_uiLastMbInSlice     ( 0 )
, m_iTopFieldPoc        ( 0 )
, m_iBotFieldPoc        ( 0 )
, m_bSCoeffResidualPred ( false )
//>>> remove
, m_uiLayerCGSSNR                     ( 0 )
, m_uiQualityLevelCGSSNR              ( 0 )
, m_uiBaseLayerCGSSNR                 ( MSYS_UINT_MAX )
, m_uiBaseQualityLevelCGSSNR          ( 0 )
, m_uiBaseLayerId                     ( MSYS_UINT_MAX )
//<<< remove
, m_iLongTermFrameIdx   ( -1 )
, m_bReconstructionLayer( false )
, m_bAdaptiveILPred( false )
, m_pcBaseSliceHeader ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
  ::memset( m_aapcRefFrameList, 0x00, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    ::memset( m_aauiNumRefIdxActiveUpdate[ui] , 0x00, 2*sizeof(UInt));
  }
}

SliceHeader::SliceHeader( const SequenceParameterSet& rcSPS, const PictureParameterSet& rcPPS )
: SliceHeaderSyntax     ( rcSPS, rcPPS )
, m_eErrorConcealMode   ( EC_NONE )
, m_bTrueSlice          ( true )
, m_uiNumMbsInSlice     ( 0 )
, m_uiLastMbInSlice     ( 0 )
, m_iTopFieldPoc        ( 0 )
, m_iBotFieldPoc        ( 0 )
, m_bSCoeffResidualPred ( false )
//>>> remove
, m_uiLayerCGSSNR                     ( 0 )
, m_uiQualityLevelCGSSNR              ( 0 )
, m_uiBaseLayerCGSSNR                 ( MSYS_UINT_MAX )
, m_uiBaseQualityLevelCGSSNR          ( 0 )
, m_uiBaseLayerId                     ( MSYS_UINT_MAX )
//<<< remove
, m_iLongTermFrameIdx   ( -1 )
, m_bReconstructionLayer( false )
, m_bAdaptiveILPred( false )
, m_pcBaseSliceHeader ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
  ::memset( m_aapcRefFrameList, 0x00, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    ::memset( m_aauiNumRefIdxActiveUpdate[ui] , 0x00, 2*sizeof(UInt));
  }
}

SliceHeader::SliceHeader( const SliceHeader& rcSliceHeader )
: SliceHeaderSyntax         ( rcSliceHeader )
, m_eErrorConcealMode       ( EC_NONE )
, m_bTrueSlice              ( true )
, m_uiNumMbsInSlice         ( 0 )
, m_uiLastMbInSlice         ( rcSliceHeader.m_uiLastMbInSlice )
, m_iTopFieldPoc            ( rcSliceHeader.m_iTopFieldPoc )
, m_iBotFieldPoc            ( rcSliceHeader.m_iBotFieldPoc )
, m_bSCoeffResidualPred     ( rcSliceHeader.m_bSCoeffResidualPred )
//>>> remove
, m_uiLayerCGSSNR                     ( 0 )
, m_uiQualityLevelCGSSNR              ( 0 )
, m_uiBaseLayerCGSSNR                 ( MSYS_UINT_MAX )
, m_uiBaseQualityLevelCGSSNR          ( 0 )
, m_uiBaseLayerId                     ( MSYS_UINT_MAX )
//<<< remove
, m_iLongTermFrameIdx   ( -1 )
, m_bReconstructionLayer( false )
, m_bAdaptiveILPred( false )
, m_pcBaseSliceHeader ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
  memcpy( m_aapcRefFrameList, rcSliceHeader.m_aapcRefFrameList, sizeof( m_aapcRefFrameList ) );
  memcpy( m_aauiNumRefIdxActiveUpdate, rcSliceHeader.m_aauiNumRefIdxActiveUpdate, sizeof( m_aauiNumRefIdxActiveUpdate ) );
  m_cFMO = rcSliceHeader.m_cFMO;
}

SliceHeader::~SliceHeader()
{
}

ErrVal
SliceHeader::init( const SequenceParameterSet& rcSPS, const PictureParameterSet& rcPPS )
{
  RNOK( SliceHeaderSyntax::init( rcSPS, rcPPS ) );
  m_eErrorConcealMode   = EC_NONE;
  m_bTrueSlice          = true;
  m_uiNumMbsInSlice     = 0;
  m_uiLastMbInSlice     = 0;
  m_iTopFieldPoc        = 0;
  m_iBotFieldPoc        = 0;
  m_bSCoeffResidualPred = false;
  m_iLongTermFrameIdx   = -1;
  m_bReconstructionLayer= false;
  ::memset( m_aapcRefFrameList, 0x00, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    ::memset( m_aauiNumRefIdxActiveUpdate[ui] , 0x00, 2*sizeof(UInt));
  }
  return Err::m_nOK;
}

Void
SliceHeader::copy( const SliceHeader& rcSliceHeader )
{
  SliceHeaderSyntax::copy ( rcSliceHeader );
  m_eErrorConcealMode     = rcSliceHeader.m_eErrorConcealMode;
  m_bTrueSlice            = rcSliceHeader.m_bTrueSlice;
  m_uiNumMbsInSlice       = rcSliceHeader.m_uiNumMbsInSlice;
  m_uiLastMbInSlice       = rcSliceHeader.m_uiLastMbInSlice;
  m_iTopFieldPoc          = rcSliceHeader.m_iTopFieldPoc;
  m_iBotFieldPoc          = rcSliceHeader.m_iBotFieldPoc;
  m_bSCoeffResidualPred   = rcSliceHeader.m_bSCoeffResidualPred;
  m_iLongTermFrameIdx     = rcSliceHeader.m_iLongTermFrameIdx;
  m_bReconstructionLayer  = rcSliceHeader.m_bReconstructionLayer;
  m_cFMO                  = rcSliceHeader.m_cFMO;
  memcpy( m_aapcRefFrameList, rcSliceHeader.m_aapcRefFrameList, sizeof( m_aapcRefFrameList ) );
  for(UInt ui=0;ui<MAX_TEMP_LEVELS;ui++)
  {
    memcpy( m_aauiNumRefIdxActiveUpdate[ui] , rcSliceHeader.m_aauiNumRefIdxActiveUpdate, 2*sizeof(UInt));
  }
}

ErrVal
SliceHeader::copyPrefix( const PrefixHeader& rcPrefixHeader )
{
  ROF( rcPrefixHeader.getNoInterLayerPredFlag() );
  ROT( rcPrefixHeader.getDependencyId() );
  ROT( rcPrefixHeader.getQualityId() );
  ROF( rcPrefixHeader.getOutputFlag() );
  ROF( rcPrefixHeader.getIdrFlag() == ( getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR ) );

  setIdrFlag                ( rcPrefixHeader.getIdrFlag             () );
  setPriorityId             ( rcPrefixHeader.getPriorityId          () );
  setNoInterLayerPredFlag   ( rcPrefixHeader.getNoInterLayerPredFlag() );
  setDependencyId           ( rcPrefixHeader.getDependencyId        () );
  setQualityId              ( rcPrefixHeader.getQualityId           () );
  setTemporalId             ( rcPrefixHeader.getTemporalId          () );
  setUseRefBasePicFlag      ( rcPrefixHeader.getUseRefBasePicFlag   () );
  setDiscardableFlag        ( rcPrefixHeader.getDiscardableFlag     () );
  setOutputFlag             ( rcPrefixHeader.getOutputFlag          () );
  setStoreRefBasePicFlag    ( rcPrefixHeader.getStoreRefBasePicFlag () );
  getDecRefBasePicMarking() = rcPrefixHeader.getDecRefBasePicMarking();
  return Err::m_nOK;
}

UInt
SliceHeader::getMapUnitFromPosition( UInt uiMbY, UInt uiMbX ) const
{
  AOF( parameterSetsInitialized() );
  UInt   uiMapUnit = uiMbY * getSPS().getFrameWidthInMbs() + uiMbX;
  return uiMapUnit;
}

UInt
SliceHeader::getMbAddressFromPosition( UInt uiMbY, UInt uiMbX ) const
{
  AOF( parameterSetsInitialized() );

  UInt uiMbAddress  = 0;
  UInt uiMbsInRow   = getSPS().getFrameWidthInMbs();
  if( isMbaffFrame() )
  {
    uiMbAddress = ( ( ( uiMbY >> 1 ) * uiMbsInRow + uiMbX ) << 1 ) + ( uiMbY & 2 );
  }
  else
  {
    uiMbAddress = uiMbY * uiMbsInRow + uiMbX;
  }
  return uiMbAddress;
}

Void
SliceHeader::getMbPositionFromAddress( UInt& ruiMbY, UInt& ruiMbX, UInt uiMbAddress ) const
{
  AOF( parameterSetsInitialized() );

  UInt uiMbsInRow = getSPS().getFrameWidthInMbs();
  if( isMbaffFrame() )
  {
    ruiMbY = ( ( uiMbAddress / 2 ) / uiMbsInRow * 2 ) + ( uiMbAddress % 2 );
    ruiMbX = ( ( uiMbAddress / 2 ) % uiMbsInRow     );
  }
  else
  {
    ruiMbY = ( uiMbAddress / uiMbsInRow );
    ruiMbX = ( uiMbAddress % uiMbsInRow );
  }
}

Void
SliceHeader::getMbPosAndIndexFromAddress( UInt& ruiMbY, UInt& ruiMbX, UInt& ruiMbIndex, UInt uiMbAddress ) const
{
  AOF( parameterSetsInitialized() );

  UInt uiMbsInRow = getSPS().getFrameWidthInMbs();
  if( isMbaffFrame() )
  {
    ruiMbY      = ( ( uiMbAddress / 2 ) / uiMbsInRow * 2 ) + ( uiMbAddress % 2 );
    ruiMbX      = ( ( uiMbAddress / 2 ) % uiMbsInRow     );
    ruiMbIndex  = uiMbsInRow * ruiMbY + ruiMbX;
  }
  else
  {
    ruiMbY      = ( uiMbAddress / uiMbsInRow );
    ruiMbX      = ( uiMbAddress % uiMbsInRow );
    ruiMbIndex  =   uiMbAddress;
  }
}

UInt
SliceHeader::getMbIndexFromAddress( UInt uiMbAddress ) const
{
  if( isMbaffFrame() )
  {
    UInt    uiMbsInRow  = getSPS().getFrameWidthInMbs();
    UInt    uiMbY       = ( ( uiMbAddress / 2 ) / uiMbsInRow * 2 ) + ( uiMbAddress % 2 );
    UInt    uiMbX       = ( ( uiMbAddress / 2 ) % uiMbsInRow     );
    UInt    uiMbIndex   = uiMbsInRow * uiMbY + uiMbX;
    return  uiMbIndex;
  }
  return    uiMbAddress;
}

Bool
SliceHeader::isFieldPair( UInt uiFrameNum, PicType ePicType, Bool bIsRefPic ) const
{
  ROTRS(  getFrameNum ()  !=  uiFrameNum, false );
  ROTRS(  isRefPic    ()  !=  bIsRefPic,  false );
  ROTRS(  getPicType  ()  &   ePicType,   false );
  return true;
}

Int
SliceHeader::getDistScaleFactorWP( const Frame* pcFrameL0, const Frame* pcFrameL1 ) const
{
  Int iDiffPocD = pcFrameL1->getPoc() - pcFrameL0->getPoc();
  if( pcFrameL0->isLongTerm() || pcFrameL1->isLongTerm() || iDiffPocD == 0 )
  {
    return 1024;
  }
  Int iDiffPocB = getPoc() - pcFrameL0->getPoc();
  Int iTDB      = gClipMinMax( iDiffPocB, -128, 127 );
  Int iTDD      = gClipMinMax( iDiffPocD, -128, 127 );
  Int iX        = ( 0x4000 + abs( iTDD / 2 ) ) / iTDD;
  Int iScale    = gClipMinMax( ( iTDB * iX + 32 ) >> 6, -1024, 1023 );
  return iScale;
}

const PredWeight&
SliceHeader::getPredWeight( ListIdx eListIdx, UInt uiRefIdx, Bool bFieldFlag ) const
{
  if( bFieldFlag )  return getPredWeightTable( eListIdx ).get( ( uiRefIdx - 1 ) / 2 );
  else              return getPredWeightTable( eListIdx ).get(   uiRefIdx - 1 );
};

PredWeight&
SliceHeader::getPredWeight( ListIdx eListIdx, UInt uiRefIdx, Bool bFieldFlag )
{
  if( bFieldFlag )  return getPredWeightTable( eListIdx ).get( ( uiRefIdx - 1 ) / 2 );
  else              return getPredWeightTable( eListIdx ).get(   uiRefIdx - 1 );
};

PicType
SliceHeader::getPicType() const
{
  ROFRS(  getFieldPicFlag   (), FRAME );
  ROFRS(  getBottomFieldFlag(), TOP_FIELD );
  return                        BOT_FIELD;
}

Int
SliceHeader::getPoc() const
{
  return ( getFieldPicFlag() ? ( getBottomFieldFlag() ? m_iBotFieldPoc : m_iTopFieldPoc ) : gMin( m_iTopFieldPoc, m_iBotFieldPoc ) );
}

Int
SliceHeader::getPoc( PicType ePicType ) const
{
  return ( ePicType == FRAME ? gMin( m_iTopFieldPoc, m_iBotFieldPoc ) : ePicType == BOT_FIELD ? m_iBotFieldPoc : m_iTopFieldPoc );
}

Void
SliceHeader::setSCoeffResidualPredFlag( ResizeParameters* pcResizeParameters )
{
  m_bSCoeffResidualPred = false;
  if( pcResizeParameters )
  {
    ROTVS( getNoInterLayerPredFlag() );
    ROTVS( pcResizeParameters->getSpatialResolutionChangeFlag() );
    ROTVS( getTCoeffLevelPredictionFlag () );
    m_bSCoeffResidualPred = true;
  }
}

Void
SliceHeader::setPicType( PicType ePicType )
{
  setFieldPicFlag   ( ePicType != FRAME );
  setBottomFieldFlag( ePicType == BOT_FIELD );
}

Bool
SliceHeader::isFirstSliceOfNextAccessUnit( const SliceHeader* pcLastSliceHeader ) const
{
  ROFRS   ( pcLastSliceHeader,                                                              false );
  ROTRS   ( pcLastSliceHeader->getRedundantPicCnt       ()  < getRedundantPicCnt        (), false );
  ROTRS   ( pcLastSliceHeader->getRedundantPicCnt       ()  > getRedundantPicCnt        (), true  );
  ROTRS   ( pcLastSliceHeader->getDependencyId          ()  < getDependencyId           (), false );
  ROTRS   ( pcLastSliceHeader->getDependencyId          ()  > getDependencyId           (), true  );
  ROTRS   ( pcLastSliceHeader->getQualityId             ()  < getQualityId              (), false );
  ROTRS   ( pcLastSliceHeader->getQualityId             ()  > getQualityId              (), true  );
  ROTRS   ( pcLastSliceHeader->getFrameNum              () != getFrameNum               (), true  );
  ROTRS   ( pcLastSliceHeader->getPicParameterSetId     () != getPicParameterSetId      (), true  );
  ROTRS   ( pcLastSliceHeader->getFieldPicFlag          () != getFieldPicFlag           (), true  );
  ROTRS   ( pcLastSliceHeader->getBottomFieldFlag       () != getBottomFieldFlag        (), true  );
  ROTRS   ( pcLastSliceHeader->isRefPic                 () != isRefPic                  (), true  );
  ROTRS   ( pcLastSliceHeader->getIdrFlag               () != getIdrFlag                (), true  );
  if( getIdrFlag() )
  {
    ROTRS ( pcLastSliceHeader->getIdrPicId              () != getIdrPicId               (), true  );
  }
  if( getSPS().getPicOrderCntType() == 0 )
  {
    ROTRS ( pcLastSliceHeader->getPicOrderCntLsb        () != getPicOrderCntLsb         (), true  );
    ROTRS ( pcLastSliceHeader->getDeltaPicOrderCntBottom() != getDeltaPicOrderCntBottom (), true  );
  }
  if( getSPS().getPicOrderCntType() == 1 )
  {
    ROTRS ( pcLastSliceHeader->getDeltaPicOrderCnt0     () != getDeltaPicOrderCnt0      (), true  );
    ROTRS ( pcLastSliceHeader->getDeltaPicOrderCnt1     () != getDeltaPicOrderCnt1      (), true  );
  }
  return false;
}

ErrVal
SliceHeader::compare( const SliceHeader* pcSH,
                      Bool&              rbNewPic,
                      Bool&              rbNewFrame ) const
{
  rbNewPic = rbNewFrame = true;

  if( getIdrFlag() )
  {
    ROTRS( NULL == pcSH,                                    Err::m_nOK ); //very first frame
    ROTRS( ! pcSH->getIdrFlag(),                          Err::m_nOK ); //previous no idr
    ROTRS( getIdrPicId() != pcSH->getIdrPicId(),            Err::m_nOK );
  }

  ROTRS( NULL == pcSH,                                      Err::m_nOK );
  ROTRS( isRefPic() != pcSH->isRefPic(),              Err::m_nOK );
  ROTRS( getFrameNum() != pcSH->getFrameNum(),              Err::m_nOK );

  ROTRS( getFieldPicFlag() != pcSH->getFieldPicFlag(),      Err::m_nOK );

  const Bool bSameParity = ( ! getFieldPicFlag() || ( getBottomFieldFlag() == pcSH->getBottomFieldFlag() ) );
  rbNewFrame = bSameParity;

  if( getSPS().getPicOrderCntType() == 0 )
  {
    ROTRS( getPicOrderCntLsb() != pcSH->getPicOrderCntLsb(),  Err::m_nOK );
    ROTRS( ! getFieldPicFlag() && (getDeltaPicOrderCntBottom() != pcSH->getDeltaPicOrderCntBottom()), Err::m_nOK );
  }

  if( getSPS().getPicOrderCntType() == 1 )
  {
    ROTRS( getDeltaPicOrderCnt0() != pcSH->getDeltaPicOrderCnt0(), Err::m_nOK );
    ROTRS( ! getFieldPicFlag() && (getDeltaPicOrderCnt1() != pcSH->getDeltaPicOrderCnt1()), Err::m_nOK );
  }

  ROTRS( ! getFieldPicFlag() && (getDeltaPicOrderCntBottom() != pcSH->getDeltaPicOrderCntBottom()), Err::m_nOK );

  rbNewFrame = false;

  if( getIdrFlag() )
  {
    ROTRS( ! pcSH->getIdrFlag(),                          Err::m_nOK ); //prev no idr
  }
  ROTRS( ! bSameParity,  Err::m_nOK ); // differ

  rbNewPic = false;

  return Err::m_nOK;
}

ErrVal
SliceHeader::compareRedPic( const SliceHeader* pcSH,
                           Bool&              rbNewFrame ) const
{
  rbNewFrame = true;

  ROTRS( NULL == pcSH,                                          Err::m_nOK );
  ROTRS( getIdrPicId() != pcSH->getIdrPicId(),                  Err::m_nOK );
  ROTRS( getFrameNum() != pcSH->getFrameNum(),                  Err::m_nOK );
  ROTRS( getDependencyId() != pcSH->getDependencyId(),                    Err::m_nOK );
  ROTRS( getQualityId() != pcSH->getQualityId(),          Err::m_nOK );
  ROTRS( getFirstMbInSlice() != pcSH->getFirstMbInSlice(),      Err::m_nOK );
  ROTRS( (getNalRefIdc() == 0 )&&(pcSH->getNalRefIdc() != 0),   Err::m_nOK );
  ROTRS( (getNalRefIdc() != 0 )&&(pcSH->getNalRefIdc() == 0),   Err::m_nOK );
  ROTRS( getPicOrderCntLsb() != pcSH->getPicOrderCntLsb(),      Err::m_nOK );

  rbNewFrame = false;

  return Err::m_nOK;
}

ErrVal
SliceHeader::sliceHeaderBackup ( SliceHeader*                pcSH )
{
  pcSH->setIdrPicId       ( getIdrPicId()       );
  pcSH->setFrameNum       ( getFrameNum()       );
  pcSH->setDependencyId        ( getDependencyId()        );
  pcSH->setQualityId   ( getQualityId()   );
  pcSH->setFirstMbInSlice ( getFirstMbInSlice() );
  pcSH->setNalRefIdc      ( getNalRefIdc()      );
  pcSH->setPicOrderCntLsb ( getPicOrderCntLsb() );

  return Err::m_nOK;
}

ErrVal
SliceHeader::FMOInit()
{
  m_cFMO.sps_.frame_mbs_only_flag            = getSPS().getFrameMbsOnlyFlag();
  m_cFMO.sps_.mb_adaptive_frame_field_flag   = getSPS().getMbAdaptiveFrameFieldFlag();
  m_cFMO.sps_.pic_height_in_map_units_minus1 = ( getSPS().getFrameMbsOnlyFlag() ? getSPS().getFrameHeightInMbs() : getSPS().getFrameHeightInMbs() >> 1 ) - 1;
  m_cFMO.sps_.pic_width_in_mbs_minus1        = getSPS().getFrameWidthInMbs() - 1;

  m_cFMO.pps_.num_slice_groups_minus1 = getPPS().getNumSliceGroupsMinus1();
  m_cFMO.pps_.slice_group_map_type    = getPPS().getSliceGroupMapType();
  m_cFMO.pps_.num_slice_group_map_units_minus1 = getPPS().getNumSliceGroupMapUnitsMinus1();
  m_cFMO.pps_.copy_run_length_minus1(getPPS().getArrayRunLengthMinus1());
  m_cFMO.pps_.copy_top_left(getPPS().getArrayTopLeft());
  m_cFMO.pps_.copy_bottom_right(getPPS().getArrayBottomRight());
  m_cFMO.pps_.slice_group_change_direction_flag = getPPS().getSliceGroupChangeDirection_flag();
  m_cFMO.pps_.slice_group_change_rate_minus1    = getPPS().getSliceGroupChangeRateMinus1();
  m_cFMO.pps_.copy_slice_group_id(getPPS().getArraySliceGroupId());

  m_cFMO.img_.field_pic_flag      = getFieldPicFlag();
  m_cFMO.img_.PicHeightInMapUnits = m_cFMO.sps_.pic_height_in_map_units_minus1 + 1;
  m_cFMO.img_.PicWidthInMbs       = getSPS().getFrameWidthInMbs();
  m_cFMO.img_.PicSizeInMbs        = (getSPS().getFrameHeightInMbs()>> (UChar)getFieldPicFlag())*getSPS().getFrameWidthInMbs();
  m_cFMO.img_.slice_group_change_cycle = getSliceGroupChangeCycle();

  m_cFMO.init();
  m_cFMO.StartPicture();
  return Err::m_nOK;
}

ErrVal
SliceHeader::FMOUninit()
{
  m_cFMO.finit();
  return Err::m_nOK;
}

Int
SliceHeader::getNumMbInSlice()
{
  Int SliceID = m_cFMO.getSliceGroupId(getFirstMbInSlice());
  return m_cFMO.getNumMbInSliceGroup(SliceID);
}


Int
SliceHeader::getDistScaleFactor( PicType eMbPicType,
                                 SChar   sL0RefIdx,
                                 SChar   sL1RefIdx ) const
{
  const Frame*  pcFrameL0 = getRefFrameList( eMbPicType, LIST_0 )->getEntry( sL0RefIdx - 1 );
  const Frame*  pcFrameL1 = getRefFrameList( eMbPicType, LIST_1 )->getEntry( sL1RefIdx - 1 );
  Int           iDiffPocD = pcFrameL1->getPoc() - pcFrameL0->getPoc();
  if( pcFrameL0->isLongTerm() || iDiffPocD == 0 )
  {
    return 1024;
  }
  else
  {
    Int iDiffPocB = getPoc( eMbPicType ) - pcFrameL0->getPoc();
    Int iTDB      = gClipMinMax( iDiffPocB, -128, 127 );
    Int iTDD      = gClipMinMax( iDiffPocD, -128, 127 );
    Int iX        = (0x4000 + abs(iTDD/2)) / iTDD;
    Int iScale    = gClipMinMax( (iTDB * iX + 32) >> 6, -1024, 1023 );
    return iScale;
  }
}


H264AVC_NAMESPACE_END
