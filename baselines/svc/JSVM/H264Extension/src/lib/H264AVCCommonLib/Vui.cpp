
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/Vui.h"
#include "H264AVCCommonLib/SequenceParameterSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// h264 namespace begin
H264AVC_NAMESPACE_BEGIN


VUI::BitstreamRestriction::BitstreamRestriction( SequenceParameterSet* pcSPS ):
  m_bBitstreamRestrictionFlag           ( false ),
  m_bMotionVectorsOverPicBoundariesFlag ( true ),
  m_uiMaxBytesPerPicDenom               ( 0 ),
  m_uiMaxBitsPerMbDenom                 ( 1 ),
  m_uiLog2MaxMvLengthHorizontal         ( 16 ),
  m_uiLog2MaxMvLengthVertical           ( 16 ),
  m_uiMaxDecFrameReordering             ( pcSPS->getMaxDPBSize() ),
  m_uiMaxDecFrameBuffering              ( pcSPS->getMaxDPBSize() )
{
}


ErrVal VUI::BitstreamRestriction::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bBitstreamRestrictionFlag,              "VUI: bitstream_restriction_flag"));
  ROFRS( m_bBitstreamRestrictionFlag, Err::m_nOK );

  RNOK( pcWriteIf->writeFlag( getMotionVectorsOverPicBoundariesFlag(),  "VUI: motion_vectors_over_pic_boundaries_flag"));
  RNOK( pcWriteIf->writeUvlc( getMaxBytesPerPicDenom(),                 "VUI: max_bytes_per_pic_denom"));
  RNOK( pcWriteIf->writeUvlc( getMaxBitsPerMbDenom(),                   "VUI: max_bits_per_mb_denom"));
  RNOK( pcWriteIf->writeUvlc( getLog2MaxMvLengthHorizontal(),           "VUI: log2_max_mv_length_horizontal"));
  RNOK( pcWriteIf->writeUvlc( getLog2MaxMvLengthVertical(),             "VUI: log2_max_mv_length_vertical"));
  RNOK( pcWriteIf->writeUvlc( getMaxDecFrameReordering(),               "VUI: max_dec_frame_reordering"));
  RNOK( pcWriteIf->writeUvlc( getMaxDecFrameBuffering(),                "VUI: max_dec_frame_buffering"));
  return Err::m_nOK;
}


ErrVal VUI::BitstreamRestriction::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bBitstreamRestrictionFlag,           "VUI: bitstream_restriction_flag"));
  ROFRS( m_bBitstreamRestrictionFlag, Err::m_nOK );

  RNOKS( pcReadIf->getFlag( m_bMotionVectorsOverPicBoundariesFlag, "VUI: motion_vectors_over_pic_boundaries_flag"));

  RNOKS( pcReadIf->getUvlc( m_uiMaxBytesPerPicDenom,               "VUI: max_bytes_per_pic_denom"));
  ROTRS( m_uiMaxBytesPerPicDenom > 16, Err::m_nInvalidParameter );

  RNOKS( pcReadIf->getUvlc( m_uiMaxBitsPerMbDenom,                 "VUI: max_bits_per_mb_denom"));
  ROTRS( m_uiMaxBitsPerMbDenom > 16, Err::m_nInvalidParameter );

  RNOKS( pcReadIf->getUvlc( m_uiLog2MaxMvLengthHorizontal,         "VUI: log2_max_mv_length_horizontal"));
  ROTRS( m_uiLog2MaxMvLengthHorizontal > 16, Err::m_nInvalidParameter );

  RNOKS( pcReadIf->getUvlc( m_uiLog2MaxMvLengthVertical,           "VUI: log2_max_mv_length_vertical"));
  ROTRS( m_uiLog2MaxMvLengthVertical > 16, Err::m_nInvalidParameter );

  RNOKS( pcReadIf->getUvlc( m_uiMaxDecFrameReordering,             "VUI: max_dec_frame_reordering"));

  UInt uiTmp;
  RNOKS( pcReadIf->getUvlc( uiTmp,                                 "VUI: max_dec_frame_buffering"));
  ROTRS(uiTmp>16, Err::m_nInvalidParameter);
  ROTRS(getMaxDecFrameReordering() > uiTmp, Err::m_nInvalidParameter);
  setMaxDecFrameBuffering(uiTmp);

  return Err::m_nOK;
}

