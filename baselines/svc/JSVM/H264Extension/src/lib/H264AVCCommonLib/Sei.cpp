
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/Sei.h"
#include "H264AVCCommonLib/ParameterSetMng.h"
#include "H264AVCCommonLib/TraceFile.h"

// JVT-V068 HRD {
#include "H264AVCCommonLib/Vui.h"
// JVT-V068 HRD }

H264AVC_NAMESPACE_BEGIN




ErrVal
SEI::read( HeaderSymbolReadIf* pcReadIf,
           MessageList&        rcSEIMessageList
           // JVT-V068 {
           ,ParameterSetMng*    pcParameterSetMng
           // JVT-V068 }
         )
{
  ROT( NULL == pcReadIf);

  while( pcReadIf->moreRBSPData() )
  {
    SEIMessage* pcActualSEIMessage = NULL;

    RNOK( xRead( pcReadIf, pcActualSEIMessage
    // JVT-V068 {
				, pcParameterSetMng
    // JVT-V068 }
		 ));

    // JVT-V068 {
    if (pcActualSEIMessage)
    // JVT-V068 }
    rcSEIMessageList.push_back( pcActualSEIMessage );
  }
  return Err::m_nOK;
}


ErrVal
SEI::write( HeaderSymbolWriteIf*  pcWriteIf,
            HeaderSymbolWriteIf*  pcWriteTestIf,
            MessageList*          rpcSEIMessageList )
{
  ROT( NULL == pcWriteIf);
  ROT( NULL == pcWriteTestIf);
  ROT( NULL == rpcSEIMessageList);

  //===== NAL unit header =====
  ETRACE_DECLARE( Bool bScalableSEI     = ( rpcSEIMessageList->front()->getMessageType() == SCALABLE_SEI ) );
  ETRACE_DECLARE( Bool bBufferingPeriod = ( rpcSEIMessageList->front()->getMessageType() == BUFFERING_PERIOD ) );
  ETRACE_DECLARE( Bool m_bTraceEnable   = true );
  ETRACE_LAYER  ( bScalableSEI ? -3 : bBufferingPeriod ? -2 : -1 );
  ETRACE_HEADER ( "SEI NAL UNIT" );
  RNOK  ( pcWriteIf->writeFlag( 0,                "NAL unit header: forbidden_zero_bit" ) );
  RNOK  ( pcWriteIf->writeCode( 0, 2,             "NAL unit header: nal_ref_idc" ) );
  RNOK  ( pcWriteIf->writeCode( NAL_UNIT_SEI, 5,  "NAL unit header: nal_unit_type" ) );

  while( rpcSEIMessageList->size() )
  {
    RNOK( xWrite( pcWriteIf, pcWriteTestIf, rpcSEIMessageList->front() ) );
    SEIMessage* pcTop = rpcSEIMessageList->front();
    rpcSEIMessageList->pop_front();
    delete pcTop;
  }
  return Err::m_nOK;
}

// JVT-V068 {
ErrVal
SEI::writeScalableNestingSei( HeaderSymbolWriteIf*  pcWriteIf,
                              HeaderSymbolWriteIf*  pcWriteTestIf,
                              MessageList*          rpcSEIMessageList )
{
  ROT( NULL == pcWriteIf);
	ROT( NULL == pcWriteTestIf);
	ROT( NULL == rpcSEIMessageList);
  ROT( rpcSEIMessageList->size() != 2 );

  SEIMessage* pcTop = rpcSEIMessageList->front();
  rpcSEIMessageList->pop_front();
  ROT( pcTop->getMessageType() != SCALABLE_NESTING_SEI );

	SEIMessage* pcBottom = rpcSEIMessageList->front();
	rpcSEIMessageList->pop_front();

	//===== NAL unit header =====
	ETRACE_DECLARE( Bool m_bTraceEnable = true );
  ETRACE_LAYER  ( pcBottom->getMessageType() == BUFFERING_PERIOD ? -2 : -1 );
	ETRACE_HEADER ( "SEI NAL UNIT" );
	RNOK  ( pcWriteIf->writeFlag( 0,                "NAL unit header: forbidden_zero_bit"  ) );
	RNOK  ( pcWriteIf->writeCode( 0, 2,             "NAL unit header: nal_ref_idc" ) );
	RNOK  ( pcWriteIf->writeCode( NAL_UNIT_SEI, 5,  "NAL unit header: nal_unit_type" ) );

	//first write testing SEI message to get payload size
	UInt uiBits = 0;
	UInt uiNestedSeiSize = 0;
	//take scene info as example, 	//can be changed here

  // First Test the payload size
  UInt uiStart  = pcWriteTestIf->getNumberOfWrittenBits();
  pcBottom->write(pcWriteTestIf);
  uiBits = pcWriteTestIf->getNumberOfWrittenBits() - uiStart;
  UInt uiSize = (uiBits+7)>>3;

  uiNestedSeiSize = uiSize;

  uiStart  = pcWriteTestIf->getNumberOfWrittenBits();
  RNOK( xWritePayloadHeader( pcWriteTestIf, pcBottom->getMessageType(), uiSize ) );
  uiBits = pcWriteTestIf->getNumberOfWrittenBits() - uiStart;
  AOT( (uiBits & 7) > 0 );
  uiSize += (uiBits>>3);

  uiStart  = pcWriteTestIf->getNumberOfWrittenBits();
  RNOK( pcTop->write( pcWriteTestIf ) );
  uiBits = pcWriteTestIf->getNumberOfWrittenBits() - uiStart;
  AOT( (uiBits & 7) > 0 );
  uiSize += (uiBits>>3); // Scalable Nesting Sei Payload Size

  //Then write actual SEI message
  uiStart  = pcWriteIf->getNumberOfWrittenBits();
  RNOK( xWritePayloadHeader( pcWriteIf, SCALABLE_NESTING_SEI, uiSize ) );
  RNOK( pcTop->write( pcWriteIf ) );
	RNOK( xWritePayloadHeader( pcWriteIf, pcBottom->getMessageType(), uiNestedSeiSize ) );
	RNOK( pcBottom->write( pcWriteIf ) );
  uiBits = pcWriteIf->getNumberOfWrittenBits() - uiStart;
  UInt uiAlignedBits = ( 8 - (uiBits&7) ) % 8;
  if(  uiAlignedBits )
  {
    RNOK( pcWriteIf->writeFlag( 1, "SEI: alignment_bit" ) );
    uiAlignedBits--;
    while( uiAlignedBits-- )
    {
      RNOK( pcWriteIf->writeFlag( 0, "SEI: alignment_bit" ) );
    }
  }
  delete pcTop;
  delete pcBottom;

  return Err::m_nOK;
}
// JVT-V068 }

ErrVal
SEI::xRead( HeaderSymbolReadIf* pcReadIf,
            SEIMessage*&        rpcSEIMessage
            // JVT-V068 {
            ,ParameterSetMng* pcParameterSetMng
            // JVT-V068 }
            )
{
  MessageType eMessageType = RESERVED_SEI;
  UInt        uiSize       = 0;

  RNOK( xReadPayloadHeader( pcReadIf, eMessageType, uiSize) );

  // special handling for bit stream extractor
  if( pcParameterSetMng == 0 && ( eMessageType == PIC_TIMING || eMessageType == BUFFERING_PERIOD ) )
  {
    RNOK( xCreate( rpcSEIMessage, eMessageType, pcParameterSetMng, uiSize ) );
    UInt uiDummy;
    while (uiSize--)
    {
      pcReadIf->getCode(uiDummy, 8, "SEI: Byte ignored.");
    }
    RNOK( pcReadIf->readByteAlign() );
    return Err::m_nOK;
  }
  RNOK( xCreate( rpcSEIMessage, eMessageType, pcParameterSetMng, uiSize ) );
  RNOK( rpcSEIMessage->read( pcReadIf, pcParameterSetMng ) );
  RNOK( pcReadIf->readByteAlign() );
  return Err::m_nOK;
}


ErrVal
SEI::xWrite( HeaderSymbolWriteIf* pcWriteIf,
             HeaderSymbolWriteIf* pcWriteTestIf,
             SEIMessage*          pcSEIMessage )
{

  UInt uiStart  = pcWriteTestIf->getNumberOfWrittenBits();
  RNOK( pcSEIMessage->write( pcWriteTestIf ) );
  UInt uiBits = pcWriteTestIf->getNumberOfWrittenBits() - uiStart;

  UInt uiSize = (uiBits+7)/8;

  RNOK( xWritePayloadHeader( pcWriteIf, pcSEIMessage->getMessageType(), uiSize ) );
  RNOK( pcSEIMessage->write( pcWriteIf ) );
  UInt uiAlignedBits = ( 8 - (uiBits&7) ) % 8;
  if(  uiAlignedBits )
  {
    RNOK( pcWriteIf->writeFlag( 1, "SEI: alignment_bit" ) );
    uiAlignedBits--;
    while( uiAlignedBits-- )
    {
      RNOK( pcWriteIf->writeFlag( 0, "SEI: alignment_bit" ) );
    }
  }
  return Err::m_nOK;
}


ErrVal
SEI::xReadPayloadHeader( HeaderSymbolReadIf*  pcReadIf,
                         MessageType&         reMessageType,
                         UInt&                ruiSize)
{
  { // type
    UInt uiTemp = 0xFF;
    UInt uiSum  = 0;
    while( 0xFF == uiTemp )
    {
      RNOK( pcReadIf->getCode( uiTemp, 8, "SEI: payload type") );
      uiSum += uiTemp;
    }
    reMessageType = (RESERVED_SEI <= uiSum ) ? RESERVED_SEI : MessageType( uiSum );
  }

  { // size
    UInt uiTemp  = 0xFF;
    UInt uiSum  = 0;

    while( 0xFF == uiTemp )
    {
      RNOK( pcReadIf->getCode( uiTemp, 8, "SEI: payload size") );
      uiSum += uiTemp;
    }
    ruiSize = uiSum;
  }
  return Err::m_nOK;
}



ErrVal
SEI::xCreate( SEIMessage*&  rpcSEIMessage,
              MessageType   eMessageType,
              // JVT-V068 {
              ParameterSetMng*& rpcParameterSetMng,
              // JVT-V068 }
              UInt          uiSize )
{
  switch( eMessageType )
  {
    case SUB_SEQ_INFO:  return SubSeqInfo ::create( (SubSeqInfo*&)  rpcSEIMessage );
    case SCALABLE_SEI:  return ScalableSei::create( (ScalableSei*&) rpcSEIMessage );
    case SUB_PIC_SEI:   return SubPicSei::create	( (SubPicSei*&)		rpcSEIMessage );
	case MOTION_SEI:	return MotionSEI::create( (MotionSEI*&) rpcSEIMessage );
    //{{Quality level estimation and modified truncation- JVTO044 and m12007
    //France Telecom R&D-(nathalie.cammas@francetelecom.com)
    //case QUALITYLEVEL_SEI: return QualityLevelSEI::create((QualityLevelSEI*&) rpcSEIMessage);//SEI changes update
		case PRIORITYLEVEL_SEI: return PriorityLevelSEI::create((PriorityLevelSEI*&) rpcSEIMessage);//SEI changes update
    //}}Quality level estimation and modified truncation- JVTO044 and m12007
  	case NON_REQUIRED_SEI: return NonRequiredSei::create((NonRequiredSei*&) rpcSEIMessage);
	// JVT-S080 LMI {
    case SCALABLE_SEI_LAYERS_NOT_PRESENT:  return ScalableSeiLayersNotPresent::create( (ScalableSeiLayersNotPresent*&) rpcSEIMessage );
	case SCALABLE_SEI_DEPENDENCY_CHANGE:   return ScalableSeiDependencyChange::create( (ScalableSeiDependencyChange*&) rpcSEIMessage );
	// JVT-S080 LMI }
    // JVT-W062
    case TL0_DEP_REP_IDX_SEI: return Tl0DepRepIdxSei::create( (Tl0DepRepIdxSei*&) rpcSEIMessage);
    // JVT-T073 {
	case SCALABLE_NESTING_SEI: return ScalableNestingSei::create( (ScalableNestingSei*&) rpcSEIMessage );
    // JVT-T073 }
// JVT-V068 {
    case AVC_COMPATIBLE_HRD_SEI:
      return AVCCompatibleHRD::create( (AVCCompatibleHRD*&)rpcSEIMessage, NULL );
    case PIC_TIMING:
      return PicTiming::create( (PicTiming*&)rpcSEIMessage, rpcParameterSetMng );
    case BUFFERING_PERIOD:
      return BufferingPeriod::create( (BufferingPeriod*&) rpcSEIMessage, rpcParameterSetMng );
// JVT-V068 }
		// JVT-W049 {
	  case REDUNDANT_PIC_SEI:
	    return RedundantPicSei::create( (RedundantPicSei*&) rpcSEIMessage );
    // JVT-W049 }
			//JVT-W052 wxwan
		case INTEGRITY_CHECK_SEI: return IntegrityCheckSEI::create((IntegrityCheckSEI*&) rpcSEIMessage );
			//JVT-W052 wxwan
		//JVT-X032 {
		case TL_SWITCHING_POINT_SEI:
			return TLSwitchingPointSei::create((TLSwitchingPointSei*&) rpcSEIMessage );
    //JVT-X032 }
    default :
      return UnsupportedSei::create( (UnsupportedSei*&)rpcSEIMessage, eMessageType, uiSize );
  }
  //return Err::m_nOK;
}


ErrVal
SEI::xWritePayloadHeader( HeaderSymbolWriteIf*  pcWriteIf,
                          MessageType           eMessageType,
                          UInt                  uiSize )
{
  { // type
    UInt uiTemp = eMessageType;
    UInt uiByte = 0xFF;

    while( 0xFF == uiByte )
    {
      uiByte  = (0xFF > uiTemp) ? uiTemp : 0xff;
      uiTemp -= 0xFF;
      RNOK( pcWriteIf->writeCode( uiByte, 8, "SEI: payload type") );
    }
  }

  { // size
    UInt uiTemp = uiSize;
    UInt uiByte = 0xFF;

    while( 0xFF == uiByte )
    {
      uiByte  = (0xFF > uiTemp) ? uiTemp : 0xff;
      uiTemp -= 0xFF;
      RNOK( pcWriteIf->writeCode( uiByte, 8, "SEI: payload size") );
    }
  }

  return Err::m_nOK;
}







//////////////////////////////////////////////////////////////////////////
//
//      S U B S E Q U E N C E     S E I
//
//////////////////////////////////////////////////////////////////////////

ErrVal
SEI::SubSeqInfo::create( SubSeqInfo*& rpcSEIMessage )
{
  SubSeqInfo* pcSubSeqInfo = new SubSeqInfo();
  rpcSEIMessage = pcSubSeqInfo;
  ROT( NULL == rpcSEIMessage )
  return Err::m_nOK;
}


