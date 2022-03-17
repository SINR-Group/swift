
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"
#include "SequenceStructure.h"
#include <string>


H264AVC_NAMESPACE_BEGIN


//================================= Frame Spec : begin ===================================
FrameSpec::FrameSpec()
: m_bInit     ( false )
, m_pcMmcoBuf ( NULL )
{
  m_apcRplrBuf[LIST_0] = NULL;
  m_apcRplrBuf[LIST_1] = NULL;
}

FrameSpec::~FrameSpec()
{
  uninit();
}

Void
FrameSpec::uninit()
{
  m_bInit = false;
}

ErrVal
FrameSpec::init( UChar        ucType,
                 UInt         uiFrameNum,
                 Bool         bAnchor,
                 UInt         uiFramesSkipped,
                 Bool         bUseBaseRep,
                 UInt         uiLayer,
                 DecRefPicMarking*  pcMmcoBuf,
                 RefPicListReOrdering*  pcRplrBufL0,
                 RefPicListReOrdering*  pcRplrBufL1 )
{
  //===== set slice_type, nal_unit_type, nal_ref_idc
  switch( ucType )
  {
  case 'A': //========== IDR picture ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE_IDR;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_HIGHEST;
      m_eSliceType    = I_SLICE;
    }
    break;
  case 'I': //========== non-IDR Intra picture stored as reference ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_HIGH;
      m_eSliceType    = I_SLICE;
    }
    break;
  case 'i': //========== non-IDR Intra picture not stored ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_LOWEST;
      m_eSliceType    = I_SLICE;
    }
    break;
  case 'P': //========== P picture stored as reference ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_HIGH;
      m_eSliceType    = P_SLICE;
    }
    break;
  case 'p': //========== P picture not stored ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_LOWEST;
      m_eSliceType    = P_SLICE;
    }
    break;
  case 'B': //========== B picture stored as reference ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_LOW;
      m_eSliceType    = B_SLICE;
    }
    break;
  case 'b': //========== B picture not stored ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_LOWEST;
      m_eSliceType    = B_SLICE;
    }
    break;
  case 'S': //========== skipped picture (not coded / not stored) ===========
    {
      m_eNalUnitType  = NAL_UNIT_CODED_SLICE;
      m_eNalRefIdc    = NAL_REF_IDC_PRIORITY_LOW;
      m_eSliceType    = SliceType   ( MSYS_UINT_MAX );
    }
    break;
  case 's': //========== skipped picture (not coded / not stored) ===========
    {
      m_eNalUnitType  = NalUnitType ( MSYS_UINT_MAX );
      m_eNalRefIdc    = NalRefIdc   ( MSYS_UINT_MAX );
      m_eSliceType    = SliceType   ( MSYS_UINT_MAX );
    }
    break;
  default:
    RERR();
    break;
  }

  //===== set remaining parameters =====
  m_bAnchor             = bAnchor;
  m_uiFramesSkipped     = uiFramesSkipped;
  m_bUseBaseRep         = bUseBaseRep;
  m_uiTemporalLayer     = uiLayer;
  m_uiContFrameNumber   = uiFrameNum;
  m_pcMmcoBuf           = pcMmcoBuf;
  m_apcRplrBuf[LIST_0]  = pcRplrBufL0;
  m_apcRplrBuf[LIST_1]  = pcRplrBufL1;
  m_bInit               = true;

  return Err::m_nOK;
}


Bool
FrameSpec::isInitialized() const
{
  return m_bInit;
}

UInt
FrameSpec::getContFrameNumber() const
{
  AOF_DBG( m_bInit );
  return m_uiContFrameNumber;
}

SliceType
FrameSpec::getSliceType() const
{
  AOF_DBG( m_bInit );
  return m_eSliceType;
}

NalUnitType
FrameSpec::getNalUnitType() const
{
  AOF_DBG( m_bInit );
  return m_eNalUnitType;
}

NalRefIdc
FrameSpec::getNalRefIdc() const
{
  AOF_DBG( m_bInit );
  return m_eNalRefIdc;
}

Bool
FrameSpec::isSkipped() const
{
  AOF_DBG( m_bInit );
  return m_eSliceType == MSYS_UINT_MAX;
}

Bool
FrameSpec::isBaseRep() const
{
  AOF_DBG( m_bInit );
  return m_bUseBaseRep;
}

Bool
FrameSpec::isAnchor() const
{
  AOF_DBG( m_bInit );
  return m_bAnchor;
}

UInt
FrameSpec::getFramesSkipped() const
{
  AOF_DBG( m_bInit );
  return m_uiFramesSkipped;
}

UInt
FrameSpec::getTemporalLayer() const
{
  AOF_DBG( m_bInit );
  return m_uiTemporalLayer;
}

const DecRefPicMarking*
FrameSpec::getMmcoBuf() const
{
  AOF_DBG( m_bInit );
  return m_pcMmcoBuf;
}

const RefPicListReOrdering*
FrameSpec::getRplrBuf( ListIdx eLstIdx) const
{
  AOF_DBG( m_bInit );
  return m_apcRplrBuf[eLstIdx];
}
//================================= Frame Spec : end ===================================





//================================= Frame Descriptor : begin ===================================
SequenceStructure::FrameDescriptor::FrameDescriptor()
: m_bInit       ( false )
, m_pcMmcoBuf   ( NULL )
{
  m_apcRplrBuf[0] = NULL;
  m_apcRplrBuf[1] = NULL;
}

SequenceStructure::FrameDescriptor::~FrameDescriptor()
{
  uninit();
}


Void
SequenceStructure::FrameDescriptor::uninit()
{
  delete m_pcMmcoBuf;
  delete m_apcRplrBuf[LIST_0];
  delete m_apcRplrBuf[LIST_1];

  m_pcMmcoBuf          = NULL;
  m_apcRplrBuf[LIST_0] = NULL;
  m_apcRplrBuf[LIST_1] = NULL;
  m_bInit              = false;
}


