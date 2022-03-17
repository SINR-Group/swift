
#include "H264AVCCommonLib.h"

#include "H264AVCCommonLib/SliceHeaderBase.h"
#include "H264AVCCommonLib/TraceFile.h"

#include "H264AVCCommonLib/CFMO.h"


H264AVC_NAMESPACE_BEGIN



RplrCommand::RplrCommand( ReOrderingOfPicNumsIdc  eReOrderingOfPicNumsIdc,
                          UInt                    uiValue )
: m_eReOrderingOfPicNumsIdc ( eReOrderingOfPicNumsIdc )
, m_uiValue                 ( uiValue )
{
}

RplrCommand::RplrCommand( const RplrCommand& rcRplrCommand )
: m_eReOrderingOfPicNumsIdc ( rcRplrCommand.m_eReOrderingOfPicNumsIdc )
, m_uiValue                 ( rcRplrCommand.m_uiValue )
{
}

RplrCommand::~RplrCommand()
{
}

Void
RplrCommand::copy( const RplrCommand& rcRplrCommand )
{
  m_eReOrderingOfPicNumsIdc = rcRplrCommand.m_eReOrderingOfPicNumsIdc;
  m_uiValue                 = rcRplrCommand.m_uiValue;
}

ErrVal
RplrCommand::write( HeaderSymbolWriteIf&  rcWriteIf,
                    Bool&                 rbEnd ) const
{
  RNOK  ( rcWriteIf.writeUvlc( m_eReOrderingOfPicNumsIdc,  "RPLR: reordering_of_pic_nums_idc" ) );
  rbEnd = ( m_eReOrderingOfPicNumsIdc == RPLR_END );
  ROTRS ( rbEnd, Err::m_nOK );

  switch( m_eReOrderingOfPicNumsIdc )
  {
  case RPLR_NEG:
  case RPLR_POS:
    RNOK( rcWriteIf.writeUvlc( m_uiValue,  "RPLR: abs_diff_pic_num_minus1" ) );
    break;
  case RPLR_LONG:
    RNOK( rcWriteIf.writeUvlc( m_uiValue,  "RPLR: long_term_pic_num" ) );
    break;
  default:
    RERR();
  }
  return Err::m_nOK;
}

ErrVal
RplrCommand::read( HeaderSymbolReadIf&  rcReadIf,
                   Bool&                rbEnd )
{
  UInt  uiReOrderingOfPicNumsIdc;
  RNOK  ( rcReadIf.getUvlc( uiReOrderingOfPicNumsIdc, "RPLR: reordering_of_pic_nums_idc" ) );
  m_eReOrderingOfPicNumsIdc = ReOrderingOfPicNumsIdc( uiReOrderingOfPicNumsIdc );
  m_uiValue                 = 0;
  rbEnd                     = ( m_eReOrderingOfPicNumsIdc == RPLR_END );
  ROTRS ( rbEnd, Err::m_nOK );

  switch( m_eReOrderingOfPicNumsIdc )
  {
  case RPLR_NEG:
  case RPLR_POS:
    RNOK( rcReadIf.getUvlc( m_uiValue, "RPLR: abs_diff_pic_num_minus1" ) );
    break;
  case RPLR_LONG:
    RNOK( rcReadIf.getUvlc( m_uiValue, "RPLR: long_term_pic_num" ) );
    break;
  default:
    RERR();
  }
  return Err::m_nOK;
}



RefPicListReOrdering::RefPicListReOrdering()
: m_bRefPicListReorderingFlag( false )
{
}

RefPicListReOrdering::RefPicListReOrdering( const RefPicListReOrdering& rcRefPicListReOrdering )
: StatBuf<RplrCommand,33>     ( rcRefPicListReOrdering )
, m_bRefPicListReorderingFlag ( false )
{
}

RefPicListReOrdering::~RefPicListReOrdering()
{
}

Void
RefPicListReOrdering::clear( Bool bRefPicListReOrderingFlag )
{
  setAll( RplrCommand() );
  m_bRefPicListReorderingFlag = bRefPicListReOrderingFlag;
}

Void
RefPicListReOrdering::copy( const RefPicListReOrdering& rcRefPicListReOrdering )
{
  StatBuf<RplrCommand,33>::copy( rcRefPicListReOrdering );
  m_bRefPicListReorderingFlag = rcRefPicListReOrdering.m_bRefPicListReorderingFlag;
}

ErrVal
RefPicListReOrdering::write( HeaderSymbolWriteIf& rcWriteIf ) const
{
  RNOK  ( rcWriteIf.writeFlag( m_bRefPicListReorderingFlag,  "RPLR: ref_pic_list_reordering_flag" ) );
  ROFRS ( m_bRefPicListReorderingFlag, Err::m_nOK );

  Bool  bEnd = false;
  for( UInt uiIndex = 0; !bEnd; uiIndex++ )
  {
    RNOK( get( uiIndex ).write( rcWriteIf, bEnd ) );
  }
  return Err::m_nOK;
}

ErrVal
RefPicListReOrdering::read( HeaderSymbolReadIf& rcReadIf, UInt uiNumRefIdx )
{
  RNOK  ( rcReadIf.getFlag( m_bRefPicListReorderingFlag,  "RPLR: ref_pic_list_reordering_flag" ) );
  ROFRS ( m_bRefPicListReorderingFlag, Err::m_nOK );

  Bool  bEnd = false;
  for( UInt uiIndex = 0; !bEnd; uiIndex++ )
  {
    RNOK( get( uiIndex ).read( rcReadIf, bEnd ) );
    ROT ( uiIndex > uiNumRefIdx && !bEnd );
  }
  return Err::m_nOK;
}



MmcoCommand::MmcoCommand( Mmco eMmco,
                          UInt uiValue1,
                          UInt uiValue2 )
: m_eMmco     ( eMmco )
, m_uiValue1  ( uiValue1 )
, m_uiValue2  ( uiValue2 )
{
}

MmcoCommand::MmcoCommand( const MmcoCommand& rcMmcoCommand )
: m_eMmco     ( rcMmcoCommand.m_eMmco )
, m_uiValue1  ( rcMmcoCommand.m_uiValue1 )
, m_uiValue2  ( rcMmcoCommand.m_uiValue2 )
{
}

MmcoCommand::~MmcoCommand()
{
}

Void
MmcoCommand::copy( const MmcoCommand& rcMmcoCommand )
{
  m_eMmco     = rcMmcoCommand.m_eMmco;
  m_uiValue1  = rcMmcoCommand.m_uiValue1;
  m_uiValue2  = rcMmcoCommand.m_uiValue2;
}

ErrVal
MmcoCommand::write( HeaderSymbolWriteIf& rcWriteIf,
                    Bool                 bRefBasePic,
                    Bool&                rbEnd ) const
{
  RNOK  ( rcWriteIf.writeUvlc( m_eMmco,     "DRPM: memory_mangement_control_operation" ) );
  rbEnd = ( m_eMmco == MMCO_END );
  ROTRS ( rbEnd, Err::m_nOK );

  switch( m_eMmco )
  {
  case MMCO_SHORT_TERM_UNUSED:
    RNOK( rcWriteIf.writeUvlc( m_uiValue1,  "DRPM: difference_of_pic_nums_minus1" ) );
    break;
  case MMCO_LONG_TERM_UNUSED:
    RNOK( rcWriteIf.writeUvlc( m_uiValue1,  "DRPM: long_term_pic_num" ) );
    break;
  case MMCO_ASSIGN_LONG_TERM:
    ROT ( bRefBasePic );
    RNOK( rcWriteIf.writeUvlc( m_uiValue1,  "DRPM: difference_of_pic_nums_minus1" ) );
    RNOK( rcWriteIf.writeUvlc( m_uiValue2,  "DRPM: long_term_frame_idx" ) );
    break;
  case MMCO_MAX_LONG_TERM_IDX:
    ROT ( bRefBasePic );
    RNOK( rcWriteIf.writeUvlc( m_uiValue1,  "DRPM: max_long_term_frame_idx_plus1" ) );
    break;
  case MMCO_RESET:
    ROT ( bRefBasePic );
    break;
  case MMCO_SET_LONG_TERM:
    ROT ( bRefBasePic );
    RNOK( rcWriteIf.writeUvlc( m_uiValue1,  "DRPM: long_term_frame_idx" ) );
    break;
  default:
    RERR();
  }
  return Err::m_nOK;
}

ErrVal
MmcoCommand::read( HeaderSymbolReadIf&  rcReadIf,
                   Bool                 bRefBasePic,
                   Bool&                rbEnd )
{
  UInt  uiMmco;
  RNOK  ( rcReadIf.getUvlc( uiMmco,     "DRPM: memory_mangement_control_operation" ) );
  m_eMmco     = Mmco( uiMmco );
  m_uiValue1  = 0;
  m_uiValue2  = 0;
  rbEnd       = ( m_eMmco == MMCO_END );
  ROTRS ( rbEnd, Err::m_nOK );

  switch( m_eMmco )
  {
  case MMCO_SHORT_TERM_UNUSED:
    RNOK( rcReadIf.getUvlc( m_uiValue1, "DRPM: difference_of_pic_nums_minus1" ) );
    break;
  case MMCO_LONG_TERM_UNUSED:
    RNOK( rcReadIf.getUvlc( m_uiValue1, "DRPM: long_term_pic_num" ) );
    break;
  case MMCO_ASSIGN_LONG_TERM:
    ROT ( bRefBasePic );
    RNOK( rcReadIf.getUvlc( m_uiValue1, "DRPM: difference_of_pic_nums_minus1" ) );
    RNOK( rcReadIf.getUvlc( m_uiValue2, "DRPM: long_term_frame_idx" ) );
    break;
  case MMCO_MAX_LONG_TERM_IDX:
    ROT ( bRefBasePic );
    RNOK( rcReadIf.getUvlc( m_uiValue1, "DRPM: max_long_term_frame_idx_plus1" ) );
    break;
  case MMCO_RESET:
    ROT ( bRefBasePic );
    break;
  case MMCO_SET_LONG_TERM:
    ROT ( bRefBasePic );
    RNOK( rcReadIf.getUvlc( m_uiValue1, "DRPM: long_term_frame_idx" ) );
    break;
  default:
    RERR();
  }
  return Err::m_nOK;
}



DecRefPicMarking::DecRefPicMarking( Bool bDecRefBasePicMarking )
: m_bDecRefBasePicMarking         ( bDecRefBasePicMarking )
, m_bAdaptiveRefPicMarkingModeFlag( false )
{
}

DecRefPicMarking::DecRefPicMarking( const DecRefPicMarking& rcDecRefPicMarking )
: StatBuf<MmcoCommand,33>         ( rcDecRefPicMarking )
, m_bDecRefBasePicMarking         ( rcDecRefPicMarking.m_bDecRefBasePicMarking )
, m_bAdaptiveRefPicMarkingModeFlag( rcDecRefPicMarking.m_bAdaptiveRefPicMarkingModeFlag )
{
}

DecRefPicMarking::~DecRefPicMarking()
{
}

Void
DecRefPicMarking::clear( Bool bDecRefBasePicMarking, Bool bAdaptiveRefPicMarkingModeFlag )
{
  setAll( MmcoCommand() );
  m_bDecRefBasePicMarking           = bDecRefBasePicMarking;
  m_bAdaptiveRefPicMarkingModeFlag  = bAdaptiveRefPicMarkingModeFlag;
}

Void
DecRefPicMarking::copy( const DecRefPicMarking& rcDecRefPicMarking )
{
  StatBuf<MmcoCommand,33>::copy( rcDecRefPicMarking );
  m_bDecRefBasePicMarking           = rcDecRefPicMarking.m_bDecRefBasePicMarking;
  m_bAdaptiveRefPicMarkingModeFlag  = rcDecRefPicMarking.m_bAdaptiveRefPicMarkingModeFlag;
}

