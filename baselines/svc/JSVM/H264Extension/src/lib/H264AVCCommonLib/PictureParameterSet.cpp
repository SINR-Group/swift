
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/PictureParameterSet.h"
#include "H264AVCCommonLib/SliceHeader.h"
#include "H264AVCCommonLib/TraceFile.h"



H264AVC_NAMESPACE_BEGIN



PictureParameterSet::PictureParameterSet()
: m_eNalUnitType                            ( NAL_UNIT_UNSPECIFIED_0 )
, m_uiDependencyId                               ( 0 )
, m_uiPicParameterSetId                     ( MSYS_UINT_MAX )
, m_uiSeqParameterSetId                     ( MSYS_UINT_MAX )
, m_bEntropyCodingModeFlag                  ( false )
, m_bPicOrderPresentFlag                    ( false )
, m_bWeightedPredFlag                       ( false )
, m_uiWeightedBiPredIdc                     ( 0 )
, m_uiPicInitQp                             ( 26 )
, m_iChromaQpIndexOffset                    ( 0 )
, m_bDeblockingFilterParametersPresentFlag  ( false )
, m_bConstrainedIntraPredFlag               ( false )
, m_bRedundantPicCntPresentFlag             ( false )  // JVT-Q054, Red. Picture
, m_bRedundantKeyPicCntPresentFlag          ( false )  // JVT-W049
, m_bEnableRedundantKeyPicCntPresentFlag    ( false )  // JVT-W049
, m_bTransform8x8ModeFlag                   ( false )
, m_bPicScalingMatrixPresentFlag            ( false )
, m_iSecondChromaQpIndexOffset              ( 0 )
, m_uiSliceGroupMapType                     ( 0 )
, m_bSliceGroupChangeDirection_flag         ( false )
, m_uiSliceGroupChangeRateMinus1            ( 0 )
, m_uiNumSliceGroupMapUnitsMinus1           ( 0 )
, m_uiSliceGroupIdArraySize                 ( 0 )
, m_pauiSliceGroupId                        ( 0 )
, m_bReferencesSubsetSPS                    ( false )
{
  m_auiNumRefIdxActive[LIST_0] = 0;
  m_auiNumRefIdxActive[LIST_1] = 0;
//TMM_FIX
	::memset( m_uiTopLeft,     0x00, MAXNumSliceGroupsMinus1*sizeof(UInt) );
	::memset( m_uiBottomRight, 0x00, MAXNumSliceGroupsMinus1*sizeof(UInt) );
//TMM_FIX
}


PictureParameterSet::~PictureParameterSet()
{
  delete [] m_pauiSliceGroupId;
}


ErrVal
PictureParameterSet::create( PictureParameterSet*& rpcPPS )
{
  rpcPPS = new PictureParameterSet;
  ROT( NULL == rpcPPS );
  return Err::m_nOK;
}