ErrVal
SequenceStructure::FrameDescriptor::init( const String& rcString,
                                          UInt          uiLastAnchorFrameNumIncrement )
{
  //===== clear object =====
  uninit();

  //===== get partial strings =====
  String cFDString;
  String cMmcoString;
  String cRplrStringL0;
  String cRplrStringL1;
  RNOK( FormattedStringParser::separatString            ( rcString,
                                                          cFDString, cMmcoString, cRplrStringL0, cRplrStringL1) );

  //===== parse basic parameters =====
  RNOKS( FormattedStringParser::extractFrameDescription ( cFDString,
                                                          m_ucType, m_uiFrameNumIncrement, m_bUseBaseRep, m_uiLayer ) );
  m_bAnchor         = ( m_uiFrameNumIncrement > uiLastAnchorFrameNumIncrement || uiLastAnchorFrameNumIncrement == MSYS_UINT_MAX );
  m_uiFramesSkipped = ( m_bAnchor ? m_uiFrameNumIncrement - uiLastAnchorFrameNumIncrement - 1 : 0 );

  //===== parse and set MMCO and RPLR commands =====
  if( ! cMmcoString.empty() )
  {
    ROT( NULL == ( m_pcMmcoBuf = new DecRefPicMarking( false ) ) );
    RNOK( FormattedStringParser::extractMmco( cMmcoString, *m_pcMmcoBuf ) );
  }

  if( ! cRplrStringL0.empty() )
  {
    ROT( NULL == ( m_apcRplrBuf[LIST_0] = new RefPicListReOrdering() ) );
    RNOK( FormattedStringParser::extractRplr( cRplrStringL0, *m_apcRplrBuf[LIST_0] ) );
  }

  if( ! cRplrStringL1.empty() )
  {
    ROT( NULL == ( m_apcRplrBuf[LIST_1] = new RefPicListReOrdering ) );
    RNOK( FormattedStringParser::extractRplr( cRplrStringL1, *m_apcRplrBuf[LIST_1] ) );
  }

  m_bInit = true;

  return Err::m_nOK;
}


ErrVal
SequenceStructure::FrameDescriptor::reduceFramesSkipped( UInt uiNotCoded )
{
  ROFRS( isAnchor(), Err::m_nOK );

  ROT( uiNotCoded > m_uiFramesSkipped );
  m_uiFramesSkipped -= uiNotCoded;
  return Err::m_nOK;
}


ErrVal
SequenceStructure::FrameDescriptor::check() const
{
  ROFS( m_bInit );

 // limited version, since encoder support is limited
  ROTS( m_ucType == 's' || m_ucType == 'S' );

  if( m_pcMmcoBuf )
  {
    for( UInt uiPos = 0; ; uiPos++ )
    {
      MmcoCommand cCommand = m_pcMmcoBuf->get( uiPos );
      if(  cCommand.isEnd() )
      {
        break;
      }
      UInt uiVal1, uiVal2;
      ROTS( cCommand.getCommand( uiVal1, uiVal2 ) != MMCO_SHORT_TERM_UNUSED );
    }
  }
  for( UInt uiList = 0; uiList < 2; uiList++ )
  {
    if( m_apcRplrBuf[uiList] )
    {
      for( UInt uiPos = 0; ; uiPos++ )
      {
        RplrCommand cCommand = m_apcRplrBuf[uiList]->get( uiPos );
        if(  cCommand.isEnd() )
        {
          break;
        }
        UInt uiVal;
        ROTS( cCommand.getCommand( uiVal ) != RPLR_NEG &&
              cCommand.getCommand( uiVal ) != RPLR_POS    );
      }
    }
  }


  return Err::m_nOK;
}

Bool
SequenceStructure::FrameDescriptor::isIDR() const
{
  AOF_DBG( m_bInit );
  return ( m_ucType == 'A' );
}

Bool
SequenceStructure::FrameDescriptor::isSkipped() const
{
  AOF_DBG( m_bInit );
  return ( m_ucType == 's' ) || ( m_ucType == 'S' );
}

Bool
SequenceStructure::FrameDescriptor::isCoded() const
{
  AOF_DBG( m_bInit );
  return ! isSkipped();
}

Bool
SequenceStructure::FrameDescriptor::isReference() const
{
  AOF_DBG( m_bInit );
  return ( m_ucType == 'A' || m_ucType == 'I' || m_ucType == 'P' || m_ucType == 'B' || m_ucType == 'S' );
}

Bool
SequenceStructure::FrameDescriptor::isAnchor() const
{
  AOF_DBG( m_bInit );
  return m_bAnchor;
}

Bool
SequenceStructure::FrameDescriptor::isBaseRep() const
{
  AOF_DBG( m_bInit );
  return m_bUseBaseRep;
}

UInt
SequenceStructure::FrameDescriptor::getIncrement() const
{
  AOF_DBG( m_bInit );
  return m_uiFrameNumIncrement;
}

UInt
SequenceStructure::FrameDescriptor::getFramesSkipped() const
{
  AOF_DBG( m_bInit );
  return m_uiFramesSkipped;
}


ErrVal
SequenceStructure::FrameDescriptor::setFrameSpec( FrameSpec&  rcFrameSpec,
                                                  UInt        uiFrameNumOffset ) const
{
  AOF_DBG( m_bInit );

  RNOK( rcFrameSpec.init( m_ucType,
                          m_uiFrameNumIncrement + uiFrameNumOffset,
                          m_bAnchor,
                          m_uiFramesSkipped,
                          m_bUseBaseRep,
                          m_uiLayer,
                          m_pcMmcoBuf,
                          m_apcRplrBuf[LIST_0],
                          m_apcRplrBuf[LIST_1] ) );

  return Err::m_nOK;
}
//================================= Frame Descriptor : end ===================================





//================================= Frame Sequence Part : begin ===================================
SequenceStructure::FrameSequencePart::FrameSequencePart()
: m_bInit               ( false )
, m_pacFrameDescriptor  ( 0     )
{
}


SequenceStructure::FrameSequencePart::~FrameSequencePart()
{
  uninit();
}


Void
SequenceStructure::FrameSequencePart::uninit()
{
  delete[] m_pacFrameDescriptor;

  m_pacFrameDescriptor  = 0;
  m_bInit               = false;
}