ErrVal
DecRefPicMarking::write( HeaderSymbolWriteIf& rcWriteIf ) const
{
  if( m_bDecRefBasePicMarking )
  {
    RNOK( rcWriteIf.writeFlag( m_bAdaptiveRefPicMarkingModeFlag,  "DPRM:  adaptive_ref_base_pic_marking_mode_flag" ) );
  }
  else
  {
    RNOK( rcWriteIf.writeFlag( m_bAdaptiveRefPicMarkingModeFlag,  "DPRM:  adaptive_ref_pic_marking_mode_flag" ) );
  }
  ROFRS ( m_bAdaptiveRefPicMarkingModeFlag, Err::m_nOK );

  Bool  bEnd = false;
  for( UInt uiIndex = 0; !bEnd; uiIndex++ )
  {
    RNOK( get( uiIndex ).write( rcWriteIf, m_bDecRefBasePicMarking, bEnd ) );
  }
  return Err::m_nOK;
}

ErrVal
DecRefPicMarking::read( HeaderSymbolReadIf& rcReadIf )
{
  if( m_bDecRefBasePicMarking )
  {
    RNOK( rcReadIf.getFlag( m_bAdaptiveRefPicMarkingModeFlag,  "DPRM:  adaptive_ref_base_pic_marking_mode_flag" ) );
  }
  else
  {
    RNOK( rcReadIf.getFlag( m_bAdaptiveRefPicMarkingModeFlag,  "DPRM:  adaptive_ref_pic_marking_mode_flag" ) );
  }
  ROFRS ( m_bAdaptiveRefPicMarkingModeFlag, Err::m_nOK );

  Bool  bEnd = false;
  for( UInt uiIndex = 0; !bEnd; uiIndex++ )
  {
    RNOK( get( uiIndex ).read( rcReadIf, m_bDecRefBasePicMarking, bEnd ) );
  }
  return Err::m_nOK;
}

Bool
DecRefPicMarking::hasMMCO5() const
{
  ROFRS( m_bAdaptiveRefPicMarkingModeFlag,  false );
  Bool bEnd = false;
  for( Int iIndex = 0; !bEnd; iIndex++ )
  {
    Mmco   eMmcoOp  = get( iIndex ).getCommand();
    ROTRS( eMmcoOp == MMCO_RESET, true );
    bEnd            = ( eMmcoOp == MMCO_END );
  }
  return false;
}



PredWeight::PredWeight()
: m_bLumaWeightFlag   ( false )
, m_iLumaWeight       ( 0 )
, m_iLumaOffset       ( 0 )
, m_bChromaWeightFlag ( false )
, m_iChromaCbWeight   ( 0 )
, m_iChromaCbOffset   ( 0 )
, m_iChromaCrWeight   ( 0 )
, m_iChromaCrOffset   ( 0 )
{
}

PredWeight::PredWeight( const PredWeight& rcPredWeight )
: m_bLumaWeightFlag   ( rcPredWeight.m_bLumaWeightFlag )
, m_iLumaWeight       ( rcPredWeight.m_iLumaWeight )
, m_iLumaOffset       ( rcPredWeight.m_iLumaOffset )
, m_bChromaWeightFlag ( rcPredWeight.m_bChromaWeightFlag )
, m_iChromaCbWeight   ( rcPredWeight.m_iChromaCbWeight )
, m_iChromaCbOffset   ( rcPredWeight.m_iChromaCbOffset )
, m_iChromaCrWeight   ( rcPredWeight.m_iChromaCrWeight )
, m_iChromaCrOffset   ( rcPredWeight.m_iChromaCrOffset )
{
}

PredWeight::~PredWeight()
{
}

Void
PredWeight::copy( const PredWeight& rcPredWeight )
{
  m_bLumaWeightFlag   = rcPredWeight.m_bLumaWeightFlag;
  m_iLumaWeight       = rcPredWeight.m_iLumaWeight;
  m_iLumaOffset       = rcPredWeight.m_iLumaOffset;
  m_bChromaWeightFlag = rcPredWeight.m_bChromaWeightFlag;
  m_iChromaCbWeight   = rcPredWeight.m_iChromaCbWeight;
  m_iChromaCbOffset   = rcPredWeight.m_iChromaCbOffset;
  m_iChromaCrWeight   = rcPredWeight.m_iChromaCrWeight;
  m_iChromaCrOffset   = rcPredWeight.m_iChromaCrOffset;
}

ErrVal
PredWeight::write( HeaderSymbolWriteIf& rcWriteIf ) const
{
  RNOK  ( rcWriteIf.writeFlag( m_bLumaWeightFlag,   "PWT: luma_weight_flag" ) );
  if( m_bLumaWeightFlag )
  {
    RNOK( rcWriteIf.writeSvlc( m_iLumaWeight,       "PWT: luma_weight" ) );
    RNOK( rcWriteIf.writeSvlc( m_iLumaOffset,       "PWT: luma_offset" ) );
  }
  RNOK  ( rcWriteIf.writeFlag( m_bChromaWeightFlag, "PWT: chroma_weight_flag" ) );
  if( m_bChromaWeightFlag )
  {
    RNOK( rcWriteIf.writeSvlc( m_iChromaCbWeight,   "PWT: chroma_weight (Cb)" ) );
    RNOK( rcWriteIf.writeSvlc( m_iChromaCbOffset,   "PWT: chroma_offset (Cb)" ) );
    RNOK( rcWriteIf.writeSvlc( m_iChromaCrWeight,   "PWT: chroma_weight (Cr)" ) );
    RNOK( rcWriteIf.writeSvlc( m_iChromaCrOffset,   "PWT: chroma_offset (Cr)" ) );
  }
  return Err::m_nOK;
}

ErrVal
PredWeight::read( HeaderSymbolReadIf& rcReadIf, UInt uiLumaLog2WeightDenom, UInt uiChromaLog2WeightDenom )
{
  RNOK  ( rcReadIf.getFlag( m_bLumaWeightFlag,   "PWT: luma_weight_flag" ) );
  if( m_bLumaWeightFlag )
  {
    RNOK( rcReadIf.getSvlc( m_iLumaWeight,       "PWT: luma_weight" ) );
    RNOK( rcReadIf.getSvlc( m_iLumaOffset,       "PWT: luma_offset" ) );
    ROT ( m_iLumaWeight < -128 || m_iLumaWeight > 127 );
    ROT ( m_iLumaOffset < -128 || m_iLumaOffset > 127 );
  }
  else
  {
    m_iLumaWeight = ( 1 << uiLumaLog2WeightDenom );
    m_iLumaOffset = 0;
  }
  RNOK  ( rcReadIf.getFlag( m_bChromaWeightFlag, "PWT: chroma_weight_flag" ) );
  if( m_bChromaWeightFlag )
  {
    RNOK( rcReadIf.getSvlc( m_iChromaCbWeight,   "PWT: chroma_weight (Cb)" ) );
    RNOK( rcReadIf.getSvlc( m_iChromaCbOffset,   "PWT: chroma_offset (Cb)" ) );
    RNOK( rcReadIf.getSvlc( m_iChromaCrWeight,   "PWT: chroma_weight (Cr)" ) );
    RNOK( rcReadIf.getSvlc( m_iChromaCrOffset,   "PWT: chroma_offset (Cr)" ) );
    ROT ( m_iChromaCbWeight < -128 || m_iChromaCbWeight > 127 );
    ROT ( m_iChromaCbOffset < -128 || m_iChromaCbOffset > 127 );
    ROT ( m_iChromaCrWeight < -128 || m_iChromaCrWeight > 127 );
    ROT ( m_iChromaCrOffset < -128 || m_iChromaCrOffset > 127 );
  }
  else
  {
    m_iChromaCbWeight = m_iChromaCrWeight = ( 1 << uiChromaLog2WeightDenom );
    m_iChromaCbOffset = m_iChromaCrOffset = 0;
  }
  return Err::m_nOK;
}

Bool
PredWeight::operator == (const h264::PredWeight &rcPredWeight) const
{
  ROFRS( m_bLumaWeightFlag   == rcPredWeight.m_bLumaWeightFlag,   false );
  ROFRS( m_iLumaWeight       == rcPredWeight.m_iLumaWeight,       false );
  ROFRS( m_iLumaOffset       == rcPredWeight.m_iLumaOffset,       false );
  ROFRS( m_bChromaWeightFlag == rcPredWeight.m_bChromaWeightFlag, false );
  ROFRS( m_iChromaCbWeight   == rcPredWeight.m_iChromaCbWeight,   false );
  ROFRS( m_iChromaCbOffset   == rcPredWeight.m_iChromaCbOffset,   false );
  ROFRS( m_iChromaCrWeight   == rcPredWeight.m_iChromaCrWeight,   false );
  ROFRS( m_iChromaCrOffset   == rcPredWeight.m_iChromaCrOffset,   false );
  return true;
}

Bool
PredWeight::operator != (const h264::PredWeight &rcPredWeight) const
{
  return ! ( *this == rcPredWeight );
}

Int
PredWeight::xRandom( Int iMin, Int iMax )
{
  Int iRange  = iMax - iMin + 1;
  Int iValue  = rand() % iRange;
  return iMin + iValue;
}

ErrVal
PredWeight::initRandomly()
{
  m_bLumaWeightFlag   = ( ( rand() & 1 ) == 0 );
  m_bChromaWeightFlag = ( ( rand() & 1 ) == 0 );
  if( m_bLumaWeightFlag )
  {
    m_iLumaWeight     = xRandom( -64, 63 );
    m_iLumaOffset     = xRandom( -64, 63 );
  }
  if( m_bChromaWeightFlag )
  {
    m_iChromaCbWeight = xRandom( -64, 63 );
    m_iChromaCbOffset = xRandom( -64, 63 );
    m_iChromaCrWeight = xRandom( -64, 63 );
    m_iChromaCrOffset = xRandom( -64, 63 );
  }
  return Err::m_nOK;
}

ErrVal
PredWeight::initWeights( Int iLumaWeight, Int iCbWeight, Int iCrWeight )
{
  m_iLumaWeight     = iLumaWeight;
  m_iChromaCbWeight = iCbWeight;
  m_iChromaCrWeight = iCrWeight;
  return Err::m_nOK;
}

ErrVal
PredWeight::initOffsets( Int iLumaOffset, Int iCbOffset, Int iCrOffset )
{
  m_iLumaOffset     = iLumaOffset;
  m_iChromaCbOffset = iCbOffset;
  m_iChromaCrOffset = iCrOffset;
  return Err::m_nOK;
}

ErrVal
PredWeight::setOffsets( const Double* padOffsets )
{
  Int iLuma = (Int)padOffsets[ 0 ];
  Int iCb   = (Int)padOffsets[ 1 ];
  Int iCr   = (Int)padOffsets[ 2 ];

  RNOK( initOffsets( iLuma, iCb, iCr ) );
  setLumaWeightFlag   ( iLuma != 0 );
  setChromaWeightFlag ( iCb != 0 || iCr != 0 );
  return Err::m_nOK;
}

ErrVal
PredWeight::setWeights( const Double* padWeights, Int iLumaScale, Int iChromaScale )
{
  Int iLuma = (Int)padWeights[ 0 ];
  Int iCb   = (Int)padWeights[ 1 ];
  Int iCr   = (Int)padWeights[ 2 ];

  RNOK( initWeights( iLuma, iCb, iCr ) );
  setLumaWeightFlag   ( iLuma != iLumaScale );
  setChromaWeightFlag ( iCb != iChromaScale || iCr != iChromaScale );
  return Err::m_nOK;
}

ErrVal
PredWeight::getOffsets( Double* padOffsets )
{
  padOffsets[ 0 ] = (Double) m_iLumaOffset;
  padOffsets[ 1 ] = (Double) m_iChromaCbOffset;
  padOffsets[ 2 ] = (Double) m_iChromaCrOffset;
  return Err::m_nOK;
}

ErrVal
PredWeight::getWeights( Double* padWeights )
{
  padWeights[ 0 ] = (Double) m_iLumaWeight;
  padWeights[ 1 ] = (Double) m_iChromaCbWeight;
  padWeights[ 2 ] = (Double) m_iChromaCrWeight;
  return Err::m_nOK;
}