ErrVal
PictureParameterSet::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal
PictureParameterSet::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  //===== NAL unit header =====
  ETRACE_DECLARE( Bool m_bTraceEnable = true );
  ETRACE_LAYER  ( 0 );
  ETRACE_HEADER ( "PICTURE PARAMETER SET" );
  RNOK  ( pcWriteIf->writeFlag( 0,                                        "NAL unit header: forbidden_zero_bit" ) );
  RNOK  ( pcWriteIf->writeCode( 3, 2,                                     "NAL unit header: nal_ref_idc" ) );
  RNOK  ( pcWriteIf->writeCode( m_eNalUnitType, 5,                        "NAL unit header: nal_unit_type" ) );

  //===== NAL unit payload =====
  RNOK( pcWriteIf->writeUvlc( getPicParameterSetId(),                     "PPS: pic_parameter_set_id" ) );
  RNOK( pcWriteIf->writeUvlc( getSeqParameterSetId(),                     "PPS: seq_parameter_set_id" ) );
  RNOK( pcWriteIf->writeFlag( getEntropyCodingModeFlag(),                 "PPS: entropy_coding_mode_flag" ) );
  RNOK( pcWriteIf->writeFlag( getPicOrderPresentFlag(),                   "PPS: pic_order_present_flag" ) );

  //--ICU/ETRI FMO Implementation : FMO stuff start
  Int iNumberBitsPerSliceGroupId;
  RNOK( pcWriteIf->writeUvlc( getNumSliceGroupsMinus1(),                         "PPS: num_slice_groups_minus1" ) );

  if(getNumSliceGroupsMinus1() > 0)
  {
    RNOK( pcWriteIf->writeUvlc( getSliceGroupMapType(),                             "PPS: slice_group_map_type" ) );
    if(getSliceGroupMapType() ==0)
    {
      for(UInt iSliceGroup=0;iSliceGroup<=getNumSliceGroupsMinus1();iSliceGroup++)
      {
        RNOK( pcWriteIf->writeUvlc( getRunLengthMinus1(iSliceGroup),                             "PPS: run_length_minus1 [iSliceGroup]" ) );
      }
    }
    else if (getSliceGroupMapType() ==2)
    {
      for(UInt iSliceGroup=0;iSliceGroup<getNumSliceGroupsMinus1();iSliceGroup++)
      {
        RNOK( pcWriteIf->writeUvlc( getTopLeft(iSliceGroup),                             "PPS: top_left [iSliceGroup]" ) );
        RNOK( pcWriteIf->writeUvlc( getBottomRight(iSliceGroup),                             "PPS: bottom_right [iSliceGroup]" ) );
      }
    }
    else if(getSliceGroupMapType() ==3 ||
      getSliceGroupMapType() ==4 ||
      getSliceGroupMapType() ==5)
    {
      RNOK( pcWriteIf->writeFlag( getSliceGroupChangeDirection_flag(),                      "PPS: slice_group_change_direction_flag" ) );
      RNOK( pcWriteIf->writeUvlc( getSliceGroupChangeRateMinus1(),                             "PPS: slice_group_change_rate_minus1" ) );
    }
    else if (getSliceGroupMapType() ==6)
    {
      if (getNumSliceGroupsMinus1()+1 >4)
        iNumberBitsPerSliceGroupId = 3;
      else if (getNumSliceGroupsMinus1()+1 > 2)
        iNumberBitsPerSliceGroupId = 2;
      else
        iNumberBitsPerSliceGroupId = 1;
      //! JVT-F078, exlicitly signal number of MBs in the map
      RNOK( pcWriteIf->writeUvlc( getNumSliceGroupMapUnitsMinus1(),                             "PPS: num_slice_group_map_units_minus1" ) );
      ROF ( getNumSliceGroupMapUnitsMinus1() < m_uiSliceGroupIdArraySize );
      for (UInt iSliceGroup=0; iSliceGroup<=getNumSliceGroupMapUnitsMinus1(); iSliceGroup++)
        RNOK( pcWriteIf->writeCode( getSliceGroupId(iSliceGroup), iNumberBitsPerSliceGroupId ,                                    "PPS: slice_group_id[iSliceGroup]" ) );
    }

  }
  //--ICU/ETRI FMO Implementation : FMO stuff end

  RNOK( pcWriteIf->writeUvlc( getNumRefIdxActive(LIST_0)-1,               "PPS: num_ref_idx_l0_active_minus1" ) );
  RNOK( pcWriteIf->writeUvlc( getNumRefIdxActive(LIST_1)-1,               "PPS: num_ref_idx_l1_active_minus1" ) );
  RNOK( pcWriteIf->writeFlag( m_bWeightedPredFlag,                        "PPS: weighted_pred_flag" ) );
  RNOK( pcWriteIf->writeCode( m_uiWeightedBiPredIdc, 2,                   "PPS: weighted_bipred_idc" ) );
  RNOK( pcWriteIf->writeSvlc( (Int)getPicInitQp() - 26,                   "PPS: pic_init_qp_minus26" ) );
  RNOK( pcWriteIf->writeSvlc( 0,                                          "PPS: pic_init_qs_minus26" ) );
  RNOK( pcWriteIf->writeSvlc( getChromaQpIndexOffset(),                   "PPS: chroma_qp_index_offset" ) );
  RNOK( pcWriteIf->writeFlag( getDeblockingFilterParametersPresentFlag(), "PPS: deblocking_filter_control_present_flag" ) ); //VB-JV 04/08
  RNOK( pcWriteIf->writeFlag( getConstrainedIntraPredFlag(),              "PPS: constrained_intra_pred_flag" ) );
  RNOK( pcWriteIf->writeFlag( getRedundantPicCntPresentFlag(),            "PPS: redundant_pic_cnt_present_flag" ) );  // JVT-Q054 Red. Picture

  if( getTransform8x8ModeFlag() || m_bPicScalingMatrixPresentFlag || m_iSecondChromaQpIndexOffset != m_iChromaQpIndexOffset )
  {
    RNOK( xWriteFrext( pcWriteIf ) );
  }

  return Err::m_nOK;
}