ErrVal
SequenceStructure::FrameSequencePart::init( const String& rcString )
{
  uninit();

  //----- get number of repetitions and number of frames -----
  String cNoRepString;
  RNOKS( FormattedStringParser::extractRepetitions( rcString, cNoRepString, m_uiNumberOfRepetitions ) );
  RNOKS( FormattedStringParser::getNumberOfFrames (           cNoRepString, m_uiNumberOfFrames      ) );
  ROFS ( m_uiNumberOfRepetitions );
  ROFS ( m_uiNumberOfFrames      );

  //----- create array -----
  ROFS( ( m_pacFrameDescriptor = new FrameDescriptor [ m_uiNumberOfFrames ] ) );

  //----- initialize array -----
  UInt uiPos, uiIndex, uiLastAnchorFrameNumIncrement = MSYS_UINT_MAX;
  for( uiPos = 0, uiIndex = 0; uiIndex < m_uiNumberOfFrames; uiIndex++ )
  {
    String cFDString;
    RNOKS( FormattedStringParser::extractNextFrameDescription ( cNoRepString, cFDString, uiPos ) );
    m_pacFrameDescriptor[uiIndex].init                        ( cFDString, uiLastAnchorFrameNumIncrement );

    if( m_pacFrameDescriptor[uiIndex].isSkipped() )
    {
      for( Int iIdx = (Int)(uiIndex-1); iIdx >= 0; iIdx-- )
      {
        if( m_pacFrameDescriptor[iIdx].isAnchor() )
        {
          if( m_pacFrameDescriptor[iIdx].getIncrement() > m_pacFrameDescriptor[uiIndex].getIncrement() )
          {
            m_pacFrameDescriptor[iIdx].reduceFramesSkipped( 1 );
          }
          break;
        }
      }
      if( (Int)m_pacFrameDescriptor[uiIndex].getIncrement() > (Int)uiLastAnchorFrameNumIncrement )
      {
        uiLastAnchorFrameNumIncrement = m_pacFrameDescriptor[uiIndex].getIncrement();
      }
    }
    else if( m_pacFrameDescriptor[uiIndex].isAnchor() )
    {
      uiLastAnchorFrameNumIncrement = m_pacFrameDescriptor[uiIndex].getIncrement();
    }
  }

  //----- reset -----
  m_uiCurrentRepetition = 0;
  m_uiCurrentFrame      = 0;

  //----- get minimum required DPB sizes -----
  UInt*   pauiStored;
  ROFS( ( pauiStored = new UInt [ m_uiNumberOfFrames ] ) );
  ::memset( pauiStored, 0x00, m_uiNumberOfFrames*sizeof(UInt) );
  m_uiMinDPBSizeRef       = 0;
  m_uiMinDPBSizeNonRef    = 0;
  UInt    uiNextOutput    = 0;
  UInt    uiStoredRef     = 0;
  UInt    uiStoredNonRef  = 0;
  for( uiIndex = 0; uiIndex < m_uiNumberOfFrames; uiIndex++ )
  {
    UInt  uiCurrPos = m_pacFrameDescriptor[uiIndex].getIncrement();
    Bool  bRefFrame = m_pacFrameDescriptor[uiIndex].isReference ();

    if( uiCurrPos > uiNextOutput )
    {
      ROT( uiCurrPos >= m_uiNumberOfFrames );
      //===== store current frame =====
      if( bRefFrame )
      {
        pauiStored[uiCurrPos] = 1;
        uiStoredRef++;
      }
      else
      {
        pauiStored[uiCurrPos] = 2;
        uiStoredNonRef++;
      }
    }
    else
    {
      //===== check if stored frames can be outputted =====
      while( (++uiNextOutput) < m_uiNumberOfFrames )
      {
        if( pauiStored[uiNextOutput] )
        {
          if( pauiStored[uiNextOutput] == 1 )
          {
            uiStoredRef--;
          }
          else
          {
            uiStoredNonRef--;
          }
          pauiStored[uiNextOutput] = 0;
        }
        else
        {
          break;
        }
      }
    }
    //----- update minimum required DPB sizes -----
    m_uiMinDPBSizeRef     = gMax( m_uiMinDPBSizeRef,    uiStoredRef    );
    m_uiMinDPBSizeNonRef  = gMax( m_uiMinDPBSizeNonRef, uiStoredNonRef );
  }
  delete[] pauiStored;

  //---- set init flag ----
  m_bInit = true;

  return Err::m_nOK;
}


Void
SequenceStructure::FrameSequencePart::reset()
{
  AOF_DBG( m_bInit );

  m_uiCurrentFrame      = 0;
  m_uiCurrentRepetition = 0;
}


ErrVal
SequenceStructure::FrameSequencePart::check()
{
  ROFS( m_bInit );
  ROFS( m_uiNumberOfRepetitions );
  ROFS( m_uiNumberOfFrames );

  Bool*   pabCovered;
  UInt    uiIndex;
  ROFS( ( pabCovered = new Bool [ m_uiNumberOfFrames ] ) );
  for( uiIndex = 0; uiIndex < m_uiNumberOfFrames; uiIndex++ )
  {
    pabCovered[uiIndex] = false;
  }
  for( uiIndex = 0; uiIndex < m_uiNumberOfFrames; uiIndex++ )
  {
    RNOKS( m_pacFrameDescriptor[uiIndex].check() );

    UInt uiInc = m_pacFrameDescriptor[uiIndex].getIncrement();

    ROTS( uiInc >= m_uiNumberOfFrames );
    ROTS( pabCovered[ uiInc ] );

    pabCovered[uiInc] = true;
  }
  delete[] pabCovered;

  return Err::m_nOK;
}


Bool
SequenceStructure::FrameSequencePart::isFirstIDR()  const
{
  AOF_DBG( m_bInit );
  AOF_DBG( m_uiNumberOfFrames );
  return m_pacFrameDescriptor[0].isIDR();
}

UInt
SequenceStructure::FrameSequencePart::getMinDPBSizeRef()  const
{
  AOF_DBG(m_bInit);
  return m_uiMinDPBSizeRef;
}


UInt
SequenceStructure::FrameSequencePart::getMinDPBSizeNonRef()  const
{
  AOF_DBG(m_bInit);
  return m_uiMinDPBSizeNonRef;
}


Bool
SequenceStructure::FrameSequencePart::getNextFrameSpec( FrameSpec& rcFrameSpec,
                                                        UInt&      uiFrameNumPartOffset )
{
  AOF_DBG( m_bInit );

  //----- set frame Spec -----
  ANOK( m_pacFrameDescriptor[ m_uiCurrentFrame ].setFrameSpec( rcFrameSpec, uiFrameNumPartOffset ) );

  //----- update parameters -----
  Bool  bPartFinished = false;
  if( ++m_uiCurrentFrame == m_uiNumberOfFrames )
  {
    m_uiCurrentFrame      = 0;
    uiFrameNumPartOffset += m_uiNumberOfFrames;

    if( ++m_uiCurrentRepetition == m_uiNumberOfRepetitions )
    {
      m_uiCurrentRepetition = 0;
      bPartFinished         = true;
    }
  }

  return bPartFinished;
}
//================================= Frame Sequence Part : end ===================================





//================================= General Sequence Part : begin ===================================
SequenceStructure::GeneralSequencePart::GeneralSequencePart()
: m_bInit               ( false )
, m_papcSequencePart    ( 0     )
{
}