Bool
VUI::BitstreamRestriction::isSame( const VUI::BitstreamRestriction& rcBitstreamRestriction )  const
{
  ROFRS( m_bBitstreamRestrictionFlag            ==  rcBitstreamRestriction.m_bBitstreamRestrictionFlag,           false );
  ROFRS( m_bBitstreamRestrictionFlag,                                                                             true  );
  ROFRS( m_bMotionVectorsOverPicBoundariesFlag  ==  rcBitstreamRestriction.m_bMotionVectorsOverPicBoundariesFlag, false );
  ROFRS( m_uiMaxBytesPerPicDenom                ==  rcBitstreamRestriction.m_uiMaxBytesPerPicDenom,               false );
  ROFRS( m_uiMaxBitsPerMbDenom                  ==  rcBitstreamRestriction.m_uiMaxBitsPerMbDenom,                 false );
  ROFRS( m_uiLog2MaxMvLengthHorizontal          ==  rcBitstreamRestriction.m_uiLog2MaxMvLengthHorizontal,         false );
  ROFRS( m_uiLog2MaxMvLengthVertical            ==  rcBitstreamRestriction.m_uiLog2MaxMvLengthVertical,           false );
  ROFRS( m_uiMaxDecFrameReordering              ==  rcBitstreamRestriction.m_uiMaxDecFrameReordering,             false );
  ROFRS( m_uiMaxDecFrameBuffering               ==  rcBitstreamRestriction.m_uiMaxDecFrameBuffering,              false );
  return true;
}

ErrVal
VUI::BitstreamRestriction::copy( const VUI::BitstreamRestriction& rcBitstreamRestriction )
{
  m_bBitstreamRestrictionFlag            = rcBitstreamRestriction.m_bBitstreamRestrictionFlag;
  m_bMotionVectorsOverPicBoundariesFlag  = rcBitstreamRestriction.m_bMotionVectorsOverPicBoundariesFlag;
  m_uiMaxBytesPerPicDenom                = rcBitstreamRestriction.m_uiMaxBytesPerPicDenom;
  m_uiMaxBitsPerMbDenom                  = rcBitstreamRestriction.m_uiMaxBitsPerMbDenom;
  m_uiLog2MaxMvLengthHorizontal          = rcBitstreamRestriction.m_uiLog2MaxMvLengthHorizontal;
  m_uiLog2MaxMvLengthVertical            = rcBitstreamRestriction.m_uiLog2MaxMvLengthVertical;
  m_uiMaxDecFrameReordering              = rcBitstreamRestriction.m_uiMaxDecFrameReordering;
  m_uiMaxDecFrameBuffering               = rcBitstreamRestriction.m_uiMaxDecFrameBuffering;
  return Err::m_nOK;
}


VUI::AspectRatioInfo::AspectRatioInfo():
  m_bAspectRatioInfoPresentFlag   ( false ),
  m_uiAspectRatioIdc              ( 0 ),
  m_uiSarWith                     ( 0 ),
  m_uiSarHeight                   ( 0 )
{
}

Bool
VUI::AspectRatioInfo::isSame( const VUI::AspectRatioInfo& rcAspectRatioInfo )  const
{
  ROFRS( m_bAspectRatioInfoPresentFlag  ==  rcAspectRatioInfo.m_bAspectRatioInfoPresentFlag,  false );
  ROFRS( m_bAspectRatioInfoPresentFlag,                                                       true  );
  ROFRS( m_uiAspectRatioIdc             ==  rcAspectRatioInfo.m_uiAspectRatioIdc,             false );
  ROFRS( m_uiAspectRatioIdc             ==  255,                                              true  );
  ROFRS( m_uiSarWith                    ==  rcAspectRatioInfo.m_uiSarWith,                    false );
  ROFRS( m_uiSarHeight                  ==  rcAspectRatioInfo.m_uiSarHeight,                  false );
  return true;
}

ErrVal
VUI::AspectRatioInfo::copy( const VUI::AspectRatioInfo& rcAspectRatioInfo )
{
  m_bAspectRatioInfoPresentFlag  = rcAspectRatioInfo.m_bAspectRatioInfoPresentFlag;
  m_uiAspectRatioIdc             = rcAspectRatioInfo.m_uiAspectRatioIdc;
  m_uiSarWith                    = rcAspectRatioInfo.m_uiSarWith;
  m_uiSarHeight                  = rcAspectRatioInfo.m_uiSarHeight;
  return Err::m_nOK;
}


