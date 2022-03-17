
#if !defined(AFX_SLICEREADER_H__5B23143A_D267_40C2_908E_164029C1298E__INCLUDED_)
#define AFX_SLICEREADER_H__5B23143A_D267_40C2_908E_164029C1298E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/HeaderSymbolReadIf.h"
#include "H264AVCCommonLib/ControlMngIf.h"
#include "NalUnitParser.h"

H264AVC_NAMESPACE_BEGIN

class MbParser;
class ParameterSetMng;
class MbStatus;

class SliceReader
{
protected:
	SliceReader();
	virtual ~SliceReader();

public:
  static ErrVal create  ( SliceReader*& rpcSliceReader );
  ErrVal        destroy ();
  ErrVal        init    ( MbParser*     pcMbParser );
  ErrVal        uninit  ();

  ErrVal        read    ( SliceHeader&  rcSH,
                          MbDataCtrl*   pcMbDataCtrl,
                          MbStatus*     pacMbStatus,
                          UInt          uiMbInRow,
                          UInt&         ruiMbRead );

protected:
  Bool      m_bInitDone;
  MbParser* m_pcMbParser;
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_SLICEREADER_H__5B23143A_D267_40C2_908E_164029C1298E__INCLUDED_)