Void
PredWeight::scaleL1Weight( Int iDistScaleFactor )
{
  iDistScaleFactor >>= 2;
  if( iDistScaleFactor > 128 || iDistScaleFactor < -64 )
  {
    iDistScaleFactor = 32;
  }
  m_iLumaWeight     = iDistScaleFactor;
  m_iChromaCbWeight = iDistScaleFactor;
  m_iChromaCrWeight = iDistScaleFactor;
}

Void
PredWeight::scaleL0Weight( const PredWeight& rcPredWeightL1 )
{
  m_iLumaWeight     = 64 - rcPredWeightL1.m_iLumaWeight;
  m_iChromaCbWeight = 64 - rcPredWeightL1.m_iChromaCbWeight;
  m_iChromaCrWeight = 64 - rcPredWeightL1.m_iChromaCrWeight;
}


PredWeightTable::PredWeightTable()
{
}

PredWeightTable::PredWeightTable( const PredWeightTable& rcPredWeightTable )
: StatBuf<PredWeight,32>( rcPredWeightTable )
{
}

PredWeightTable::~PredWeightTable()
{
}

ErrVal
PredWeightTable::initRandomly()
{
  for( UInt uiIndex = 0; uiIndex < size(); uiIndex++ )
  {
    RNOK( get( uiIndex ).initRandomly() );
  }
  return Err::m_nOK;
}

ErrVal
PredWeightTable::initDefaults( UInt uiLumaWeightDenom, UInt uiChromaWeightDenom )
{
  Int iLumaWeight   = ( 1 << uiLumaWeightDenom );
  Int iChromaWeight = ( 1 << uiChromaWeightDenom );
  Int iLumaOffset   = 0;
  Int iChromaOffset = 0;
  for( UInt uiIndex = 0; uiIndex < size(); uiIndex++ )
  {
    RNOK( get( uiIndex ).initWeights( iLumaWeight, iChromaWeight, iChromaWeight ) );
    RNOK( get( uiIndex ).initOffsets( iLumaOffset, iChromaOffset, iChromaOffset ) );
  }
  return Err::m_nOK;
}

ErrVal
PredWeightTable::setOffsets( const Double (*padOffsets)[3] )
{
  for( UInt uiIndex = 0; uiIndex < size(); uiIndex++ )
  {
    RNOK( get( uiIndex ).setOffsets( padOffsets[ uiIndex ] ) );
  }
  return Err::m_nOK;
}

ErrVal
PredWeightTable::setWeights( const Double (*padWeights)[3], Int iLumaScale, Int iChromaScale )
{
  for( UInt uiIndex = 0; uiIndex < size(); uiIndex++ )
  {
    RNOK( get( uiIndex ).setWeights( padWeights[ uiIndex ], iLumaScale, iChromaScale ) );
  }
  return Err::m_nOK;
}

Void
PredWeightTable::clear()
{
  setAll( PredWeight() );
}

Void
PredWeightTable::copy( const PredWeightTable& rcPredWeightTable )
{
  StatBuf<PredWeight,32>::copy( rcPredWeightTable );
}

ErrVal
PredWeightTable::write( HeaderSymbolWriteIf& rcWriteIf, UInt uiNumRefIdxActiveMinus1 ) const
{
  for( UInt uiIndex = 0; uiIndex <= uiNumRefIdxActiveMinus1; uiIndex++ )
  {
    RNOK( get( uiIndex ).write( rcWriteIf ) );
  }
  return Err::m_nOK;
}

ErrVal
PredWeightTable::read( HeaderSymbolReadIf& rcReadIf, UInt uiNumRefIdxActiveMinus1, UInt uiLumaLog2WeightDenom, UInt uiChromaLog2WeightDenom )
{
  for( UInt uiIndex = 0; uiIndex <= uiNumRefIdxActiveMinus1; uiIndex++ )
  {
    RNOK( get( uiIndex ).read( rcReadIf, uiLumaLog2WeightDenom, uiChromaLog2WeightDenom ) );
  }
  return Err::m_nOK;
}



DBFilterParameter::DBFilterParameter( Bool bInterLayerParameters )
: m_bInterLayerParameters       ( bInterLayerParameters )
, m_uiDisableDeblockingFilterIdc( false )
, m_iSliceAlphaC0OffsetDiv2     ( 0 )
, m_iSliceBetaOffsetDiv2        ( 0 )
{
}

DBFilterParameter::DBFilterParameter( const DBFilterParameter& rcDBFilterParameter )
: m_bInterLayerParameters       ( rcDBFilterParameter.m_bInterLayerParameters )
, m_uiDisableDeblockingFilterIdc( rcDBFilterParameter.m_uiDisableDeblockingFilterIdc )
, m_iSliceAlphaC0OffsetDiv2     ( rcDBFilterParameter.m_iSliceAlphaC0OffsetDiv2 )
, m_iSliceBetaOffsetDiv2        ( rcDBFilterParameter.m_iSliceBetaOffsetDiv2 )
{
}

DBFilterParameter::~DBFilterParameter()
{
}

Void
DBFilterParameter::clear( Bool bInterLayerParameters )
{
  m_bInterLayerParameters        = bInterLayerParameters;
  m_uiDisableDeblockingFilterIdc = false;
  m_iSliceAlphaC0OffsetDiv2      = 0;
  m_iSliceBetaOffsetDiv2         = 0;
}

Void
DBFilterParameter::copy( const DBFilterParameter& rcDBFilterParameter )
{
  m_bInterLayerParameters        = rcDBFilterParameter.m_bInterLayerParameters;
  m_uiDisableDeblockingFilterIdc = rcDBFilterParameter.m_uiDisableDeblockingFilterIdc;
  m_iSliceAlphaC0OffsetDiv2      = rcDBFilterParameter.m_iSliceAlphaC0OffsetDiv2;
  m_iSliceBetaOffsetDiv2         = rcDBFilterParameter.m_iSliceBetaOffsetDiv2;
}

ErrVal
DBFilterParameter::write( HeaderSymbolWriteIf& rcWriteIf ) const
{
  if( ! m_bInterLayerParameters )
  {
    RNOK  ( rcWriteIf.writeUvlc( m_uiDisableDeblockingFilterIdc,  "SH: disable_deblocking_filter_idc" ) );
    if( m_uiDisableDeblockingFilterIdc != 1 )
    {
      RNOK( rcWriteIf.writeSvlc( m_iSliceAlphaC0OffsetDiv2,       "SH: slice_alpha_c0_offset_div2" ) );
      RNOK( rcWriteIf.writeSvlc( m_iSliceBetaOffsetDiv2,          "SH: slice_beta_offset_div2" ) );
    }
  }
  else
  {
    RNOK  ( rcWriteIf.writeUvlc( m_uiDisableDeblockingFilterIdc,  "SH: disable_inter_layer_deblocking_filter_idc" ) );
    if( m_uiDisableDeblockingFilterIdc != 1 )
    {
      RNOK( rcWriteIf.writeSvlc( m_iSliceAlphaC0OffsetDiv2,       "SH: inter_layer_slice_alpha_c0_offset_div2" ) );
      RNOK( rcWriteIf.writeSvlc( m_iSliceBetaOffsetDiv2,          "SH: inter_layer_slice_beta_offset_div2" ) );
    }
  }
  return Err::m_nOK;
}

ErrVal
DBFilterParameter::read( HeaderSymbolReadIf& rcReadIf, Bool bSVCNalUnit )
{
  if( ! m_bInterLayerParameters )
  {
    RNOK  ( rcReadIf.getUvlc( m_uiDisableDeblockingFilterIdc, "SH: disable_deblocking_filter_idc" ) );
    if( m_uiDisableDeblockingFilterIdc != 1 )
    {
      RNOK( rcReadIf.getSvlc( m_iSliceAlphaC0OffsetDiv2,      "SH: slice_alpha_c0_offset_div2" ) );
      RNOK( rcReadIf.getSvlc( m_iSliceBetaOffsetDiv2,         "SH: slice_beta_offset_div2" ) );
    }
  }
  else
  {
    RNOK  ( rcReadIf.getUvlc( m_uiDisableDeblockingFilterIdc, "SH: disable_inter_layer_deblocking_filter_idc" ) );
    if( m_uiDisableDeblockingFilterIdc != 1 )
    {
      RNOK( rcReadIf.getSvlc( m_iSliceAlphaC0OffsetDiv2,      "SH: inter_layer_slice_alpha_c0_offset_div2" ) );
      RNOK( rcReadIf.getSvlc( m_iSliceBetaOffsetDiv2,         "SH: inter_layer_slice_beta_offset_div2" ) );
    }
  }
  ROT(  bSVCNalUnit && m_uiDisableDeblockingFilterIdc > 6 );
  ROT( !bSVCNalUnit && m_uiDisableDeblockingFilterIdc > 2 );
  ROT( m_iSliceAlphaC0OffsetDiv2  < -6 || m_iSliceAlphaC0OffsetDiv2 > 6 );
  ROT( m_iSliceBetaOffsetDiv2     < -6 || m_iSliceBetaOffsetDiv2    > 6 );
  return Err::m_nOK;
}



NalUnitHeader::NalUnitHeader()
: m_bForbiddenZeroBit     ( false )
, m_eNalRefIdc            ( NAL_REF_IDC_PRIORITY_LOWEST )
, m_eNalUnitType          ( NAL_UNIT_UNSPECIFIED_0 )
, m_bReservedOneBit       ( true )
, m_bIdrFlag              ( false )
, m_uiPriorityId          ( 0 )
, m_bNoInterLayerPredFlag ( true )
, m_uiDependencyId        ( 0 )
, m_uiQualityId           ( 0 )
, m_uiTemporalId          ( 0 )
, m_bUseRefBasePicFlag    ( false )
, m_bDiscardableFlag      ( false )
, m_bOutputFlag           ( true )
, m_uiReservedThree2Bits  ( 3 )
{
}

NalUnitHeader::NalUnitHeader( const NalUnitHeader& rcNalUnitHeader )
: m_bForbiddenZeroBit     ( rcNalUnitHeader.m_bForbiddenZeroBit )
, m_eNalRefIdc            ( rcNalUnitHeader.m_eNalRefIdc )
, m_eNalUnitType          ( rcNalUnitHeader.m_eNalUnitType )
, m_bReservedOneBit       ( rcNalUnitHeader.m_bReservedOneBit )
, m_bIdrFlag              ( rcNalUnitHeader.m_bIdrFlag )
, m_uiPriorityId          ( rcNalUnitHeader.m_uiPriorityId )
, m_bNoInterLayerPredFlag ( rcNalUnitHeader.m_bNoInterLayerPredFlag )
, m_uiDependencyId        ( rcNalUnitHeader.m_uiDependencyId )
, m_uiQualityId           ( rcNalUnitHeader.m_uiQualityId )
, m_uiTemporalId          ( rcNalUnitHeader.m_uiTemporalId )
, m_bUseRefBasePicFlag    ( rcNalUnitHeader.m_bUseRefBasePicFlag )
, m_bDiscardableFlag      ( rcNalUnitHeader.m_bDiscardableFlag )
, m_bOutputFlag           ( rcNalUnitHeader.m_bOutputFlag )
, m_uiReservedThree2Bits  ( rcNalUnitHeader.m_uiReservedThree2Bits )
{
}

NalUnitHeader::~NalUnitHeader()
{
}

Void
NalUnitHeader::copy( const NalUnitHeader& rcNalUnitHeader, Bool bInclusiveSVCExtension )
{
  m_bForbiddenZeroBit     = rcNalUnitHeader.m_bForbiddenZeroBit;
  m_eNalRefIdc            = rcNalUnitHeader.m_eNalRefIdc;
  m_eNalUnitType          = rcNalUnitHeader.m_eNalUnitType;
  ROFVS( bInclusiveSVCExtension );

  m_bReservedOneBit       = rcNalUnitHeader.m_bReservedOneBit;
  m_bIdrFlag              = rcNalUnitHeader.m_bIdrFlag;
  m_uiPriorityId          = rcNalUnitHeader.m_uiPriorityId;
  m_bNoInterLayerPredFlag = rcNalUnitHeader.m_bNoInterLayerPredFlag;
  m_uiDependencyId        = rcNalUnitHeader.m_uiDependencyId;
  m_uiQualityId           = rcNalUnitHeader.m_uiQualityId;
  m_uiTemporalId          = rcNalUnitHeader.m_uiTemporalId;
  m_bUseRefBasePicFlag    = rcNalUnitHeader.m_bUseRefBasePicFlag;
  m_bDiscardableFlag      = rcNalUnitHeader.m_bDiscardableFlag;
  m_bOutputFlag           = rcNalUnitHeader.m_bOutputFlag;
  m_uiReservedThree2Bits  = rcNalUnitHeader.m_uiReservedThree2Bits;
}