ErrVal
PictureParameterSet::read( HeaderSymbolReadIf*  pcReadIf,
                           NalUnitType          eNalUnitType )
{
  //===== NAL unit header =====
  setNalUnitType    ( eNalUnitType );

  UInt  uiTmp;
  Int   iTmp;

  //--ICU/ETRI FMO Implementation
  Int iNumberBitsPerSliceGroupId;


  RNOK( pcReadIf->getUvlc( m_uiPicParameterSetId,                         "PPS: pic_parameter_set_id" ) );
  ROT ( m_uiPicParameterSetId > 255 );
  RNOK( pcReadIf->getUvlc( m_uiSeqParameterSetId,                         "PPS: seq_parameter_set_id" ) );
  ROT ( m_uiSeqParameterSetId > 31 );
  RNOK( pcReadIf->getFlag( m_bEntropyCodingModeFlag,                      "PPS: entropy_coding_mode_flag" ) );
  RNOK( pcReadIf->getFlag( m_bPicOrderPresentFlag,                        "PPS: pic_order_present_flag" ) );

  //--ICU/ETRI FMO Implementation : FMO stuff start
  RNOK( pcReadIf->getUvlc( m_uiNumSliceGroupsMinus1,                         "PPS: num_slice_groups_minus1" ) );
  ROT ( m_uiNumSliceGroupsMinus1 > MAXNumSliceGroupsMinus1);

  if(m_uiNumSliceGroupsMinus1 > 0)
  {
    RNOK( pcReadIf->getUvlc( m_uiSliceGroupMapType,                             "PPS: slice_group_map_type" ) );
    if(m_uiSliceGroupMapType ==0)
    {
      for(UInt i=0;i<=m_uiNumSliceGroupsMinus1;i++)
      {
        RNOK( pcReadIf->getUvlc( m_uiRunLengthMinus1[i],                             "PPS: run_length_minus1 [i]" ) );
      }
    }
    else if (m_uiSliceGroupMapType ==2)
    {
      for(UInt i=0;i<m_uiNumSliceGroupsMinus1;i++)
      {
        RNOK( pcReadIf->getUvlc( m_uiTopLeft[i],                             "PPS: top_left [i]" ) );
        RNOK( pcReadIf->getUvlc( m_uiBottomRight[i],                             "PPS: bottom_right [i]" ) );
      }
    }
    else if(m_uiSliceGroupMapType ==3 ||
      m_uiSliceGroupMapType ==4 ||
      m_uiSliceGroupMapType ==5)
    {
      RNOK( pcReadIf->getFlag( m_bSliceGroupChangeDirection_flag,                      "PPS: slice_group_change_direction_flag" ) );
      RNOK( pcReadIf->getUvlc( m_uiSliceGroupChangeRateMinus1,                             "PPS: slice_group_change_rate_minus1" ) );
    }
    else if (m_uiSliceGroupMapType ==6)
    {
      if( m_uiNumSliceGroupsMinus1+1 > 4 )      iNumberBitsPerSliceGroupId = 3;
      else if(m_uiNumSliceGroupsMinus1+1 > 2)   iNumberBitsPerSliceGroupId = 2;
      else                                      iNumberBitsPerSliceGroupId = 1;
      //! JVT-F078, exlicitly signal number of MBs in the map
      RNOK( pcReadIf->getUvlc( m_uiNumSliceGroupMapUnitsMinus1,                             "PPS: num_slice_group_map_units_minus1" ) );
      if( m_uiSliceGroupIdArraySize <= m_uiNumSliceGroupMapUnitsMinus1 )
      {
        delete [] m_pauiSliceGroupId;
        m_uiSliceGroupIdArraySize = m_uiNumSliceGroupMapUnitsMinus1 + 1;
        m_pauiSliceGroupId        = new UInt [m_uiSliceGroupIdArraySize];
      }
      for( UInt i=0; i<=m_uiNumSliceGroupMapUnitsMinus1; i++ )
      {
        RNOK( pcReadIf->getCode( m_pauiSliceGroupId[i], iNumberBitsPerSliceGroupId ,       "PPS: slice_group_id[i]" ) );
      }
    }

  }
  //--ICU/ETRI FMO Implementation : FMO stuff end


  RNOK( pcReadIf->getUvlc( uiTmp,                                         "PPS: num_ref_idx_l0_active_minus1" ) );
  setNumRefIdxActive( LIST_0, uiTmp + 1 );
  RNOK( pcReadIf->getUvlc( uiTmp,                                         "PPS: num_ref_idx_l1_active_minus1" ) );
  setNumRefIdxActive( LIST_1, uiTmp + 1 );
  RNOK( pcReadIf->getFlag( m_bWeightedPredFlag,                           "PPS: weighted_pred_flag" ) );
  RNOK( pcReadIf->getCode( m_uiWeightedBiPredIdc, 2,                      "PPS: weighted_bipred_idc" ) );
  RNOK( pcReadIf->getSvlc( iTmp,                                          "PPS: pic_init_qp_minus26" ) );
  ROT ( iTmp < -26 || iTmp > 25 );
  setPicInitQp( (UInt)( iTmp + 26 ) );
  RNOK( pcReadIf->getSvlc( iTmp,                                          "PPS: pic_init_qs_minus26" ) );
  RNOK( pcReadIf->getSvlc( iTmp,                                          "PPS: chroma_qp_index_offset" ) );
  ROT ( iTmp < -12 || iTmp > 12 );
  setChromaQpIndexOffset    ( iTmp );
  set2ndChromaQpIndexOffset ( iTmp ); // default
  RNOK( pcReadIf->getFlag( m_bDeblockingFilterParametersPresentFlag,      "PPS: deblocking_filter_control_present_flag" ) ); //VB-JV 04/08
  RNOK( pcReadIf->getFlag( m_bConstrainedIntraPredFlag,                   "PPS: constrained_intra_pred_flag" ) );
  RNOK( pcReadIf->getFlag( m_bRedundantPicCntPresentFlag,                 "PPS: redundant_pic_cnt_present_flag" ) );  // JVT-Q054 Red. Picture
  RNOK( xReadFrext( pcReadIf ) );

  return Err::m_nOK;
}