ErrVal
SEI::SubSeqInfo::write( HeaderSymbolWriteIf* pcWriteIf )
{
  RNOK(   pcWriteIf->writeUvlc( m_uiSubSeqLayerNum,       "SubSeqSEI: sub_seq_layer_num") );
  RNOK(   pcWriteIf->writeUvlc( m_uiSubSeqId,             "SubSeqSEI: sub_seq_layer_id") );
  RNOK(   pcWriteIf->writeFlag( m_bFirstRefPicFlag,       "SubSeqSEI: first_ref_pic_flag" ) );
  RNOK(   pcWriteIf->writeFlag( m_bLeadingNonRefPicFlag,  "SubSeqSEI: leading_non_ref_pic_flag" ) );
  RNOK(   pcWriteIf->writeFlag( m_bLastPicFlag,           "SubSeqSEI: last_pic_flag" ) );
  RNOK(   pcWriteIf->writeFlag( m_bSubSeqFrameNumFlag,    "SubSeqSEI: sub_seq_frame_num_flag" ) );
  if( m_bSubSeqFrameNumFlag )
  {
    RNOK( pcWriteIf->writeUvlc( m_uiSubSeqFrameNum,       "SubSeqSEI: sub_seq_frame_num") );
  }
  return Err::m_nOK;
}


ErrVal
SEI::SubSeqInfo::read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  RNOK(   pcReadIf->getUvlc( m_uiSubSeqLayerNum,       "SubSeqSEI: sub_seq_layer_num") );
  RNOK(   pcReadIf->getUvlc( m_uiSubSeqId,             "SubSeqSEI: sub_seq_layer_id") );
  RNOK(   pcReadIf->getFlag( m_bFirstRefPicFlag,       "SubSeqSEI: first_ref_pic_flag" ) );
  RNOK(   pcReadIf->getFlag( m_bLeadingNonRefPicFlag,  "SubSeqSEI: leading_non_ref_pic_flag" ) );
  RNOK(   pcReadIf->getFlag( m_bLastPicFlag,           "SubSeqSEI: last_pic_flag" ) );
  RNOK(   pcReadIf->getFlag( m_bSubSeqFrameNumFlag,    "SubSeqSEI: sub_seq_frame_num_flag" ) );
  if( m_bSubSeqFrameNumFlag )
  {
    RNOK( pcReadIf->getUvlc( m_uiSubSeqFrameNum,       "SubSeqSEI: sub_seq_frame_num") );
  }
  return Err::m_nOK;
}


ErrVal
SEI::SubSeqInfo::init( UInt  uiSubSeqLayerNum,
                       UInt  uiSubSeqId,
                       Bool  bFirstRefPicFlag,
                       Bool  bLeadingNonRefPicFlag,
                       Bool  bLastPicFlag,
                       Bool  bSubSeqFrameNumFlag,
                       UInt  uiSubSeqFrameNum )

{
  m_uiSubSeqLayerNum      = uiSubSeqLayerNum;
  m_uiSubSeqId            = uiSubSeqId;
  m_bFirstRefPicFlag      = bFirstRefPicFlag;
  m_bLeadingNonRefPicFlag = bLeadingNonRefPicFlag;
  m_bLastPicFlag          = bLastPicFlag;
  m_bSubSeqFrameNumFlag   = bSubSeqFrameNumFlag;
  m_uiSubSeqFrameNum      = uiSubSeqFrameNum;
  return Err::m_nOK;
}





//////////////////////////////////////////////////////////////////////////
//
//      S C A L A B L E     S E I
//
//////////////////////////////////////////////////////////////////////////

SEI::ScalableSei::ScalableSei	()
: SEIMessage									( SCALABLE_SEI )
// JVT-U085 LMI
, m_temporal_id_nesting_flag( false )
//JVT-W051 {
, m_priority_layer_info_present_flag( false )
, m_pr_num_dId_minus1( 0 )
//JVT-W051 }
, m_priority_id_setting_flag	 ( true )//JVT-W053
, m_num_layers_minus1					( 0	)
{
	::memset( m_layer_id, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_priority_id, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
  ::memset( m_discardable_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_temporal_level, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_dependency_id, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_quality_level, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_sub_pic_layer_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_sub_region_layer_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_iroi_division_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) ); //JVT-W051
	::memset( m_profile_level_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_bitrate_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_frm_rate_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_frm_size_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_layer_dependency_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
  ::memset( m_parameter_sets_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_bitstream_restriction_info_present_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_exact_interlayer_pred_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_exact_sample_value_match_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );  //JVT-S036
  ::memset( m_layer_conversion_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_layer_output_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_layer_profile_level_idc, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );//JVT-W051
	::memset( m_avg_bitrate, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_max_bitrate_layer, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_max_bitrate_layer_representation, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );//JVT-W051
	::memset( m_max_bitrate_calc_window, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_constant_frm_rate_idc, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_avg_frm_rate, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_frm_width_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_frm_height_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_base_region_layer_id, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_dynamic_rect_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_horizontal_offset, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_vertical_offset, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_region_width, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_region_height, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_roi_id, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_iroi_grid_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool) );
	::memset( m_grid_width_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );//JVT-W051
	::memset( m_grid_height_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );//JVT-W051
	::memset( m_num_rois_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_first_mb_in_roi, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_roi_width_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_roi_height_in_mbs_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_num_directly_dependent_layers, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_directly_dependent_layer_id_delta_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_layer_dependency_info_src_layer_id_delta, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_num_seq_parameter_set_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_seq_parameter_set_id_delta, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_num_subset_seq_parameter_set_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
  ::memset( m_subset_seq_parameter_set_id_delta, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_num_pic_parameter_set_minus1, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_pic_parameter_set_id_delta, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_parameter_sets_info_src_layer_id_delta, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) ); //JVT-S036
	//JVT-W051 {
	::memset( m_motion_vectors_over_pic_boundaries_flag, 0x01, MAX_SCALABLE_LAYERS*sizeof(Bool));
	::memset( m_max_bytes_per_pic_denom, 0x02, MAX_SCALABLE_LAYERS*sizeof(UInt));
	::memset( m_max_bits_per_mb_denom, 0x01, MAX_SCALABLE_LAYERS*sizeof(UInt));
	::memset( m_log2_max_mv_length_horizontal, 0x10, MAX_SCALABLE_LAYERS*sizeof(UInt));
	::memset( m_log2_max_mv_length_vertical, 0x10, MAX_SCALABLE_LAYERS*sizeof(UInt));
	::memset( m_num_reorder_frames, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt));
	::memset( m_max_dec_frame_buffering, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt));
	//JVT-W051 }
	//JVT-W046 {
  ::memset( m_conversion_type_idc  , 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
	::memset( m_rewriting_info_flag, 0x00, MAX_SCALABLE_LAYERS*sizeof(Bool*) );
	::memset( m_rewriting_profile_level_idc, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt*) );
	::memset( m_rewriting_avg_bitrate, 0x00, MAX_SCALABLE_LAYERS*sizeof(Double*) );
	::memset( m_rewriting_max_bitrate, 0x00, MAX_SCALABLE_LAYERS*sizeof(Double*) );
	//JVT-W046 }
	//JVT-W051 { 
  ::memset( m_pr_dependency_id, 0x00, MAX_LAYERS*sizeof(UInt));
	::memset( m_pr_num_minus1, 0x00, MAX_LAYERS*sizeof(UInt));
	::memset( m_pr_id, 0x00, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt));
	::memset( m_pr_profile_level_idc, 0x00, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt));
	::memset( m_pr_avg_bitrate, 0x00, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt));
	::memset( m_pr_max_bitrate, 0x00, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt));
	//JVT-W051 }
  ::memset(m_aiNumRoi, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt));//really bad!!!
}

SEI::ScalableSei::~ScalableSei()
{
	// JVT-S054 (ADD) ->
	UInt i;
	for( i = 0; i < MAX_SCALABLE_LAYERS; i++ )
	{
		if ( m_first_mb_in_roi[i] != NULL )
		{
			free( m_first_mb_in_roi[i] );
			m_first_mb_in_roi[i] = NULL;
		}
		if ( m_roi_width_in_mbs_minus1[i] != NULL )
		{
			free( m_roi_width_in_mbs_minus1[i] );
			m_roi_width_in_mbs_minus1[i] = NULL;
		}
		if ( m_roi_height_in_mbs_minus1[i] != NULL )
		{
			free( m_roi_height_in_mbs_minus1[i] );
			m_roi_height_in_mbs_minus1[i] = NULL;
		}
	}
	// JVT-S054 (ADD) <-
}

ErrVal
SEI::ScalableSei::create( ScalableSei*& rpcSeiMessage )
{
	rpcSeiMessage = new ScalableSei();
	ROT( NULL == rpcSeiMessage )
		return Err::m_nOK;
}

//TMM_FIX
ErrVal
SEI::ScalableSei::destroy()
{
	delete this;
	return Err::m_nOK;
}
//TMM_FIX