VUI::VideoSignalType::VideoSignalType():
  m_bVideoSignalTypePresentFlag   ( false ),
  m_uiVideoFormat                 ( 5 ),
  m_bVideoFullRangeFlag           ( false ),
  m_bColourDescriptionPresentFlag ( false ),
  m_uiColourPrimaries             ( 2 ),
  m_uiTransferCharacteristics     ( 2 ),
  m_uiMatrixCoefficients          ( 2 )
{
}

Bool
VUI::VideoSignalType::isSame( const VUI::VideoSignalType& rcVideoSignalType )  const
{
  ROFRS( m_bVideoSignalTypePresentFlag    ==  rcVideoSignalType.m_bVideoSignalTypePresentFlag,    false );
  ROFRS( m_bVideoSignalTypePresentFlag,                                                           true  );
  ROFRS( m_uiVideoFormat                  ==  rcVideoSignalType.m_uiVideoFormat,                  false );
  ROFRS( m_bVideoFullRangeFlag            ==  rcVideoSignalType.m_bVideoFullRangeFlag,            false );
  ROFRS( m_bColourDescriptionPresentFlag  ==  rcVideoSignalType.m_bColourDescriptionPresentFlag,  false );
  ROFRS( m_bColourDescriptionPresentFlag,                                                         true  );
  ROFRS( m_uiColourPrimaries              ==  rcVideoSignalType.m_uiColourPrimaries,              false );
  ROFRS( m_uiTransferCharacteristics      ==  rcVideoSignalType.m_uiTransferCharacteristics,      false );
  ROFRS( m_uiMatrixCoefficients           ==  rcVideoSignalType.m_uiMatrixCoefficients,           false );
  return true;
}

ErrVal
VUI::VideoSignalType::copy( const VUI::VideoSignalType& rcVideoSignalType )
{
  m_bVideoSignalTypePresentFlag    = rcVideoSignalType.m_bVideoSignalTypePresentFlag;
  m_uiVideoFormat                  = rcVideoSignalType.m_uiVideoFormat;
  m_bVideoFullRangeFlag            = rcVideoSignalType.m_bVideoFullRangeFlag;
  m_bColourDescriptionPresentFlag  = rcVideoSignalType.m_bColourDescriptionPresentFlag;
  m_uiColourPrimaries              = rcVideoSignalType.m_uiColourPrimaries;
  m_uiTransferCharacteristics      = rcVideoSignalType.m_uiTransferCharacteristics;
  m_uiMatrixCoefficients           = rcVideoSignalType.m_uiMatrixCoefficients;
  return Err::m_nOK;
}


VUI::ChromaLocationInfo::ChromaLocationInfo():
  m_bChromaLocationInfoPresentFlag( false ),
  m_uiChromaLocationFrame         ( 0 ),
  m_uiChromaLocationField         ( 0 )
{
}

Bool
VUI::ChromaLocationInfo::isSame( const VUI::ChromaLocationInfo& rcChromaLocationInfo )  const
{
  ROFRS( m_bChromaLocationInfoPresentFlag ==  rcChromaLocationInfo.m_bChromaLocationInfoPresentFlag,  false );
  ROFRS( m_bChromaLocationInfoPresentFlag,                                                            true  );
  ROFRS( m_uiChromaLocationFrame          ==  rcChromaLocationInfo.m_uiChromaLocationFrame,           false );
  ROFRS( m_uiChromaLocationField          ==  rcChromaLocationInfo.m_uiChromaLocationField,           false );
  return true;
}

ErrVal
VUI::ChromaLocationInfo::copy( const VUI::ChromaLocationInfo& rcChromaLocationInfo )
{
  m_bChromaLocationInfoPresentFlag = rcChromaLocationInfo.m_bChromaLocationInfoPresentFlag;
  m_uiChromaLocationFrame          = rcChromaLocationInfo.m_uiChromaLocationFrame;
  m_uiChromaLocationField          = rcChromaLocationInfo.m_uiChromaLocationField;
  return Err::m_nOK;
}


VUI::TimingInfo::TimingInfo():
  m_uiNumUnitsInTick              ( 0 ),
  m_uiTimeScale                   ( 0 ),
  m_bFixedFrameRateFlag           ( 0 )
{
}

Bool
VUI::TimingInfo::isSame( const VUI::TimingInfo& rcTimingInfo )  const
{
  ROFRS( m_bTimingInfoPresentFlag ==  rcTimingInfo.m_bTimingInfoPresentFlag,  false );
  ROFRS( m_bTimingInfoPresentFlag,                                            true  );
  ROFRS( m_uiNumUnitsInTick       ==  rcTimingInfo.m_uiNumUnitsInTick,        false );
  ROFRS( m_uiTimeScale            ==  rcTimingInfo.m_uiTimeScale,             false );
  ROFRS( m_bFixedFrameRateFlag    ==  rcTimingInfo.m_bFixedFrameRateFlag,     false );
  return true;
}