ErrVal
NalUnitHeader::write( HeaderSymbolWriteIf& rcWriteIf )  const
{
  RNOK(   rcWriteIf.writeFlag( m_bForbiddenZeroBit,         "NAL unit header: forbidden_zero_bit" ) );
  RNOK(   rcWriteIf.writeCode( m_eNalRefIdc,            2,  "NAL unit header: nal_ref_idc" ) );
  RNOK(   rcWriteIf.writeCode( m_eNalUnitType,          5,  "NAL unit header: nal_unit_type" ) );

  if( m_eNalUnitType == NAL_UNIT_PREFIX || m_eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
  {
    RNOK( rcWriteIf.writeFlag( m_bReservedOneBit,           "NAL unit header: reserved_one_bit" ) );
    RNOK( rcWriteIf.writeFlag( m_bIdrFlag,                  "NAL unit header: idr_flag" ) );
    RNOK( rcWriteIf.writeCode( m_uiPriorityId,          6,  "NAL unit header: priority_id" ) );

    RNOK( rcWriteIf.writeFlag( m_bNoInterLayerPredFlag,     "NAL unit header: no_inter_layer_pred_flag" ) );
    RNOK( rcWriteIf.writeCode( m_uiDependencyId,        3,  "NAL unit header: dependency_id" ) );
    RNOK( rcWriteIf.writeCode( m_uiQualityId,           4,  "NAL unit header: quality_id" ) );

    RNOK( rcWriteIf.writeCode( m_uiTemporalId,          3,  "NAL unit header: temporal_id" ) );
    RNOK( rcWriteIf.writeFlag( m_bUseRefBasePicFlag,        "NAL unit header: use_ref_base_pic_flag" ) );
    RNOK( rcWriteIf.writeFlag( m_bDiscardableFlag,          "NAL unit header: discardable_flag" ) );
    RNOK( rcWriteIf.writeFlag( m_bOutputFlag,               "NAL unit header: output_flag" ) );
    RNOK( rcWriteIf.writeCode( m_uiReservedThree2Bits,  2,  "NAL unit header: reserved_three_2bits" ) );
  }

  return Err::m_nOK;
}

ErrVal
NalUnitHeader::read( HeaderSymbolReadIf& rcReadIf )
{
  UInt    uiNalRefIdc   = 0;
  UInt    uiNalUnitType = 0;
  RNOK(   rcReadIf.getFlag( m_bForbiddenZeroBit,         "NAL unit header: forbidden_zero_bit" ) );
  RNOK(   rcReadIf.getCode( uiNalRefIdc,             2,  "NAL unit header: nal_ref_idc" ) );
  RNOK(   rcReadIf.getCode( uiNalUnitType,           5,  "NAL unit header: nal_unit_type" ) );
  m_eNalRefIdc          = NalRefIdc   ( uiNalRefIdc   );
  m_eNalUnitType        = NalUnitType ( uiNalUnitType );

  if( m_eNalUnitType == NAL_UNIT_PREFIX || m_eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
  {
    RNOK( rcReadIf.getFlag( m_bReservedOneBit,           "NAL unit header: reserved_one_bit" ) );
    RNOK( rcReadIf.getFlag( m_bIdrFlag,                  "NAL unit header: idr_flag" ) );
    RNOK( rcReadIf.getCode( m_uiPriorityId,          6,  "NAL unit header: priority_id" ) );

    RNOK( rcReadIf.getFlag( m_bNoInterLayerPredFlag,     "NAL unit header: no_inter_layer_pred_flag" ) );
    RNOK( rcReadIf.getCode( m_uiDependencyId,        3,  "NAL unit header: dependency_id" ) );
    RNOK( rcReadIf.getCode( m_uiQualityId,           4,  "NAL unit header: quality_id" ) );

    RNOK( rcReadIf.getCode( m_uiTemporalId,          3,  "NAL unit header: temporal_id" ) );
    RNOK( rcReadIf.getFlag( m_bUseRefBasePicFlag,        "NAL unit header: use_ref_base_pic_flag" ) );
    RNOK( rcReadIf.getFlag( m_bDiscardableFlag,          "NAL unit header: discardable_flag" ) );
    RNOK( rcReadIf.getFlag( m_bOutputFlag,               "NAL unit header: output_flag" ) );
    RNOK( rcReadIf.getCode( m_uiReservedThree2Bits,  2,  "NAL unit header: reserved_three_2bits" ) );
  }
  else // set NAL unit header SVC extension to default values
  {
    m_bIdrFlag              = ( m_eNalUnitType == NAL_UNIT_CODED_SLICE_IDR );
    m_uiPriorityId          = 0;
    m_bNoInterLayerPredFlag = true;
    m_uiDependencyId        = 0;
    m_uiQualityId           = 0;
    m_uiTemporalId          = 0;
    m_bUseRefBasePicFlag    = false;
    m_bDiscardableFlag      = false;
    m_bOutputFlag           = ( m_eNalUnitType == NAL_UNIT_CODED_SLICE_IDR ||
                                m_eNalUnitType == NAL_UNIT_CODED_SLICE ||
                                m_eNalUnitType == NAL_UNIT_CODED_SLICE_DATAPART_A ||
                                m_eNalUnitType == NAL_UNIT_CODED_SLICE_DATAPART_B ||
                                m_eNalUnitType == NAL_UNIT_CODED_SLICE_DATAPART_C );
  }

  //----- check forbidden and reserved bits -----
  if( m_bForbiddenZeroBit || !m_bReservedOneBit || m_uiReservedThree2Bits != 3 )
  {
    m_bForbiddenZeroBit     = false;
    m_bReservedOneBit       = true;
    m_uiReservedThree2Bits  = 3;
    ROT( true );
  }

  return Err::m_nOK;
}



AUDelimiter::AUDelimiter()
: m_uiPrimaryPicType  ( 0 )
{
}

AUDelimiter::AUDelimiter( const NalUnitHeader& rcNalUnitHeader )
: NalUnitHeader       ( rcNalUnitHeader )
, m_uiPrimaryPicType  ( 0 )
{
}

AUDelimiter::AUDelimiter( const AUDelimiter& rcAUDelimiter )
: NalUnitHeader       ( rcAUDelimiter )
, m_uiPrimaryPicType  ( rcAUDelimiter.m_uiPrimaryPicType )
{
}

AUDelimiter::~AUDelimiter()
{
}

Void
AUDelimiter::copy( const AUDelimiter& rcAUDelimiter, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    NalUnitHeader::copy( rcAUDelimiter );
  }
  m_uiPrimaryPicType  = rcAUDelimiter.m_uiPrimaryPicType;
}

ErrVal
AUDelimiter::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNALUnitHeader )  const
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::write( rcWriteIf ) );
  }
  RNOK( rcWriteIf.writeCode( m_uiPrimaryPicType, 3, "primary_pic_type" ) );
  return Err::m_nOK;
}

ErrVal
AUDelimiter::read( HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::read( rcReadIf ) );
  }
  RNOK( rcReadIf.getCode( m_uiPrimaryPicType, 3, "primary_pic_type" ) );
  return Err::m_nOK;
}



EndOfSequence::EndOfSequence()
{
}

EndOfSequence::EndOfSequence( const NalUnitHeader& rcNalUnitHeader )
: NalUnitHeader( rcNalUnitHeader )
{
}

EndOfSequence::EndOfSequence( const EndOfSequence& rcEndOfSequence )
: NalUnitHeader( rcEndOfSequence )
{
}

EndOfSequence::~EndOfSequence()
{
}

Void
EndOfSequence::copy( const EndOfSequence& rcEndOfSequence, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    NalUnitHeader::copy( rcEndOfSequence );
  }
}

ErrVal
EndOfSequence::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNALUnitHeader )  const
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::write( rcWriteIf ) );
  }
  return Err::m_nOK;
}

ErrVal
EndOfSequence::read( HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::read( rcReadIf ) );
  }
  return Err::m_nOK;
}



EndOfStream::EndOfStream()
{
}

EndOfStream::EndOfStream( const NalUnitHeader& rcNalUnitHeader )
: NalUnitHeader( rcNalUnitHeader )
{
}

EndOfStream::EndOfStream( const EndOfStream& rcEndOfStream )
: NalUnitHeader( rcEndOfStream )
{
}

EndOfStream::~EndOfStream()
{
}

Void
EndOfStream::copy( const EndOfStream& rcEndOfStream, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    NalUnitHeader::copy( rcEndOfStream );
  }
}

ErrVal
EndOfStream::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNALUnitHeader )  const
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::write( rcWriteIf ) );
  }
  return Err::m_nOK;
}

ErrVal
EndOfStream::read( HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::read( rcReadIf ) );
  }
  return Err::m_nOK;
}



FillerData::FillerData()
: m_uiNumFFBytes( 0 )
{
}

FillerData::FillerData( const NalUnitHeader& rcNalUnitHeader )
: NalUnitHeader ( rcNalUnitHeader )
, m_uiNumFFBytes( 0 )
{
}

FillerData::FillerData( const FillerData& rcFillerData )
: NalUnitHeader ( rcFillerData )
, m_uiNumFFBytes( rcFillerData.m_uiNumFFBytes )
{
}

FillerData::~FillerData()
{
}

Void
FillerData::copy( const FillerData& rcFillerData, Bool bInclusiveNalUnitHeader )
{
  if( bInclusiveNalUnitHeader )
  {
    NalUnitHeader::copy( rcFillerData );
  }
  m_uiNumFFBytes = rcFillerData.m_uiNumFFBytes;
}

ErrVal
FillerData::writePrefix( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNalUnitHeader ) const
{
  PrefixHeader cPrefixHeader( *this );
  cPrefixHeader.setNalUnitType( NAL_UNIT_PREFIX );
  RNOK( cPrefixHeader.write( rcWriteIf, bInclusiveNalUnitHeader ) );
  return Err::m_nOK;
}

ErrVal
FillerData::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNalUnitHeader ) const
{
  if( bInclusiveNalUnitHeader )
  {
    RNOK( NalUnitHeader::write( rcWriteIf ) );
  }
  for( UInt uiIndex = 0; uiIndex < m_uiNumFFBytes; uiIndex++ )
  {
    RNOK( rcWriteIf.writeCode( 0xFF, 8, "ff_byte" ) );
  }
  return Err::m_nOK;
}

ErrVal
FillerData::read( HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNalUnitHeader )
{
  if( bInclusiveNalUnitHeader )
  {
    RNOK( NalUnitHeader::read( rcReadIf ) );
  }
  for( m_uiNumFFBytes = 0; rcReadIf.moreRBSPData(); m_uiNumFFBytes++ )
  {
    UInt  uiFFByte;
    RNOK( rcReadIf.getCode( uiFFByte, 8, "ff_byte" ) );
    ROF ( uiFFByte == 0xFF );
  }
  return Err::m_nOK;
}



PrefixHeader::PrefixHeader()
: m_bStoreRefBasePicFlag                  ( false )
, m_cDecRefBasePicMarking                 ( true  )
, m_bPrefixNalUnitAdditionalExtensionFlag ( false )
{
}

PrefixHeader::PrefixHeader( const NalUnitHeader& rcNalUnitHeader )
: NalUnitHeader                           ( rcNalUnitHeader )
, m_bStoreRefBasePicFlag                  ( false )
, m_cDecRefBasePicMarking                 ( true  )
, m_bPrefixNalUnitAdditionalExtensionFlag ( false )
{
}

