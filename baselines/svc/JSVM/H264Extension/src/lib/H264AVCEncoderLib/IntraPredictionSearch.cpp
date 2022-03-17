
#include "H264AVCEncoderLib.h"
#include "IntraPredictionSearch.h"


H264AVC_NAMESPACE_BEGIN

IntraPredictionSearch::IntraPredictionSearch()
{
}

IntraPredictionSearch::~IntraPredictionSearch()
{
}


ErrVal IntraPredictionSearch::create( IntraPredictionSearch*& rpcIntraPredictionSearch )
{
  rpcIntraPredictionSearch = new IntraPredictionSearch;

  ROT( NULL == rpcIntraPredictionSearch );

  return Err::m_nOK;
}


ErrVal IntraPredictionSearch::destroy()
{
  delete this;

  return Err::m_nOK;
}




ErrVal IntraPredictionSearch::predictSLumaMb( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode, Bool& rbValid )
{
  rbValid = false;
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  XPel *pPel    = pcYuvBuffer->getMbLumAddr();
  Int   iStride = pcYuvBuffer->getLStride();

  switch( uiPredMode )
  {
  case 0:
    {
      if( xIsAboveRef() )
      {
        xPred16x16IMode0Vert( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 1:
    {
      if( xIsLeftRef() )
      {
        xPred16x16IMode1Hori( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 2:
    {
      xPred16x16IMode2DC( pPel, iStride );
      rbValid = true;
      break;
    }
  case 3:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPred16x16IMode3Plane( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  default:
    {
      AF();
      return Err::m_nERR;
    }
  }
  return Err::m_nOK;
}



ErrVal IntraPredictionSearch::predictSLumaBlock( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode, LumaIdx cIdx, Bool &rbValid )
{
  rbValid       = false;
  XPel* pPel    = pcYuvBuffer->getLumBlk();
  Int   iStride = pcYuvBuffer->getLStride();

  m_uiAvailable = xGetAvailableMask( cIdx );

  switch( uiPredMode & 0xf )
  {
  case 0:
    {
      if( xIsAboveRef() )
      {
        xPredMode0Vert( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 1:
    {
      if( xIsLeftRef() )
      {
        xPredMode1Horiz( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 2:
    {
      xPredMode2Dc( pPel, iStride );
      rbValid = true;
      break;
    }
  case 3:
    {
      if( xIsAboveRef() )
      {
        xPredMode3DiagDownLeft( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 4:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredMode4DiagDownRight( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 5:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredMode5VertRight( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 6:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredMode6HorizDown( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 7:
    {
      if( xIsAboveRef() )
      {
        xPredMode7VertLeft( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 8:
    {
      if( xIsLeftRef() )
      {
        xPredMode8HorizUp( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}



ErrVal IntraPredictionSearch::predictSChromaBlock( YuvMbBuffer *pcYuvBuffer, UInt uiPredMode, Bool &rbValid )
{
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  rbValid = false;
  XPel *pucCb = pcYuvBuffer->getMbCbAddr();
  XPel *pucCr = pcYuvBuffer->getMbCrAddr();
  Int iStride = pcYuvBuffer->getLStride();

  switch( uiPredMode )
  {
  case 0:
    {
      xPred8x8IMode0DC( pucCb, iStride );
      xPred8x8IMode0DC( pucCr, iStride );
      rbValid = true;
      break;
    }
  case 1:
    {
      ROFRS( xIsLeftRef(), Err::m_nOK );

      xPred8x8IMode1Hori( pucCb, iStride );
      xPred8x8IMode1Hori( pucCr, iStride );
      rbValid = true;
      break;
    }
  case 2:
    {
      ROFRS( xIsAboveRef(), Err::m_nOK );

      xPred8x8IMode2Vert( pucCb, iStride );
      xPred8x8IMode2Vert( pucCr, iStride );
      rbValid = true;
      break;
    }
  case 3:
    {
      ROFRS( xIsAllLeftAboveRef(), Err::m_nOK );

      xPred8x8IMode3Plane( pucCb, iStride );
      xPred8x8IMode3Plane( pucCr, iStride );
      rbValid = true;
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}





ErrVal IntraPredictionSearch::predictSLumaBlock8x8( YuvMbBuffer* pcYuvBuffer,
                                                    UInt            uiPredMode,
                                                    B8x8Idx         c8x8Idx,
                                                    Bool&           rbValid )
{
  rbValid       = false;
  XPel* pPel    = pcYuvBuffer->getLumBlk  ();
  Int   iStride = pcYuvBuffer->getLStride ();

  xSet8x8AvailableMask( c8x8Idx );

  switch( uiPredMode & 0xf )
  {
  case 0:
    {
      if( xIsAboveRef() )
      {
        xPredLum8x8Mode0Vert( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 1:
    {
      if( xIsLeftRef() )
      {
        xPredLum8x8Mode1Horiz( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 2:
    {
      xPredLum8x8Mode2Dc( pPel, iStride );
      rbValid = true;
      break;
    }
  case 3:
    {
      if( xIsAboveRef() )
      {
        xPredLum8x8Mode3DiagDownLeft( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 4:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredLum8x8Mode4DiagDownRight( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 5:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredLum8x8Mode5VertRight( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 6:
    {
      if( xIsAllLeftAboveRef() )
      {
        xPredLum8x8Mode6HorizDown( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 7:
    {
      if( xIsAboveRef() )
      {
        xPredLum8x8Mode7VertLeft( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  case 8:
    {
      if( xIsLeftRef() )
      {
        xPredLum8x8Mode8HorizUp( pPel, iStride );
        rbValid = true;
      }
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