ErrVal
VUI::TimingInfo::copy( const VUI::TimingInfo& rcTimingInfo )
{
  m_bTimingInfoPresentFlag = rcTimingInfo.m_bTimingInfoPresentFlag;
  m_uiNumUnitsInTick       = rcTimingInfo.m_uiNumUnitsInTick;
  m_uiTimeScale            = rcTimingInfo.m_uiTimeScale;
  m_bFixedFrameRateFlag    = rcTimingInfo.m_bFixedFrameRateFlag;
  return Err::m_nOK;
}

ErrVal VUI::AspectRatioInfo::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bAspectRatioInfoPresentFlag,       "VUI: aspect_ratio_info_present_flag"));
  ROFRS( m_bAspectRatioInfoPresentFlag, Err::m_nOK );

  RNOKS( pcReadIf->getCode( m_uiAspectRatioIdc, 8,               "VUI: aspect_ratio_idc"));

  if( m_uiAspectRatioIdc == 0xFF ) //Extendet_SAR
  {
    RNOKS( pcReadIf->getCode( m_uiSarWith, 16,                   "VUI: sar_width"));
    ROTRS(0 == m_uiSarWith, Err::m_nInvalidParameter);

    RNOKS( pcReadIf->getCode( m_uiSarHeight, 16,                 "VUI: sar_height"));
    ROTRS(0 == m_uiSarHeight, Err::m_nInvalidParameter);
  }
  return Err::m_nOK;
}

ErrVal VUI::VideoSignalType::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bVideoSignalTypePresentFlag,       "VUI: video_signal_type_present_flag"));
  ROFRS( m_bVideoSignalTypePresentFlag, Err::m_nOK );

  RNOKS( pcReadIf->getCode( m_uiVideoFormat, 3,                  "VUI: video_format"));
  RNOKS( pcReadIf->getFlag( m_bVideoFullRangeFlag,               "VUI: video_full_range_flag"));
  RNOKS( pcReadIf->getFlag( m_bColourDescriptionPresentFlag,     "VUI: colour_description_present_flag"));

  if( getColourDescriptionPresentFlag() )
  {
    RNOKS( pcReadIf->getCode( m_uiColourPrimaries, 8,            "VUI: colour_primaries"));
    RNOKS( pcReadIf->getCode( m_uiTransferCharacteristics, 8,    "VUI: transfer_characteristics"));
    RNOKS( pcReadIf->getCode( m_uiMatrixCoefficients, 8,         "VUI: matrix_coefficients"));
  }
  return Err::m_nOK;
}

ErrVal VUI::ChromaLocationInfo::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bChromaLocationInfoPresentFlag,    "VUI: chroma_location_info_present_flag"));
  ROFRS( m_bChromaLocationInfoPresentFlag, Err::m_nOK );

  RNOKS( pcReadIf->getUvlc( m_uiChromaLocationFrame,             "VUI: chroma_location_frame"));
  ROTRS( m_uiChromaLocationFrame>3, Err::m_nInvalidParameter);
  RNOKS( pcReadIf->getUvlc( m_uiChromaLocationField,             "VUI: chroma_location_field"));
  ROTRS( m_uiChromaLocationField>3, Err::m_nInvalidParameter);
  return Err::m_nOK;
}

ErrVal VUI::TimingInfo::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( pcReadIf->getFlag( m_bTimingInfoPresentFlag,            "VUI: timing_info_present_flag"));
  ROFRS( m_bTimingInfoPresentFlag, Err::m_nOK );

  RNOKS( pcReadIf->getCode( m_uiNumUnitsInTick, 32,              "VUI: num_units_in_tick"));
  RNOKS( pcReadIf->getCode( m_uiTimeScale, 32,                   "VUI: time_scale"));
  RNOKS( pcReadIf->getFlag( m_bFixedFrameRateFlag,               "VUI: fixed_frame_rate_flag"));
  return Err::m_nOK;
}

