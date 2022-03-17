
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CabacContextModel2DBuffer.h"

H264AVC_NAMESPACE_BEGIN

CabacContextModel2DBuffer::CabacContextModel2DBuffer( UInt uiSizeY, UInt uiSizeX ) :
  m_pcCContextModel( NULL ),
  m_uiSizeX( uiSizeX ),
  m_uiSizeY( uiSizeY )
{
  AOF_DBG( uiSizeY );
  AOF_DBG( uiSizeX );
  m_pcCContextModel = new CabacContextModel[ m_uiSizeY * m_uiSizeX ];
}


CabacContextModel2DBuffer::~CabacContextModel2DBuffer()
{
  delete [] m_pcCContextModel;
  m_pcCContextModel = NULL;
}

H264AVC_NAMESPACE_END






