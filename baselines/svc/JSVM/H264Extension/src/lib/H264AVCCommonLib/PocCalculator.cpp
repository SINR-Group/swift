
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/PocCalculator.h"
#include <math.h>



H264AVC_NAMESPACE_BEGIN



PocCalculator::PocCalculator()
  : m_iLastIdrFieldNum    ( 0 )
  , m_iBitsLsb            ( 0 )
  , m_iTop2BotOffset      ( 0 )
  , m_iPrevRefPocMsb      ( 0 )
  , m_iPrevRefPocLsb      ( -1) //--ICU/ETRI FMO Implementation
  , m_iMaxPocLsb          ( 0 )
  , m_iFrameNumOffset     ( 0 )
  , m_iRefOffsetSum       ( 0 )
  , m_iPrevFrameNum       ( 0 )
{
}


ErrVal PocCalculator::create( PocCalculator*& rpcPocCalculator )
{
  rpcPocCalculator = new PocCalculator;

  ROT( NULL == rpcPocCalculator );

  return Err::m_nOK;
}


ErrVal PocCalculator::copy( PocCalculator*& rpcPocCalculator )
{
  rpcPocCalculator = new PocCalculator;

  ROT( NULL == rpcPocCalculator );

	rpcPocCalculator->m_iLastIdrFieldNum = m_iLastIdrFieldNum;
  rpcPocCalculator->m_iBitsLsb         = m_iBitsLsb;
  rpcPocCalculator->m_iTop2BotOffset   = m_iTop2BotOffset;
  rpcPocCalculator->m_iPrevRefPocMsb   = m_iPrevRefPocMsb;
  rpcPocCalculator->m_iPrevRefPocLsb   = m_iPrevRefPocLsb;
  rpcPocCalculator->m_iMaxPocLsb       = m_iMaxPocLsb;
  rpcPocCalculator->m_iFrameNumOffset  = m_iFrameNumOffset;
  rpcPocCalculator->m_iRefOffsetSum    = m_iRefOffsetSum;
  rpcPocCalculator->m_iPrevFrameNum    = m_iPrevFrameNum;

  return Err::m_nOK;
}