ErrVal VUI::AspectRatioInfo::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bAspectRatioInfoPresentFlag,    "VUI: aspect_ratio_info_present_flag"));
  ROFRS( m_bAspectRatioInfoPresentFlag, Err::m_nOK );

  RNOK( pcWriteIf->writeCode( m_uiAspectRatioIdc, 8,            "VUI: aspect_ratio_idc"));

  if( 0xFF == m_uiAspectRatioIdc ) //Extendet_SAR
  {
    RNOK( pcWriteIf->writeCode( m_uiSarWith, 16,                "VUI: sar_width"));
    RNOK( pcWriteIf->writeCode( m_uiSarHeight, 16,              "VUI: sar_height"));
  }
  return Err::m_nOK;
}

ErrVal VUI::VideoSignalType::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bVideoSignalTypePresentFlag,    "VUI: video_signal_type_present_flag"));
  ROFRS( m_bVideoSignalTypePresentFlag, Err::m_nOK );

  RNOK( pcWriteIf->writeCode( m_uiVideoFormat, 3,               "VUI: video_format"));
  RNOK( pcWriteIf->writeFlag( m_bVideoFullRangeFlag,            "VUI: video_full_range_flag"));
  RNOK( pcWriteIf->writeFlag( m_bColourDescriptionPresentFlag,  "VUI: colour_description_present_flag"));
  if( m_bColourDescriptionPresentFlag )
  {
    RNOK( pcWriteIf->writeCode( m_uiColourPrimaries, 8,         "VUI: colour_primaries"));
    RNOK( pcWriteIf->writeCode( m_uiTransferCharacteristics, 8, "VUI: transfer_characteristics"));
    RNOK( pcWriteIf->writeCode( m_uiMatrixCoefficients, 8,      "VUI: matrix_coefficients"));
  }
  return Err::m_nOK;
}

ErrVal VUI::ChromaLocationInfo::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bChromaLocationInfoPresentFlag, "VUI: chroma_location_info_present_flag"));
  ROFRS( m_bChromaLocationInfoPresentFlag, Err::m_nOK );

  RNOK( pcWriteIf->writeUvlc( m_uiChromaLocationFrame,          "VUI: chroma_location_frame"));
  RNOK( pcWriteIf->writeUvlc( m_uiChromaLocationField,          "VUI: chroma_location_field"));
  return Err::m_nOK;
}

ErrVal VUI::TimingInfo::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bTimingInfoPresentFlag,         "VUI: timing_info_present_flag"));
  ROFRS( m_bTimingInfoPresentFlag, Err::m_nOK );

  RNOK( pcWriteIf->writeCode( m_uiNumUnitsInTick, 32,           "VUI: num_units_in_tick"));
  RNOK( pcWriteIf->writeCode( m_uiTimeScale, 32,                "VUI: time_scale"));
  RNOK( pcWriteIf->writeFlag( m_bFixedFrameRateFlag,            "VUI: fixed_frame_rate_flag"));
  return Err::m_nOK;
}


VUI::VUI( SequenceParameterSet* pcSPS):
  m_bVuiParametersPresentFlag     ( false ),
  m_bOverscanInfoPresentFlag      ( false ),
  m_bOverscanAppropriateFlag      ( false ),
  m_cBitstreamRestriction         ( pcSPS ),
  m_uiDefaultIdx                  ( MSYS_UINT_MAX )
{
  m_eProfileIdc = pcSPS->getProfileIdc();
}

VUI::~VUI()
{
}


Bool
VUI::isSameExceptHRDParametersAndSVCExt( const VUI& rcVUI )  const
{
  ROFRS( m_bVuiParametersPresentFlag    == rcVUI.m_bVuiParametersPresentFlag, false );
  ROFRS( m_bVuiParametersPresentFlag,                                         true  );
  ROFRS( m_bOverscanInfoPresentFlag     == rcVUI.m_bOverscanInfoPresentFlag,  false );
  if( m_bOverscanInfoPresentFlag )
  {
    ROFRS( m_bOverscanAppropriateFlag   == rcVUI.m_bOverscanAppropriateFlag,  false );
  }
  ROFRS( m_cAspectRatioInfo       .isSame( rcVUI.m_cAspectRatioInfo       ),  false );
  ROFRS( m_cVideoSignalType       .isSame( rcVUI.m_cVideoSignalType       ),  false );
  ROFRS( m_cChromaLocationInfo    .isSame( rcVUI.m_cChromaLocationInfo    ),  false );
  ROFRS( m_cBitstreamRestriction  .isSame( rcVUI.m_cBitstreamRestriction  ),  false );
  return true;
}