ErrVal
SEI::ScalableSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
  UInt i=0, j=0;
  // JVT-U085 LMI
  RNOK    ( pcWriteIf->writeFlag( m_temporal_id_nesting_flag,                "ScalableSEI: temporal_id_nesting_flag"                         ) );
	RNOK    ( pcWriteIf->writeFlag( m_priority_layer_info_present_flag,	       "ScalableSEI: priority_layer_info_present_flag"                 ) );//JVT-W051
	RNOK    ( pcWriteIf->writeFlag( m_priority_id_setting_flag,                "ScalableSEI: priority_id_setting_flag"                         ) );//JVT-W053
	ROF     ( m_num_layers_minus1+1 );
	RNOK		( pcWriteIf->writeUvlc( m_num_layers_minus1,											 "ScalableSEI: num_layers_minus1"										             ) );

	for( i = 0; i <= m_num_layers_minus1; i++ )
	{
		if (0 < m_aiNumRoi[m_dependency_id[i]])
		{
			m_sub_pic_layer_flag[i] = true;
			m_roi_id[i]				= m_aaiRoiID[m_dependency_id[i]][0];
		}

		//JVT-W051 {
		RNOK	( pcWriteIf->writeUvlc( m_layer_id[i],	                                                   "ScalableSEI: layer_id"	                              ) );
		//JVT-W051 }
	//JVT-S036 start
		RNOK	( pcWriteIf->writeCode( m_priority_id[i],					                            6,		       "ScalableSEI: priority_id"										          ) );
		RNOK  ( pcWriteIf->writeFlag( m_discardable_flag[i],								                             "ScalableSEI: discardable_flag"											  ) );
		RNOK	( pcWriteIf->writeCode( m_dependency_id[i],							                      3,		       "ScalableSEI: dependency_id"											      ) );
    RNOK	( pcWriteIf->writeCode( m_quality_level[i],									                  4,		       "ScalableSEI: quality_id"	   												  ) );
		RNOK	( pcWriteIf->writeCode( m_temporal_level[i],								                  3,		       "ScalableSEI: temporal_id"												      ) );
		RNOK	( pcWriteIf->writeFlag( m_sub_pic_layer_flag[i],										                       "ScalableSEI: sub_pic_layer_flag"										  ) );
		RNOK	( pcWriteIf->writeFlag( m_sub_region_layer_flag[i],									                       "ScalableSEI: sub_region_layer_flag"									  ) );
		RNOK	( pcWriteIf->writeFlag( m_iroi_division_info_present_flag[i],					                     "ScalableSEI: iroi_division_info_present_flag"				  ) ); //JVT-W051
		RNOK	( pcWriteIf->writeFlag( m_profile_level_info_present_flag[i],				                       "ScalableSEI: profile_level_info_present_flag"				  ) );
	//JVT-S036 end
		RNOK	( pcWriteIf->writeFlag( m_bitrate_info_present_flag[i],							                       "ScalableSEI: bitrate_info_present_flag"							  ) );
		RNOK	( pcWriteIf->writeFlag( m_frm_rate_info_present_flag[i],						                       "ScalableSEI: frm_rate_info_present_flag"						  ) );
		RNOK	( pcWriteIf->writeFlag( m_frm_size_info_present_flag[i],						                       "ScalableSEI: frm_size_info_present_flag"						  ) );
		RNOK	( pcWriteIf->writeFlag( m_layer_dependency_info_present_flag[i],		                       "ScalableSEI: layer_dependency_info_present_flag"		  ) );
		RNOK	( pcWriteIf->writeFlag( m_parameter_sets_info_present_flag[i],	                           "ScalableSEI: parameter_sets_info_present_flag"        ) );//SEI changes update
		RNOK	( pcWriteIf->writeFlag(	m_bitstream_restriction_info_present_flag[i],					             "ScalableSEI: bitstream_restriction_info_present_flag" ) );  //JVT-051 & JVT-W064
		RNOK	( pcWriteIf->writeFlag( m_exact_interlayer_pred_flag[i],						                       "ScalableSEI: exact_interlayer_pred_flag"              ) );//JVT-S036
		if( m_sub_pic_layer_flag[i] || m_iroi_division_info_present_flag[i] )
			RNOK	( pcWriteIf->writeFlag( m_exact_sample_value_match_flag[i],						                   "ScalableSEI: exact_sample_value_match_flag"           ) );
		RNOK	( pcWriteIf->writeFlag( m_layer_conversion_flag[i],					   	                           "ScalableSEI: layer_conversion_flag"                   ) );//JVT-W046
		RNOK	( pcWriteIf->writeFlag( m_layer_output_flag[i],											                       "ScalableSEI: layer_output_flag"                       ) );//JVT-W047 wxwan
		if ( m_profile_level_info_present_flag[i] )
		{
			//JVT-W051 {
			RNOK	( pcWriteIf->writeCode( m_layer_profile_level_idc[i],							           24,		        "ScalableSEI: layer_profile_level_idc"								) );
			//JVT-W051 }
		}

		if ( m_bitrate_info_present_flag[i] )
		{
			RNOK	( pcWriteIf->writeCode( m_avg_bitrate[i],										                 16,		        "ScalableSEI: avg_bitrate"														) );
//JVT-S036 lsj start
			RNOK	( pcWriteIf->writeCode( m_max_bitrate_layer[i],										           16,		        "ScalableSEI: max_bitrate_layer"											) );
			RNOK	( pcWriteIf->writeCode( m_max_bitrate_layer_representation[i],							 16,		        "ScalableSEI: max_bitrate_layer_representation"						) );//JVT-W051
			RNOK	( pcWriteIf->writeCode( m_max_bitrate_calc_window[i],							           16,		        "ScalableSEI: max_bitrate_calc_window"											) );
//JVT-S036 lsj end
		}
		if ( m_frm_rate_info_present_flag[i] )
		{
			RNOK	( pcWriteIf->writeCode( m_constant_frm_rate_idc[i],			                      2,		        "ScalableSEI: constant_frm_bitrate_idc"							) );
			RNOK	( pcWriteIf->writeCode( m_avg_frm_rate[i],									                 16,		        "ScalableSEI: avg_frm_rate"													) );
		}

		if ( m_frm_size_info_present_flag[i] || m_iroi_division_info_present_flag[i] )
		{
			RNOK	( pcWriteIf->writeUvlc( m_frm_width_in_mbs_minus1[i],							                          "ScalableSEI: frm_width_in_mbs_minus1"							) );
			RNOK	( pcWriteIf->writeUvlc( m_frm_height_in_mbs_minus1[i],						                          "ScalableSEI: frm_height_in_mbs_minus1"							) );
		}

		if ( m_sub_region_layer_flag[i] )
		{
			//JVT-W051 {
			RNOK	( pcWriteIf->writeUvlc ( m_base_region_layer_id[i],	                                        "ScalableSEI: base_region_layer_id"	                ) );
			//JVT-W051 }
			RNOK	( pcWriteIf->writeFlag ( m_dynamic_rect_flag[i],									                          "ScalableSEI: dynamic_rect_flag"										) );
			//JVT-W051 {
			if ( !m_dynamic_rect_flag[i] )
			//JVT-W051 }
			{
				RNOK	( pcWriteIf->writeCode ( m_horizontal_offset[i],					                16,		          "ScalableSEI: horizontal_offset"										) );
				RNOK	( pcWriteIf->writeCode ( m_vertical_offset[i],						                16,		          "ScalableSEI: vertical_offset"											) );
				RNOK	( pcWriteIf->writeCode ( m_region_width[i],								                16,		          "ScalableSEI: region_width"													) );
				RNOK	( pcWriteIf->writeCode ( m_region_height[i],							                16,		          "ScalableSEI: region_height"												) );
			}
		}

		if( m_sub_pic_layer_flag[i] )
		{//JVT-S036 lsj
			//JVT-W051 {
			RNOK	( pcWriteIf->writeUvlc( m_roi_id[i],	                                                      "Scalable: roi_id"                                  ) );
			//JVT-W051 }
		}

	//JVT-S036 lsj start
		if ( m_iroi_division_info_present_flag[i] )//JVT-W051
		{
			//JVT-W051 {
			RNOK	( pcWriteIf->writeFlag( m_iroi_grid_flag[i],				                                        "ScalableSEI: iroi_grid_flag"                       ) );
			//JVT-W051 }
			if( m_iroi_grid_flag[i] )
			{
				RNOK	( pcWriteIf->writeUvlc( m_grid_width_in_mbs_minus1[i],                                    "ScalableSEI: grid_width_in_mbs_minus1"             ) );//JVT-W051
				RNOK	( pcWriteIf->writeUvlc( m_grid_height_in_mbs_minus1[i],                                   "ScalableSEI: grid_height_in_mbs_minus1"            ) );//JVT-W051
			}
			else
			{
				RNOK	( pcWriteIf->writeUvlc( m_num_rois_minus1[i],		                                          "ScalableSEI: num_rois_minus1"                      ) );
				for ( j = 0; j <= m_num_rois_minus1[i]; j++ )
				{
					RNOK	( pcWriteIf->writeUvlc( m_first_mb_in_roi[i][j],				                                "ScalableSEI: first_mb_in_roi"                      ) );
					RNOK	( pcWriteIf->writeUvlc( m_roi_width_in_mbs_minus1[i][j],		                            "ScalableSEI: roi_width_in_mbs_minus1"              ) );
					RNOK	( pcWriteIf->writeUvlc( m_roi_height_in_mbs_minus1[i][j],		                            "ScalableSEI: roi_height_in_mbs_minus1"             ) );
				}
      }
    }

	//JVT-S036 lsj end
		if ( m_layer_dependency_info_present_flag[i] )
		{
			RNOK	( pcWriteIf->writeUvlc ( m_num_directly_dependent_layers[i],			                          "ScalableSEI: num_directly_dependent_layers"			  ) );
// BUG_FIX liuhui{
		    for( j = 0; j < m_num_directly_dependent_layers[i]; j++ )
		    {
		      RNOK( pcWriteIf->writeUvlc (m_directly_dependent_layer_id_delta_minus1[i][j],                 "ScalableSEI: directly_dependent_layers_id_delta_minus1"		) );  //JVT-S036 lsj
		    }
// BUG_FIX liuhui}
		}
		else
		{//JVT-S036 lsj
			RNOK( pcWriteIf->writeUvlc(m_layer_dependency_info_src_layer_id_delta[i],	                        "ScalableSEI: layer_dependency_info_src_layer_id_delta"     ) );
		}

		if ( m_parameter_sets_info_present_flag[i] )
		{

// BUG_FIX liuhui{
			RNOK	( pcWriteIf->writeUvlc ( m_num_seq_parameter_set_minus1[i],	                                "ScalableSEI: num_seq_parameter_set_minus1"			    ) );
			for( j = 0; j <= m_num_seq_parameter_set_minus1[i]; j++ )
			{
			  RNOK( pcWriteIf->writeUvlc ( m_seq_parameter_set_id_delta[i][j],                                "ScalableSEI: seq_parameter_set_id_delta"				    ) );
			}
      RNOK	( pcWriteIf->writeUvlc ( m_num_subset_seq_parameter_set_minus1[i],	                        "ScalableSEI: num_subset_seq_parameter_set_minus1"	) );
			for( j = 0; j <= m_num_subset_seq_parameter_set_minus1[i]; j++ )
			{
			  RNOK( pcWriteIf->writeUvlc ( m_subset_seq_parameter_set_id_delta[i][j],                         "ScalableSEI: subset_seq_parameter_set_id_delta"		) );
			}
			RNOK( pcWriteIf->writeUvlc ( m_num_pic_parameter_set_minus1[i],	                                  "ScalableSEI: num_pic_parameter_set_minus1"			    ) );
			for( j = 0; j <= m_num_pic_parameter_set_minus1[i]; j++ )
			{
			  RNOK ( pcWriteIf->writeUvlc ( m_pic_parameter_set_id_delta[i][j],                               "ScalableSEI: pic_parameter_set_id_delta"				    ) );
			}
// BUG_FIX liuhui}
		}
		else
		{//JVT-S036 lsj
			RNOK	(pcWriteIf->writeUvlc( m_parameter_sets_info_src_layer_id_delta[i],                         "ScalableSEI: parameter_sets_info_src_layer_id_delta"	) );
		}
		//JVT-W051 & JVT-W064 {
		if (m_bitstream_restriction_info_present_flag[i])
		{
			RNOK	(pcWriteIf->writeFlag(m_motion_vectors_over_pic_boundaries_flag[i],                         "ScalableSEI: motion_vectors_over_pic_boundaries_flag") );
			RNOK	(pcWriteIf->writeUvlc(m_max_bytes_per_pic_denom[i],                                         "ScalableSEI: max_bytes_per_pic_denom"              ) );
			RNOK	(pcWriteIf->writeUvlc(m_max_bits_per_mb_denom[i],                                           "ScalableSEI: max_bits_per_mb_denom"                ) );
			RNOK	(pcWriteIf->writeUvlc(m_log2_max_mv_length_horizontal[i],                                   "ScalableSEI: log2_max_mv_length_horizontal"        ) );
			RNOK	(pcWriteIf->writeUvlc(m_log2_max_mv_length_vertical[i],                                     "ScalableSEI: log2_max_mv_length_vertical"          ) );
			RNOK	(pcWriteIf->writeUvlc(m_num_reorder_frames[i],                                              "ScalableSEI: num_reorder_frames"                   ) );
			RNOK	(pcWriteIf->writeUvlc(m_max_dec_frame_buffering[i],                                         "ScalableSEI: max_dec_frame_buffering"              ) );
		}
		//JVT-W051 & JVT-W064  }
		//JVT-W046 {
		if( m_layer_conversion_flag[i] )
		{
		  RNOK ( pcWriteIf->writeUvlc ( m_conversion_type_idc[i],                                           "ScalableSEI: m_conversion_type_idc"				        ) );
		  for ( j=0; j<2; j++ )
		  {
		    RNOK	( pcWriteIf->writeFlag ( m_rewriting_info_flag[i][j],						                          "ScalableSEI: m_rewriting_info_flag"								) );
			if( m_rewriting_info_flag[i][j] )
			{
			  RNOK	( pcWriteIf->writeCode( m_rewriting_profile_level_idc[i][j],		             24,		      "ScalableSEI: m_rewriting_profile_level_idc"        ) );
			  RNOK	( pcWriteIf->writeCode( m_rewriting_avg_bitrate[i][j],		                   16,		      "ScalableSEI: m_rewriting_avg_bitrate"              ) );
			  RNOK	( pcWriteIf->writeCode( m_rewriting_max_bitrate[i][j],		                   16,		      "ScalableSEI: m_rewriting_max_bitrate"              ) );
			}
		  }
		}
		//JVT-W046 }
	}
	//JVT-W051 {
	if (m_priority_layer_info_present_flag)
	{
		RNOK	(pcWriteIf->writeUvlc(m_pr_num_dId_minus1,                                                    "ScalableSEI: pr_num_dId_minus1"                    ) );
		for (i=0; i<=m_pr_num_dId_minus1; i++)
		{
			RNOK	(pcWriteIf->writeCode(m_pr_dependency_id[i],                                    3,          "ScalableSEI: pr_dependency_id"                     ) );
			RNOK	(pcWriteIf->writeUvlc(m_pr_num_minus1[i],                                                   "ScalableSEI: pr_num_minus"                         ) );
			for (j=0; j<=m_pr_num_minus1[i]; j++)
			{
				RNOK	(pcWriteIf->writeUvlc(m_pr_id[i][j],                                                      "ScalableSEI: pr_id"                                ) );
				RNOK	(pcWriteIf->writeCode(m_pr_profile_level_idc[i][j],                          24,          "ScalableSEI: pr_profile_level_idc"                 ) );
				RNOK	(pcWriteIf->writeCode(m_pr_avg_bitrate[i][j],                                16,          "ScalableSEI: pr_avg_bitrate"                       ) );
				RNOK	(pcWriteIf->writeCode(m_pr_max_bitrate[i][j],                                16,          "ScalableSEI: pr_max_bitrate"                       ) );
			}
		}
	}
	//JVT-W051 }
	//JVT-W053 wxwan
	if(m_priority_id_setting_flag)
	{
		UInt PriorityIdSettingUriIdx = 0;
		do
		{
			UInt uiTemp = priority_id_setting_uri[PriorityIdSettingUriIdx];
			RNOK( pcWriteIf->writeCode( uiTemp,                                                   8,          "ScalableSEI: priority_id_setting_uri"              ) );
		}while( priority_id_setting_uri[ PriorityIdSettingUriIdx++ ]  !=  0 );
	}
	//JVT-W053 wxwan

	return Err::m_nOK;
}