ErrVal PocCalculator::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal PocCalculator::calculatePoc( SliceHeader& rcSliceHeader )
{
  if( rcSliceHeader.getIdrFlag() )
  {
    RNOK( xInitSPS( rcSliceHeader.getSPS() ) );
  }

  switch( rcSliceHeader.getSPS().getPicOrderCntType() )
  {
  case 0:
    {
      //===== POC mode 0 =====
      Int iCurrPocMsb = m_iPrevRefPocMsb;
      Int iCurrPocLsb = rcSliceHeader.getPicOrderCntLsb();
      Int iDiffPocLsb = m_iPrevRefPocLsb - iCurrPocLsb;

      if( rcSliceHeader.getIdrFlag() )
      {
        iCurrPocMsb   = 0;
        iCurrPocLsb   = 0;
      }
      else if( iDiffPocLsb >= ( m_iMaxPocLsb >> 1 ) )
      {
        iCurrPocMsb  += m_iMaxPocLsb;
      }
      else if( -iDiffPocLsb > ( m_iMaxPocLsb >> 1 ) )
      {
        iCurrPocMsb  -= m_iMaxPocLsb;
      }
      if( rcSliceHeader.getNalRefIdc() )
      {
        m_iPrevRefPocMsb = iCurrPocMsb;
        m_iPrevRefPocLsb = iCurrPocLsb;
      }

			if( rcSliceHeader.getPicType() & TOP_FIELD )
			{
				rcSliceHeader.setTopFieldPoc( iCurrPocMsb + iCurrPocLsb );

				if( rcSliceHeader.getPicType() == FRAME )
				{
					rcSliceHeader.setBotFieldPoc( rcSliceHeader.getTopFieldPoc() + rcSliceHeader.getDeltaPicOrderCntBottom() );
				}
			}
			else
			{
				rcSliceHeader.setBotFieldPoc( iCurrPocMsb + iCurrPocLsb );
			}
    }
    break;
  case 1:
    {
      //===== POC mode 1 =====
      Int   iExpectedPoc    = 0;
      UInt  uiAbsFrameNum   = 0;

			//----- update parameters, set AbsFrameNum -----
			if( rcSliceHeader.getIdrFlag() )
			{
				m_iFrameNumOffset  = 0;
			}
			else if( m_iPrevFrameNum > (Int)rcSliceHeader.getFrameNum() )
      {
        m_iFrameNumOffset  += ( 1 << rcSliceHeader.getSPS().getLog2MaxFrameNum() );
	    }
      m_iPrevFrameNum       = rcSliceHeader.getFrameNum();
      if( rcSliceHeader.getSPS().getNumRefFramesInPicOrderCntCycle() )
      {
        uiAbsFrameNum       = m_iFrameNumOffset + m_iPrevFrameNum;
        if( uiAbsFrameNum > 0 && rcSliceHeader.getNalRefIdc() == 0 )
        {
          uiAbsFrameNum--;
        }
      }

      //--- get expected POC ---
      if( uiAbsFrameNum > 0 )
      {
        Int iPocCycleCount  = ( uiAbsFrameNum - 1 ) / rcSliceHeader.getSPS().getNumRefFramesInPicOrderCntCycle();
        Int iFrameNumCycle  = ( uiAbsFrameNum - 1 ) % rcSliceHeader.getSPS().getNumRefFramesInPicOrderCntCycle();
        iExpectedPoc        = iPocCycleCount * m_iRefOffsetSum;

        for( Int iIndex = 0; iIndex <= iFrameNumCycle; iIndex++ )
        {
          iExpectedPoc     += rcSliceHeader.getSPS().getOffsetForRefFrame( iIndex );
        }
      }
			else
			{
				iExpectedPoc = 0;
			}
      if( rcSliceHeader.getNalRefIdc() == 0 )
      {
        iExpectedPoc       += rcSliceHeader.getSPS().getOffsetForNonRefPic();
      }

			//----- set Poc -----
			if( rcSliceHeader.getPicType() & TOP_FIELD )
			{
				rcSliceHeader.setTopFieldPoc( iExpectedPoc + rcSliceHeader.getDeltaPicOrderCnt0() );

				if( rcSliceHeader.getPicType() == FRAME )
				{
					rcSliceHeader.setBotFieldPoc( rcSliceHeader.getTopFieldPoc() + rcSliceHeader.getDeltaPicOrderCnt1() + rcSliceHeader.getSPS().getOffsetForTopToBottomField() );
				}
			}
			else
			{
				rcSliceHeader.setBotFieldPoc( iExpectedPoc + rcSliceHeader.getDeltaPicOrderCnt0() + rcSliceHeader.getSPS().getOffsetForTopToBottomField() );
			}
    }
    break;
  case 2:
    {
      //===== POC mode 2 =====
      Int iCurrPoc;
			if( rcSliceHeader.getIdrFlag() )
			{
				m_iFrameNumOffset  = 0;
				iCurrPoc           = 0;
			}
			else
      {
        if( (Int)rcSliceHeader.getFrameNum() < m_iPrevFrameNum )
        {
          m_iFrameNumOffset += ( 1 << rcSliceHeader.getSPS().getLog2MaxFrameNum() );
        }
        if( rcSliceHeader.getNalRefIdc() )
        {
          iCurrPoc           = 2 * ( m_iFrameNumOffset + rcSliceHeader.getFrameNum() );
        }
        else
        {
          iCurrPoc           = 2 * ( m_iFrameNumOffset + rcSliceHeader.getFrameNum() ) - 1;
        }
      }
      m_iPrevFrameNum = rcSliceHeader.getFrameNum();

			if( rcSliceHeader.getPicType() == FRAME )
			{
				rcSliceHeader.setTopFieldPoc( iCurrPoc );
				rcSliceHeader.setBotFieldPoc( iCurrPoc );
			}
			else if ( rcSliceHeader.getPicType() == TOP_FIELD )
			{
				rcSliceHeader.setTopFieldPoc( iCurrPoc );
			}
			else
			{
				rcSliceHeader.setBotFieldPoc( iCurrPoc );
			}
    }
    break;
  default:
    RERR();
    break;
  }
  return Err::m_nOK;
}