ErrVal
VUI::copyExceptHRDParametersAndSVCExt( const VUI& rcVUI )
{
  m_bVuiParametersPresentFlag  = rcVUI.m_bVuiParametersPresentFlag;
  m_bOverscanInfoPresentFlag   = rcVUI.m_bOverscanInfoPresentFlag;
  m_bOverscanAppropriateFlag   = rcVUI.m_bOverscanAppropriateFlag;
  RNOK( m_cAspectRatioInfo       .copy( rcVUI.m_cAspectRatioInfo       ) );
  RNOK( m_cVideoSignalType       .copy( rcVUI.m_cVideoSignalType       ) );
  RNOK( m_cChromaLocationInfo    .copy( rcVUI.m_cChromaLocationInfo    ) );
  RNOK( m_cBitstreamRestriction  .copy( rcVUI.m_cBitstreamRestriction  ) );
  return Err::m_nOK;
}



ErrVal VUI::InitHrd( UInt uiIndex, HRD::HrdParamType eHrdType, UInt uiBitRate, UInt uiCpbSize)
{
  HRD& rcHrd = (eHrdType == HRD::NAL_HRD) ? m_acNalHrd[uiIndex] : m_acVclHrd[uiIndex];

  if( rcHrd.getHrdParametersPresentFlag())
  {
    // we provide only one set of parameters. the following code is pretty hard coded
    rcHrd.setCpbCnt(1);
    UInt uiExponentBR  = gMax(6, gGetNumberOfLSBZeros(uiBitRate));
    UInt uiExponentCpb = gMax(4, gGetNumberOfLSBZeros(uiCpbSize));
    rcHrd.setBitRateScale(uiExponentBR-6);
    rcHrd.setCpbSizeScale(uiExponentCpb-4);
    rcHrd.init(1);
    for( UInt CpbCnt=0; CpbCnt<1; CpbCnt++)
    {
      Int iBitNumLSBZeros = gGetNumberOfLSBZeros(uiBitRate);
      Int iCpbNumLSBZeros = gGetNumberOfLSBZeros(uiCpbSize);
      // we're doing a bit complicated rounding here to find the nearest value
      // probably we should use the exact or bigger bit rate value:
      //for values with 6 or more LSBZeros:
      if( iBitNumLSBZeros >= 5)
      {
        rcHrd.getCntBuf(CpbCnt).setBitRateValue((UInt)floor ( ((Double)(uiBitRate) /  (1 << uiExponentBR))));
      }
      else//for values with less than 6 LSBZeros
      {
        rcHrd.getCntBuf(CpbCnt).setBitRateValue((UInt)floor ( ((Double)(uiBitRate) /  (1 << uiExponentBR)) + 0.5));
      }

      //for values with 4 or more LSBZeros:
      if( iCpbNumLSBZeros >= 3)
      {
        rcHrd.getCntBuf(CpbCnt).setCpbSizeValue((UInt)floor ( ((Double)(uiCpbSize) /  (1<< uiExponentCpb)) ));
      }
      else//for values with less than 4 LSBZeros
      {
        rcHrd.getCntBuf(CpbCnt).setCpbSizeValue((UInt)floor ( ((Double)(uiCpbSize) /  (1<< uiExponentCpb)) + 0.5));
      }
      //0 VBR 1 CBR
      rcHrd.getCntBuf(CpbCnt).setVbrCbrFlag(0);// 1: stuffing needed
    }
    rcHrd.setInitialCpbRemovalDelayLength(24);
    rcHrd.setCpbRemovalDelayLength(24);
    rcHrd.setDpbOutputDelayLength(24);
    rcHrd.setTimeOffsetLength(0);
  }
  return Err::m_nOK;
}


ErrVal VUI::init( UInt uiNumTemporalLevels, UInt uiNumFGSLevels )
{
  m_uiNumTemporalLevels = uiNumTemporalLevels;
  m_uiNumFGSLevels = uiNumFGSLevels;
  UInt uiNumLayers = uiNumTemporalLevels * uiNumFGSLevels;
  RNOK( m_acLayerInfo.init( uiNumLayers ) );
  RNOK( m_acTimingInfo.init( uiNumLayers ) );
  RNOK( m_acNalHrd.init( uiNumLayers ) );
  RNOK( m_acVclHrd.init( uiNumLayers ) );
  RNOK( m_abLowDelayHrdFlag.init( uiNumLayers ) );
  RNOK( m_abPicStructPresentFlag.init( uiNumLayers ) );

  return Err::m_nOK;
}