ErrVal
SEI::ScalableSei::read ( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  UInt i, j=0;
  UInt rl;//JVT-S036 lsj
  // JVT-U085 LMI
  RNOK  ( pcReadIf->getFlag( m_temporal_id_nesting_flag,                               "ScalableSEI: temporal_id_nesting_flag" ) );
	RNOK	( pcReadIf->getFlag( m_priority_layer_info_present_flag,					             "ScalableSEI: priority_layer_info_present_flag" ) );//JVT-W051
	RNOK  ( pcReadIf->getFlag( m_priority_id_setting_flag,                               "ScalableSEI: priority_id_setting_flag" ) );//JVT-W053
	RNOK	( pcReadIf->getUvlc( m_num_layers_minus1 ,																     "ScalableSEI: num_layers_minus1"	) );

	for ( i = 0; i <= m_num_layers_minus1; i++ )
	{
		//JVT-W051 {
		RNOK	( pcReadIf->getUvlc( m_layer_id[i],	                                         "ScalableSEI: layer_id" ) );
		//JVT-W051 }
	//JVT-S036 lsj start
		RNOK	( pcReadIf->getCode( m_priority_id[i],													        6,	 "ScalableSEI: priority_id" ) );
		RNOK	( pcReadIf->getFlag( m_discardable_flag[i],													         "ScalableSEI: discardable_flag" ) );
		RNOK	( pcReadIf->getCode( m_dependency_id[i],												        3,	 "ScalableSEI: dependency_id"	) );
    RNOK	( pcReadIf->getCode( m_quality_level[i],													      4,	 "ScalableSEI: quality_id"	) );
    RNOK	( pcReadIf->getCode( m_temporal_level[i],													      3,	 "ScalableSEI: temporal_id"	) );
		RNOK	( pcReadIf->getFlag( m_sub_pic_layer_flag[i],														     "ScalableSEI: sub_pic_layer_flag"	) );
		RNOK	( pcReadIf->getFlag( m_sub_region_layer_flag[i],													   "ScalableSEI: sub_region_layer_flag"	) );
		RNOK	( pcReadIf->getFlag( m_iroi_division_info_present_flag[i],						       "ScalableSEI: iroi_division_info_present_flag" ) );//JVT-W051
		RNOK	( pcReadIf->getFlag( m_profile_level_info_present_flag[i],								   "ScalableSEI: profile_level_info_present_flag"	) );
   //JVT-S036 lsj end
		RNOK	( pcReadIf->getFlag( m_bitrate_info_present_flag[i],											  "ScalableSEI: bitrate_info_present_flag"	) );
		RNOK	( pcReadIf->getFlag( m_frm_rate_info_present_flag[i],											  "ScalableSEI: frm_rate_info_present_flag"	) );
		RNOK	( pcReadIf->getFlag( m_frm_size_info_present_flag[i],											  "ScalableSEI: frm_size_info_present_flag"	) );
		RNOK	( pcReadIf->getFlag( m_layer_dependency_info_present_flag[i],								"ScalableSEI: layer_dependency_info_present_flag"	) );
		RNOK	( pcReadIf->getFlag( m_parameter_sets_info_present_flag[i],						      "ScalableSEI: parameter_sets_info_present_flag"	) );
		RNOK	( pcReadIf->getFlag( m_bitstream_restriction_info_present_flag[i],          "ScalableSEI: bitstream_restriction_info_present_flag"  ) );//JVT-W051
		RNOK	( pcReadIf->getFlag( m_exact_interlayer_pred_flag[i],											  "ScalableSEI: exact_interlayer_pred_flag"  ) );//JVT-S036 lsj
		if( m_sub_pic_layer_flag[ i ] || m_iroi_division_info_present_flag[ i ] )
	    RNOK	( pcReadIf->getFlag( m_exact_sample_value_match_flag[i],									"ScalableSEI: exact_sample_value_match_flag"  ) );
    RNOK	( pcReadIf->getFlag( m_layer_conversion_flag[i],											      "ScalableSEI: layer_conversion_flag"  ) );//JVT-W046
		RNOK	( pcReadIf->getFlag( m_layer_output_flag[i],											          "ScalableSEI: layer_output_flag"  ) );//JVT-W047 wxwan
		if( m_profile_level_info_present_flag[i] )
		{
      //JVT-W051 {
      RNOK( pcReadIf->getCode( m_layer_profile_level_idc[i],	24,		                  "ScalableSEI: layer_profile_level_idc"	) );
      //JVT-W051 }
		}

		if( m_bitrate_info_present_flag[i] )
		{
			RNOK	( pcReadIf->getCode( m_avg_bitrate[i],														  16,		"ScalableSEI: avg_bitrate"	) );
	//JVT-S036 lsj start
			RNOK	( pcReadIf->getCode( m_max_bitrate_layer[i],												16,		"ScalableSEI: max_bitrate_layer"	) );
			RNOK	( pcReadIf->getCode( m_max_bitrate_layer_representation[i],					16,		"ScalableSEI: max_bitrate_layer_representation"	) );//JVT-W051
			RNOK	( pcReadIf->getCode( m_max_bitrate_calc_window[i],									16,		"ScalableSEI: max_bitrate_calc_window"	) );
	//JVT-S036 lsj end
		}

		if( m_frm_rate_info_present_flag[i] )
		{
			RNOK	( pcReadIf->getCode( m_constant_frm_rate_idc[i],									   2,		"ScalableSEI: constant_frm_bitrate_idc"	) );
			RNOK	( pcReadIf->getCode( m_avg_frm_rate[i],														  16,		"ScalableSEI: avg_frm_rate"	) );
		}

		if( m_frm_size_info_present_flag[i] ||  m_iroi_division_info_present_flag[i] )
		{
			RNOK	( pcReadIf->getUvlc( m_frm_width_in_mbs_minus1[i],											  "ScalableSEI: frm_width_in_mbs_minus1"	) );
			RNOK	( pcReadIf->getUvlc( m_frm_height_in_mbs_minus1[i],											  "ScalableSEI: frm_height_in_mbs_minus1"	) );
		}

		if( m_sub_region_layer_flag[i] )
		{
			//JVT-W051 {
			RNOK	( pcReadIf->getUvlc( m_base_region_layer_id[i],                          "ScalableSEI: base_region_layer_id"  ) );
			//JVT-W051 }
			RNOK	( pcReadIf->getFlag( m_dynamic_rect_flag[i],														 "ScalableSEI: dynamic_rect_flag"	 ) );
			//JVT-W051 {
			if ( !m_dynamic_rect_flag[i] )
			//JVT-W051 }
			{
				RNOK( pcReadIf->getCode( m_horizontal_offset[i],											  16,  "ScalableSEI: horizontal_offset"	 ) );
				RNOK( pcReadIf->getCode( m_vertical_offset[i],												  16,	 "ScalableSEI: vertical_offset"	 ) );
				RNOK( pcReadIf->getCode( m_region_width[i],														  16,	 "ScalableSEI: region_width"	 ) );
				RNOK( pcReadIf->getCode( m_region_height[i],													  16,	 "ScalableSEI: region_height"	 ) );
			}
		}

	//JVT-S036 lsj start
		if( m_sub_pic_layer_flag[i] )
		{
			//JVT-W051 {
			RNOK	( pcReadIf->getUvlc( m_roi_id[i],                                       "Scalable: roi_id"   ) );
			//JVT-W051 }
		}
		if( m_iroi_division_info_present_flag[i] )//JVT-W051
		{
			//JVT-W051 {
			RNOK	( pcReadIf->getFlag( m_iroi_grid_flag[i],			                   "ScalableSEI: iroi_grid_flag" ) );
			//JVT-W051 }
			if( m_iroi_grid_flag[i] )
			{
				RNOK	( pcReadIf->getUvlc( m_grid_width_in_mbs_minus1[i],            "ScalableSEI: grid_width_in_mbs_minus1"  ) );//JVT-W051
				RNOK	( pcReadIf->getUvlc( m_grid_height_in_mbs_minus1[i],           "ScalableSEI: grid_height_in_mbs_minus1" ) );//JVT-W051
			}
			else
			{
				RNOK	( pcReadIf->getUvlc( m_num_rois_minus1[i],		                 "ScalableSEI: num_rois_minus1"           ) );
    		// JVT-S054 (ADD) ->
				if ( m_first_mb_in_roi[i] != NULL )
					free( m_first_mb_in_roi[i] );
        m_first_mb_in_roi[i] = (UInt*)malloc( (m_num_rois_minus1[i]+1)*sizeof(UInt) );
				if ( m_roi_width_in_mbs_minus1[i] != NULL )
					free( m_roi_width_in_mbs_minus1[i] );
        m_roi_width_in_mbs_minus1[i] = (UInt*)malloc( (m_num_rois_minus1[i]+1)*sizeof(UInt) );
				if ( m_roi_height_in_mbs_minus1[i] != NULL )
					free( m_roi_height_in_mbs_minus1[i] );
        m_roi_height_in_mbs_minus1[i] = (UInt*)malloc( (m_num_rois_minus1[i]+1)*sizeof(UInt) );
    		// JVT-S054 (ADD) <-
				for ( j = 0; j <= m_num_rois_minus1[i]; j++ )
				{
					RNOK	( pcReadIf->getUvlc( m_first_mb_in_roi[i][j],				        "ScalableSEI: first_mb_in_roi" ) );
					RNOK	( pcReadIf->getUvlc( m_roi_width_in_mbs_minus1[i][j],		    "ScalableSEI: roi_width_in_mbs_minus1" ) );
					RNOK	( pcReadIf->getUvlc( m_roi_height_in_mbs_minus1[i][j],		  "ScalableSEI: roi_height_in_mbs_minus1" ) );
				}
			}
		}
   //JVT-S036 lsj end

		if( m_layer_dependency_info_present_flag[i] )
		{
			RNOK	( pcReadIf->getUvlc( m_num_directly_dependent_layers[i],								"ScalableSEI: num_directly_dependent_layers"	) );
// BUG_FIX liuhui{
			for( j = 0; j < m_num_directly_dependent_layers[i]; j++ )
			{
				RNOK  ( pcReadIf->getUvlc( m_directly_dependent_layer_id_delta_minus1[i][j],				"ScalableSEI: directly_dependent_layers_id_delta_minus1"  ) );//JVT-S036 lsj
			}
// BUG_FIX liuhui}
		}
		else
		{//JVT-S036 lsj
			RNOK	( pcReadIf->getUvlc( m_layer_dependency_info_src_layer_id_delta[i],			"ScalableSEI: layer_dependency_info_src_layer_id_delta"  ) );
			rl = m_layer_id[i] - m_layer_dependency_info_src_layer_id_delta[i];
			m_num_directly_dependent_layers[i] = m_num_directly_dependent_layers[rl];
			for( j = 0; j < m_num_directly_dependent_layers[i]; j++ )
			{
				m_directly_dependent_layer_id_delta_minus1[i][j] = m_directly_dependent_layer_id_delta_minus1[rl][j];
			}
		}

		if( m_parameter_sets_info_present_flag[i] )
		{
// BUG_FIX liuhui{
			RNOK    ( pcReadIf->getUvlc( m_num_seq_parameter_set_minus1[i],              "ScalableSEI: num_seq_parameter_set_minus1"  ) );
			for( j = 0; j <= m_num_seq_parameter_set_minus1[i]; j++ )
			{
			  RNOK	( pcReadIf->getUvlc( m_seq_parameter_set_id_delta[i][j],					     "ScalableSEI: seq_parameter_set_id_delta"	) );
			}
			RNOK    ( pcReadIf->getUvlc( m_num_subset_seq_parameter_set_minus1[i],              "ScalableSEI: num_subset_seq_parameter_set_minus1"  ) );
			for( j = 0; j <= m_num_subset_seq_parameter_set_minus1[i]; j++ )
			{
			  RNOK	( pcReadIf->getUvlc( m_subset_seq_parameter_set_id_delta[i][j],				"ScalableSEI: subset_seq_parameter_set_id_delta"	) );
			}
			RNOK	( pcReadIf->getUvlc( m_num_pic_parameter_set_minus1[i],						     "ScalableSEI: num_pic_parameter_set_minus1"	 ) );
			for( j = 0; j <= m_num_pic_parameter_set_minus1[i]; j++ )
			{
				RNOK	( pcReadIf->getUvlc( m_pic_parameter_set_id_delta[i][j],					     "ScalableSEI: pic_parameter_set_id_delta"	) );
			}
// BUG_FIX liuhui}
		}
		else
		{//JVT-S036 lsj
			RNOK	( pcReadIf->getUvlc( m_parameter_sets_info_src_layer_id_delta[i],     "ScalableSEI: parameter_sets_info_src_layer_id_delta"  ) );
			rl = m_layer_id[i] - m_parameter_sets_info_src_layer_id_delta[i];
			m_num_seq_parameter_set_minus1[i] = m_num_seq_parameter_set_minus1[rl];
			for( j = 0; j <= m_num_seq_parameter_set_minus1[i]; j++ )
			{
			  m_seq_parameter_set_id_delta[i][j] = m_seq_parameter_set_id_delta[rl][j];
			}
      m_num_subset_seq_parameter_set_minus1[i] = m_num_subset_seq_parameter_set_minus1[rl];
			for( j = 0; j <= m_num_seq_parameter_set_minus1[i]; j++ )
			{
			  m_subset_seq_parameter_set_id_delta[i][j] = m_subset_seq_parameter_set_id_delta[rl][j];
			}
			m_num_pic_parameter_set_minus1[i] = m_num_pic_parameter_set_minus1[rl];
			for( j = 0; j <= m_num_pic_parameter_set_minus1[i]; j++ )
			{
				m_pic_parameter_set_id_delta[i][j] = m_pic_parameter_set_id_delta[rl][j];
			}
		}
		//JVT-W051 {
		if (m_bitstream_restriction_info_present_flag[i])
		{
			RNOK	(pcReadIf->getFlag( m_motion_vectors_over_pic_boundaries_flag[i],   "ScalableSEI: motion_vectors_over_pic_boundaries_flag"   ) );
			RNOK	(pcReadIf->getUvlc( m_max_bytes_per_pic_denom[i],                   "ScalableSEI: max_bytes_per_pic_denom"   ) );
			RNOK	(pcReadIf->getUvlc( m_max_bits_per_mb_denom[i],                     "ScalableSEI: max_bits_per_mb_denom"   ) );
			RNOK	(pcReadIf->getUvlc( m_log2_max_mv_length_horizontal[i],             "ScalableSEI: log2_max_mv_length_horizontal"   ) );
			RNOK	(pcReadIf->getUvlc( m_log2_max_mv_length_vertical[i],               "ScalableSEI: log2_max_mv_length_vertical"   ) );
			RNOK	(pcReadIf->getUvlc( m_num_reorder_frames[i],                        "ScalableSEI: num_reorder_frames"   ) );
			RNOK	(pcReadIf->getUvlc( m_max_dec_frame_buffering[i],                   "ScalableSEI: max_dec_frame_buffering"   ) );
		}
		//JVT-W051 }
		//JVT-W046 {
		if( m_layer_conversion_flag[i] )
		{
      RNOK ( pcReadIf->getUvlc ( m_conversion_type_idc[i],                            "ScalableSEI: m_conversion_type_idc"		     ) );
		  for ( j=0; j<2; j++ )
		  {
		    RNOK	( pcReadIf->getFlag ( m_rewriting_info_flag[i][j],						          "ScalableSEI: m_rewriting_info_flag"			   ) );
			  if( m_rewriting_info_flag[i][j] )
			  {
          RNOK	( pcReadIf->getCode ( m_rewriting_profile_level_idc[i][j],	24,		    "ScalableSEI: m_rewriting_profile_level_idc"        ) );
			    RNOK	( pcReadIf->getCode ( m_rewriting_avg_bitrate[i][j],		        16,		"ScalableSEI: m_rewriting_avg_bitrate"         ) );
			    RNOK	( pcReadIf->getCode ( m_rewriting_max_bitrate[i][j],		        16,		"ScalableSEI: m_rewriting_max_bitrate"         ) );
			  }
		  }
		}
		//JVT-W046 }
	}

	//JVT-W051 {
	if (m_priority_layer_info_present_flag)
	{
		RNOK	(pcReadIf->getUvlc( m_pr_num_dId_minus1, "ScalableSEI: pr_num_dId_minus1"));
		for (i=0; i<=m_pr_num_dId_minus1; i++)
		{
			RNOK	(pcReadIf->getCode(m_pr_dependency_id[i], 3, "ScalableSEI: pr_dependency_id"));
			RNOK	(pcReadIf->getUvlc(m_pr_num_minus1[i], "ScalableSEI: pr_num_minus"));
			for (j=0; j<=m_pr_num_minus1[i]; j++)
			{
				RNOK	(pcReadIf->getUvlc(m_pr_id[i][j], "ScalableSEI: pr_id"));
        RNOK	(pcReadIf->getCode(m_pr_profile_level_idc[i][j], 24, "ScalableSEI: pr_profile_level_idc" ) );
				RNOK	(pcReadIf->getCode(m_pr_avg_bitrate[i][j], 16, "ScalableSEI: pr_avg_bitrate"));
				RNOK	(pcReadIf->getCode(m_pr_max_bitrate[i][j], 16, "ScalableSEI: pr_max_bitrate"));
			}
		}
	}
	//JVT-W051 }
	//JVT-W053 wxwan
	if(m_priority_id_setting_flag)
	{
		UInt PriorityIdSettingUriIdx = 0;
		do{
			UInt uiTemp;
			RNOK( pcReadIf->getCode( uiTemp,  8,  "ScalableSEI: priority_id_setting_uri" ) );
			priority_id_setting_uri[PriorityIdSettingUriIdx] = (char) uiTemp;
			PriorityIdSettingUriIdx++;
		}while( priority_id_setting_uri[ PriorityIdSettingUriIdx-1 ]  !=  0 );
	}
	//JVT-W053 wxwan
	return Err::m_nOK;
}
//SEI changes update }
//////////////////////////////////////////////////////////////////////////
//
//			SUB-PICTURE SCALABLE LAYER SEI
//
//////////////////////////////////////////////////////////////////////////

SEI::SubPicSei::SubPicSei ()
: SEIMessage		( SUB_PIC_SEI ),
m_uiDependencyId			( 0 )
{
}

SEI::SubPicSei::~SubPicSei ()
{
}

ErrVal
SEI::SubPicSei::create( SubPicSei*& rpcSeiMessage)
{
	rpcSeiMessage = new SubPicSei();
	ROT( NULL == rpcSeiMessage );
	return Err::m_nOK;
}

ErrVal
SEI::SubPicSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
	RNOK	( pcWriteIf->writeUvlc( m_uiDependencyId, "Sub-picture scalable SEI: m_uiDependencyId" ) );
	return Err::m_nOK;
}

ErrVal
SEI::SubPicSei::read( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
	RNOK	( pcReadIf->getUvlc( m_uiDependencyId, "Sub-picture scalable SEI: m_uiLayerd" ) );
	return Err::m_nOK;
}



//////////////////////////////////////////////////////////////////////////
//
//      MOTION     S E I  FOR  ROI
//
//////////////////////////////////////////////////////////////////////////

SEI::MotionSEI::MotionSEI     ()
 : SEIMessage                     ( MOTION_SEI ),
 m_num_slice_groups_in_set_minus1(0),
 m_exact_sample_value_match_flag(true),
 m_pan_scan_rect_flag(false)
{
}

SEI::MotionSEI::~MotionSEI()
{
}

ErrVal
SEI::MotionSEI::create( MotionSEI*& rpcSeiMessage )
{
  rpcSeiMessage = new MotionSEI();
  ROT( NULL == rpcSeiMessage )
  return Err::m_nOK;
}

ErrVal
SEI::MotionSEI::write( HeaderSymbolWriteIf* pcWriteIf )
{

  RNOK  ( pcWriteIf->writeUvlc( m_num_slice_groups_in_set_minus1,               "Motion Constrainted SEI: Num_slice_groups_in_set_minus1"   ) );

  for(UInt i = 0; i <= m_num_slice_groups_in_set_minus1; i++)
  {
    RNOK  ( pcWriteIf->writeUvlc( m_slice_group_id[ i ],               "Motion Constrainted SEI: slice_group_id[ i ]"   ) );
  }


  RNOK  ( pcWriteIf->writeFlag(m_exact_sample_value_match_flag           ,     "Motion Constrainted SEI: exact_sample_value_match_flag"            ) );
  RNOK  ( pcWriteIf->writeFlag(m_pan_scan_rect_flag                      ,     "Motion Constrainted SEI: frm_rate_info_present_flag"           ) );

  return Err::m_nOK;
}