PrefixHeader::PrefixHeader( const PrefixHeader &rcPrefixHeader )
: NalUnitHeader                           ( rcPrefixHeader )
, m_bStoreRefBasePicFlag                  ( rcPrefixHeader.m_bStoreRefBasePicFlag )
, m_cDecRefBasePicMarking                 ( rcPrefixHeader.m_cDecRefBasePicMarking )
, m_bPrefixNalUnitAdditionalExtensionFlag ( rcPrefixHeader.m_bPrefixNalUnitAdditionalExtensionFlag )
{
}

PrefixHeader::~PrefixHeader()
{
}

Void
PrefixHeader::copy( const h264::PrefixHeader &rcPrefixHeader, Bool bInclusiveNALUnitHeader )
{
  if( bInclusiveNALUnitHeader )
  {
    NalUnitHeader::copy( rcPrefixHeader );
  }
  m_bStoreRefBasePicFlag                  = rcPrefixHeader.m_bStoreRefBasePicFlag;
  m_cDecRefBasePicMarking            .copy( rcPrefixHeader.m_cDecRefBasePicMarking );
  m_bPrefixNalUnitAdditionalExtensionFlag = rcPrefixHeader.m_bPrefixNalUnitAdditionalExtensionFlag;
}

ErrVal
PrefixHeader::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNALUnitHeader ) const
{
  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::write( rcWriteIf ) );
  }
  if( getNalRefIdc() != NAL_REF_IDC_PRIORITY_LOWEST )
  {
    RNOK( rcWriteIf.writeFlag( m_bStoreRefBasePicFlag,                  "store_ref_base_pic_flag" ) );
    if( ( getUseRefBasePicFlag() || m_bStoreRefBasePicFlag ) && ! getIdrFlag() )
    {
      RNOK( m_cDecRefBasePicMarking.write( rcWriteIf ) );
    }
    RNOK( rcWriteIf.writeFlag( m_bPrefixNalUnitAdditionalExtensionFlag, "additional_prefix_nal_unit_extension_flag" ) );
    ROT ( m_bPrefixNalUnitAdditionalExtensionFlag ); // not supported in encoder
  }
  return Err::m_nOK;
}

ErrVal
PrefixHeader::read( HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNALUnitHeader )
{
  Bool bDummy;

  if( bInclusiveNALUnitHeader )
  {
    RNOK( NalUnitHeader::read( rcReadIf ) );
  }
  if( getNalRefIdc() != NAL_REF_IDC_PRIORITY_LOWEST )
  {
    RNOK( rcReadIf.getFlag( m_bStoreRefBasePicFlag,                   "store_ref_base_pic_flag" ) );
    if( ( getUseRefBasePicFlag() || m_bStoreRefBasePicFlag ) && ! getIdrFlag() )
    {
      RNOK( m_cDecRefBasePicMarking.read( rcReadIf ) );
    }
    RNOK( rcReadIf.getFlag( m_bPrefixNalUnitAdditionalExtensionFlag,  "additional_prefix_nal_unit_extension_flag" ) );
    if( m_bPrefixNalUnitAdditionalExtensionFlag ) // read, but ignore following data
    {
      while( rcReadIf.moreRBSPData() )
      {
        RNOK( rcReadIf.getFlag( bDummy, "additional_prefix_nal_unit_extension_data_flag" ) );
      }
    }
  }
  else // read, but ignore following data
  {
    while( rcReadIf.moreRBSPData() )
    {
      RNOK( rcReadIf.getFlag( bDummy, "additional_prefix_nal_unit_extension_data_flag" ) );
    }
  }
  return Err::m_nOK;
}



SliceHeaderSyntax::SliceHeaderSyntax()
{
  xInit();
}

SliceHeaderSyntax::SliceHeaderSyntax( const NalUnitHeader& rcNalUnitHeader )
: PrefixHeader( rcNalUnitHeader )
{
  xInit();
}

SliceHeaderSyntax::SliceHeaderSyntax( const PrefixHeader& rcPrefixHeader )
: PrefixHeader( rcPrefixHeader )
{
  xInit();
}

SliceHeaderSyntax::SliceHeaderSyntax( const SliceHeaderSyntax& rcSliceHeaderSyntax )
: PrefixHeader( rcSliceHeaderSyntax )
{
  xCopy( rcSliceHeaderSyntax );
}

SliceHeaderSyntax::SliceHeaderSyntax( const SequenceParameterSet& rcSPS,
                                      const PictureParameterSet&  rcPPS )
{
  xInit();
  ANOK( xInitParameterSets( rcSPS, rcPPS ) );
}

SliceHeaderSyntax::~SliceHeaderSyntax()
{
}

ErrVal
SliceHeaderSyntax::init( const SequenceParameterSet& rcSPS, const PictureParameterSet& rcPPS )
{
  xInit();
  RNOK( xInitParameterSets( rcSPS, rcPPS ) );
  return Err::m_nOK;
}

Void
SliceHeaderSyntax::copy( const SliceHeaderSyntax& rcSliceHeaderSyntax, Bool bInclusiveNalUnitHeader )
{
  PrefixHeader::copy( rcSliceHeaderSyntax, bInclusiveNalUnitHeader );
  xCopy( rcSliceHeaderSyntax );
}

ErrVal
SliceHeaderSyntax::writePrefix( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNalUnitHeader ) const
{
  NalUnitType eNalUnitType = getNalUnitType();
  const_cast<SliceHeaderSyntax&>(*this).setNalUnitType( NAL_UNIT_PREFIX );
  RNOK( PrefixHeader::write( rcWriteIf, bInclusiveNalUnitHeader ) );
  const_cast<SliceHeaderSyntax&>(*this).setNalUnitType( eNalUnitType );
  return Err::m_nOK;
}


ErrVal
SliceHeaderSyntax::write( HeaderSymbolWriteIf& rcWriteIf, Bool bInclusiveNalUnitHeader ) const
{
  ROF( parameterSetsInitialized() );

  ETRACE_DECLARE( Bool m_bTraceEnable = true );
  ETRACE_LAYER  ( Int( 16 * getDependencyId() + getQualityId() ) );
  ETRACE_NEWSLICE;

  if( bInclusiveNalUnitHeader )
  {
    NalUnitHeader::write( rcWriteIf );
  }

  RNOK(         rcWriteIf.writeUvlc ( m_uiFirstMbInSlice,                             "SH: first_mb_in_slice" ) );
  RNOK(         rcWriteIf.writeUvlc ( m_eSliceType,                                   "SH: slice_type" ) );
  RNOK(         rcWriteIf.writeUvlc ( m_uiPicParameterSetId,                          "SH: pic_parameter_set_id" ) );
  RNOK(         rcWriteIf.writeCode ( m_uiFrameNum,
                                      getSPS().getLog2MaxFrameNum(),                  "SH: frame_num" ) );
  if( ! getSPS().getFrameMbsOnlyFlag() )
  {
    RNOK(       rcWriteIf.writeFlag ( m_bFieldPicFlag,                                "SH: field_pic_flag" ) );
    if( m_bFieldPicFlag )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bBottomFieldFlag,                             "SH: bottom_field_flag" ) );
    }
  }
  if( getIdrFlag() )
  {
    RNOK(       rcWriteIf.writeUvlc ( m_uiIdrPicId,                                   "SH: idr_pic_id" ) );
  }
  if( getSPS().getPicOrderCntType() == 0 )
  {
    RNOK(       rcWriteIf.writeCode ( m_uiPicOrderCntLsb,
                                      getSPS().getLog2MaxPicOrderCntLsb(),            "SH: pic_order_cnt_lsb" ) );
    if( getPPS().getPicOrderPresentFlag() && ! m_bFieldPicFlag )
    {
      RNOK(     rcWriteIf.writeSvlc ( m_iDeltaPicOrderCntBottom,                      "SH: delta_pic_order_cnt_bottom" ) );
    }
  }
  if( getSPS().getPicOrderCntType() == 1 && !getSPS().getDeltaPicOrderAlwaysZeroFlag() )
  {
    RNOK(       rcWriteIf.writeSvlc ( m_iDeltaPicOrderCnt0,                           "SH: delta_pic_order_cnt[0]" ) );
    if( getPPS().getPicOrderPresentFlag() && ! m_bFieldPicFlag )
    {
      RNOK(     rcWriteIf.writeSvlc ( m_iDeltaPicOrderCnt1,                           "SH: delta_pic_order_cnt[1]" ) );
    }
  }
  if( getPPS().getRedundantPicCntPresentFlag() )
  {
    RNOK(       rcWriteIf.writeUvlc ( m_uiRedundantPicCnt,                            "SH: redundant_pic_cnt" ) );
  }
  if( ! getQualityId() )
  {
    if( isBSlice() )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bDirectSpatialMvPredFlag,                     "SH: direct_spatial_mv_pred_flag" ) );
    }
    if( isInterSlice() )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bNumRefIdxActiveOverrideFlag,                 "SH: num_ref_idx_active_override_flag" ) );
      if( m_bNumRefIdxActiveOverrideFlag )
      {
        RNOK(   rcWriteIf.writeUvlc ( m_uiNumRefIdxL0ActiveMinus1,                    "SH: num_ref_idx_l0_active_minus1" ) );
        if( isBSlice() )
        {
          RNOK( rcWriteIf.writeUvlc ( m_uiNumRefIdxL1ActiveMinus1,                    "SH: num_ref_idx_l1_active_minus1" ) );
        }
      }
    }
    if( isInterSlice() )
    {
      RNOK(     m_cRefPicListReorderingL0.write( rcWriteIf ) );
    }
    if( isBSlice() )
    {
      RNOK(     m_cRefPicListReorderingL1.write( rcWriteIf ) );
    }
    if( ( getPPS().getWeightedPredFlag() && isPorSPSlice() ) || ( getPPS().getWeightedBiPredIdc() == 1 && isBSlice() ) )
    {
      if( ! getNoInterLayerPredFlag() )
      {
        RNOK(   rcWriteIf.writeFlag ( m_bBasePredWeightTableFlag,                     "SH: base_pred_weight_table_flag" ) );
      }
      if( getNoInterLayerPredFlag() || ! m_bBasePredWeightTableFlag )
      {
        RNOK(   rcWriteIf.writeUvlc ( m_uiLumaLog2WeightDenom,                        "PWT: luma_log_weight_denom" ) );
        RNOK(   rcWriteIf.writeUvlc ( m_uiChromaLog2WeightDenom,                      "PWT: chroma_log_weight_denom" ) );
        RNOK(   m_cPredWeightTableL0.write( rcWriteIf, m_uiNumRefIdxL0ActiveMinus1 ) );
        if( isBSlice() )
        {
          RNOK( m_cPredWeightTableL1.write( rcWriteIf, m_uiNumRefIdxL1ActiveMinus1 ) );
        }
      }
    }
    if( getNalRefIdc() )
    {
      if( getIdrFlag() )
      {
        RNOK(   rcWriteIf.writeFlag ( m_bNoOutputOfPriorPicsFlag,                     "DRPM: no_output_of_prior_pics_flag" ) );
        RNOK(   rcWriteIf.writeFlag ( m_bLongTermReferenceFlag,                       "DRPM: long_term_reference_flag" ) );
      }
      else
      {
        RNOK(   m_cDecRefPicMarking.write( rcWriteIf ) );
      }
      if( ! getSPS().getAVCHeaderRewriteFlag() && ! isH264AVCCompatible() )
      {
        RNOK(   rcWriteIf.writeFlag ( getStoreRefBasePicFlag(),                       "SH: store_ref_base_pic_flag" ) );
        if( ( getUseRefBasePicFlag() || getStoreRefBasePicFlag() ) && !getIdrFlag() )
        {
          RNOK( getDecRefBasePicMarking().write( rcWriteIf ) );
        }
      }
    }
  }
  if( getPPS().getEntropyCodingModeFlag() && !isIntraSlice() )
  {
    RNOK(       rcWriteIf.writeUvlc ( m_uiCabacInitIdc,                               "SH: cabac_init_idc" ) );
  }
  RNOK(         rcWriteIf.writeSvlc ( m_iSliceQpDelta,                                "SH: slice_qp_delta" ) );
  if( isSPSlice() || isSISlice() )
  {
    if( isSPSlice() )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bSPForSwitchFlag,                             "SH: sp_for_switch_flag" ) );
    }
    RNOK(       rcWriteIf.writeSvlc ( m_iSliceQsDelta,                                "SH: slice_qs_delta" ) );
  }
  if( getPPS().getDeblockingFilterParametersPresentFlag() )
  {
    RNOK(       m_cDeblockingFilterParameter.write( rcWriteIf ) );
  }
  if( getPPS().getNumSliceGroupsMinus1() > 0 &&
      getPPS().getSliceGroupMapType() >= 3 && getPPS().getSliceGroupMapType() <= 5 )
  {
    RNOK(       rcWriteIf.writeCode ( m_uiSliceGroupChangeCycle,
                                      getPPS().getLog2MaxSliceGroupChangeCycle( getSPS().getMbInFrame() ),
                                                                                      "SH: slice_group_change_cycle" ) );
  }
  if( !getNoInterLayerPredFlag() && getQualityId() == 0 )
  {
    RNOK(       rcWriteIf.writeUvlc ( m_uiRefLayerDQId,                               "SH: ref_layer_dq_id" ) );
    if( getSPS().getInterlayerDeblockingPresent() )
    {
      RNOK(     m_cInterLayerDeblockingFilterParameter.write( rcWriteIf ) );
    }
    RNOK(       rcWriteIf.writeFlag ( m_bConstrainedIntraResamplingFlag,              "SH: constrained_intra_resampling_flag"));
    if( getSPS().getExtendedSpatialScalability() == ESS_PICT )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bRefLayerChromaPhaseXPlus1Flag,               "SH: ref_layer_chroma_phase_x_plus1_flag" ) );
      RNOK(     rcWriteIf.writeCode ( m_uiRefLayerChromaPhaseYPlus1, 2,               "SH: ref_layer_chroma_phase_y_plus1" ) );
      RNOK(     rcWriteIf.writeSvlc ( m_iScaledRefLayerLeftOffset,                    "SH: scaled_ref_layer_left_offset" ) );
      RNOK(     rcWriteIf.writeSvlc ( m_iScaledRefLayerTopOffset,                     "SH: scaled_ref_layer_top_offset" ) );
      RNOK(     rcWriteIf.writeSvlc ( m_iScaledRefLayerRightOffset,                   "SH: scaled_ref_layer_right_offset" ) );
      RNOK(     rcWriteIf.writeSvlc ( m_iScaledRefLayerBottomOffset,                  "SH: scaled_ref_layer_bottom_offset" ) );
    }
  }
  if( !getNoInterLayerPredFlag() )
  {
    RNOK(       rcWriteIf.writeFlag ( m_bSliceSkipFlag,                               "SH: slice_skip_flag" ) );
    if( m_bSliceSkipFlag )
    {
      RNOK(     rcWriteIf.writeUvlc ( m_uiNumMbsInSliceMinus1,                        "SH: num_mbs_in_slice_minus1" ) );
    }
    else
    {
      RNOK(     rcWriteIf.writeFlag ( m_bAdaptiveBaseModeFlag,                        "SH: adaptive_base_mode_flag" ) );
      if( !m_bAdaptiveBaseModeFlag )
      {
        RNOK(   rcWriteIf.writeFlag ( m_bDefaultBaseModeFlag,                         "SH: default_base_mode_flag" ) );
      }
      if( !m_bDefaultBaseModeFlag )
      {
        RNOK(   rcWriteIf.writeFlag ( m_bAdaptiveMotionPredictionFlag,                "SH: adaptive_motion_prediction_flag" ) );
        if( !m_bAdaptiveMotionPredictionFlag )
        {
          RNOK( rcWriteIf.writeFlag ( m_bDefaultMotionPredictionFlag,                 "SH: default_motion_prediction_flag" ) );
        }
      }
      RNOK(     rcWriteIf.writeFlag ( m_bAdaptiveResidualPredictionFlag,              "SH: adaptive_residual_prediction_flag" ) );
      if( !m_bAdaptiveResidualPredictionFlag )
      {
        RNOK(   rcWriteIf.writeFlag ( m_bDefaultResidualPredictionFlag,               "SH: default_residual_prediction_flag" ) );
      }
    }
    if( getSPS().getAVCAdaptiveRewriteFlag() )
    {
      RNOK(     rcWriteIf.writeFlag ( m_bTCoeffLevelPredictionFlag,                   "SH: tcoeff_level_prediction_flag" ) );
    }
  }
  if( !getSPS().getAVCHeaderRewriteFlag() && ! isH264AVCCompatible() && ! m_bSliceSkipFlag )
  {
    UInt  uiScanIdxStart  = ( m_uiScanIdxStart == m_uiScanIdxStop ? 1 : m_uiScanIdxStart );
    UInt  uiScanIdxEnd    = ( m_uiScanIdxStart == m_uiScanIdxStop ? 0 : m_uiScanIdxStop - 1 );
    RNOK(       rcWriteIf.writeCode ( uiScanIdxStart, 4,                              "SH: scan_idx_start" ) );
    RNOK(       rcWriteIf.writeCode ( uiScanIdxEnd,   4,                              "SH: scan_idx_end" ) );
  }
  return Err::m_nOK;
}