ErrVal VUI::LayerInfo::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeCode( m_uiDependencyID, 3,  "VUI: dependency_id"  ) );
  RNOK( pcWriteIf->writeCode( m_uiQualityId,    4,  "VUI: quality_level"  ) );
  RNOK( pcWriteIf->writeCode( m_uiTemporalId,   3,  "VUI: temporal_level" ) );
  return Err::m_nOK;
}

ErrVal VUI::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  RNOK( pcWriteIf->writeFlag( m_bVuiParametersPresentFlag,            "SPS: vui_parameters_present_flag" ) );

  if (!m_bVuiParametersPresentFlag) return Err::m_nOK;

  RNOK( m_cAspectRatioInfo.write( pcWriteIf ) );

  RNOK( pcWriteIf->writeFlag( getOverscanInfoPresentFlag(),     "VUI: overscan_info_present_flag"));
  if( getOverscanInfoPresentFlag() )
  {
    RNOK( pcWriteIf->writeFlag( getOverscanAppropriateFlag(),   "VUI: overscan_appropriate_flag"));
  }

  RNOK( m_cVideoSignalType.write( pcWriteIf ) );
  RNOK( m_cChromaLocationInfo.write( pcWriteIf ) );

  if( m_eProfileIdc == SCALABLE_BASELINE_PROFILE || m_eProfileIdc == SCALABLE_HIGH_PROFILE )
  {
    RNOK( pcWriteIf->writeFlag( false,  "VUI: timing_info_present_flag"     ) );
    RNOK( pcWriteIf->writeFlag( false,  "HRD: hdr_parameters_present_flag"  ) );
    RNOK( pcWriteIf->writeFlag( false,  "HRD: hdr_parameters_present_flag"  ) );
    RNOK( pcWriteIf->writeFlag( false,  "VUI: pic_struct_present_flag"      ) );
  }
  else
  {
    ROT( m_uiDefaultIdx == MSYS_UINT_MAX );
    RNOK( m_acTimingInfo.get(m_uiDefaultIdx).write( pcWriteIf ) );
    RNOK( m_acNalHrd.get(m_uiDefaultIdx).write( pcWriteIf ) );
    RNOK( m_acVclHrd.get(m_uiDefaultIdx).write( pcWriteIf ) );
    if( m_acNalHrd.get(m_uiDefaultIdx).getHrdParametersPresentFlag() || m_acVclHrd.get(m_uiDefaultIdx).getHrdParametersPresentFlag() )
    {
      RNOK( pcWriteIf->writeFlag( m_abLowDelayHrdFlag.get(m_uiDefaultIdx),           "VUI: low_delay_hrd_flag"));
    }
    RNOK( pcWriteIf->writeFlag( m_abPicStructPresentFlag.get(m_uiDefaultIdx),        "VUI: pic_struct_present_flag"));
  }

  RNOK( m_cBitstreamRestriction.write( pcWriteIf ) );

  return Err::m_nOK;
}


ErrVal VUI::writeSVCExtension( HeaderSymbolWriteIf* pcWriteIf ) const
{
  ROF( getNumLayers() );
  RNOK( pcWriteIf->writeUvlc( getNumLayers() - 1,            "SPS: num_layers_minus1" ) );

  for( UInt uiLayer = 0; uiLayer < getNumLayers(); uiLayer++ )
  {
    RNOK( m_acLayerInfo.get(uiLayer).write( pcWriteIf ) );
    RNOK( m_acTimingInfo.get(uiLayer).write( pcWriteIf ) );
    RNOK( m_acNalHrd.get(uiLayer).write( pcWriteIf ) );
    RNOK( m_acVclHrd.get(uiLayer).write( pcWriteIf ) );
    if( m_acNalHrd.get(uiLayer).getHrdParametersPresentFlag() || m_acVclHrd.get(uiLayer).getHrdParametersPresentFlag() )
    {
      RNOK( pcWriteIf->writeFlag( m_abLowDelayHrdFlag.get(uiLayer),           "VUI: low_delay_hrd_flag"));
    }
    RNOK( pcWriteIf->writeFlag( m_abPicStructPresentFlag.get(uiLayer),        "VUI: pic_struct_present_flag"));
  }

  return Err::m_nOK;
}

ErrVal VUI::LayerInfo::read( HeaderSymbolReadIf* pcReadIf )
{
  RNOKS( pcReadIf->getCode( m_uiDependencyID, 3,  "VUI: dependency_id"  ) );
  RNOKS( pcReadIf->getCode( m_uiQualityId,    4,  "VUI: quality_level"  ) );
  RNOKS( pcReadIf->getCode( m_uiTemporalId,   3,  "VUI: temporal_level" ) );
  return Err::m_nOK;
}