ErrVal
SEI::MotionSEI::read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  RNOK  ( pcReadIf->getUvlc( m_num_slice_groups_in_set_minus1,               "Motion Constrainted SEI: Num_slice_groups_in_set_minus1"   ) );

  for(UInt i = 0; i <= m_num_slice_groups_in_set_minus1; i++)
  {
    RNOK  ( pcReadIf->getUvlc( m_slice_group_id[ i ],               "Motion Constrainted SEI: slice_group_id[ i ]"   ) );
  }

  RNOK  ( pcReadIf->getFlag(m_exact_sample_value_match_flag           ,     "Motion Constrainted SEI: exact_sample_value_match_flag"            ) );
  RNOK  ( pcReadIf->getFlag(m_pan_scan_rect_flag                      ,     "Motion Constrainted SEI: frm_rate_info_present_flag"           ) );

  assert(m_exact_sample_value_match_flag==true);
  assert(m_pan_scan_rect_flag ==false);

  return Err::m_nOK;
}

ErrVal
SEI::MotionSEI::setSliceGroupId(UInt id)
{
  m_slice_group_id[0] = id;
  return Err::m_nOK;
};

//////////////////////////////////////////////////////////////////////////
//
//      PRIORITY LAYER INFORMATION  S E I
//
//////////////////////////////////////////////////////////////////////////

SEI::PriorityLevelSEI::PriorityLevelSEI     ()
 : SEIMessage                     ( PRIORITYLEVEL_SEI ),
 m_uiNumPriorityIds         ( 0 ),
 m_uiPrDependencyId      ( 0 )
{
  ::memset( m_auiAltPriorityId,  0x00, MAX_NUM_RD_LEVELS*sizeof(UInt) );
}


SEI::PriorityLevelSEI::~PriorityLevelSEI()
{
}


ErrVal
SEI::PriorityLevelSEI::create( PriorityLevelSEI*& rpcSeiMessage )
{
  rpcSeiMessage = new PriorityLevelSEI();
  ROT( NULL == rpcSeiMessage )
  return Err::m_nOK;
}


ErrVal
SEI::PriorityLevelSEI::write( HeaderSymbolWriteIf* pcWriteIf )
{
  RNOK  ( pcWriteIf->writeCode( m_uiPrDependencyId, 3,"PriorityLevelSEI: DependencyId"   ) );
  RNOK  ( pcWriteIf->writeCode( m_uiNumPriorityIds, 4,"PriorityLevelSEI: NumPriorityIds"   ) );
  for(UInt ui = 0; ui < m_uiNumPriorityIds; ui++)
  {
	RNOK  ( pcWriteIf->writeCode( m_auiAltPriorityId[ui], 6,"PriorityLevelSEI: AltPriorityId"   ) );
  }

  return Err::m_nOK;
}


ErrVal
SEI::PriorityLevelSEI::read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  RNOK  ( pcReadIf->getCode( m_uiPrDependencyId, 3,"PriorityLevelSEI: DependencyId"   ) );
  RNOK  ( pcReadIf->getCode( m_uiNumPriorityIds, 4,"PriorityLevelSEI: NumLevels"   ) );
  for(UInt ui = 0; ui < m_uiNumPriorityIds; ui++)
  {
	RNOK  ( pcReadIf->getCode( m_auiAltPriorityId[ui], 6,"PriorityLevelSEI: AltPriorityId"   ) );
  }
  return Err::m_nOK;
}

//NonRequired JVT-Q066 (06-04-08){{
SEI::NonRequiredSei::NonRequiredSei	()
: SEIMessage						( NON_REQUIRED_SEI )
, m_uiNumInfoEntriesMinus1			(MSYS_UINT_MAX)
{
	::memset( m_uiEntryDependencyId,			MSYS_UINT_MAX, MAX_NUM_INFO_ENTRIES*sizeof(UInt) );
	::memset( m_uiNumNonRequiredPicsMinus1,		MSYS_UINT_MAX, MAX_NUM_INFO_ENTRIES*sizeof(UInt) );
	::memset( m_uiNonRequiredPicDependencyId,	MSYS_UINT_MAX, MAX_NUM_INFO_ENTRIES*MAX_NUM_NON_REQUIRED_PICS*sizeof(UInt) );
	::memset( m_uiNonRequiredPicQulityLevel,	MSYS_UINT_MAX, MAX_NUM_INFO_ENTRIES*MAX_NUM_NON_REQUIRED_PICS*sizeof(UInt) );
	::memset( m_uiNonRequiredPicFragmentOrder,  MSYS_UINT_MAX, MAX_NUM_INFO_ENTRIES*MAX_NUM_NON_REQUIRED_PICS*sizeof(UInt) );
}
//NonRequired JVT-Q066 (06-04-08)}}

SEI::NonRequiredSei::~NonRequiredSei ()
{
}

ErrVal
SEI::NonRequiredSei::create ( NonRequiredSei*& rpcSeiMessage )
{
	rpcSeiMessage = new NonRequiredSei();
	ROT( NULL == rpcSeiMessage)
		return Err::m_nOK;
}

ErrVal
SEI::NonRequiredSei::destroy()
{
	delete this;
	return Err::m_nOK;
}

ErrVal
SEI::NonRequiredSei::write( HeaderSymbolWriteIf* pcWriteIf )
{
	RNOK	(pcWriteIf->writeUvlc( m_uiNumInfoEntriesMinus1,	"NonRequiredSEI: NumInfoEntriesMinus1"	));
	for( UInt uiLayer = 0; uiLayer <= m_uiNumInfoEntriesMinus1; uiLayer++)
	{
		RNOK(pcWriteIf->writeCode( m_uiEntryDependencyId[uiLayer],	3,	"NonRequiredSEI: EntryDependencyId"		));
		RNOK(pcWriteIf->writeUvlc( m_uiNumNonRequiredPicsMinus1[uiLayer],	"NonRequiredSEI: NumNonRequiredPicsMinus1"	));
		for( UInt NonRequiredLayer = 0; NonRequiredLayer <= m_uiNumNonRequiredPicsMinus1[uiLayer]; NonRequiredLayer++)
		{
			RNOK(pcWriteIf->writeCode( m_uiNonRequiredPicDependencyId[uiLayer][NonRequiredLayer],	3,	"NonRequiredSEI: NonRequiredPicDependencyId"));
			RNOK(pcWriteIf->writeCode( m_uiNonRequiredPicQulityLevel[uiLayer][NonRequiredLayer],	4,	"NonRequiredSEI: NonRequiredPicQulityLevel"	));
		}
	}
	return Err::m_nOK;
}

ErrVal
SEI::NonRequiredSei::read( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
	RNOK	(pcReadIf->getUvlc( m_uiNumInfoEntriesMinus1,	"NonRequiredSEI: NumInfoEntriesMinus1"	));
	for( UInt uiLayer = 0; uiLayer <= m_uiNumInfoEntriesMinus1; uiLayer++)
	{
		RNOK(pcReadIf->getCode( m_uiEntryDependencyId[uiLayer],	3,	"NonRequiredSEI: EntryDependencyId"		));
		RNOK(pcReadIf->getUvlc( m_uiNumNonRequiredPicsMinus1[uiLayer],	"NonRequiredSEI: NumNonRequiredPicsMinus1"	));
		for( UInt NonRequiredLayer = 0; NonRequiredLayer <= m_uiNumNonRequiredPicsMinus1[uiLayer]; NonRequiredLayer++)
		{
			RNOK(pcReadIf->getCode( m_uiNonRequiredPicDependencyId[uiLayer][NonRequiredLayer],	3,	"NonRequiredSEI: NonRequiredPicDependencyId"));
			RNOK(pcReadIf->getCode( m_uiNonRequiredPicQulityLevel[uiLayer][NonRequiredLayer],	4,	"NonRequiredSEI: NonRequiredPicQulityLevel"	));
		}
	}
	return Err::m_nOK;
}
//SEI changes update }
// JVT-S080 LMI {
//////////////////////////////////////////////////////////////////////////
//
//			SCALABLE SEI LAYERS NOT PRESENT
//
//////////////////////////////////////////////////////////////////////////

UInt SEI::ScalableSeiLayersNotPresent::m_uiLeftNumLayers = 0;
UInt SEI::ScalableSeiLayersNotPresent::m_auiLeftLayerId[MAX_SCALABLE_LAYERS];

SEI::ScalableSeiLayersNotPresent::ScalableSeiLayersNotPresent (): SEIMessage		( SCALABLE_SEI_LAYERS_NOT_PRESENT )
{
}

SEI::ScalableSeiLayersNotPresent::~ScalableSeiLayersNotPresent ()
{
}

ErrVal
SEI::ScalableSeiLayersNotPresent::create( ScalableSeiLayersNotPresent*& rpcSeiMessage )
{
	rpcSeiMessage = new ScalableSeiLayersNotPresent();
	ROT( NULL == rpcSeiMessage )
		return Err::m_nOK;
}
  //TMM_FIX
ErrVal
SEI::ScalableSeiLayersNotPresent::destroy()
{
	delete this ;
	return Err::m_nOK;
}
  //TMM_FIX
ErrVal
SEI::ScalableSeiLayersNotPresent::write( HeaderSymbolWriteIf *pcWriteIf )
{
  UInt i;

	RNOK ( pcWriteIf->writeUvlc(m_uiNumLayers,													"ScalableSEILayersNotPresent: num_layers"											) );
	for( i = 0; i < m_uiNumLayers; i++ )
	{
		RNOK ( pcWriteIf->writeCode( m_auiLayerId[i],												8,		"ScalableSEILayersNotPresent: layer_id"															) );
	}
	return Err::m_nOK;
}

ErrVal
SEI::ScalableSeiLayersNotPresent::read ( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
	UInt i;
	RNOK ( pcReadIf->getUvlc( m_uiNumLayers ,																"ScalableSEILayersNotPresent: num_layers"	) );
	for ( i = 0; i < m_uiNumLayers; i++ )
	{
		RNOK ( pcReadIf->getCode( m_auiLayerId[i],																	8,			"ScalableSEILayersNotPresent: layer_id"	) );
	}
	return Err::m_nOK;
}

//////////////////////////////////////////////////////////////////////////
//
//			SCALABLE SEI DEPENDENCY CHANGE
//
//////////////////////////////////////////////////////////////////////////

SEI::ScalableSeiDependencyChange::ScalableSeiDependencyChange (): SEIMessage		( SCALABLE_SEI_DEPENDENCY_CHANGE )
{
}

SEI::ScalableSeiDependencyChange::~ScalableSeiDependencyChange ()
{
}

ErrVal
SEI::ScalableSeiDependencyChange::create( ScalableSeiDependencyChange*& rpcSeiMessage )
{
	rpcSeiMessage = new ScalableSeiDependencyChange();
	ROT( NULL == rpcSeiMessage )
		return Err::m_nOK;
}

ErrVal
SEI::ScalableSeiDependencyChange::write( HeaderSymbolWriteIf *pcWriteIf )
{
  UInt i, j;

	ROF( m_uiNumLayersMinus1+1 );
	RNOK		( pcWriteIf->writeUvlc(m_uiNumLayersMinus1,													"ScalableSeiDependencyChange: num_layers_minus1"											) );
	for( i = 0; i <= m_uiNumLayersMinus1; i++ )
	{
		RNOK	( pcWriteIf->writeCode( m_auiLayerId[i],												8,		"ScalableSeiDependencyChange: layer_id"															) );
		RNOK	( pcWriteIf->writeFlag( m_abLayerDependencyInfoPresentFlag[i],		"ScalableSeiDependencyChange: layer_dependency_info_present_flag"															) );
		if( m_abLayerDependencyInfoPresentFlag[i] )
		{
	       RNOK		( pcWriteIf->writeUvlc(m_auiNumDirectDependentLayers[i],													"ScalableSeiDependencyChange: num_directly_dependent_layers"											) );
	       for ( j = 0; j < m_auiNumDirectDependentLayers[i]; j++)
	            RNOK( pcWriteIf->writeUvlc(m_auiDirectDependentLayerIdDeltaMinus1[i][j],													"ScalableSeiDependencyChange: directly_dependent_layer_id_delta_minus1"											) );
		}
		else
	            RNOK	( pcWriteIf->writeUvlc(m_auiLayerDependencyInfoSrcLayerIdDeltaMinus1[i],													"ScalableSeiDependencyChange: layer_dependency_info_src_layer_id_delta_minus1"											) );
	}
	return Err::m_nOK;
}

ErrVal
SEI::ScalableSeiDependencyChange::read ( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  UInt i, j;

	RNOK		( pcReadIf->getUvlc(m_uiNumLayersMinus1,													"ScalableSeiDependencyChange: num_layers_minus1"											) );
	for( i = 0; i <= m_uiNumLayersMinus1; i++ )
	{
		RNOK	( pcReadIf->getCode( m_auiLayerId[i],												8,		"ScalableSeiDependencyChange: layer_id"															) );
		RNOK	( pcReadIf->getFlag( m_abLayerDependencyInfoPresentFlag[i],		"ScalableSeiDependencyChange: layer_dependency_info_present_flag"															) );
		if( m_abLayerDependencyInfoPresentFlag[i] )
		{
	       RNOK		( pcReadIf->getUvlc(m_auiNumDirectDependentLayers[i],													"ScalableSeiDependencyChange: num_directly_dependent_layers"											) );
	       for ( j = 0; j < m_auiNumDirectDependentLayers[i]; j++)
	            RNOK		( pcReadIf->getUvlc(m_auiDirectDependentLayerIdDeltaMinus1[i][j],													"ScalableSeiDependencyChange: directly_dependent_layer_id_delta_minus1"											) );
		}
		else
	            RNOK		( pcReadIf->getUvlc(m_auiLayerDependencyInfoSrcLayerIdDeltaMinus1[i],													"ScalableSeiDependencyChange: layer_dependency_info_src_layer_id_delta_minus1"											) );
	}
	return Err::m_nOK;
}
// JVT-S080 LMI }
// JVT-T073 {
//////////////////////////////////////////////////////////////////////////
//
//			SCALABLE NESTING SEI
//
//////////////////////////////////////////////////////////////////////////
ErrVal
SEI::ScalableNestingSei::create( ScalableNestingSei* &rpcSeiMessage )
{
    rpcSeiMessage = new ScalableNestingSei();
	ROT( NULL == rpcSeiMessage );
	return Err::m_nOK;
}
ErrVal
SEI::ScalableNestingSei::destroy()
{
    delete this;
	return Err::m_nOK;
}