SequenceStructure::GeneralSequencePart::~GeneralSequencePart()
{
  uninit();
}


Void
SequenceStructure::GeneralSequencePart::uninit()
{
  if( m_papcSequencePart )
  {
    for( UInt uiIndex = 0; uiIndex < m_uiNumberOfParts; uiIndex++ )
    {
      delete m_papcSequencePart[uiIndex];
    }
    delete[] m_papcSequencePart;
  }

  m_papcSequencePart  = 0;
  m_bInit             = false;
}


ErrVal
SequenceStructure::GeneralSequencePart::init( const String& rcString )
{
  uninit();

  //----- get number of repetitions and number of frames -----
  String cNoRepString;
  RNOKS( FormattedStringParser::extractRepetitions ( rcString, cNoRepString, m_uiNumberOfRepetitions ) );
  RNOKS( FormattedStringParser::getNumberOfParts   (           cNoRepString, m_uiNumberOfParts       ) );
  ROFS ( m_uiNumberOfRepetitions );
  ROFS ( m_uiNumberOfParts       );

  //----- create array -----
  ROFS( ( m_papcSequencePart = new SequencePart* [ m_uiNumberOfParts ] ) );

  //----- initialize array -----
  UInt uiPos, uiIndex;
  for( uiPos = 0, uiIndex = 0; uiIndex < m_uiNumberOfParts; uiIndex++ )
  {
    String cPartString;
    RNOKS( FormattedStringParser::extractPart( cNoRepString, cPartString, uiPos ) );

    if( FormattedStringParser::isFrameSequencePart( cPartString ) )
    {
      ROFS( ( m_papcSequencePart[uiIndex] = new FrameSequencePart  () ) );
    }
    else
    {
      ROFS( ( m_papcSequencePart[uiIndex] = new GeneralSequencePart() ) );
    }

    m_papcSequencePart[uiIndex]->init( cPartString );
  }

  //----- reset -----
  m_uiCurrentRepetition = 0;
  m_uiCurrentPart       = 0;

  //----- check required DPB buffer sizes -----
  m_uiMinDPBSizeRef     = 0;
  m_uiMinDPBSizeNonRef  = 0;
  for( uiIndex = 0; uiIndex < m_uiNumberOfParts; uiIndex++ )
  {
    m_uiMinDPBSizeRef    = gMax( m_uiMinDPBSizeRef,    m_papcSequencePart[uiIndex]->getMinDPBSizeRef   () );
    m_uiMinDPBSizeNonRef = gMax( m_uiMinDPBSizeNonRef, m_papcSequencePart[uiIndex]->getMinDPBSizeNonRef() );
  }

  //----- set inititalization flag -----
  m_bInit = true;

  return Err::m_nOK;
}


Void
SequenceStructure::GeneralSequencePart::reset()
{
  AOF_DBG( m_bInit );

  for( UInt uiIndex = 0; uiIndex < m_uiNumberOfParts; uiIndex++ )
  {
    m_papcSequencePart[uiIndex]->reset();
  }

  m_uiCurrentPart       = 0;
  m_uiCurrentRepetition = 0;
}


ErrVal
SequenceStructure::GeneralSequencePart::check()
{
  ROFS( m_bInit );
  ROFS( m_uiNumberOfRepetitions );
  ROFS( m_uiNumberOfParts );

  for( UInt uiIndex = 0; uiIndex < m_uiNumberOfParts; uiIndex++ )
  {
    RNOKS( m_papcSequencePart[uiIndex]->check() );
  }

  return Err::m_nOK;
}


Bool
SequenceStructure::GeneralSequencePart::isFirstIDR()  const
{
  AOF_DBG( m_bInit );
  AOF_DBG( m_uiNumberOfParts );
  return m_papcSequencePart[0]->isFirstIDR();
}

UInt
SequenceStructure::GeneralSequencePart::getMinDPBSizeRef()  const
{
  AOF_DBG(m_bInit);
  return m_uiMinDPBSizeRef;
}


UInt
SequenceStructure::GeneralSequencePart::getMinDPBSizeNonRef()  const
{
  AOF_DBG(m_bInit);
  return m_uiMinDPBSizeNonRef;
}


Bool
SequenceStructure::GeneralSequencePart::getNextFrameSpec( FrameSpec& rcFrameSpec,
                                                          UInt&      uiFrameNumPartOffset)
{
  AOF_DBG( m_bInit );

  //----- set frame Spec -----
  Bool  bPartFinished     = false;
  Bool  bCurrPartFinished = m_papcSequencePart[ m_uiCurrentPart ]->getNextFrameSpec( rcFrameSpec, uiFrameNumPartOffset );

  //----- update parameters -----
  if( bCurrPartFinished )
  {
    if( ++m_uiCurrentPart == m_uiNumberOfParts )
    {
      m_uiCurrentPart = 0;

      if( ++m_uiCurrentRepetition == m_uiNumberOfRepetitions )
      {
        m_uiCurrentRepetition = 0;
        bPartFinished         = true;
      }
    }
  }

  return bPartFinished;
}
//================================= General Sequence Part : end ===================================





//================================= Sequence Structure : begin ===================================
SequenceStructure::SequenceStructure()
: m_bInit             ( false )
, m_pcSequencePart    ( 0     )
{
}

SequenceStructure::~SequenceStructure()
{
  uninit();
}


ErrVal
SequenceStructure::create( SequenceStructure*& rpcSequenceStructure )
{
  rpcSequenceStructure = new SequenceStructure;
  ROT( rpcSequenceStructure == NULL );
  return Err::m_nOK;
}

ErrVal
SequenceStructure::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
SequenceStructure::uninit()
{
  m_cFrameSpec.uninit();

  delete m_pcSequencePart;
  m_pcSequencePart = 0;

  m_bInit = false;
  return Err::m_nOK;
}

ErrVal
SequenceStructure::init( const String& rcString,
                         UInt          uiNumberOfFrames )
{
  uninit();

  if( FormattedStringParser::isFrameSequencePart( rcString ) )
  {
    ROFS( ( m_pcSequencePart = new FrameSequencePart   () ) );
    RNOKS(   m_pcSequencePart->init( rcString ) );
  }
  else
  {
    ROFS( ( m_pcSequencePart = new GeneralSequencePart () ) );
    String cString = "*n{" + rcString + "}";   // to be on the safe side
    RNOKS(   m_pcSequencePart->init( cString ) );
  }

  //----- init parameters -----
  m_bInit                     = true;
  m_uiNumberOfTotalFrames     = uiNumberOfFrames;
  m_uiNumberOfFramesProcessed = 0;
  m_uiFrameNumPartOffset      = 0;
  m_uiNumIDR                  = 0;
  m_uiNumIntra                = 0;
  m_uiNumRef                  = 0;
  m_uiNumCoded                = 0;
  m_uiMaxAbsFrameDiffRef      = 0;
  m_uiMinDelay                = 0;
  m_uiMaxLayer                = 0;

  if( uiNumberOfFrames != MSYS_UINT_MAX )
  {
    xInitParameters();
  }

  return Err::m_nOK;
}