ErrVal
PocCalculator::resetMMCO5( SliceHeader& rcSliceHeader )
{
  PicType ePicType    = rcSliceHeader.getPicType();
  Int     iTempPOC    = ( ePicType == TOP_FIELD ? rcSliceHeader.getTopFieldPoc() :
                          ePicType == BOT_FIELD ? rcSliceHeader.getBotFieldPoc() :
                          gMin( rcSliceHeader.getTopFieldPoc(), rcSliceHeader.getBotFieldPoc() ) );
  Int     iTopFldPoc  = rcSliceHeader.getTopFieldPoc() - iTempPOC;
  Int     iBotFldPoc  = rcSliceHeader.getBotFieldPoc() - iTempPOC;
  m_iPrevRefPocMsb    = 0;
  m_iPrevRefPocLsb    = ( ePicType == BOT_FIELD ? iTopFldPoc : 0 );
  m_iFrameNumOffset   = 0;
  m_iPrevFrameNum     = 0;
  rcSliceHeader.setTopFieldPoc( iTopFldPoc );
  rcSliceHeader.setBotFieldPoc( iBotFldPoc );
  return Err::m_nOK;
}

ErrVal
PocCalculator::xInitSPS( const SequenceParameterSet& rcSPS )
{
  switch( rcSPS.getPicOrderCntType() )
  {
  case 0:
    {
      m_iPrevRefPocMsb  = 0;
      m_iPrevRefPocLsb  = 0;
      m_iMaxPocLsb      = ( 1 << rcSPS.getLog2MaxPicOrderCntLsb() );
    }
    break;
  case 1:
    {
      m_iFrameNumOffset = 0;
      m_iRefOffsetSum   = 0;
      for( UInt uiIndex = 0; uiIndex < rcSPS.getNumRefFramesInPicOrderCntCycle(); uiIndex++ )
      {
        m_iRefOffsetSum+= rcSPS.getOffsetForRefFrame( uiIndex );
      }
    }
    break;
  case 2:
    {
      m_iFrameNumOffset = 0;
    }
    break;
  default:
    RERR();
    break;
  }
  return Err::m_nOK;
}


ErrVal PocCalculator::setPoc( SliceHeader&  rcSliceHeader,
                              Int           iContNumber )
{
  ROTRS( iContNumber > ( INT_MAX - 1 ), Err::m_nERR );

  if( rcSliceHeader.getIdrFlag() )
  {
    m_iBitsLsb          = rcSliceHeader.getSPS().getLog2MaxPicOrderCntLsb();
    m_iLastIdrFieldNum  = iContNumber;
  }

  Int iCurrPoc = iContNumber - m_iLastIdrFieldNum;

	if( rcSliceHeader.getPicType() & TOP_FIELD )
	{
		rcSliceHeader.setTopFieldPoc  ( iCurrPoc );

		if( rcSliceHeader.getPicType() == FRAME )
		{
			rcSliceHeader.setBotFieldPoc( rcSliceHeader.getTopFieldPoc() + ( rcSliceHeader.getPPS().getPicOrderPresentFlag() ? m_iTop2BotOffset : 0 ) );
			rcSliceHeader.setDeltaPicOrderCntBottom ( rcSliceHeader.getBotFieldPoc() - rcSliceHeader.getTopFieldPoc() );
		}
	}
	else
	{
		rcSliceHeader.setBotFieldPoc  ( iCurrPoc );
	}
  rcSliceHeader.setPicOrderCntLsb ( iCurrPoc & ~( -1 << m_iBitsLsb ) );

  return Err::m_nOK;
}


H264AVC_NAMESPACE_END