ErrVal
SEI::ScalableNestingSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
    UInt uiStartBits  = pcWriteIf->getNumberOfWrittenBits();
	UInt uiPayloadSize = 0;
	UInt uiIndex;
	RNOK( pcWriteIf->writeFlag( m_bAllPicturesInAuFlag, "ScalableNestingSei: AllPicturesInAuFlag " ) );
	if( m_bAllPicturesInAuFlag == 0 )
	{
    ROT( m_uiNumPictures == 0 );
		RNOK( pcWriteIf->writeUvlc( m_uiNumPictures-1, "ScalableNestingSei: NumPictures" ) );
		for( uiIndex = 0; uiIndex < m_uiNumPictures; uiIndex++ )
		{
			RNOK( pcWriteIf->writeCode( m_auiDependencyId[uiIndex],3, "ScalableNestingSei: uiDependencyId " ) );
			RNOK( pcWriteIf->writeCode( m_auiQualityLevel[uiIndex],4, "ScalableNestingSei: uiQualityLevel " ) );
		}
  	RNOK( pcWriteIf->writeCode( m_uiTemporalId,3, "ScalableNestingSei: uiTemporalLevel " ) );
	}
	UInt uiBits       = pcWriteIf->getNumberOfWrittenBits()-uiStartBits;
	UInt uiBitsMod8   = uiBits%8;
  UInt uiAlignBits  = ( uiBitsMod8 ? 8-uiBitsMod8 : 0 );
  while( uiAlignBits-- )
  {
    pcWriteIf->writeFlag( 0, "Scalable NestingSEI: zero_alignment_bit" );
  }
	uiBits = pcWriteIf->getNumberOfWrittenBits();
	uiPayloadSize = (uiBits+7)/8;

	return Err::m_nOK;
}

ErrVal
SEI::ScalableNestingSei::read( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  UInt uiMaxDQId = 0;
	RNOK( pcReadIf->getFlag( m_bAllPicturesInAuFlag, "ScalableNestingSei: AllPicturesInAuFlag " ) );
	if( m_bAllPicturesInAuFlag == 0 )
	{
		RNOK( pcReadIf->getUvlc( m_uiNumPictures, "ScalableNestingSei: NumPictures" ) );
		m_uiNumPictures++;
		for( UInt uiIndex = 0; uiIndex < m_uiNumPictures; uiIndex++ )
		{
			RNOK( pcReadIf->getCode( m_auiDependencyId[uiIndex],3, "ScalableNestingSei: uiDependencyId " ) );
			RNOK( pcReadIf->getCode( m_auiQualityLevel[uiIndex],4, "ScalableNestingSei: uiQualityLevel " ) );
      if( 16*m_auiDependencyId[uiIndex] + m_auiQualityLevel[uiIndex] > uiMaxDQId )
      {
        uiMaxDQId = 16*m_auiDependencyId[uiIndex] + m_auiQualityLevel[uiIndex];
      }
		}
    RNOK( pcReadIf->getCode( m_uiTemporalId,3, "ScalableNestingSei: uiTemporalLevel " ) );
	}
	RNOK( pcReadIf->readZeroByteAlign() ); //nesting_zero_bit

	//Read the following SEI message
  do
  {
	  UInt uiType, uiPayloadSize;
	  while(1)
	  {
      RNOK( pcReadIf->getCode( uiType, 8, "SEI: payload type" ) );
		  if( uiType != 0xff )
			  break;
	  }
	  while(1)
	  {
      RNOK( pcReadIf->getCode( uiPayloadSize, 8, "SEI: payload size" ) );
		  if( uiPayloadSize != 0xff )
			  break;
	  }
    if( pcParameterSetMng == 0 && ( uiType == PIC_TIMING || uiType == BUFFERING_PERIOD ) )
    {
      uiType = RESERVED_SEI; // don't read these SEIs in extractor
    }
	  switch( uiType )
	  {
	  case SCENE_INFO_SEI:
		  {
			  SEI::SceneInfoSei* pcSceneInfoSei;
			  RNOK( SEI::SceneInfoSei::create(pcSceneInfoSei) );
			  RNOK( pcSceneInfoSei->read(pcReadIf, pcParameterSetMng) );
			  RNOK( pcSceneInfoSei->destroy() );
			  break;
		  }
    case SEI::TL0_DEP_REP_IDX_SEI:
      {
        SEI::Tl0DepRepIdxSei* pcTl0DepRepIdxSei;
  		  RNOK( SEI::Tl0DepRepIdxSei::create(pcTl0DepRepIdxSei) );
	      RNOK( pcTl0DepRepIdxSei->read(pcReadIf, pcParameterSetMng) );
	      RNOK( pcTl0DepRepIdxSei->destroy() );
        break;
      }
    case SEI::PIC_TIMING:
      {
        SEI::PicTiming* pcPicTimingSei;
        RNOK( SEI::PicTiming::create(pcPicTimingSei,pcParameterSetMng) );
        RNOK( pcPicTimingSei->read(pcReadIf, pcParameterSetMng) );
        delete pcPicTimingSei;
        break;
      }
    case SEI::BUFFERING_PERIOD:
      {
        SEI::BufferingPeriod* pcBufferingPeriodSei;
        RNOK( SEI::BufferingPeriod::create(pcBufferingPeriodSei,pcParameterSetMng, true, uiMaxDQId, m_uiTemporalId ) );
        RNOK( pcBufferingPeriodSei->read(pcReadIf, pcParameterSetMng ) );
        delete pcBufferingPeriodSei;
        m_bHasBufferingPeriod = true;
        break;
      }
	  //more case can be added here
	  default:
      {
        for (UInt ui=0; ui<uiPayloadSize; ui++)
        {
          UInt uiDummy;
          pcReadIf->getCode(uiDummy, 8, "SEI: Byte ignored" );
        }
      }
		  break;
	  }
    RNOK( pcReadIf->readByteAlign() );
  } while( pcReadIf->moreRBSPData() );

	return Err::m_nOK;
}

//Scene Info, simplified
ErrVal
SEI::SceneInfoSei::create( SceneInfoSei* &rpcSeiMessage )
{
    rpcSeiMessage = new SceneInfoSei();
	ROT( NULL == rpcSeiMessage );
	return Err::m_nOK;
}

ErrVal
SEI::SceneInfoSei::destroy()
{
    delete this;
	return Err::m_nOK;
}

ErrVal
SEI::SceneInfoSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
	UInt uiStart = pcWriteIf->getNumberOfWrittenBits();
	UInt uiPayloadSize = 0;
	Bool bSceneInfoPresentFlag = getSceneInfoPresentFlag();
	RNOK( pcWriteIf->writeFlag( bSceneInfoPresentFlag, "SceneInfo: SceneInfoPresentFlag" ) );
	if( bSceneInfoPresentFlag )
	{
		RNOK( pcWriteIf->writeUvlc( getSceneId(), "SceneInfo: SceneId" ) );
		RNOK( pcWriteIf->writeUvlc( getSceneTransitionType(), "SceneInfo: SceneTransitionType" ) );
		if( getSceneTransitionType() > 3 )
		{
			RNOK( pcWriteIf->writeUvlc( getSecondSceneId(), "SceneInfo: SecondSceneId" ) );
		}
	}
	uiPayloadSize = ( pcWriteIf->getNumberOfWrittenBits() - uiStart + 7 )/8;

	return Err::m_nOK;
}

ErrVal
SEI::SceneInfoSei::read( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
	RNOK( pcReadIf->getFlag( m_bSceneInfoPresentFlag, "SceneInfo: SceneInfoPresentFlag" ) );
	if( m_bSceneInfoPresentFlag )
	{
		RNOK( pcReadIf->getUvlc( m_uiSceneId,  "SceneInfo: SceneId" ) );
		RNOK( pcReadIf->getUvlc( m_uiSceneTransitionType,  "SceneInfo: SceneTransitionType " ) );
		if( m_uiSceneTransitionType > 3 )
		{
			RNOK( pcReadIf->getUvlc( m_uiSecondSceneId, "SceneInfo: SecondSceneId" ) );
		}
	}
	return Err::m_nOK;
}
// JVT-T073 }


// JVT-W052 wxwan
//////////////////////////////////////////////////////////////////////////
//
//			Quality layer integrity check SEI
//
//////////////////////////////////////////////////////////////////////////
ErrVal
SEI::IntegrityCheckSEI::create( IntegrityCheckSEI* &rpcSeiMessage )
{
	rpcSeiMessage = new IntegrityCheckSEI();
	ROT( NULL == rpcSeiMessage );
	return Err::m_nOK;
}
ErrVal
SEI::IntegrityCheckSEI::destroy()
{
	delete this;
	return Err::m_nOK;
}
ErrVal
SEI::IntegrityCheckSEI::write( HeaderSymbolWriteIf *pcWriteIf )
{
	RNOK( pcWriteIf->writeUvlc( m_uinuminfoentriesminus1, "IntegrityCheckSEI: num_info_entries_minus1 " ) );
	for( UInt i = 0; i<= m_uinuminfoentriesminus1; i++ )
	{
		RNOK( pcWriteIf->writeCode( m_uientrydependency_id[i],3,  "IntegrityCheckSEI: entry_dependency_id[ i ] " ) );
		RNOK( pcWriteIf->writeCode( m_uiquality_layer_crc [i],16, "IntegrityCheckSEI: quality_layer_crc  [ i ] " ) );
	}
	return Err::m_nOK;
}
ErrVal
SEI::IntegrityCheckSEI::read( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
	RNOK( pcReadIf->getUvlc( m_uinuminfoentriesminus1, "IntegrityCheckSEI: num_info_entries_minus1 ") );
	for( UInt i = 0; i<= m_uinuminfoentriesminus1; i++ )
	{
		RNOK( pcReadIf->getCode( m_uientrydependency_id[i],3,  "IntegrityCheckSEI: entry_dependency_id[ i ] " ) );
		RNOK( pcReadIf->getCode( m_uiquality_layer_crc [i],16, "IntegrityCheckSEI: quality_layer_crc  [ i ] " ) );
	}
	return Err::m_nOK;
}
// JVT-W052 wxwan



// JVT-V068 HRD {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BufferingPeriod
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
ErrVal SEI::BufferingPeriod::create( BufferingPeriod*& rpcBufferingPeriod, BufferingPeriod* pcBufferingPeriod )
{
  rpcBufferingPeriod = new BufferingPeriod( pcBufferingPeriod->m_pcParameterSetMng, pcBufferingPeriod->m_bNested, pcBufferingPeriod->m_uiDQId, pcBufferingPeriod->m_uiTId );
  ROT( NULL == rpcBufferingPeriod );
  rpcBufferingPeriod->m_uiSeqParameterSetId       = pcBufferingPeriod->m_uiSeqParameterSetId;
  rpcBufferingPeriod->m_aacSchedSel[HRD::NAL_HRD] = pcBufferingPeriod->m_aacSchedSel[HRD::NAL_HRD];
  rpcBufferingPeriod->m_aacSchedSel[HRD::VCL_HRD] = pcBufferingPeriod->m_aacSchedSel[HRD::VCL_HRD];
  return Err::m_nOK;
}

SEI::BufferingPeriod::~BufferingPeriod()
{
  if (m_abHrdParametersPresentFlag[HRD::NAL_HRD])
  {
    m_aacSchedSel[HRD::NAL_HRD].uninit();
  }
  if (m_abHrdParametersPresentFlag[HRD::VCL_HRD])
  {
    m_aacSchedSel[HRD::VCL_HRD].uninit();
  }
}

ErrVal SEI::BufferingPeriod::create( BufferingPeriod*& rpcBufferingPeriod, ParameterSetMng*& rpcParameterSetMng, Bool bNested, UInt uiDQId, UInt uiTId )
{
  rpcBufferingPeriod = new BufferingPeriod( rpcParameterSetMng, bNested, uiDQId, uiTId );
  ROT( NULL == rpcBufferingPeriod );
  return Err::m_nOK;
}

ErrVal SEI::BufferingPeriod::setHRD( UInt uiSPSId, Bool bSubSetSPS, const HRD* apcHrd[] )
{
  ROF( m_pcParameterSetMng->isValidSPS(uiSPSId, bSubSetSPS ));
  m_uiSeqParameterSetId = uiSPSId;

  SequenceParameterSet *pcSPS = NULL;
  m_pcParameterSetMng->get( pcSPS, m_uiSeqParameterSetId, bSubSetSPS);

  m_apcHrd[HRD::NAL_HRD] = apcHrd[HRD::NAL_HRD];
  m_apcHrd[HRD::VCL_HRD] = apcHrd[HRD::VCL_HRD];

  m_abHrdParametersPresentFlag[HRD::NAL_HRD]  = m_apcHrd[HRD::NAL_HRD]->getHrdParametersPresentFlag();
  m_abHrdParametersPresentFlag[HRD::VCL_HRD]  = m_apcHrd[HRD::VCL_HRD]->getHrdParametersPresentFlag();

  if (m_apcHrd[HRD::NAL_HRD]->getHrdParametersPresentFlag())
  {
    m_aacSchedSel[HRD::NAL_HRD].init( m_apcHrd[HRD::NAL_HRD]->getCpbCnt() );
  }

  if (m_apcHrd[HRD::VCL_HRD]->getHrdParametersPresentFlag())
  {
    m_aacSchedSel[HRD::VCL_HRD].init( m_apcHrd[HRD::VCL_HRD]->getCpbCnt() );
  }

  return Err::m_nOK;
}

ErrVal SEI::BufferingPeriod::write( HeaderSymbolWriteIf* pcWriteIf )
{
  RNOK( pcWriteIf->writeUvlc( m_uiSeqParameterSetId, "SEI: seq_parameter_set_id" ) );

  if ( m_apcHrd[HRD::NAL_HRD]->getHrdParametersPresentFlag() )
  {
    for( UInt n = 0; n < m_apcHrd[HRD::NAL_HRD]->getCpbCnt(); n++ )
    {
      RNOK( getSchedSel( HRD::NAL_HRD, n ).write( pcWriteIf, *(m_apcHrd[HRD::NAL_HRD]) ) );
    }
  }

  if (m_apcHrd[HRD::VCL_HRD]->getHrdParametersPresentFlag())
  {
    for( UInt n = 0; n < m_apcHrd[HRD::VCL_HRD]->getCpbCnt(); n++ )
    {
      RNOK( getSchedSel( HRD::VCL_HRD, n ).write( pcWriteIf, *(m_apcHrd[HRD::VCL_HRD]) ) );
    }
  }
  return Err::m_nOK;
}