Void
SequenceStructure::reset()
{
  m_pcSequencePart->reset  ();
  m_cFrameSpec    . uninit ();

  m_uiNumberOfFramesProcessed = 0;
  m_uiFrameNumPartOffset      = 0;
}


ErrVal
SequenceStructure::check()
{
  ROFS  ( m_bInit );
  RNOKS ( m_pcSequencePart->check() );
  return Err::m_nOK;
}


const FrameSpec&
SequenceStructure::getFrameSpec()
{
  AOF_DBG( m_bInit );
  AOF_DBG( m_cFrameSpec.isInitialized() );
  return m_cFrameSpec;
}


const FrameSpec&
SequenceStructure::getNextFrameSpec()
{
  AOF_DBG( m_bInit );
  ANOK   ( xGetNextValidFrameSpec( m_cFrameSpec, m_uiNumberOfTotalFrames ) );
  return m_cFrameSpec;
}


Bool
SequenceStructure::checkString( const String&  rcString )
{
  SequenceStructure cSStruct;

  ROFRS( cSStruct.init        ( rcString, MSYS_UINT_MAX ) == Err::m_nOK, true );
  ROFRS( cSStruct.check       ()                          == Err::m_nOK, true );
  ROFRS( cSStruct.xIsFirstIDR (),                                        true );
  return false;
}


ErrVal
SequenceStructure::debugOutput( const String&  rcString,
                                UInt           uiNumberOfFrames,
                                FILE*          pcFile )
{
  SequenceStructure cSStruct;

  RNOKS( cSStruct.init  ( rcString, uiNumberOfFrames )  );
  RNOKS( cSStruct.check () );

  for( UInt uiIndex = 0; uiIndex < uiNumberOfFrames; uiIndex++ )
  {
    const FrameSpec&  cFSpec   = cSStruct.getNextFrameSpec();
    String            cTypeStr;
    String            cStoreStr;
    String            cKeyStr;
    String            cMmcoString;
    String            cRplrL0String;
    String            cRplrL1String;

    if( cFSpec.getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR )
    {
      cTypeStr = " IDR ";
    }
    else if( cFSpec.getSliceType() == I_SLICE )
    {
      cTypeStr = "   I ";
    }
    else if( cFSpec.getSliceType() == P_SLICE )
    {
      cTypeStr = "   P ";
    }
    else if( cFSpec.getSliceType() == B_SLICE )
    {
      cTypeStr = "   B ";
    }
    else
    {
     AF();
    }

    if( cFSpec.getNalRefIdc() == NAL_REF_IDC_PRIORITY_LOWEST )
    {
      cStoreStr = "        ";
    }
    else
    {
      cStoreStr = " stored ";
    }

    if( cFSpec.isBaseRep() )
    {
      cKeyStr   = " base ";
    }
    else
    {
      cKeyStr   = "     ";
    }

    if( cFSpec.getMmcoBuf() )
    {
      cMmcoString = " MMCO ";
    }
    else
    {
      cMmcoString = "      ";
    }

    if( cFSpec.getRplrBuf( LIST_0 ) )
    {
      cRplrL0String = " RPLR-L0 ";
    }
    else
    {
      cRplrL0String = "         ";
    }

    if( cFSpec.getRplrBuf( LIST_1 ) )
    {
      cRplrL1String = " RPLR-L1 ";
    }
    else
    {
      cRplrL1String = "         ";
    }

    fprintf( pcFile, "%5d%sL%d%s%s%s%s%s\n",
      cFSpec.getContFrameNumber(),
      cTypeStr.c_str(),
      cFSpec.getTemporalLayer(),
      cStoreStr.c_str(),
      cKeyStr.c_str(),
      cMmcoString.c_str(),
      cRplrL0String.c_str(),
      cRplrL1String.c_str() );
  }

  return Err::m_nOK;
}


UInt
SequenceStructure::getNumberOfTotalFrames()  const
{
  AOF_DBG(m_bInit);
  return m_uiNumberOfTotalFrames;
}

UInt
SequenceStructure::getNumberOfIDRFrames()  const
{
  AOF_DBG(m_bInit);
  return m_uiNumIDR;
}

UInt
SequenceStructure::getNumberOfIntraFrames()  const
{
  AOF_DBG(m_bInit);
  return m_uiNumIntra;
}

UInt
SequenceStructure::getNumberOfInterFrames()  const
{
  AOF_DBG(m_bInit);
  return m_uiNumCoded - m_uiNumIntra;
}

UInt
SequenceStructure::getNumberOfRefFrames()  const
{
  AOF_DBG( m_bInit );
  return m_uiNumRef;
}

UInt
SequenceStructure::getNumberOfCodedFrames()  const
{
  AOF_DBG( m_bInit );
  return m_uiNumCoded;
}

UInt
SequenceStructure::getMaxAbsFrameDiffRef()  const
{
  AOF_DBG( m_bInit );
  return m_uiMaxAbsFrameDiffRef;
}

UInt
SequenceStructure::getMinDelay()  const
{
  AOF_DBG( m_bInit );
  return m_uiMinDelay;
}

UInt
SequenceStructure::getNumTemporalLayers()  const
{
  AOF_DBG( m_bInit );
  return m_uiMaxLayer + 1;
}



Bool
SequenceStructure::xIsFirstIDR()
{
  AOF_DBG( m_bInit );
  return m_pcSequencePart->isFirstIDR();
}