ErrVal
SliceHeaderSyntax::read( ParameterSetMng& rcParameterSetMng, HeaderSymbolReadIf& rcReadIf, Bool bInclusiveNalUnitHeader )
{
  if( bInclusiveNalUnitHeader )
  {
    NalUnitHeader::read( rcReadIf );
  }

  RNOK(         rcReadIf.getUvlc ( m_uiFirstMbInSlice,                             "SH: first_mb_in_slice" ) );
  UInt uiSliceType;
  RNOK(         rcReadIf.getUvlc ( uiSliceType,                                    "SH: slice_type" ) );
  m_eSliceType = SliceType( uiSliceType );
  RNOK(         rcReadIf.getUvlc ( m_uiPicParameterSetId,                          "SH: pic_parameter_set_id" ) );
  RNOK( xInitParameterSets( rcParameterSetMng, m_uiPicParameterSetId, getNalUnitType() == NAL_UNIT_CODED_SLICE_SCALABLE ) );

  RNOK(         rcReadIf.getCode ( m_uiFrameNum,
                                   getSPS().getLog2MaxFrameNum(),                  "SH: frame_num" ) );
  if( ! getSPS().getFrameMbsOnlyFlag() )
  {
    RNOK(       rcReadIf.getFlag ( m_bFieldPicFlag,                                "SH: field_pic_flag" ) );
    if( m_bFieldPicFlag )
    {
      RNOK(     rcReadIf.getFlag ( m_bBottomFieldFlag,                             "SH: bottom_field_flag" ) );
    }
  }
  if( getIdrFlag() )
  {
    RNOK(       rcReadIf.getUvlc ( m_uiIdrPicId,                                   "SH: idr_pic_id" ) );
  }
  if( getSPS().getPicOrderCntType() == 0 )
  {
    RNOK(       rcReadIf.getCode ( m_uiPicOrderCntLsb,
                                   getSPS().getLog2MaxPicOrderCntLsb(),            "SH: pic_order_cnt_lsb" ) );
    if( getPPS().getPicOrderPresentFlag() && ! m_bFieldPicFlag )
    {
      RNOK(     rcReadIf.getSvlc ( m_iDeltaPicOrderCntBottom,                      "SH: delta_pic_order_cnt_bottom" ) );
    }
  }
  if( getSPS().getPicOrderCntType() == 1 && !getSPS().getDeltaPicOrderAlwaysZeroFlag() )
  {
    RNOK(       rcReadIf.getSvlc ( m_iDeltaPicOrderCnt0,                           "SH: delta_pic_order_cnt[0]" ) );
    if( getPPS().getPicOrderPresentFlag() && ! m_bFieldPicFlag )
    {
      RNOK(     rcReadIf.getSvlc ( m_iDeltaPicOrderCnt1,                           "SH: delta_pic_order_cnt[1]" ) );
    }
  }
  if( getPPS().getRedundantPicCntPresentFlag() )
  {
    RNOK(       rcReadIf.getUvlc ( m_uiRedundantPicCnt,                            "SH: redundant_pic_cnt" ) );
  }
  if( ! getQualityId() )
  {
    if( isBSlice() )
    {
      RNOK(     rcReadIf.getFlag ( m_bDirectSpatialMvPredFlag,                     "SH: direct_spatial_mv_pred_flag" ) );
    }
    if( isInterSlice() )
    {
      RNOK(     rcReadIf.getFlag ( m_bNumRefIdxActiveOverrideFlag,                 "SH: num_ref_idx_active_override_flag" ) );
      if( m_bNumRefIdxActiveOverrideFlag )
      {
        RNOK(   rcReadIf.getUvlc ( m_uiNumRefIdxL0ActiveMinus1,                    "SH: num_ref_idx_l0_active_minus1" ) );
        if( isBSlice() )
        {
          RNOK( rcReadIf.getUvlc ( m_uiNumRefIdxL1ActiveMinus1,                    "SH: num_ref_idx_l1_active_minus1" ) );
        }
      }
    }
    if( isInterSlice() )
    {
      RNOK(     m_cRefPicListReorderingL0.read( rcReadIf ) );
    }
    if( isBSlice() )
    {
      RNOK(     m_cRefPicListReorderingL1.read( rcReadIf ) );
    }
    if( ( getPPS().getWeightedPredFlag() && isPorSPSlice() ) || ( getPPS().getWeightedBiPredIdc() == 1 && isBSlice() ) )
    {
      if( ! getNoInterLayerPredFlag() )
      {
        RNOK(   rcReadIf.getFlag ( m_bBasePredWeightTableFlag,                     "SH: base_pred_weight_table_flag" ) );
      }
      if( getNoInterLayerPredFlag() || ! m_bBasePredWeightTableFlag )
      {
        RNOK(   rcReadIf.getUvlc ( m_uiLumaLog2WeightDenom,                        "PWT: luma_log_weight_denom" ) );
        RNOK(   rcReadIf.getUvlc ( m_uiChromaLog2WeightDenom,                      "PWT: chroma_log_weight_denom" ) );
        RNOK(   m_cPredWeightTableL0.read( rcReadIf, m_uiNumRefIdxL0ActiveMinus1, m_uiLumaLog2WeightDenom, m_uiChromaLog2WeightDenom ) );
        if( isBSlice() )
        {
          RNOK( m_cPredWeightTableL1.read( rcReadIf, m_uiNumRefIdxL1ActiveMinus1, m_uiLumaLog2WeightDenom, m_uiChromaLog2WeightDenom ) );
        }
      }
    }
    if( getNalRefIdc() )
    {
      if( getIdrFlag() )
      {
        RNOK(   rcReadIf.getFlag ( m_bNoOutputOfPriorPicsFlag,                     "DRPM: no_output_of_prior_pics_flag" ) );
        RNOK(   rcReadIf.getFlag ( m_bLongTermReferenceFlag,                       "DRPM: long_term_reference_flag" ) );
      }
      else
      {
        RNOK(   m_cDecRefPicMarking.read( rcReadIf ) );
      }
      if( ! getSPS().getAVCHeaderRewriteFlag() && ! isH264AVCCompatible() )
      {
        Bool bStoreRefBasePicFlag;
        RNOK(   rcReadIf.getFlag ( bStoreRefBasePicFlag,                           "SH: store_ref_base_pic_flag" ) );
        setStoreRefBasePicFlag( bStoreRefBasePicFlag );
        if( ( getUseRefBasePicFlag() || getStoreRefBasePicFlag() ) && !getIdrFlag() )
        {
          RNOK( getDecRefBasePicMarking().read( rcReadIf ) );
        }
      }
    }
  }
  if( getPPS().getEntropyCodingModeFlag() && !isIntraSlice() )
  {
    RNOK(       rcReadIf.getUvlc ( m_uiCabacInitIdc,                               "SH: cabac_init_idc" ) );
  }
  RNOK(         rcReadIf.getSvlc ( m_iSliceQpDelta,                                "SH: slice_qp_delta" ) );
  if( isSPSlice() || isSISlice() )
  {
    if( isSPSlice() )
    {
      RNOK(     rcReadIf.getFlag ( m_bSPForSwitchFlag,                             "SH: sp_for_switch_flag" ) );
    }
    RNOK(       rcReadIf.getSvlc ( m_iSliceQsDelta,                                "SH: slice_qs_delta" ) );
  }
  if( getPPS().getDeblockingFilterParametersPresentFlag() )
  {
    RNOK(       m_cDeblockingFilterParameter.read( rcReadIf, getNalUnitType() == NAL_UNIT_CODED_SLICE_SCALABLE ) );
  }
  if( getPPS().getNumSliceGroupsMinus1() > 0 &&
      getPPS().getSliceGroupMapType() >= 3 && getPPS().getSliceGroupMapType() <= 5 )
  {
    RNOK(       rcReadIf.getCode ( m_uiSliceGroupChangeCycle,
                                   getPPS().getLog2MaxSliceGroupChangeCycle( getSPS().getMbInFrame() ),
                                                                                   "SH: slice_group_change_cycle" ) );
  }
  if( !getNoInterLayerPredFlag() && getQualityId() == 0 )
  {
    RNOK(       rcReadIf.getUvlc ( m_uiRefLayerDQId,                               "SH: ref_layer_dq_id" ) );
    if( getSPS().getInterlayerDeblockingPresent() )
    {
      RNOK(     m_cInterLayerDeblockingFilterParameter.read( rcReadIf, getNalUnitType() == NAL_UNIT_CODED_SLICE_SCALABLE ) );
    }
    RNOK(       rcReadIf.getFlag ( m_bConstrainedIntraResamplingFlag,              "SH: constrained_intra_resampling_flag"));
    if( getSPS().getExtendedSpatialScalability() == ESS_PICT )
    {
      RNOK(     rcReadIf.getFlag ( m_bRefLayerChromaPhaseXPlus1Flag,               "SH: ref_layer_chroma_phase_x_plus1_flag" ) );
      RNOK(     rcReadIf.getCode ( m_uiRefLayerChromaPhaseYPlus1, 2,               "SH: ref_layer_chroma_phase_y_plus1" ) );
      RNOK(     rcReadIf.getSvlc ( m_iScaledRefLayerLeftOffset,                    "SH: scaled_ref_layer_left_offset" ) );
      RNOK(     rcReadIf.getSvlc ( m_iScaledRefLayerTopOffset,                     "SH: scaled_ref_layer_top_offset" ) );
      RNOK(     rcReadIf.getSvlc ( m_iScaledRefLayerRightOffset,                   "SH: scaled_ref_layer_right_offset" ) );
      RNOK(     rcReadIf.getSvlc ( m_iScaledRefLayerBottomOffset,                  "SH: scaled_ref_layer_bottom_offset" ) );
    }
  }
  else
  {
    if( getNoInterLayerPredFlag() )
    {
      m_uiRefLayerDQId  = MSYS_UINT_MAX;
    }
    else
    {
      m_uiRefLayerDQId  = ( getDependencyId() << 4 ) + getQualityId() - 1;
    }
  }
  if( !getNoInterLayerPredFlag() )
  {
    RNOK(       rcReadIf.getFlag ( m_bSliceSkipFlag,                               "SH: slice_skip_flag" ) );
    if( m_bSliceSkipFlag )
    {
      RNOK(     rcReadIf.getUvlc ( m_uiNumMbsInSliceMinus1,                        "SH: num_mbs_in_slice_minus1" ) );
    }
    else
    {
      RNOK(     rcReadIf.getFlag ( m_bAdaptiveBaseModeFlag,                        "SH: adaptive_base_mode_flag" ) );
      if( !m_bAdaptiveBaseModeFlag )
      {
        RNOK(   rcReadIf.getFlag ( m_bDefaultBaseModeFlag,                         "SH: default_base_mode_flag" ) );
      }
      if( !m_bDefaultBaseModeFlag )
      {
        RNOK(   rcReadIf.getFlag ( m_bAdaptiveMotionPredictionFlag,                "SH: adaptive_motion_prediction_flag" ) );
        if( !m_bAdaptiveMotionPredictionFlag )
        {
          RNOK( rcReadIf.getFlag ( m_bDefaultMotionPredictionFlag,                 "SH: default_motion_prediction_flag" ) );
        }
      }
      RNOK(     rcReadIf.getFlag ( m_bAdaptiveResidualPredictionFlag,              "SH: adaptive_residual_prediction_flag" ) );
      if( !m_bAdaptiveResidualPredictionFlag )
      {
        RNOK(   rcReadIf.getFlag ( m_bDefaultResidualPredictionFlag,               "SH: default_residual_prediction_flag" ) );
      }
    }
    if( getSPS().getAVCAdaptiveRewriteFlag() )
    {
      RNOK(     rcReadIf.getFlag ( m_bTCoeffLevelPredictionFlag,                   "SH: tcoeff_level_prediction_flag" ) );
    }
  }
  else
  { 
    m_bTCoeffLevelPredictionFlag = false; 
  }
  if( !getSPS().getAVCHeaderRewriteFlag() && ! isH264AVCCompatible() && ! m_bSliceSkipFlag )
  {
    RNOK(       rcReadIf.getCode ( m_uiScanIdxStart, 4,                            "SH: scan_idx_start" ) );
    RNOK(       rcReadIf.getCode ( m_uiScanIdxStop,  4,                            "SH: scan_idx_end" ) );
    if( m_uiScanIdxStart > m_uiScanIdxStop )
    {
      m_uiScanIdxStart  = 0;
      m_uiScanIdxStop   = 0;
    }
    else
    {
      m_uiScanIdxStop++;
    }
  }
  return Err::m_nOK;
}


