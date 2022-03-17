
#if !defined(AFX_ABACCONTEXTMODEL2DBUFFER_H__230F593F_17F5_459A_B22A_79B6C74D56D9__INCLUDED_)
#define AFX_ABACCONTEXTMODEL2DBUFFER_H__230F593F_17F5_459A_B22A_79B6C74D56D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/CabacContextModel.h"

H264AVC_NAMESPACE_BEGIN

class H264AVCCOMMONLIB_API CabacContextModel2DBuffer
{
public:
  CabacContextModel2DBuffer( UInt uiSizeY, UInt uiSizeX );
  ~CabacContextModel2DBuffer();

  CabacContextModel& get( UInt uiY, UInt uiX )
  {
    AOT_DBG( uiY >= m_uiSizeY );
    AOT_DBG( uiX >= m_uiSizeX );
    return m_pcCContextModel[ uiY * m_uiSizeX + uiX];
  }
  CabacContextModel* get( UInt uiY )
  {
    AOT_DBG( uiY >= m_uiSizeY );
    return &m_pcCContextModel[ uiY * m_uiSizeX];
  }

  ErrVal initBuffer( Short* psCtxModel, Int iQp )
  {
    ROT( NULL == m_pcCContextModel );

    for( UInt n = 0; n < m_uiSizeY * m_uiSizeX; n++ )
    {
      m_pcCContextModel[n].init( psCtxModel + 2*n, iQp );
    }
    return Err::m_nOK;
  }
	//JVT-X046 {
	CabacContextModel* getCabacContextModel(void)
	{
		return m_pcCContextModel;
	}
	void setCabacContextModel(CabacContextModel* pcCContextModel)
	{
		m_pcCContextModel->set(pcCContextModel);
	}
  //JVT-X046 }
protected:
  CabacContextModel*  m_pcCContextModel;
  const UInt          m_uiSizeX;
  const UInt          m_uiSizeY;
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_ABACCONTEXTMODEL2DBUFFER_H__230F593F_17F5_459A_B22A_79B6C74D56D9__INCLUDED_)