Void
SequenceStructure::xInitParameters()
{
  AOF_DBG( m_bInit );

  m_uiNumIDR                  = 0;
  m_uiNumIntra                = 0;
  m_uiNumRef                  = 0;
  m_uiNumCoded                = 0;
  m_uiMaxAbsFrameDiffRef      = 0;
  m_uiMinDelay                = 0;
  m_uiMaxLayer                = 0;
  Int       iLastFrameNumRef  = 0;
  FrameSpec cFSpec;

  for( UInt uiIndex = 0; uiIndex < m_uiNumberOfTotalFrames; uiIndex++ )
  {
    ANOK( xGetNextValidFrameSpec( cFSpec, m_uiNumberOfTotalFrames ) );

    //----- add up frame counts -----
    if( ! cFSpec.isSkipped() )
    {
      m_uiNumCoded++;
    }
    if( cFSpec.getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR )
    {
      m_uiNumIDR++;
    }
    if( cFSpec.getSliceType() == I_SLICE )
    {
      m_uiNumIntra++;
    }
    if( cFSpec.getNalRefIdc() )
    {
      m_uiNumRef++;
    }

    //----- check Delay -----
    if( (Int)uiIndex - (Int)cFSpec.getContFrameNumber() > (Int)m_uiMinDelay )
    {
      m_uiMinDelay = uiIndex - cFSpec.getContFrameNumber ();
    }

    //----- check Layer -----
    if( cFSpec.getTemporalLayer() > m_uiMaxLayer )
    {
      m_uiMaxLayer = cFSpec.getTemporalLayer();
    }

    //----- check POC differences -----
    if( uiIndex == 0 )
    {
      AOT_DBG( cFSpec.getNalUnitType() != NAL_UNIT_CODED_SLICE_IDR );
      iLastFrameNumRef = cFSpec.getContFrameNumber();
    }
    else
    {
      if( ! cFSpec.isSkipped() )
      {
        Int   iFrameNumber      = cFSpec.getContFrameNumber();
        UInt  uiAbsFrameDiffRef = abs( iFrameNumber - iLastFrameNumRef );
        if( uiAbsFrameDiffRef > m_uiMaxAbsFrameDiffRef )
        {
          m_uiMaxAbsFrameDiffRef = uiAbsFrameDiffRef;
        }
        if( cFSpec.getNalRefIdc() )
        {
          iLastFrameNumRef = iFrameNumber;
        }
      }
    }
  }

  reset();
}


ErrVal
SequenceStructure::xGetNextValidFrameSpec( FrameSpec& rcFrameSpec,
                                           UInt       uiTotalFrames )
{
  rcFrameSpec.uninit();

  ROF( m_uiNumberOfFramesProcessed < uiTotalFrames );

  while( ! rcFrameSpec.isInitialized() )
  {
    m_pcSequencePart->getNextFrameSpec( rcFrameSpec, m_uiFrameNumPartOffset );

    if( rcFrameSpec.getContFrameNumber() >= uiTotalFrames )
    {
      rcFrameSpec.uninit();
    }
  }

  m_uiNumberOfFramesProcessed++;
  return Err::m_nOK;
}
//================================= Sequence Structure : end ===================================





//================================= FormattedStringParser : begin ===================================
const String FormattedStringParser::sm_cSetOfTypes      ("AIPBSipbs");
const String FormattedStringParser::sm_cSetOfDigits     ("0123456789");
const String FormattedStringParser::sm_cSetOfPartStart  ("AIPBSipbs*");


ErrVal
FormattedStringParser::separatString( const String& rcString,
                                      String&       rcFDString,
                                      String&       rcMmcoString,
                                      String&       rcRplrStringL0,
                                      String&       rcRplrStringL1 )
{
  UInt uiMPos   = (UInt)rcString.find_first_of( "M" );
  UInt uiR1Pos  = (UInt)rcString.find_first_of( "R" );
  UInt uiR2Pos  = (UInt)rcString.find_last_of ( "R" );
  UInt uiSize   = (UInt)rcString.size();

  if( UInt(String::npos) == uiMPos ) // MMCO commands are not present
  {
    rcMmcoString           = "";

    if( UInt(String::npos) == uiR1Pos )
    {
      rcFDString           = rcString;
      rcRplrStringL0       = "";
      rcRplrStringL1       = "";
    }
    else
    {
      rcFDString           = rcString.substr( 0,     uiR1Pos );

      if( uiR1Pos == uiR2Pos )
      {
        rcRplrStringL0     = rcString.substr( uiR1Pos, uiSize - uiR1Pos );
        rcRplrStringL1     = "";
      }
      else
      {
        rcRplrStringL0     = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
        rcRplrStringL1     = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );
      }
    }
  }
  else
  {
    if( UInt(String::npos) == uiR1Pos )
    {
      rcFDString           = rcString.substr( 0,       uiMPos );
      rcMmcoString         = rcString.substr( uiMPos,  uiSize - uiMPos );
      rcRplrStringL0       = "";
      rcRplrStringL1       = "";
    }
    else
    {
      if( uiMPos < uiR1Pos )
      {
        rcFDString         = rcString.substr( 0,       uiMPos );
        rcMmcoString       = rcString.substr( uiMPos,  uiR1Pos - uiMPos );

        if( uiR1Pos == uiR2Pos )
        {
          rcRplrStringL0   = rcString.substr( uiR1Pos, uiSize - uiR1Pos );
          rcRplrStringL1   = "";
        }
        else
        {
          rcRplrStringL0   = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
          rcRplrStringL1   = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );
        }
      }
      else
      {
        rcFDString         = rcString.substr( 0,      uiR1Pos );

        if( uiR1Pos == uiR2Pos )
        {
          rcRplrStringL0   = rcString.substr( uiR1Pos, uiMPos - uiR1Pos );
          rcRplrStringL1   = "";
          rcMmcoString     = rcString.substr( uiMPos,  uiSize - uiMPos );
        }
        else
        {
          if( uiMPos < uiR2Pos )
          {
            rcRplrStringL0 = rcString.substr( uiR1Pos, uiMPos  - uiR1Pos );
            rcMmcoString   = rcString.substr( uiMPos, uiR2Pos - uiMPos );
            rcRplrStringL1 = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );

          }
          else
          {
            rcRplrStringL0 = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
            rcRplrStringL1 = rcString.substr( uiR2Pos, uiMPos  - uiR2Pos );
            rcMmcoString   = rcString.substr( uiMPos,  uiSize  - uiMPos );
          }
        }
      }
    }
  }

  if( rcRplrStringL0.size() == 1 )
  {
    rcRplrStringL0 = "";
  }
  if( rcRplrStringL1.size() == 1 )
  {
    rcRplrStringL1 = "";
  }

  return Err::m_nOK;
}