Void
SliceHeaderSyntax::xInit()
{
  m_uiFirstMbInSlice                      = 0;
  m_eSliceType                            = I_SLICE;
  m_uiPicParameterSetId                   = 0;
  m_uiColourPlaneId                       = 0;
  m_uiFrameNum                            = 0;
  m_bFieldPicFlag                         = false;
  m_bBottomFieldFlag                      = false;
  m_uiIdrPicId                            = 0;
  m_uiPicOrderCntLsb                      = 0;
  m_iDeltaPicOrderCntBottom               = 0;
  m_iDeltaPicOrderCnt0                    = 0;
  m_iDeltaPicOrderCnt1                    = 0;
  m_uiRedundantPicCnt                     = 0;
  m_bDirectSpatialMvPredFlag              = true;
  m_bNumRefIdxActiveOverrideFlag          = false;
  m_uiNumRefIdxL0ActiveMinus1             = 0;
  m_uiNumRefIdxL1ActiveMinus1             = 0;
  m_cRefPicListReorderingL0               .clear( false );
  m_cRefPicListReorderingL1               .clear( false );
  m_bBasePredWeightTableFlag              = false;
  m_uiLumaLog2WeightDenom                 = 0;
  m_uiChromaLog2WeightDenom               = 0;
  m_cPredWeightTableL0                    .clear();
  m_cPredWeightTableL1                    .clear();
  m_bNoOutputOfPriorPicsFlag              = true;
  m_bLongTermReferenceFlag                = false;
  m_cDecRefPicMarking                     .clear( false, false );
  m_uiCabacInitIdc                        = 0;
  m_iSliceQpDelta                         = 0;
  m_bSPForSwitchFlag                      = false;
  m_iSliceQsDelta                         = 0;
  m_cDeblockingFilterParameter            .clear( false );
  m_uiSliceGroupChangeCycle               = 0;
  m_uiRefLayerDQId                        = MSYS_UINT_MAX;
  m_cInterLayerDeblockingFilterParameter  .clear( true );
  m_bConstrainedIntraResamplingFlag       = false;
  m_bRefLayerChromaPhaseXPlus1Flag        = false;
  m_uiRefLayerChromaPhaseYPlus1           = 0;
  m_iScaledRefLayerLeftOffset             = 0;
  m_iScaledRefLayerTopOffset              = 0;
  m_iScaledRefLayerRightOffset            = 0;
  m_iScaledRefLayerBottomOffset           = 0;
  m_bSliceSkipFlag                        = false;
  m_uiNumMbsInSliceMinus1                 = 0;
  m_bAdaptiveBaseModeFlag                 = false;
  m_bDefaultBaseModeFlag                  = false;
  m_bAdaptiveMotionPredictionFlag         = false;
  m_bDefaultMotionPredictionFlag          = false;
  m_bAdaptiveResidualPredictionFlag       = false;
  m_bDefaultResidualPredictionFlag        = false;
  m_bTCoeffLevelPredictionFlag            = false;
  m_uiScanIdxStart                        = 0;
  m_uiScanIdxStop                         = 16;
  m_pcSPS                                 = 0;
  m_pcPPS                                 = 0;
  m_acScalingMatrix                       .setAll( 0 );
}

Void
SliceHeaderSyntax::xCopy( const SliceHeaderSyntax&  rcSliceHeaderSyntax )
{
  m_uiFirstMbInSlice                      = rcSliceHeaderSyntax.m_uiFirstMbInSlice;
  m_eSliceType                            = rcSliceHeaderSyntax.m_eSliceType;
  m_uiPicParameterSetId                   = rcSliceHeaderSyntax.m_uiPicParameterSetId;
  m_uiColourPlaneId                       = rcSliceHeaderSyntax.m_uiColourPlaneId;
  m_uiFrameNum                            = rcSliceHeaderSyntax.m_uiFrameNum;
  m_bFieldPicFlag                         = rcSliceHeaderSyntax.m_bFieldPicFlag;
  m_bBottomFieldFlag                      = rcSliceHeaderSyntax.m_bBottomFieldFlag;
  m_uiIdrPicId                            = rcSliceHeaderSyntax.m_uiIdrPicId;
  m_uiPicOrderCntLsb                      = rcSliceHeaderSyntax.m_uiPicOrderCntLsb;
  m_iDeltaPicOrderCntBottom               = rcSliceHeaderSyntax.m_iDeltaPicOrderCntBottom;
  m_iDeltaPicOrderCnt0                    = rcSliceHeaderSyntax.m_iDeltaPicOrderCnt0;
  m_iDeltaPicOrderCnt1                    = rcSliceHeaderSyntax.m_iDeltaPicOrderCnt1;
  m_uiRedundantPicCnt                     = rcSliceHeaderSyntax.m_uiRedundantPicCnt;
  m_bDirectSpatialMvPredFlag              = rcSliceHeaderSyntax.m_bDirectSpatialMvPredFlag;
  m_bNumRefIdxActiveOverrideFlag          = rcSliceHeaderSyntax.m_bNumRefIdxActiveOverrideFlag;
  m_uiNumRefIdxL0ActiveMinus1             = rcSliceHeaderSyntax.m_uiNumRefIdxL0ActiveMinus1;
  m_uiNumRefIdxL1ActiveMinus1             = rcSliceHeaderSyntax.m_uiNumRefIdxL1ActiveMinus1;
  m_cRefPicListReorderingL0               = rcSliceHeaderSyntax.m_cRefPicListReorderingL0;
  m_cRefPicListReorderingL1               = rcSliceHeaderSyntax.m_cRefPicListReorderingL1;
  m_bBasePredWeightTableFlag              = rcSliceHeaderSyntax.m_bBasePredWeightTableFlag;
  m_uiLumaLog2WeightDenom                 = rcSliceHeaderSyntax.m_uiLumaLog2WeightDenom;
  m_uiChromaLog2WeightDenom               = rcSliceHeaderSyntax.m_uiChromaLog2WeightDenom;
  m_cPredWeightTableL0                    = rcSliceHeaderSyntax.m_cPredWeightTableL0;
  m_cPredWeightTableL1                    = rcSliceHeaderSyntax.m_cPredWeightTableL1;
  m_bNoOutputOfPriorPicsFlag              = rcSliceHeaderSyntax.m_bNoOutputOfPriorPicsFlag;
  m_bLongTermReferenceFlag                = rcSliceHeaderSyntax.m_bLongTermReferenceFlag;
  m_cDecRefPicMarking                     = rcSliceHeaderSyntax.m_cDecRefPicMarking;
  m_uiCabacInitIdc                        = rcSliceHeaderSyntax.m_uiCabacInitIdc;
  m_iSliceQpDelta                         = rcSliceHeaderSyntax.m_iSliceQpDelta;
  m_bSPForSwitchFlag                      = rcSliceHeaderSyntax.m_bSPForSwitchFlag;
  m_iSliceQsDelta                         = rcSliceHeaderSyntax.m_iSliceQsDelta;
  m_cDeblockingFilterParameter            = rcSliceHeaderSyntax.m_cDeblockingFilterParameter;
  m_uiSliceGroupChangeCycle               = rcSliceHeaderSyntax.m_uiSliceGroupChangeCycle;
  m_uiRefLayerDQId                        = rcSliceHeaderSyntax.m_uiRefLayerDQId;
  m_cInterLayerDeblockingFilterParameter  = rcSliceHeaderSyntax.m_cInterLayerDeblockingFilterParameter;
  m_bConstrainedIntraResamplingFlag       = rcSliceHeaderSyntax.m_bConstrainedIntraResamplingFlag;
  m_bRefLayerChromaPhaseXPlus1Flag        = rcSliceHeaderSyntax.m_bRefLayerChromaPhaseXPlus1Flag;
  m_uiRefLayerChromaPhaseYPlus1           = rcSliceHeaderSyntax.m_uiRefLayerChromaPhaseYPlus1;
  m_iScaledRefLayerLeftOffset             = rcSliceHeaderSyntax.m_iScaledRefLayerLeftOffset;
  m_iScaledRefLayerTopOffset              = rcSliceHeaderSyntax.m_iScaledRefLayerTopOffset;
  m_iScaledRefLayerRightOffset            = rcSliceHeaderSyntax.m_iScaledRefLayerRightOffset;
  m_iScaledRefLayerBottomOffset           = rcSliceHeaderSyntax.m_iScaledRefLayerBottomOffset;
  m_bSliceSkipFlag                        = rcSliceHeaderSyntax.m_bSliceSkipFlag;
  m_uiNumMbsInSliceMinus1                 = rcSliceHeaderSyntax.m_uiNumMbsInSliceMinus1;
  m_bAdaptiveBaseModeFlag                 = rcSliceHeaderSyntax.m_bAdaptiveBaseModeFlag;
  m_bDefaultBaseModeFlag                  = rcSliceHeaderSyntax.m_bDefaultBaseModeFlag;
  m_bAdaptiveMotionPredictionFlag         = rcSliceHeaderSyntax.m_bAdaptiveMotionPredictionFlag;
  m_bDefaultMotionPredictionFlag          = rcSliceHeaderSyntax.m_bDefaultMotionPredictionFlag;
  m_bAdaptiveResidualPredictionFlag       = rcSliceHeaderSyntax.m_bAdaptiveResidualPredictionFlag;
  m_bDefaultResidualPredictionFlag        = rcSliceHeaderSyntax.m_bDefaultMotionPredictionFlag;
  m_bTCoeffLevelPredictionFlag            = rcSliceHeaderSyntax.m_bTCoeffLevelPredictionFlag;
  m_uiScanIdxStart                        = rcSliceHeaderSyntax.m_uiScanIdxStart;
  m_uiScanIdxStop                         = rcSliceHeaderSyntax.m_uiScanIdxStop;
  m_pcSPS                                 = rcSliceHeaderSyntax.m_pcSPS;
  m_pcPPS                                 = rcSliceHeaderSyntax.m_pcPPS;
  m_acScalingMatrix                       = rcSliceHeaderSyntax.m_acScalingMatrix;
}