ErrVal SEI::BufferingPeriod::read( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  RNOKS( pcReadIf->getUvlc( m_uiSeqParameterSetId, "SEI: seq_parameter_set_id" ) );

  Bool  bSubsetSPS  = m_bNested;
  ROF( m_pcParameterSetMng->isValidSPS(m_uiSeqParameterSetId, bSubsetSPS ) );

  SequenceParameterSet *pcSPS = NULL;
  m_pcParameterSetMng->get( pcSPS,m_uiSeqParameterSetId, bSubsetSPS);

  VUI* pcVUI = pcSPS->getVUI ();// get a pointer to VUI of the SPS with m_uiSeqParameterSetId

  UInt  uiHRDIdx  = pcVUI->getDefaultIdx();
  if( m_bNested )
  {
    uiHRDIdx = MSYS_UINT_MAX;
    for( UInt uiIdx = 0; uiIdx < pcVUI->getNumEntries(); uiIdx++ )
    {
      const VUI::LayerInfo& rcLayerInfo = pcVUI->getLayerInfo( uiIdx );
      UInt                  uiDQId      = ( rcLayerInfo.getDependencyID() << 4 ) + rcLayerInfo.getQualityId();
      UInt                  uiTId       = rcLayerInfo.getTemporalId();
      if( uiDQId == m_uiDQId && uiTId == m_uiTId )
      {
        uiHRDIdx = uiIdx;
        break;
      }
    }
    ROT( uiHRDIdx == MSYS_UINT_MAX );
  }

  Bool bNalHrdBpPresentFlag = pcVUI ? pcVUI->getNalHrd( uiHRDIdx ).getHrdParametersPresentFlag() : false;
  Bool bVclHrdBpPresentFlag = pcVUI ? pcVUI->getVclHrd( uiHRDIdx ).getHrdParametersPresentFlag() : false;

  m_abHrdParametersPresentFlag[HRD::NAL_HRD]  = bNalHrdBpPresentFlag;
  m_abHrdParametersPresentFlag[HRD::VCL_HRD]  = bVclHrdBpPresentFlag;

  if (bNalHrdBpPresentFlag)
  {
    m_aacSchedSel[HRD::NAL_HRD].init( pcVUI->getNalHrd( uiHRDIdx ).getCpbCnt() + 1 );
  }
  if (bVclHrdBpPresentFlag)
  {
    m_aacSchedSel[HRD::VCL_HRD].init( pcVUI->getVclHrd( uiHRDIdx ).getCpbCnt() + 1 );
  }

  if (bNalHrdBpPresentFlag)
  {
    const HRD& rcNalHRD = pcVUI->getNalHrd( uiHRDIdx );
    for( UInt n = 0; n < rcNalHRD.getCpbCnt(); n++ )
    {
      RNOKS( getSchedSel( HRD::NAL_HRD, n ).read( pcReadIf, rcNalHRD ) );
    }
  }

  if (bVclHrdBpPresentFlag)
  {
    const HRD& rcVlcHRD = pcVUI->getVclHrd( uiHRDIdx );
    for( UInt n = 0; n < rcVlcHRD.getCpbCnt(); n++ )
    {
      RNOKS( getSchedSel( HRD::VCL_HRD, n ).read( pcReadIf, rcVlcHRD ) );
    }
  }
  return Err::m_nOK;
}



ErrVal SEI::BufferingPeriod::SchedSel::write( HeaderSymbolWriteIf* pcWriteIf, const HRD& rcHrd )
{
  const UInt uiLength = rcHrd.getInitialCpbRemovalDelayLength();
  RNOK( pcWriteIf->writeCode( m_uiInitialCpbRemovalDelay,       uiLength, "SEI: initial_cpb_removal_delay ") );
  RNOK( pcWriteIf->writeCode( m_uiInitialCpbRemovalDelayOffset, uiLength, "SEI: initial_cpb_removal_delay_offset ") );
  return Err::m_nOK;
}