ErrVal
FormattedStringParser::extractFrameDescription( const String&  rcString,
                                                UChar&         rucType,
                                                UInt&          ruiIncrement,
                                                Bool&          rbUseBaseRep,
                                                UInt&          ruiLayer)
{
  UInt    uiKeyPos   = (UInt)rcString.find_first_of( "K" );
  UInt    uiLayerPos = (UInt)rcString.find_first_of( "L" );
  String  cKeyString;
  String  cLayerString;
  String  cFrameString;

  //===== separate strings =====
  if( uiLayerPos == UInt(String::npos) )
  {
    if( uiKeyPos == UInt(String::npos) )
    {
      cFrameString  = rcString;
      cKeyString    = "";
      cLayerString  = "";
    }
    else
    {
      cFrameString  = rcString.substr( 0,           uiKeyPos );
      cKeyString    = rcString.substr( uiKeyPos,    rcString.size() - uiKeyPos );
      cLayerString  = "";
    }
  }
  else if( uiKeyPos == UInt(String::npos) )
  {
    cFrameString    = rcString.substr( 0,           uiLayerPos );
    cLayerString    = rcString.substr( uiLayerPos,  rcString.size() - uiLayerPos );
    cKeyString      = "";
  }
  else if( uiKeyPos < uiLayerPos )
  {
    cFrameString    = rcString.substr( 0,           uiKeyPos );
    cKeyString      = rcString.substr( uiKeyPos,    uiLayerPos      - uiKeyPos );
    cLayerString    = rcString.substr( uiLayerPos,  rcString.size() - uiLayerPos );
  }
  else
  {
    cFrameString    = rcString.substr( 0,           uiLayerPos );
    cLayerString    = rcString.substr( uiLayerPos,  uiKeyPos        - uiLayerPos );
    cKeyString      = rcString.substr( uiKeyPos,    rcString.size() - uiKeyPos );
  }

  //===== check frame string =====
  ROFS  ( cFrameString.find_first_of   ( sm_cSetOfTypes   ) == 0 );   // first character must be a type
  ROFS  ( cFrameString.find_first_of   ( sm_cSetOfDigits  ) == 1 );   // second character must be a digit
  ROFS  ( cFrameString.find_last_not_of( sm_cSetOfDigits  ) == 0 );   // all other characters must be digits

  //===== check layer string =====
  if( ! cLayerString.empty() )
  {
    ROFS( cLayerString.find_first_of   ( sm_cSetOfDigits  ) == 1 );   // second character must be a digit
    ROFS( cLayerString.find_last_not_of( sm_cSetOfDigits  ) == 0 );   // all other characters must be digits
  }

  //===== check key string =====
  if( ! cKeyString.empty() )
  {
    ROFS( cKeyString.size() == 1 ); // single character string
  }

  //===== assign parameters =====
  rucType       = cFrameString[0];
  ruiIncrement  = atoi( cFrameString.c_str() + 1 );
  rbUseBaseRep  = ( ! cKeyString.empty() );
  ruiLayer      = ( cLayerString.empty() ? 0 : atoi( cLayerString.c_str() + 1 ) );

  return Err::m_nOK;
}


ErrVal
FormattedStringParser::extractRplr( const String& rcString,
                                    RefPicListReOrdering&   rcRplrBuf)
{
  //--- check if string is correct ---
  ROFS( rcString.find_first_of( "R" ) == 0 );

  UInt uiSize = (UInt)rcString.size();
  UInt uiNext = 0;

  for( UInt nBuf = 0, nPos = 1; nPos < uiSize; nPos = uiNext, nBuf++ )
  {
    uiNext         = gMin( (UInt)rcString.find_first_of ( "L+-", nPos  +1    ), uiSize);
    String cString =            rcString.substr        ( nPos,  uiNext-nPos );

    RNOK( extractSingleRplrCommand( cString, rcRplrBuf.get( nBuf ) ) );
  }

  return Err::m_nOK;
}


ErrVal
FormattedStringParser::extractMmco( const String& rcString,
                                    DecRefPicMarking&   rcMmcoBuf )
{
  //--- check if string is correct ---
  ROFS( rcString.find_first_of   ( "M" ) == 0 );

  UInt uiSize = (UInt)rcString.size();
  UInt uiNext = 0;

  for( UInt nBuf = 0, nPos = 1; nPos < uiSize; nPos = uiNext, nBuf++ )
  {
    uiNext          = gMin( (UInt) rcString.find_first_of( "LNE",  nPos  +1    ), uiSize);
    String cString  =             rcString.substr       ( nPos,   uiNext-nPos );

    RNOK( extractSingleMmcoCommand( cString, rcMmcoBuf.get( nBuf ) ) );
  }

  return Err::m_nOK;
}


ErrVal
FormattedStringParser::extractSingleRplrCommand( const String&  rcString,
                                                 RplrCommand&          rcRplr )
{
  //--- check if string is correct ---
  ROFS( rcString.find_first_of   ( "L+-" ) == 0 );
  Char cCommand = rcString[0];

  ROFS( 1 == rcString.find_first_of   ( sm_cSetOfDigits  ) );
  ROFS( 0 == rcString.find_last_not_of( sm_cSetOfDigits  ) );
  UInt uiNum  = atoi( rcString.c_str() + 1 );

  if( cCommand == 'L' )
  {
    rcRplr = RplrCommand( RPLR_LONG, uiNum );
    return Err::m_nOK;
  }
  else if( cCommand == '+' )
  {
    rcRplr = RplrCommand( RPLR_POS, uiNum );
    return Err::m_nOK;
  }
  else if( cCommand == '-' )
  {
    rcRplr = RplrCommand( RPLR_NEG, uiNum );
    return Err::m_nOK;
  }

  AF();
  return Err::m_nERR;
}


