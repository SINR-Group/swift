
#include "ScalableSEIModifyCode.h"

ScalableSEIModifyCode::ScalableSEIModifyCode() :
  m_pcBinData( NULL ),
  m_pulStreamPacket( NULL ),
  m_uiBitCounter( 0 ),
  m_uiPosCounter( 0 ),
  m_uiCoeffCost ( 0 ),
  m_uiDWordsLeft    ( 0 ),
  m_uiBitsWritten   ( 0 ),
  m_iValidBits      ( 0 ),
  m_ulCurrentBits   ( 0 )
{

}

ErrVal
ScalableSEIModifyCode::Create(  ScalableSEIModifyCode*& rpcScalableSEIModifyCode )
{
  rpcScalableSEIModifyCode = new ScalableSEIModifyCode;
	ROT( rpcScalableSEIModifyCode == NULL );
	return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::Destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::init( UInt* pulStream, UInt uiBytes )
{
  ROT( pulStream == NULL );
  m_pulStreamPacket = pulStream;

  m_uiDWordsLeft = uiBytes/4;
//  m_uiDWordsLeft = 0x400/4;
  m_iValidBits = 32;
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::Uninit( )
{
  m_uiBitCounter  = 0;
  m_uiPosCounter  = 0;
  m_uiCoeffCost   = 0;
  m_uiDWordsLeft  = 0;
  m_uiBitsWritten = 0;
  m_iValidBits    = 0;
  m_ulCurrentBits = 0;
	m_pulStreamPacket = NULL;

	return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::WriteUVLC( UInt uiValue )
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiValue;

  while( uiTemp != 1 )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }

  RNOK( Write( uiValue, uiLength ) );
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::WriteCode( UInt uiValue, UInt uiLength )
{
  RNOK( Write( uiValue, uiLength ) );
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::WriteFlag( Bool bFlag )
{
  RNOK( Write( bFlag? 1 : 0 , 1) );
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::Write( UInt uiBits, UInt uiNumberOfBits )
{
  m_uiBitsWritten += uiNumberOfBits;

  if( (Int)uiNumberOfBits < m_iValidBits)  // one word
  {
    m_iValidBits -= uiNumberOfBits;

    m_ulCurrentBits |= uiBits << m_iValidBits;

    return Err::m_nOK;
  }


  ROT( 0 == m_uiDWordsLeft );
  m_uiDWordsLeft--;

  UInt uiShift = uiNumberOfBits - m_iValidBits;

  // add the last bits
  m_ulCurrentBits |= uiBits >> uiShift;

  *m_pulStreamPacket++ = xSwap( m_ulCurrentBits );


  // note: there is a problem with left shift with 32
  m_iValidBits = 32 - uiShift;

  m_ulCurrentBits = uiBits << m_iValidBits;

  if( 0 == uiShift )
  {
    m_ulCurrentBits = 0;
  }

  return Err::m_nOK;
}


ErrVal
ScalableSEIModifyCode::WritePayloadHeader( enum h264::SEI::MessageType eType, UInt uiSize )
{
  //type
  {
    UInt uiTemp = eType;
    UInt uiByte = 0xFF;
    while( 0xFF == uiByte )
    {
      uiByte  = (0xFF > uiTemp) ? uiTemp : 0xff;
      uiTemp -= 0xFF;
      RNOK( WriteCode( uiByte, 8 ) );
    }
  }

  // size
  {
    UInt uiTemp = uiSize;
    UInt uiByte = 0xFF;

    while( 0xFF == uiByte )
    {
      uiByte  = (0xFF > uiTemp) ? uiTemp : 0xff;
      uiTemp -= 0xFF;
      RNOK( WriteCode( uiByte, 8 ) );
    }
  }
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::WriteAlignZero()
{
  return Write( 0, m_iValidBits & 0x7 );
}

ErrVal
ScalableSEIModifyCode::WriteTrailingBits()
{
  RNOK( WriteFlag( 1 ) );
  RNOK( WriteAlignZero() );
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::flushBuffer()
{
  *m_pulStreamPacket = xSwap( m_ulCurrentBits );

  m_uiBitsWritten = (m_uiBitsWritten+7)/8;

  m_uiBitsWritten *= 8;

  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::ConvertRBSPToPayload( UChar* m_pucBuffer,
                                         UChar pulStreamPacket[],
                                      UInt& ruiBytesWritten,
                                      UInt  uiHeaderBytes )
{
  UInt uiZeroCount    = 0;
  UInt uiReadOffset   = uiHeaderBytes;
  UInt uiWriteOffset  = uiHeaderBytes;

  //===== NAL unit header =====
  for( UInt uiIndex = 0; uiIndex < uiHeaderBytes; uiIndex++ )
  {
    m_pucBuffer[uiIndex] = (UChar)pulStreamPacket[uiIndex];
  }

  //===== NAL unit payload =====
  for( ; uiReadOffset < ruiBytesWritten ; uiReadOffset++, uiWriteOffset++ )
  {
    if( 2 == uiZeroCount && 0 == ( pulStreamPacket[uiReadOffset] & 0xfc ) )
    {
      uiZeroCount                   = 0;
      m_pucBuffer[uiWriteOffset++]  = 0x03;
    }

    m_pucBuffer[uiWriteOffset] = (UChar)pulStreamPacket[uiReadOffset];

    if( 0 == pulStreamPacket[uiReadOffset] )
    {
      uiZeroCount++;
    }
    else
    {
      uiZeroCount = 0;
    }
  }
  if( ( 0x00 == m_pucBuffer[uiWriteOffset-1] ) && ( 0x00 == m_pucBuffer[uiWriteOffset-2] ) )
  {
    m_pucBuffer[uiWriteOffset++] = 0x03;
  }
  ruiBytesWritten = uiWriteOffset;

  return Err::m_nOK;
}




ErrVal
ScalableSEIModifyCode::SEICode( h264::SEI::ScalableSei* pcScalableSei, ScalableSEIModifyCode *pcScalableModifyCode )
{
  // JVT-U085 LMI
  pcScalableModifyCode->WriteFlag( pcScalableSei->getTlevelNestingFlag() );
	pcScalableModifyCode->WriteFlag( pcScalableSei->getPriorityLayerInfoPresentFlag() );//SEI changes update
	pcScalableModifyCode->WriteFlag( pcScalableSei->getPriorityIdSettingFlag() );//JVT-W053 wxwan
  UInt uiNumScalableLayersMinus1 = pcScalableSei->getNumLayersMinus1();
  pcScalableModifyCode->WriteUVLC( uiNumScalableLayersMinus1 );
  for( UInt uiLayer = 0; uiLayer <= uiNumScalableLayersMinus1; uiLayer++ )
  {
		//JVT-W051 {
		pcScalableModifyCode->WriteUVLC( pcScalableSei->getLayerId( uiLayer ) );
		//JVT-W051 }
//JVT-S036 lsj start
    pcScalableModifyCode->WriteCode( pcScalableSei->getPriorityId( uiLayer ), 6 );//SEI changes update
    pcScalableModifyCode->WriteFlag( pcScalableSei->getDiscardableFlag( uiLayer ) );
    pcScalableModifyCode->WriteCode( pcScalableSei->getDependencyId( uiLayer ), 3 );
    pcScalableModifyCode->WriteCode( pcScalableSei->getQualityId( uiLayer ), 4 );
    pcScalableModifyCode->WriteCode( pcScalableSei->getTemporalId( uiLayer ), 3 );//SEI changes update
		pcScalableModifyCode->WriteFlag( pcScalableSei->getSubPicLayerFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getSubRegionLayerFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getIroiSliceDivisionInfoPresentFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getProfileLevelInfoPresentFlag( uiLayer ) );
//JVT-S036 lsj end
    pcScalableModifyCode->WriteFlag( pcScalableSei->getBitrateInfoPresentFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getFrmRateInfoPresentFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getFrmSizeInfoPresentFlag( uiLayer ) );
    pcScalableModifyCode->WriteFlag( pcScalableSei->getLayerDependencyInfoPresentFlag( uiLayer ) );
    //SEI changes update {
		pcScalableModifyCode->WriteFlag( pcScalableSei->getParameterSetsInfoPresentFlag( uiLayer ) );
		pcScalableModifyCode->WriteFlag( pcScalableSei->getBitstreamRestrictionInfoPresentFlag( uiLayer ) );//JVT-W051
		//SEI changes update }
    pcScalableModifyCode->WriteFlag( pcScalableSei->getExactInterlayerPredFlag( uiLayer ) ); //JVT-S036 lsj
    if( pcScalableSei->getSubPicLayerFlag( uiLayer ) || pcScalableSei->getIroiSliceDivisionInfoPresentFlag( uiLayer ) )
    {
      pcScalableModifyCode->WriteFlag( pcScalableSei->getExactSampleValueMatchFlag( uiLayer ) );
    }
		pcScalableModifyCode->WriteFlag( pcScalableSei->getLayerConversionFlag( uiLayer ) ); //JVT-W046 SEI changes update
		pcScalableModifyCode->WriteFlag( pcScalableSei->getLayerOutputFlag( uiLayer ) );//JVT-W047 wxwan
    if( pcScalableSei->getProfileLevelInfoPresentFlag( uiLayer ) )
    {
      pcScalableModifyCode->WriteCode( pcScalableSei->getLayerProfileLevelIdc( uiLayer ), 24 );
    }
    //SEI changes update {
		//else
  //  {//JVT-S036 lsj
  //    pcScalableModifyCode->WriteUVLC( pcScalableSei->getProfileLevelInfoSrcLayerIdDelta( uiLayer ) );
  //  }
    //SEI changes update }
  // JVT-S036 lsj delete
    if( pcScalableSei->getBitrateInfoPresentFlag( uiLayer ) )
    {
      pcScalableModifyCode->WriteCode( pcScalableSei->getAvgBitrateCode( uiLayer ), 16 );
    //JVT-S036 lsj start
      pcScalableModifyCode->WriteCode( pcScalableSei->getMaxBitrateLayerCode( uiLayer ), 16 );
      pcScalableModifyCode->WriteCode( pcScalableSei->getMaxBitrateDecodedPictureCode( uiLayer ), 16 );
      pcScalableModifyCode->WriteCode( pcScalableSei->getMaxBitrateCalcWindow( uiLayer ), 16 );
    //JVT-S036 lsj end
    }

    if( pcScalableSei->getFrmRateInfoPresentFlag( uiLayer ) )
    {
      pcScalableModifyCode->WriteCode( pcScalableSei->getConstantFrmRateIdc( uiLayer ), 2 );
      pcScalableModifyCode->WriteCode( pcScalableSei->getAvgFrmRate( uiLayer ), 16 );
    }
    //SEI changes update
    //else
    //{//JVT-S036 lsj
    //  pcScalableModifyCode->WriteUVLC( pcScalableSei->getFrmRateInfoSrcLayerIdDelta( uiLayer ) );
    //}

		if( pcScalableSei->getFrmSizeInfoPresentFlag( uiLayer ) || pcScalableSei->getIroiSliceDivisionInfoPresentFlag(uiLayer) )//SEI changes update
    {
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getFrmWidthInMbsMinus1( uiLayer ) );
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getFrmHeightInMbsMinus1( uiLayer ) );
    }
    //SEI changes update
    //else
    //{//JVT-S036 lsj
    //  pcScalableModifyCode->WriteUVLC( pcScalableSei->getFrmSizeInfoSrcLayerIdDelta( uiLayer ) );
    //}

    if( pcScalableSei->getSubRegionLayerFlag( uiLayer ) )
    {
			//JVT-W051 {
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getBaseRegionLayerId( uiLayer ) );
			//JVT-W051 }
			pcScalableModifyCode->WriteFlag( pcScalableSei->getDynamicRectFlag( uiLayer ) );
			//JVT-W051 {
			if( !pcScalableSei->getDynamicRectFlag( uiLayer ) )
			//JVT-W051 }
      {
        pcScalableModifyCode->WriteCode( pcScalableSei->getHorizontalOffset( uiLayer ), 16 );
        pcScalableModifyCode->WriteCode( pcScalableSei->getVerticalOffset( uiLayer ), 16 );
        pcScalableModifyCode->WriteCode( pcScalableSei->getRegionWidth( uiLayer ), 16 );
        pcScalableModifyCode->WriteCode( pcScalableSei->getRegionHeight( uiLayer ), 16 );
      }
    }

  //JVT-S036 lsj start
    if( pcScalableSei->getSubPicLayerFlag( uiLayer ) )
    {
			//JVT-W051 {
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getRoiId( uiLayer ) );
			//JVT-W051 }
    }
		if( pcScalableSei->getIroiSliceDivisionInfoPresentFlag( uiLayer ) )
    {
			//JVT-W051 {
			pcScalableModifyCode->WriteFlag( pcScalableSei->getIroiGridFlag( uiLayer ));
			//JVT-W051 }
      if( pcScalableSei->getIroiGridFlag(uiLayer) )
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getGridSliceWidthInMbsMinus1( uiLayer ) );
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getGridSliceHeightInMbsMinus1( uiLayer ) );
      }
      else
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumSliceMinus1( uiLayer ) );
        for (UInt nslice = 0; nslice <= pcScalableSei->getNumSliceMinus1( uiLayer ) ; nslice ++ )
        {
          pcScalableModifyCode->WriteUVLC( pcScalableSei->getFirstMbInSlice( uiLayer, nslice ) );
          pcScalableModifyCode->WriteUVLC( pcScalableSei->getSliceWidthInMbsMinus1( uiLayer, nslice ) );
          pcScalableModifyCode->WriteUVLC( pcScalableSei->getSliceHeightInMbsMinus1( uiLayer, nslice ) );
        }
      }
    }
  //JVT-S036 lsj end
  //SEI changes update
    if( pcScalableSei->getLayerDependencyInfoPresentFlag( uiLayer ) )
    {
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumDirectlyDependentLayers( uiLayer ) );
      for( UInt ui = 0; ui < pcScalableSei->getNumDirectlyDependentLayers( uiLayer ); ui++ )
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumDirectlyDependentLayerIdDeltaMinus1(uiLayer, ui ) ); //JVT-S036 lsj
      }
    }
    else
    {//JVT-S036 lsj
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getLayerDependencyInfoSrcLayerIdDelta( uiLayer ) );
    }

    if( pcScalableSei->getParameterSetsInfoPresentFlag( uiLayer ) )//SEI changes update
    {
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumInitSPSMinus1( uiLayer ) );
      UInt ui;
      for( ui = 0; ui <= pcScalableSei->getNumInitSPSMinus1( uiLayer ); ui++ )
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getInitSPSIdDelta( uiLayer, ui ) );
      }
			//SEI changes update {
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumInitSSPSMinus1( uiLayer ) );
			for( ui = 0; ui <= pcScalableSei->getNumInitSSPSMinus1( uiLayer ); ui++ )
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getInitSSPSIdDelta( uiLayer, ui ) );
      }
      //SEI changes update }
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumInitPPSMinus1( uiLayer ) );
      for( ui = 0; ui <= pcScalableSei->getNumInitPPSMinus1( uiLayer ); ui++ )
      {
        pcScalableModifyCode->WriteUVLC( pcScalableSei->getInitPPSIdDelta( uiLayer, ui ) );
      }
    }
    else
    {//JVT-S036 lsj
      pcScalableModifyCode->WriteUVLC( pcScalableSei->getInitParameterSetsInfoSrcLayerIdDelta( uiLayer ) );
    }
		//JVT-W051 & JVT-W064 {
    if ( pcScalableSei->getBitstreamRestrictionInfoPresentFlag( uiLayer ) )//SEI changes update
		{
			pcScalableModifyCode->WriteFlag( pcScalableSei->getMotionVectorsOverPicBoundariesFlag( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getMaxBytesPerPicDenom( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getMaxBitsPerMbDenom( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getLog2MaxMvLengthHorizontal( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getLog2MaxMvLengthVertical( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getNumReorderFrames( uiLayer ) );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getMaxDecFrameBuffering( uiLayer ) );
		}
		//JVT-W051 & JVT-W064 }
  //SEI changes update {
		//JVT-W046 {
		if( pcScalableSei->getLayerConversionFlag( uiLayer ) )
    {
      UInt ui;
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getConversionTypeIdc( uiLayer ) );
      for( ui = 0; ui < 2; ui++)
      {
				pcScalableModifyCode->WriteFlag( pcScalableSei->getRewritingInfoFlag( uiLayer,ui ) );
        if(pcScalableSei->getRewritingInfoFlag( uiLayer,ui ))
        {
					pcScalableModifyCode->WriteCode(pcScalableSei->getRewritingProfileLevelIdc( uiLayer,ui ),24 );
          pcScalableModifyCode->WriteCode(pcScalableSei->getRewritingAvgBitrateCode( uiLayer,ui ),16 );
          pcScalableModifyCode->WriteCode(pcScalableSei->getRewritingMaxBitrateCode( uiLayer,ui ),16 );
        }
      }
    }
  //JVT-W046 }

  }// for
	//JVT-W051 {
	//if ( pcScalableSei->getQualityLayerInfoPresentFlag() )//SEI changes update
	//{
	//	pcScalableModifyCode->WriteUVLC( pcScalableSei->getQlNumdIdMinus1() );
	//	for ( UInt i = 0; i <= pcScalableSei->getQlNumdIdMinus1(); i++ )
	//	{
	//		pcScalableModifyCode->WriteCode( pcScalableSei->getQlDependencyId( i ), 3 );
	//		pcScalableModifyCode->WriteUVLC( pcScalableSei->getQlNumMinus1( i ) );
	//		for ( UInt j = 0; j <= pcScalableSei->getQlNumMinus1( i ); j++ )
	//		{
	//			pcScalableModifyCode->WriteUVLC( pcScalableSei->getQlId( i, j ) );
	//			pcScalableModifyCode->WriteCode( pcScalableSei->getQlProfileLevelIdc( i, j ),24 );
	//			pcScalableModifyCode->WriteCode( pcScalableSei->getQlAvgBitrate( i, j ), 16 );
	//			pcScalableModifyCode->WriteCode( pcScalableSei->getQlMaxBitrate( i, j ), 16 );
	//		}
	//	}
	//}
  if ( pcScalableSei->getPriorityLayerInfoPresentFlag() )
  {
		pcScalableModifyCode->WriteUVLC( pcScalableSei->getPrNumdIdMinus1() );
		for ( UInt i = 0; i <= pcScalableSei->getPrNumdIdMinus1(); i++ )
		{
			pcScalableModifyCode->WriteCode( pcScalableSei->getPrDependencyId( i ), 3 );
			pcScalableModifyCode->WriteUVLC( pcScalableSei->getPrNumMinus1( i ) );
			for ( UInt j = 0; j <= pcScalableSei->getPrNumMinus1( i ); j++ )
			{
				pcScalableModifyCode->WriteUVLC( pcScalableSei->getPrId( i, j ) );
				pcScalableModifyCode->WriteCode( pcScalableSei->getPrProfileLevelIdc( i, j ),24 );
				pcScalableModifyCode->WriteCode( pcScalableSei->getPrAvgBitrateCode( i, j ), 16 );
				pcScalableModifyCode->WriteCode( pcScalableSei->getPrMaxBitrateCode( i, j ), 16 );
			}
		}
	}
	//JVT-W051 }
	//JVT-W053 wxwan
  //SEI changes update }
	if(pcScalableSei->getPriorityIdSettingFlag() )
	{
		UInt PriorityIdSettingUriIdx = 0;
		do{
			pcScalableModifyCode->WriteCode( pcScalableSei->getPriorityIdSettingUri(PriorityIdSettingUriIdx),   8) ;
			PriorityIdSettingUriIdx++;
		}while( pcScalableSei->getPriorityIdSettingUri( PriorityIdSettingUriIdx-1 )  !=  0 );
	}
	//JVT-W053 wxwan

  return Err::m_nOK;
}

//JVT-S080 LMI {
ErrVal
ScalableSEIModifyCode::SEICode( h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent, ScalableSEIModifyCode *pcScalableModifyCode )
{
  UInt uiNumScalableLayers = pcScalableSeiLayersNotPresent->getNumLayers();
  UInt uiLayer;
  pcScalableModifyCode->WriteUVLC( uiNumScalableLayers );
  for( uiLayer = 0; uiLayer < uiNumScalableLayers; uiLayer++ )
  {
    pcScalableModifyCode->WriteCode( pcScalableSeiLayersNotPresent->getLayerId( uiLayer ), 8);
  }
  return Err::m_nOK;
}

ErrVal
ScalableSEIModifyCode::SEICode  ( h264::SEI::ScalableSeiDependencyChange* pcScalableSeiDependencyChange, ScalableSEIModifyCode *pcScalableModifyCode )
{
  UInt uiNumScalableLayersMinus1 = pcScalableSeiDependencyChange->getNumLayersMinus1();
     UInt uiLayer, uiDirectLayer;
  pcScalableModifyCode->WriteUVLC( uiNumScalableLayersMinus1 );
  for( uiLayer = 0; uiLayer <= uiNumScalableLayersMinus1; uiLayer++ )
  {
    pcScalableModifyCode->WriteCode( pcScalableSeiDependencyChange->getDependencyId( uiLayer ), 8);
    pcScalableModifyCode->WriteFlag( pcScalableSeiDependencyChange->getLayerDependencyInfoPresentFlag( uiLayer ) );
    if ( pcScalableSeiDependencyChange->getLayerDependencyInfoPresentFlag( uiLayer ) )
    {
          pcScalableModifyCode->WriteUVLC( pcScalableSeiDependencyChange->getNumDirectDependentLayers( uiLayer ) );
      for ( uiDirectLayer = 0; uiDirectLayer < pcScalableSeiDependencyChange->getNumDirectDependentLayers( uiLayer ); uiDirectLayer++)
              pcScalableModifyCode->WriteUVLC(pcScalableSeiDependencyChange->getDirectDependentLayerIdDeltaMinus1( uiLayer, uiDirectLayer ));
    }
    else
             pcScalableModifyCode->WriteUVLC(pcScalableSeiDependencyChange->getLayerDependencyInfoSrcLayerIdDeltaMinus1( uiLayer ) );
  }
  return Err::m_nOK;
}
//JVT-S080 LMI }