ErrVal
SliceHeaderSyntax::xInitScalingMatrix()
{
  ROF( parameterSetsInitialized() );

  const Bool    bSeqScalMatPresent  = getSPS().getSeqScalingMatrixPresentFlag();
  const Bool    bPicScalMatPresent  = getPPS().getPicScalingMatrixPresentFlag();
  const UChar*  apucSeqScalMat[12]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  const UChar*  apucPicScalMat[12]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  //----- check for flat scaling matrices -----
  if( ! bSeqScalMatPresent && ! bPicScalMatPresent )
  {
    m_acScalingMatrix.setAll( 0 ); // use flat scaling matrices
    return Err::m_nOK;
  }

  //----- derive sequence scaling matrices -----
  if( bSeqScalMatPresent )
  {
    for( UInt uiIndex = 0; uiIndex < m_acScalingMatrix.size(); uiIndex++ )
    {
      const UChar*  puc = getSPS().getSeqScalingMatrix().get( uiIndex );
      if( ! puc )
      { // fall-back rule A for sequence-level scaling matrices
        switch( uiIndex )
        {
        case  0:    puc = g_aucScalingMatrixDefault4x4Intra;  break;
        case  3:    puc = g_aucScalingMatrixDefault4x4Inter;  break;
        case  6:    puc = g_aucScalingMatrixDefault8x8Intra;  break;
        case  7:    puc = g_aucScalingMatrixDefault8x8Inter;  break;
        default:    puc = apucSeqScalMat[ uiIndex - 1 ];
        }
      }
      apucSeqScalMat[ uiIndex ] = puc;
    }
  }

  //----- derive picture scaling matrices -----
  if( bPicScalMatPresent )
  {
    for( UInt uiIndex = 0; uiIndex < m_acScalingMatrix.size(); uiIndex++ )
    {
      const Bool    bChroma = ( uiIndex != 0 && uiIndex != 3 && uiIndex != 6 && uiIndex != 7 );
      const UChar*  puc     = getPPS().getPicScalingMatrix().get( uiIndex );
      if( ! puc )
      {
        if( getSPS().getSeqScalingMatrixPresentFlag() )
        { // fall-back rule B
          if( bChroma )
          {
            puc = apucPicScalMat[ uiIndex - 1 ];
          }
          else
          {
            puc = apucSeqScalMat[ uiIndex ];
          }
        }
        else
        { // fall-back rule A
          switch( uiIndex )
          {
          case  0:    puc = g_aucScalingMatrixDefault4x4Intra;  break;
          case  3:    puc = g_aucScalingMatrixDefault4x4Inter;  break;
          case  6:    puc = g_aucScalingMatrixDefault8x8Intra;  break;
          case  7:    puc = g_aucScalingMatrixDefault8x8Inter;  break;
          default:    puc = apucPicScalMat[ uiIndex - 1 ];
          }
        }
      }
      apucPicScalMat[ uiIndex ] = puc;
    }
  }
  else
  { // use sequence-level scaling matrices
    for( UInt uiIndex = 0; uiIndex < m_acScalingMatrix.size(); uiIndex++ )
    {
      apucPicScalMat[ uiIndex ] = apucSeqScalMat[ uiIndex ];
    }
  }
  
  //----- set scaling matrices -----
  for( UInt uiIndex = 0; uiIndex < m_acScalingMatrix.size(); uiIndex++ )
  {
    ROF( apucPicScalMat[ uiIndex ] );
    if ( apucPicScalMat[ uiIndex ][ 0 ] == 0 )
    {
      switch( uiIndex )
      {
      case  0: 
      case  1:
      case  2:  apucPicScalMat[ uiIndex ] = g_aucScalingMatrixDefault4x4Intra;  break;
      case  3:
      case  4:
      case  5:  apucPicScalMat[ uiIndex ] = g_aucScalingMatrixDefault4x4Inter;  break;
      case  6: 
      case  8:
      case 10:  apucPicScalMat[ uiIndex ] = g_aucScalingMatrixDefault8x8Intra;  break;
      case  7:
      case  9:
      case 11:  apucPicScalMat[ uiIndex ] = g_aucScalingMatrixDefault8x8Inter;  break;
      default:  RERR();
      }
    }
    m_acScalingMatrix.set( uiIndex, apucPicScalMat[ uiIndex ] );
  }
  return Err::m_nOK;
}


ErrVal
SliceHeaderSyntax::xInitParameterSets( ParameterSetMng& rcParameterSetMng, UInt uiPPSId, Bool bSubSetSPS )
{
  SequenceParameterSet* pcSPS   = 0;
  PictureParameterSet*  pcPPS   = 0;
  UInt                  uiDQId  = (getDependencyId()<<4)+getQualityId();
  RNOK( rcParameterSetMng.get( pcPPS, uiPPSId ) );
  RNOK( rcParameterSetMng.get( pcSPS, pcPPS->getSeqParameterSetId(), bSubSetSPS ) );
  {
    Bool bSupported = true;
    if( bSubSetSPS )
    {
      if( pcSPS->getProfileIdc() != SCALABLE_HIGH_PROFILE && pcSPS->getProfileIdc() != SCALABLE_BASELINE_PROFILE )
      {
        bSupported = false;
      }
    }
    else
    {
      Bool  bClassAProfile  = ( pcSPS->getProfileIdc() == BASELINE_PROFILE || pcSPS->getProfileIdc() == EXTENDED_PROFILE || pcSPS->getProfileIdc() == MAIN_PROFILE || pcSPS->getProfileIdc() == HIGH_PROFILE );
      Bool  bClassBProfile  = ( pcSPS->getProfileIdc() == HIGH_10_PROFILE || pcSPS->getProfileIdc() == HIGH_422_PROFILE || pcSPS->getProfileIdc() == HIGH_444_PROFILE  || pcSPS->getProfileIdc() == CAVLC_444_PROFILE );
      Bool  bConstraintSet  = ( pcSPS->getConstrainedSet0Flag() || pcSPS->getConstrainedSet1Flag() );
      if( ! bClassAProfile && ! ( bClassBProfile && bConstraintSet ) )
      {
        // SP,SI slices are separately checked
        bSupported = false;
      }
    }
    if( ! bSupported )
    {
      fprintf( stderr, "\n" );
      fprintf( stderr, "Unsupported conformance point:\n" );
      fprintf( stderr, "   profile_idc          = %d\n", (Int)pcSPS->getProfileIdc() );
      fprintf( stderr, "   constraint_set0_flag = %d\n", ( pcSPS->getConstrainedSet0Flag() ? 1 : 0 ) );
      fprintf( stderr, "   constraint_set1_flag = %d\n", ( pcSPS->getConstrainedSet1Flag() ? 1 : 0 ) );
      fprintf( stderr, "   constraint_set2_flag = %d\n", ( pcSPS->getConstrainedSet2Flag() ? 1 : 0 ) );
      fprintf( stderr, "   constraint_set3_flag = %d\n", ( pcSPS->getConstrainedSet3Flag() ? 1 : 0 ) );
      fprintf( stderr, "\n" );
      RERR();
    }
  }
  rcParameterSetMng.setActiveSPS( pcSPS->getSeqParameterSetId(), uiDQId );
  RNOK( xInitParameterSets( *pcSPS, *pcPPS ) );
  return Err::m_nOK;
}

ErrVal
SliceHeaderSyntax::xInitParameterSets( const SequenceParameterSet& rcSPS, const PictureParameterSet& rcPPS )
{
  m_pcPPS                           = &rcPPS;
  m_pcSPS                           = &rcSPS;
  m_uiPicParameterSetId             = rcPPS.getPicParameterSetId();
  m_uiNumRefIdxL0ActiveMinus1       = rcPPS.getNumRefIdxActive( LIST_0 ) - 1;
  m_uiNumRefIdxL1ActiveMinus1       = rcPPS.getNumRefIdxActive( LIST_1 ) - 1;
  m_bRefLayerChromaPhaseXPlus1Flag  = rcSPS.getBaseChromaPhaseXPlus1() != 0;
  m_uiRefLayerChromaPhaseYPlus1     = rcSPS.getBaseChromaPhaseYPlus1();
  m_iScaledRefLayerLeftOffset       = rcSPS.getScaledBaseLeftOffset();
  m_iScaledRefLayerTopOffset        = rcSPS.getScaledBaseTopOffset();
  m_iScaledRefLayerRightOffset      = rcSPS.getScaledBaseRightOffset();
  m_iScaledRefLayerBottomOffset     = rcSPS.getScaledBaseBottomOffset();
  m_bTCoeffLevelPredictionFlag      = rcSPS.getTCoeffLevelPredictionFlag();

  RNOK( xInitScalingMatrix() );
  return Err::m_nOK;
}



H264AVC_NAMESPACE_END