ErrVal
FormattedStringParser::extractSingleMmcoCommand( const String&  rcString,
                                                MmcoCommand&          rcMmco )
{
  //--- check if string is correct ---
  ROFS( rcString.find_first_of( "LNE" ) == 0 );

  UInt uiSize     = (UInt)rcString.size();
  Char cCommand1  = rcString[0];

  if( cCommand1 == 'L' )
  {
    UInt  uiEnd = (UInt)rcString.find_first_of( "+-$:" );
    ROTS( uiEnd == UInt(String::npos) );
    String cString = rcString.substr( 1, uiEnd-1 );

    ROTS( 0            != cString.find_first_of   ( sm_cSetOfDigits  ) );
    ROTS( UInt(String::npos) != cString.find_last_not_of( sm_cSetOfDigits  ) );
    UInt uiLtId  = atoi( cString.c_str() );

    Char cCommand2 = rcString[uiEnd];
    if( cCommand2 == '+' )
    {
      rcMmco = MmcoCommand( MMCO_SET_LONG_TERM, uiLtId );
      return Err::m_nOK;
    }
    else if( cCommand2 == '-' )
    {
      rcMmco = MmcoCommand( MMCO_LONG_TERM_UNUSED, uiLtId );
      return Err::m_nOK;
    }
    else if( cCommand2 == '$' )
    {
      rcMmco = MmcoCommand( MMCO_MAX_LONG_TERM_IDX, uiLtId );
      return Err::m_nOK;
    }
    else if( cCommand2 == ':' )
    {
      String cString2 = rcString.substr( uiEnd+1, uiSize );
      ROTS( 0            != cString2.find_first_of   ( sm_cSetOfDigits  ) );
      ROTS( UInt(String::npos) != cString2.find_last_not_of( sm_cSetOfDigits  ) );

      UInt uiStId  = atoi( cString2.c_str() );
      rcMmco = MmcoCommand( MMCO_ASSIGN_LONG_TERM, uiLtId, uiStId );
      return Err::m_nOK;
    }
    else
    {
      AF();
      return Err::m_nERR;
    }
  }
  else if( cCommand1 == 'N' )
  {
    ROFS( 1 == rcString.find_first_of   ( sm_cSetOfDigits  ) );
    ROFS( 0 == rcString.find_last_not_of( sm_cSetOfDigits  ) );
    UInt uiNum  = atoi( rcString.c_str() + 1 );
    rcMmco = MmcoCommand( MMCO_SHORT_TERM_UNUSED, uiNum );
    return Err::m_nOK;
  }
  else if( cCommand1 == 'E' )
  {
    rcMmco = MmcoCommand( MMCO_RESET );
    return Err::m_nOK;
  }

  AF();
  return Err::m_nERR;
}


Bool
FormattedStringParser::isFrameSequencePart( const String& rcString )
{
  return ( rcString.find( '*', 1 ) == String::npos );
}


ErrVal
FormattedStringParser::extractRepetitions( const String&  rcString,
                                           String&        rcNoRepString,
                                           UInt&          ruiNumberOfRepetitions )
{
  if( rcString[0] != '*' )
  {
    ruiNumberOfRepetitions  = 1;
    rcNoRepString           = rcString;
  }
  else
  {
    UInt  uiLastPos   = (UInt)rcString.length () - 1;
    UInt  uiOpenPos   = (UInt)rcString.find   ( '{' );
    UInt  uiClosePos  = (UInt)rcString.rfind  ( '}' );

    ROTS(  uiOpenPos   == UInt(String::npos)  );
    ROFS(  uiClosePos  == uiLastPos          );

    rcNoRepString     = rcString.substr( uiOpenPos+1, uiClosePos-uiOpenPos-1 );
    String cNStr      = rcString.substr( 1,           uiOpenPos - 1          );

    if( uiOpenPos==2 && cNStr[0]=='n' )
    {
      ruiNumberOfRepetitions  = MSYS_UINT_MAX;
    }
    else
    {
      ROFS( cNStr.find_first_not_of( sm_cSetOfDigits ) == String::npos );
      ruiNumberOfRepetitions  = atoi( cNStr.c_str() );
    }
  }

  return Err::m_nOK;
}



ErrVal
FormattedStringParser::getNumberOfFrames( const String&  rcString,
                                          UInt&          ruiNumberOfFrames )
{
  UInt  uiPos       = (UInt)rcString.find_first_of( sm_cSetOfTypes );
  ruiNumberOfFrames = 0;

  while( uiPos != UInt(String::npos) )
  {
    ruiNumberOfFrames++;
    uiPos = (UInt)rcString.find_first_of( sm_cSetOfTypes, uiPos+1 );
  }

  return Err::m_nOK;
}



ErrVal
FormattedStringParser::extractNextFrameDescription( const String&  rcString,
                                                    String&        rcFDString,
                                                    UInt&          ruiStartPos )
{
  ROTS( ruiStartPos >= rcString.length() - 1 );

  UInt uiEndPos = (UInt)rcString.find_first_of( sm_cSetOfTypes, ruiStartPos + 1 );
  rcFDString    = rcString.substr       ( ruiStartPos, uiEndPos - ruiStartPos );
  ruiStartPos   = uiEndPos;

  return Err::m_nOK;
}



ErrVal
FormattedStringParser::getNumberOfParts( const String& rcString,
                                         UInt&         ruiNumberOfParts )
{
  UInt  uiPos       = (UInt)rcString.find_first_of( sm_cSetOfPartStart );
  ruiNumberOfParts  = 0;

  while( uiPos != UInt(String::npos) )
  {
    ruiNumberOfParts++;

    if( rcString[uiPos] == '*' )
    {
      UInt  uiEndPos        = (UInt)rcString.find( '{', uiPos+1 );
      UInt  uiOpenBrackets  = 1;
      ROTS( uiEndPos == UInt(String::npos) );

      while( uiOpenBrackets )
      {
        uiEndPos  = (UInt)rcString.find_first_of( "{}", uiEndPos+1 );
        ROTS( uiEndPos == UInt(String::npos) );

        if( rcString[uiEndPos] == '{' )   uiOpenBrackets++;
        else                              uiOpenBrackets--;
      }

      uiPos = (UInt)rcString.find_first_of( sm_cSetOfPartStart, uiEndPos + 1 );
    }
    else
    {
      uiPos = (UInt)rcString.find( '*', uiPos+1 );
    }
  }

  return Err::m_nOK;
}



ErrVal
FormattedStringParser::extractPart( const String&  rcString,
                                    String&        rcPartString,
                                    UInt&          ruiStartPos )
{
  ROTS( ruiStartPos >= rcString.length() - 1 );

  UInt uiNextStartPos;

  if( rcString[ruiStartPos] == '*' )
  {
    UInt  uiEndPos        = (UInt)rcString.find( '{', ruiStartPos+1 );
    UInt  uiOpenBrackets  = 1;
    ROTS( uiEndPos == UInt(String::npos) );

    while( uiOpenBrackets )
    {
      uiEndPos  = (UInt)rcString.find_first_of( "{}", uiEndPos+1 );
      ROTS( uiEndPos == UInt(String::npos) );

      if( rcString[uiEndPos] == '{' )   uiOpenBrackets++;
      else                              uiOpenBrackets--;
    }

    uiNextStartPos = (UInt)rcString.find_first_of( sm_cSetOfPartStart, uiEndPos + 1 );
  }
  else
  {
    uiNextStartPos = (UInt)rcString.find( '*', ruiStartPos+1 );
  }

  rcPartString  = rcString.substr( ruiStartPos, uiNextStartPos - ruiStartPos );
  ruiStartPos   = uiNextStartPos;

  return Err::m_nOK;
}
//================================= FormattedStringParser : end ===================================



H264AVC_NAMESPACE_END