ErrVal
PictureParameterSet::xWriteFrext( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK  ( pcWriteIf->writeFlag( m_bTransform8x8ModeFlag,                  "PPS: transform_8x8_mode_flag" ) );
  RNOK  ( pcWriteIf->writeFlag( m_bPicScalingMatrixPresentFlag,           "PPS: pic_scaling_matrix_present_flag" ) );
  if( m_bPicScalingMatrixPresentFlag )
  {
    RNOK( m_cPicScalingMatrix.write( pcWriteIf, m_bTransform8x8ModeFlag ) );
  }
  RNOK  ( pcWriteIf->writeSvlc( m_iSecondChromaQpIndexOffset,             "PPS: second_chroma_qp_index_offset" ) );

  return Err::m_nOK;
}


ErrVal
PictureParameterSet::xReadFrext( HeaderSymbolReadIf* pcReadIf )
{
  ROTRS( ! pcReadIf->moreRBSPData(), Err::m_nOK );

  RNOK  ( pcReadIf->getFlag ( m_bTransform8x8ModeFlag,                    "PPS: transform_8x8_mode_flag" ) );
  RNOK  ( pcReadIf->getFlag ( m_bPicScalingMatrixPresentFlag,             "PPS: pic_scaling_matrix_present_flag" ) );
  if( m_bPicScalingMatrixPresentFlag )
  {
    RNOK( m_cPicScalingMatrix.read( pcReadIf, m_bTransform8x8ModeFlag ) );
  }
  RNOK  ( pcReadIf->getSvlc ( m_iSecondChromaQpIndexOffset,               "PPS: second_chroma_qp_index_offset" ) );
  ROT   ( m_iSecondChromaQpIndexOffset < -12 || m_iSecondChromaQpIndexOffset > 12 );

  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
