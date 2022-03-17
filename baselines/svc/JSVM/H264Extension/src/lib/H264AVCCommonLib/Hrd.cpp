
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/Hrd.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// h264 namespace begin
H264AVC_NAMESPACE_BEGIN

ErrVal HRD::Cnt::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getUvlc( m_uiBitRateValueMinus1, "HRD: bit_rate_value_minus1" ) );
  RNOKS( pcReadIf->getUvlc( m_uiCpbSizeValueMinus1, "HRD: cpb_size_value_minus1" ) );
  RNOKS( pcReadIf->getFlag( m_bVbrCbrFlag,    "HRD: vbr_cbr_flag" ) );
  return Err::m_nOK;
}


ErrVal HRD::Cnt::write( HeaderSymbolWriteIf *pcWriteIf ) const
{
  RNOK( pcWriteIf->writeUvlc( m_uiBitRateValueMinus1, "HRD: bit_rate_value_minus1"));
  RNOK( pcWriteIf->writeUvlc( m_uiCpbSizeValueMinus1, "HRD: cpb_size_value_minus1"));
  RNOK( pcWriteIf->writeFlag( m_bVbrCbrFlag,    "HRD: vbr_cbr_flag"));
  return Err::m_nOK;
}


HRD::HRD():
  m_bHrdParametersPresentFlag       ( false ),
  m_uiCpbCnt                        ( 0 ),
  m_uiBitRateScale                  ( 0 ),
  m_uiCpbSizeScale                  ( 0 ),
  m_uiInitialCpbRemovalDelayLength  ( 0 ),
  m_uiCpbRemovalDelayLength         ( 0 ),
  m_uiDpbOutputDelayLength          ( 0 ),
  m_uiTimeOffsetLength              ( 0 )
{
}


HRD::~HRD()
{
  m_cCntBuf.uninit();
}


ErrVal HRD::init( UInt uiCpbCnt )
{
  m_cCntBuf.init(uiCpbCnt);
  return Err::m_nOK;
}

ErrVal HRD::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bHrdParametersPresentFlag,              "HRD: hdr_parameters_present_flag"));
  ROFRS( m_bHrdParametersPresentFlag, Err::m_nOK );

  UInt uiTmp = 0;


  RNOKS( pcReadIf->getUvlc( uiTmp,                                    "HRD: cpb_cnt_minus1"));
  uiTmp++;
  ROTRS( uiTmp > 32, Err::m_nInvalidParameter );
  setCpbCnt(uiTmp);


  RNOKS( pcReadIf->getCode( m_uiBitRateScale, 4,                       "HRD: bit_rate_scale"));
  RNOKS( pcReadIf->getCode( m_uiCpbSizeScale, 4,                       "HRD: cpb_size_scale"));

  RNOK( m_cCntBuf.uninit() );
  RNOK( m_cCntBuf.init( m_uiCpbCnt ) );

  for( UInt i = 0; i < m_uiCpbCnt; i++)
  {
    RNOKS( m_cCntBuf.get( i ).read( pcReadIf ) );
  }

  RNOKS( pcReadIf->getCode( uiTmp, 5,                                  "HRD: initial_cpb_removal_delay_length_minus1"));
  setInitialCpbRemovalDelayLength(uiTmp+1);

  RNOKS( pcReadIf->getCode( uiTmp, 5,                                  "HRD: cpb_removal_delay_length_minus1"));
  setCpbRemovalDelayLength(uiTmp+1);

  RNOKS( pcReadIf->getCode( uiTmp, 5,                                  "HRD: dpb_output_delay_length_minus1"));
  setDpbOutputDelayLength(uiTmp+1);

  RNOKS( pcReadIf->getCode( uiTmp, 5,                                  "HRD: time_offset_length"));
  setTimeOffsetLength(uiTmp);

  return Err::m_nOK;
}


ErrVal HRD::write( HeaderSymbolWriteIf* pcWriteIf) const
{
  RNOK( pcWriteIf->writeFlag( m_bHrdParametersPresentFlag,            "HRD: hdr_parameters_present_flag"));
  ROFRS( m_bHrdParametersPresentFlag, Err::m_nOK );

  const UInt uiCpbCnt = getCpbCnt();

  RNOK( pcWriteIf->writeUvlc( uiCpbCnt-1,                             "HRD: cpb_cnt_minus1"));
  RNOK( pcWriteIf->writeCode( getBitRateScale(), 4,                   "HRD: bit_rate_scale"));
  RNOK( pcWriteIf->writeCode( getCpbSizeScale(), 4,                   "HRD: cpb_size_scale"));

  for( UInt i=0; i < uiCpbCnt; i++ )
  {
    RNOK( m_cCntBuf.get( i ).write( pcWriteIf ) );
  }

  RNOK( pcWriteIf->writeCode( getInitialCpbRemovalDelayLength()-1, 5, "HRD: initial_cpb_removal_delay_length_minus1"));
  RNOK( pcWriteIf->writeCode( getCpbRemovalDelayLength()-1, 5,        "HRD: cpb_removal_delay_length_minus1"));
  RNOK( pcWriteIf->writeCode( getDpbOutputDelayLength()-1, 5,         "HRD: dpb_output_delay_length_minus1"));
  RNOK( pcWriteIf->writeCode( getTimeOffsetLength(), 5,               "HRD: time_offset_length"));
  return Err::m_nOK;
}

// h264 namespace end
H264AVC_NAMESPACE_END
