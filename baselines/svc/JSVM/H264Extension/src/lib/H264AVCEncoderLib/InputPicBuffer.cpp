
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"

#include "InputPicBuffer.h"


H264AVC_NAMESPACE_BEGIN


InputAccessUnit::InputAccessUnit( UInt        uiContFrameNumber,
                                  PicBuffer*  pcInputPicBuffer )
: m_uiContFrameNumber ( uiContFrameNumber )
, m_pcInputPicBuffer  ( pcInputPicBuffer  )
{
}

InputAccessUnit::~InputAccessUnit()
{
}




InputPicBuffer::InputPicBuffer()
: m_bInit ( false )
{
}

InputPicBuffer::~InputPicBuffer()
{
  uninit();
}

ErrVal
InputPicBuffer::create( InputPicBuffer*& rpcInputPicBuffer )
{
  rpcInputPicBuffer = new InputPicBuffer;
  ROF( rpcInputPicBuffer );
  return Err::m_nOK;
}

ErrVal
InputPicBuffer::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
InputPicBuffer::init()
{
  ROF( m_cInputUnitList.empty() );
  m_uiContFrameNumber = 0;
  m_bInit             = true;
  return Err::m_nOK;
}

ErrVal
InputPicBuffer::uninit()
{
  ROF( m_cInputUnitList.empty() );
  m_bInit = false;
  return Err::m_nOK;
}

Bool
InputPicBuffer::empty()
{
  return m_cInputUnitList.empty();
}

ErrVal
InputPicBuffer::add( PicBuffer* pcInputPicBuffer )
{
  ROF( m_bInit );

  //===== add to list =====
  InputAccessUnit* pcInputAccessUnit = new InputAccessUnit( m_uiContFrameNumber++, pcInputPicBuffer );
  m_cInputUnitList.push_back( pcInputAccessUnit );

  return Err::m_nOK;
}


InputAccessUnit*
InputPicBuffer::remove( UInt  uiContFrameNumber )
{
  InputAccessUnit*                      pcIAU = NULL;
  std::list<InputAccessUnit*>::iterator iIter = m_cInputUnitList.begin();
  std::list<InputAccessUnit*>::iterator iEnd  = m_cInputUnitList.end  ();
  for( ; iIter != iEnd; iIter++ )
  {
    if( (*iIter)->getContFrameNumber() == uiContFrameNumber )
    {
      pcIAU = *iIter;
      m_cInputUnitList.erase( iIter );
      break;
    }
  }
  return pcIAU;
}

H264AVC_NAMESPACE_END