ErrVal SEI::BufferingPeriod::SchedSel::read ( HeaderSymbolReadIf* pcReadIf, const HRD& rcHrd )
{
  const UInt uiLength = rcHrd.getInitialCpbRemovalDelayLength();
  RNOKS( pcReadIf->getCode( m_uiInitialCpbRemovalDelay ,       uiLength, "SEI: initial_cpb_removal_delay ") );
  RNOKS( pcReadIf->getCode( m_uiInitialCpbRemovalDelayOffset,  uiLength, "SEI: initial_cpb_removal_delay_offset ") );
  return Err::m_nOK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PicTimiming
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

UInt SEI::PicTiming::getNumClockTs()
{
  switch ( m_ePicStruct )
  {
  case PS_FRAME:
  case PS_TOP:
  case PS_BOT:
    return 1;
  case PS_TOP_BOT:
  case PS_BOT_TOP:
  case PS_FRM_DOUBLING:
    return 2;
  case PS_TOP_BOT_TOP:
  case PS_BOT_TOP_BOT:
  case PS_FRM_TRIPLING:
    return 3;
  default:
    return 0; // reserved
  }
}

ErrVal SEI::PicTiming::create( PicTiming*& rpcPicTiming, ParameterSetMng* pcParameterSetMng )
{
  if( ! pcParameterSetMng )
  {
    rpcPicTiming = new PicTiming( 0, 0 ); // not usable
    ROT( NULL == rpcPicTiming );
    return Err::m_nOK;
  }
  SequenceParameterSet* pcSPS = 0;
  RNOK( pcParameterSetMng->getActiveSPSDQ0( pcSPS ) );
  ROF ( pcSPS );
  VUI* pcVUI = pcSPS->getVUI();
  ROF ( pcVUI );
  rpcPicTiming = new PicTiming( pcVUI, 0 ); // not for nested
  ROT( NULL == rpcPicTiming );
  return Err::m_nOK;
}

ErrVal SEI::PicTiming::create(PicTiming*& rpcPicTiming, const VUI* pcVUI, UInt uiLayerIndex )
{
  ROT( NULL == pcVUI );
  rpcPicTiming = new PicTiming( pcVUI, uiLayerIndex );
  ROT( NULL == rpcPicTiming );
  return Err::m_nOK;
}

ErrVal SEI::PicTiming::create(PicTiming*& rpcPicTiming, ParameterSetMng* pcParameterSetMng, UInt uiSPSId, Bool bSubSetSPS, UInt uiLayerIndex)
{
	ROF( pcParameterSetMng->isValidSPS(uiSPSId, bSubSetSPS));

	SequenceParameterSet *pcSPS = NULL;
	pcParameterSetMng->get( pcSPS, uiSPSId, bSubSetSPS );

	VUI& rcVUI = *(pcSPS->getVUI ()); // get a pointer to VUI of SPS with m_uiSeqParameterSetId

	rpcPicTiming = new PicTiming( &rcVUI, uiLayerIndex );
	ROT( NULL == rpcPicTiming );

	return Err::m_nOK;
}

ErrVal SEI::PicTiming::write( HeaderSymbolWriteIf* pcWriteIf )
{
  const HRD* pcHRD = NULL;
  if( m_pcVUI->getNalHrd(m_uiLayerIndex).getHrdParametersPresentFlag() )
  {
    pcHRD = &m_pcVUI->getNalHrd(m_uiLayerIndex);
  }
  else if( m_pcVUI->getVclHrd(m_uiLayerIndex).getHrdParametersPresentFlag() )
  {
    pcHRD = &m_pcVUI->getVclHrd(m_uiLayerIndex);
  }

  if( NULL != pcHRD )
  {
    RNOK( pcWriteIf->writeCode( m_uiCpbRemovalDelay,  pcHRD->getCpbRemovalDelayLength(), "SEI: cpb_removal_delay"  ) );
    RNOK( pcWriteIf->writeCode( m_uiDpbOutputDelay,   pcHRD->getDpbOutputDelayLength(),  "SEI: dpb_output_delay"   ) );
  }

  if( m_pcVUI->getPicStructPresentFlag(m_uiLayerIndex) )
  {
    RNOK( pcWriteIf->writeCode( m_ePicStruct, 4,      "SEI: pic_struct"         ) );
    AOT( NULL == pcHRD );
    UInt uiNumClockTs = getNumClockTs();
    for( UInt n = 0; n < uiNumClockTs ; n++ )
    {
      RNOK( m_acClockTimestampBuf.get( n ).write( pcWriteIf, *pcHRD ) );
    }
  }

  return Err::m_nOK;
}

Int  SEI::PicTiming::getTimestamp( UInt uiNum, UInt uiLayerIndex )
{
  return m_acClockTimestampBuf.get( uiNum ).get( *m_pcVUI, uiLayerIndex );
}

ErrVal  SEI::PicTiming::setDpbOutputDelay( UInt uiDpbOutputDelay)
{
  m_uiDpbOutputDelay = uiDpbOutputDelay;
  return Err::m_nOK;
}


ErrVal SEI::PicTiming::setTimestamp( UInt uiNum, UInt uiLayerIndex, Int iTimestamp )
{
  m_acClockTimestampBuf.get( uiNum ).set( *m_pcVUI, uiLayerIndex, iTimestamp );
  return Err::m_nOK;
}

ErrVal  SEI::PicTiming::setCpbRemovalDelay(UInt uiCpbRemovalDelay)
{
  m_uiCpbRemovalDelay = uiCpbRemovalDelay;
  return Err::m_nOK;
}


ErrVal SEI::PicTiming::read( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  const HRD* pcHRD = NULL;
  if( m_pcVUI->getNalHrd(m_uiLayerIndex).getHrdParametersPresentFlag() )
  {
    pcHRD = &m_pcVUI->getNalHrd(m_uiLayerIndex);
  }
  else if( m_pcVUI->getVclHrd(m_uiLayerIndex).getHrdParametersPresentFlag() )
  {
    pcHRD = &m_pcVUI->getVclHrd(m_uiLayerIndex);
  }

  if( NULL != pcHRD )
  {
    RNOKS( pcReadIf->getCode( m_uiCpbRemovalDelay,  pcHRD->getCpbRemovalDelayLength(), "SEI: cpb_removal_delay"  ) );
    RNOKS( pcReadIf->getCode( m_uiDpbOutputDelay,   pcHRD->getDpbOutputDelayLength(),  "SEI: dpb_output_delay"   ) );
  }

  if( m_pcVUI->getPicStructPresentFlag(m_uiLayerIndex) )
  {
    UInt uiCode = 0;
    RNOKS( pcReadIf->getCode( uiCode, 4, "SEI: pic_struct" ) );
    m_ePicStruct = (PicStruct)uiCode;
    AOT( NULL == pcHRD );
    UInt uiNumClockTs = getNumClockTs();
    for( UInt n = 0; n < uiNumClockTs ; n++ )
    {
      RNOKS( m_acClockTimestampBuf.get( n ).read( pcReadIf, *pcHRD ) );
    }
  }

  return Err::m_nOK;
}

ErrVal SEI::PicTiming::ClockTimestamp::write( HeaderSymbolWriteIf* pcWriteIf, const HRD& rcHRD )
{
  RNOK( pcWriteIf->writeFlag( m_bClockTimestampFlag, "SEI: clock_timestamp_flag"   ) );

  ROFRS( m_bClockTimestampFlag, Err::m_nOK );

  m_bFullTimestampFlag = (0 != m_uiHours);

  RNOK( pcWriteIf->writeCode( m_uiCtType, 2,         "SEI: ct_type"                ) );
  RNOK( pcWriteIf->writeFlag( m_bNuitFieldBasedFlag, "SEI: nuit_field_based_flag"  ) );
  RNOK( pcWriteIf->writeCode( m_uiCountingType, 5,   "SEI: counting_type"          ) );
  RNOK( pcWriteIf->writeFlag( m_bFullTimestampFlag,  "SEI: full_timestamp_flag"    ) );
  RNOK( pcWriteIf->writeFlag( m_bDiscontinuityFlag,  "SEI: discontinuity_flag"     ) );
  RNOK( pcWriteIf->writeFlag( m_bCntDroppedFlag,     "SEI: cnt_dropped_flag"       ) );
  RNOK( pcWriteIf->writeCode( m_uiNFrames, 8,        "SEI: n_frames"               ) );

  if( m_bFullTimestampFlag )
  {
    RNOK( pcWriteIf->writeCode( m_uiSeconds, 6,      "SEI: seconds_value"          ) );
    RNOK( pcWriteIf->writeCode( m_uiMinutes, 6,      "SEI: minutes_value"          ) );
    RNOK( pcWriteIf->writeCode( m_uiHours,   5,      "SEI: hours_value"            ) );
  }
  else
  {
    Bool bHoursFlag   = ( 0 != m_uiHours );
    Bool bMinutesFlag = ( 0 != m_uiMinutes ) || bHoursFlag;
    Bool bSecondFlag  = ( 0 != m_uiSeconds ) || bMinutesFlag;

    RNOK( pcWriteIf->writeFlag( bSecondFlag,         "SEI: seconds_flag"           ) );
    if( bSecondFlag )
    {
      RNOK( pcWriteIf->writeCode( m_uiSeconds, 6,    "SEI: seconds_value"          ) );
      RNOK( pcWriteIf->writeFlag( bMinutesFlag,      "SEI: minutes_flag"           ) );
      if( bMinutesFlag )
      {
        RNOK( pcWriteIf->writeCode( m_uiMinutes, 6,  "SEI: minutes_value"          ) );
        RNOK( pcWriteIf->writeFlag( bHoursFlag,      "SEI: hours_flag"             ) );
        if( bHoursFlag )
        {
          RNOK( pcWriteIf->writeCode( m_uiHours, 5,  "SEI: hours_value"            ) );
        }
      }
    }
  }

  if( rcHRD.getTimeOffsetLength() > 0 )
  {
    RNOK( pcWriteIf->writeSCode( m_iTimeOffset, rcHRD.getTimeOffsetLength(), "SEI: time_offset" ) );
  }
  return Err::m_nOK;
}

ErrVal SEI::PicTiming::ClockTimestamp::read( HeaderSymbolReadIf* pcReadIf, const HRD& rcHRD )
{
  RNOKS( pcReadIf->getFlag( m_bClockTimestampFlag, "SEI: clock_timestamp_flag"   ) );

  ROFRS( m_bClockTimestampFlag, Err::m_nOK );

  RNOKS( pcReadIf->getCode( m_uiCtType, 2,         "SEI: ct_type"                ) );
  RNOKS( pcReadIf->getFlag( m_bNuitFieldBasedFlag, "SEI: nuit_field_based_flag"  ) );
  RNOKS( pcReadIf->getCode( m_uiCountingType, 5,   "SEI: counting_type"          ) );
  RNOKS( pcReadIf->getFlag( m_bFullTimestampFlag,  "SEI: full_timestamp_flag"    ) );
  RNOKS( pcReadIf->getFlag( m_bDiscontinuityFlag,  "SEI: discontinuity_flag"     ) );
  RNOKS( pcReadIf->getFlag( m_bCntDroppedFlag,     "SEI: cnt_dropped_flag"       ) );
  RNOKS( pcReadIf->getCode( m_uiNFrames, 8,        "SEI: n_frames"               ) );

  if( m_bFullTimestampFlag )
  {
    RNOKS( pcReadIf->getCode( m_uiSeconds, 6,      "SEI: seconds_value"          ) );
    RNOKS( pcReadIf->getCode( m_uiMinutes, 6,      "SEI: minutes_value"          ) );
    RNOKS( pcReadIf->getCode( m_uiHours,   5,      "SEI: hours_value"            ) );
  }
  else
  {
    Bool bSecondFlag;
    RNOKS( pcReadIf->getFlag( bSecondFlag,         "SEI: seconds_flag"           ) );
    if( bSecondFlag )
    {
      RNOKS( pcReadIf->getCode( m_uiSeconds, 6,    "SEI: seconds_value"          ) );
      Bool bMinutesFlag;
      RNOKS( pcReadIf->getFlag( bMinutesFlag,      "SEI: minutes_flag"           ) );
      if( bMinutesFlag )
      {
        RNOKS( pcReadIf->getCode( m_uiMinutes, 6,  "SEI: minutes_value"          ) );
        Bool bHoursFlag;
        RNOKS( pcReadIf->getFlag( bHoursFlag,      "SEI: hours_flag"             ) );
        if( bHoursFlag )
        {
          RNOKS( pcReadIf->getCode( m_uiHours, 5,  "SEI: hours_value"            ) );
        }
      }
    }
  }

  if( rcHRD.getTimeOffsetLength() > 0 )
  {
    RNOKS( pcReadIf->getSCode( m_iTimeOffset, rcHRD.getTimeOffsetLength(), "SEI: time_offset" ) );
  }
  return Err::m_nOK;
}

Int SEI::PicTiming::ClockTimestamp::get( const VUI& rcVUI, UInt uiLayerIndex )
{
  const VUI::TimingInfo& rcTimingInfo = rcVUI.getTimingInfo( uiLayerIndex );
  Int iTimeOffset  = (((m_uiHours * 60 + m_uiMinutes) * 60) + m_uiSeconds ) * rcTimingInfo.getTimeScale();
  Int iFrameOffset = m_uiNFrames * ( rcTimingInfo.getNumUnitsInTick() * (m_bNuitFieldBasedFlag?2:1));

  return iTimeOffset + iFrameOffset + m_iTimeOffset;
}

Void SEI::PicTiming::ClockTimestamp::set( const VUI& rcVUI, UInt uiLayerIndex, Int iTimestamp )
{
  const VUI::TimingInfo& rcTimingInfo = rcVUI.getTimingInfo(uiLayerIndex);
  Int iTime     = iTimestamp / rcTimingInfo.getTimeScale();
  Int iTimeSub  = 0;
  m_uiHours     = iTime / 360;
  iTimeSub      = m_uiHours * 360;
  m_uiMinutes   = (iTime - iTimeSub) / 60;
  iTimeSub     += m_uiMinutes * 60;
  m_uiSeconds   = iTime - iTimeSub;
  iTimeSub     += m_uiSeconds;

  iTime         = iTimestamp - iTimeSub * rcTimingInfo.getTimeScale();

  Int iScale    = ( rcTimingInfo.getNumUnitsInTick() * (m_bNuitFieldBasedFlag?2:1));
  m_uiNFrames   = iTime / iScale;
  m_iTimeOffset = iTime - m_uiNFrames * iScale;
}

ErrVal SEI::AVCCompatibleHRD::create( AVCCompatibleHRD*& rpcAVCCompatibleHRD, VUI* pcVUI )
{
  rpcAVCCompatibleHRD = new AVCCompatibleHRD( pcVUI );
  ROT( NULL == rpcAVCCompatibleHRD );
  return Err::m_nOK;
}

ErrVal SEI::AVCCompatibleHRD::write( HeaderSymbolWriteIf* pcWriteIf )
{
  ROF( m_pcVUI );
  ROF( m_pcVUI->getNumLayers() == m_pcVUI->getNumTemporalLevels() );

  RNOK( pcWriteIf->writeUvlc( m_pcVUI->getNumLayers() - 1,  "SEI: num_of_temporal_layers_in_base_layer_minus1"  ) );

  for ( UInt ui = 0; ui < m_pcVUI->getNumLayers(); ui++ )
  {
    ROT( m_pcVUI->getLayerInfo(ui).getDependencyID() );
    ROT( m_pcVUI->getLayerInfo(ui).getQualityId() );

    RNOK( pcWriteIf->writeCode( m_pcVUI->getLayerInfo(ui).getTemporalId(), 3,  "SEI: temporal_level"  ) );
    RNOK( m_pcVUI->getTimingInfo(ui).write( pcWriteIf) );
    RNOK( m_pcVUI->getNalHrd(ui).write( pcWriteIf ) );
    RNOK( m_pcVUI->getVclHrd(ui).write( pcWriteIf ) );
    if( m_pcVUI->getNalHrd(ui).getHrdParametersPresentFlag() || m_pcVUI->getVclHrd(ui).getHrdParametersPresentFlag() )
    {
      RNOK( pcWriteIf->writeFlag( m_pcVUI->getLowDelayHrdFlag(ui),           "VUI: low_delay_hrd_flag[i]"));
    }
    RNOK( pcWriteIf->writeFlag( m_pcVUI->getPicStructPresentFlag(ui),        "VUI: pic_struct_present_flag[i]"));
  }

  return Err::m_nOK;
}

ErrVal
SEI::AVCCompatibleHRD::read( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  // read, but don't store
  UInt uiNumLayers = 0;
  RNOK( pcReadIf->getUvlc( uiNumLayers,  "SEI: num_of_temporal_layers_in_base_layer_minus1"  ) );
  uiNumLayers++;

  for( UInt ui = 0; ui < uiNumLayers; ui++ )
  {
    UInt            uiTemporalId          = 0;
    VUI::TimingInfo cTimingInfo;
    HRD             cNalHrd, cVclHrd;
    Bool            bLowDelayFlag         = false;
    Bool            bPicStructPresentFlag = false;

    RNOK( pcReadIf->getCode( uiTemporalId, 3,  "SEI: temporal_level"  ) );
    RNOK( cTimingInfo.read( pcReadIf) );
    RNOK( cNalHrd.read( pcReadIf ) );
    RNOK( cVclHrd.read( pcReadIf ) );
    if( cNalHrd.getHrdParametersPresentFlag() || cVclHrd.getHrdParametersPresentFlag() )
    {
      RNOK( pcReadIf->getFlag( bLowDelayFlag,           "VUI: low_delay_hrd_flag[i]"));
    }
    RNOK( pcReadIf->getFlag( bPicStructPresentFlag,        "VUI: pic_struct_present_flag[i]"));
  }

  return Err::m_nOK;
}

// JVT-V068 HRD }
//JVT-W049 {
//////////////////////////////////////////////////////////////////////////
//
//      REDUNDANT PIC       S E I
//
//////////////////////////////////////////////////////////////////////////
SEI::RedundantPicSei::RedundantPicSei	()
: SEIMessage									( REDUNDANT_PIC_SEI )
{
    UInt i=0, j=0, k=0;
	m_num_dId_minus1=0;
	::memset(  m_dependency_id, 0x00, MAX_LAYERS*sizeof(UInt)  );
    ::memset(  m_num_qId_minus1, 0x00, MAX_LAYERS*sizeof(UInt)  );
	::memset(  m_quality_id, 0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(UInt*)  );
	::memset(  m_num_redundant_pics_minus1, 0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(UInt*)  );
	for( i = 0; i < MAX_LAYERS; i++ )
	{
		for( j = 0; j < MAX_QUALITY_LEVELS; j++ )
		{
			for( k = 0; k < MAX_REDUNDANT_PICTURES_NUM; k++ )
			{
				m_redundant_pic_cnt_minus1[ i ][ j ][ k ]     = 0;
				m_pic_match_flag[ i ][ j ][ k ]               = 0;
                m_mb_type_match_flag[ i ][ j ][ k ]           = 0;
				m_motion_match_flag[ i ][ j ][ k ]            = 0;
				m_residual_match_flag[ i ][ j ][ k ]          = 0;
				m_intra_samples_match_flag[ i ][ j ][ k ]     = 0;

			}
		}
	}
}

SEI::RedundantPicSei::~RedundantPicSei()
{
}

ErrVal
SEI::RedundantPicSei::create( RedundantPicSei*& rpcSeiMessage )
{
	rpcSeiMessage = new RedundantPicSei();
	ROT( NULL == rpcSeiMessage )
		return Err::m_nOK;
}

ErrVal
SEI::RedundantPicSei::write(HeaderSymbolWriteIf* pcWriteIf)
{
	UInt i=0, j=0, k=0;
	ROF  ( m_num_dId_minus1+1 );
	RNOK ( pcWriteIf->writeUvlc(m_num_dId_minus1,						                           "RedundantPicSEI: num_layers_minus1"		           ) );
	for( i = 0; i <= m_num_dId_minus1; i++ )
	{
		RNOK	( pcWriteIf->writeCode( m_dependency_id[ i ] ,                              3 ,    "RedundantPicSEI: dependency_id"                    ) );
		RNOK    ( pcWriteIf->writeUvlc( m_num_qId_minus1[ i ],                                     "RedundantPicSEI: m_num_qId_minus1"                 ) );
		for( j = 0; j <= m_num_qId_minus1[ i ]; j++ )
		{
			RNOK	( pcWriteIf->writeCode( m_quality_id[ i ][ j ],                         4 ,    "RedundantPicSEI: m_quality_id"                     ) );
			RNOK    ( pcWriteIf->writeUvlc( m_num_redundant_pics_minus1[ i ][ j ],                 "RedundantPicSEI: m_num_redundant_pics_minus1"      ) );
			for( k = 0; k <= m_num_redundant_pics_minus1[ i ][ j ]; k++ )
			{
				RNOK    ( pcWriteIf->writeUvlc( m_redundant_pic_cnt_minus1[ i ][ j ][ k ],         "RedundantPicSEI: m_redundant_pic_cnt_minus1"       ) );
				RNOK    ( pcWriteIf->writeFlag( m_pic_match_flag[ i ][ j ][ k ],                   "RedundantPicSEI: m_pic_match_flag"                 ) );
				if( !m_pic_match_flag[ i ][ j ][ k ])
				{
					RNOK    ( pcWriteIf->writeFlag( m_mb_type_match_flag[ i ][ j ][ k ],           "RedundantPicSEI: m_mb_type_match_flag"             ) );
					RNOK    ( pcWriteIf->writeFlag( m_motion_match_flag[ i ][ j ][ k ],            "RedundantPicSEI: m_motion_match_flag"              ) );
					RNOK    ( pcWriteIf->writeFlag( m_residual_match_flag[ i ][ j ][ k ],          "RedundantPicSEI: m_residual_match_flag"            ) );
					RNOK    ( pcWriteIf->writeFlag( m_intra_samples_match_flag[ i ][ j ][ k ],     "RedundantPicSEI: m_intra_samples_match_flag"       ) );
				}
			}
		}
	}
 return Err::m_nOK;
}

ErrVal
SEI::RedundantPicSei::read(HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng)
{
	UInt i=0, j=0, k=0;
	RNOK ( pcReadIf->getUvlc(m_num_dId_minus1,						                            ""		            ) );
	for( i = 0; i <= m_num_dId_minus1; i++ )
	{
		RNOK	( pcReadIf->getCode( m_dependency_id[ i ] ,                              3 ,    ""                  ) );
		RNOK    ( pcReadIf->getUvlc( m_num_qId_minus1[ i ],                                     ""                  ) );
		for( j = 0; j <= m_num_qId_minus1[ i ]; j++ )
		{
			RNOK	( pcReadIf->getCode( m_quality_id[ i ][ j ],                         4 ,    ""                  ) );
			RNOK    ( pcReadIf->getUvlc( m_num_redundant_pics_minus1[ i ][ j ],                 ""                  ) );
			for( k = 0; k <= m_num_redundant_pics_minus1[ i ][ j ]; k++ )
			{
				RNOK    ( pcReadIf->getUvlc( m_redundant_pic_cnt_minus1[ i ][ j ][ k ],         ""                  ) );
				RNOK    ( pcReadIf->getFlag( m_pic_match_flag[ i ][ j ][ k ],                   ""                  ) );
				if( !m_pic_match_flag[ i ][ j ][ k ])
				{
					RNOK    ( pcReadIf->getFlag( m_mb_type_match_flag[ i ][ j ][ k ],           ""                  ) );
					RNOK    ( pcReadIf->getFlag( m_motion_match_flag[ i ][ j ][ k ],            ""                  ) );
					RNOK    ( pcReadIf->getFlag( m_residual_match_flag[ i ][ j ][ k ],          ""                  ) );
					RNOK    ( pcReadIf->getFlag( m_intra_samples_match_flag[ i ][ j ][ k ],     ""                  ) );
				}
			}
		}
	}
	return Err::m_nOK;
}
//JVT-W049}
//JVT-X032{
//////////////////////////////////////////////////////////////////////////
//
//      TEMPORAL LEVEL SWITCHING POINT      S E I
//
//////////////////////////////////////////////////////////////////////////
SEI::TLSwitchingPointSei::TLSwitchingPointSei	()
: SEIMessage									( TL_SWITCHING_POINT_SEI )
{
   m_delta_frame_num = 0;
}

SEI::TLSwitchingPointSei::~TLSwitchingPointSei()
{
}

ErrVal
SEI::TLSwitchingPointSei::create( TLSwitchingPointSei*& rpcSeiMessage )
{
	rpcSeiMessage = new TLSwitchingPointSei();
	ROT( NULL == rpcSeiMessage )
		return Err::m_nOK;
}

ErrVal
SEI::TLSwitchingPointSei::write(HeaderSymbolWriteIf* pcWriteIf)
{
	RNOK ( pcWriteIf->writeSvlc(m_delta_frame_num,						                           "TLSwitchingPointSEI: m_delta_frame_num"		           ) );
    return Err::m_nOK;
}

ErrVal
SEI::TLSwitchingPointSei::read(HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng)
{
	RNOK ( pcReadIf->getSvlc(m_delta_frame_num,						                            ""		            ) );
	return Err::m_nOK;
}
//JVT-X032}

//JVT-W062 {
//////////////////////////////////////////////////////////////////////////
//
//			TL0 DEP REP INDEX SEI
//
//////////////////////////////////////////////////////////////////////////

SEI::Tl0DepRepIdxSei::Tl0DepRepIdxSei (): SEIMessage		( TL0_DEP_REP_IDX_SEI )
{
}

SEI::Tl0DepRepIdxSei::~Tl0DepRepIdxSei ()
{
}

ErrVal
SEI::Tl0DepRepIdxSei::create ( Tl0DepRepIdxSei*& rpcSeiMessage)
{
  rpcSeiMessage = new Tl0DepRepIdxSei();
  ROT( NULL == rpcSeiMessage );
  return Err::m_nOK;
}

ErrVal
SEI::Tl0DepRepIdxSei::destroy()
{
    delete this;
	return Err::m_nOK;
}

ErrVal
SEI::Tl0DepRepIdxSei::read( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  RNOK( pcReadIf->getCode( m_uiTl0DepRepIdx, 8, "Tl0DepRepIdxSei: tl0_dep_rep_idx" ) );
  RNOK( pcReadIf->getCode( m_uiEfIdrPicId, 16, "Tl0DepRepIdxSei: effective_idr_pic_id" ) );
  return Err::m_nOK;
}

ErrVal
SEI::Tl0DepRepIdxSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
  RNOK( pcWriteIf->writeCode( m_uiTl0DepRepIdx, 8, "Tl0DepRepIdxSei: tl0_dep_rep_idx" ) );
  RNOK( pcWriteIf->writeCode( m_uiEfIdrPicId, 16, "Tl0DepRepIdxSei: effective_idr_pic_id" ) );
  return Err::m_nOK;
}
//JVT-W062 }


SEI::UnsupportedSei::UnsupportedSei( MessageType eMessageType, UInt uiSize )
: SEIMessage( eMessageType )
, m_uiSize  ( uiSize )
{
}

SEI::UnsupportedSei::~UnsupportedSei()
{
}
 
ErrVal
SEI::UnsupportedSei::create( UnsupportedSei*& rpcSeiMessage, MessageType eMessageType, UInt uiSize )
{
  rpcSeiMessage = new UnsupportedSei( eMessageType, uiSize );
  ROT( NULL == rpcSeiMessage );
  return Err::m_nOK;
}

ErrVal
SEI::UnsupportedSei::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
SEI::UnsupportedSei::read( HeaderSymbolReadIf *pcReadIf, ParameterSetMng* pcParameterSetMng )
{
  for( UInt ui = 0; ui < m_uiSize; ui++ )
  {
    UInt uiSEIByte = 0;
    RNOK( pcReadIf->getCode( uiSEIByte, 8, "SEI: byte of unsupported SEI message" ) );
  }
  return Err::m_nOK;
}

ErrVal
SEI::UnsupportedSei::write( HeaderSymbolWriteIf *pcWriteIf )
{
  ROT(1);
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