ErrVal VUI::read( HeaderSymbolReadIf *pcReadIf )
{
  RNOKS( m_cAspectRatioInfo.read( pcReadIf ) );

  RNOKS( pcReadIf->getFlag( m_bOverscanInfoPresentFlag,          "VUI: overscan_info_present_flag"));
  if( m_bOverscanInfoPresentFlag )
  {
    RNOKS( pcReadIf->getFlag( m_bOverscanAppropriateFlag,        "VUI: overscan_appropriate_flag"));
  }

  RNOKS( m_cVideoSignalType.read( pcReadIf ) );
  RNOKS( m_cChromaLocationInfo.read( pcReadIf ) );

  m_acLayerInfo           .uninit();
  m_acTimingInfo          .uninit();
  m_acNalHrd              .uninit();
  m_acVclHrd              .uninit();
  m_abLowDelayHrdFlag     .uninit();
  m_abPicStructPresentFlag.uninit();
  m_acLayerInfo           .init( 1 );
	m_acTimingInfo          .init( 1 );
  m_acNalHrd              .init( 1 );
  m_acVclHrd              .init( 1 );
  m_abLowDelayHrdFlag     .init( 1 );
  m_abPicStructPresentFlag.init( 1 );

  // fill in the LayerInfo of AVC compatible layer
  m_uiDefaultIdx = 0;
  m_acLayerInfo[m_uiDefaultIdx].setDependencyID(0);
  m_acLayerInfo[m_uiDefaultIdx].setQualityLevel(0);
  m_acLayerInfo[m_uiDefaultIdx].setTemporalId(0);

	RNOKS( m_acTimingInfo.get(m_uiDefaultIdx).read( pcReadIf ) );
  RNOKS( m_acNalHrd.get(m_uiDefaultIdx).read( pcReadIf ) );
  RNOKS( m_acVclHrd.get(m_uiDefaultIdx).read( pcReadIf ) );
  if( m_acNalHrd.get(m_uiDefaultIdx).getHrdParametersPresentFlag() || m_acVclHrd.get(m_uiDefaultIdx).getHrdParametersPresentFlag() )
  {
    RNOKS( pcReadIf->getFlag( m_abLowDelayHrdFlag[m_uiDefaultIdx],                "VUI: low_delay_hrd_flag"));
  }
  RNOKS( pcReadIf->getFlag( m_abPicStructPresentFlag[m_uiDefaultIdx],             "VUI: pic_struct_present_flag"));

  RNOKS( m_cBitstreamRestriction.read( pcReadIf ) );
  return Err::m_nOK;
}


ErrVal VUI::readSVCExtension( HeaderSymbolReadIf *pcReadIf )
{
  UInt uiNumLayers = 0;
  RNOK( pcReadIf->getUvlc( uiNumLayers,            "SPS: num_layers_minus1" ) );
  uiNumLayers++;

  UInt uiStartIdx = m_acLayerInfo.size();
  m_acLayerInfo           .reinit( uiNumLayers );
  m_acTimingInfo          .reinit( uiNumLayers );
  m_acNalHrd              .reinit( uiNumLayers );
  m_acVclHrd              .reinit( uiNumLayers );
  m_abLowDelayHrdFlag     .reinit( uiNumLayers );
  m_abPicStructPresentFlag.reinit( uiNumLayers );

  for( UInt uiLayer = uiStartIdx; uiLayer < uiNumLayers + uiStartIdx; uiLayer++ )
  {
    RNOK( m_acLayerInfo.get(uiLayer).read( pcReadIf ) );
    RNOK( m_acTimingInfo.get(uiLayer).read( pcReadIf ) );
    RNOK( m_acNalHrd.get(uiLayer).read( pcReadIf ) );
    RNOK( m_acVclHrd.get(uiLayer).read( pcReadIf ) );
    if( m_acNalHrd.get(uiLayer).getHrdParametersPresentFlag() || m_acVclHrd.get(uiLayer).getHrdParametersPresentFlag() )
    {
      RNOK( pcReadIf->getFlag( m_abLowDelayHrdFlag.get(uiLayer),           "VUI: low_delay_hrd_flag"));
    }
    RNOK( pcReadIf->getFlag( m_abPicStructPresentFlag.get(uiLayer),        "VUI: pic_struct_present_flag"));
  }

  return Err::m_nOK;
}

// h264 namespace end
H264AVC_NAMESPACE_END

