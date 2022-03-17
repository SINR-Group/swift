
#if !defined(AFX_CABADECODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
#define AFX_CABADECODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class BitReadBuffer;
class CabacContextModel;

class CabaDecoder
{
protected:
  CabaDecoder();
  virtual ~CabaDecoder();

public:
  ErrVal init   ( BitReadBuffer* pcBitReadBuffer );
  ErrVal start  ();
  ErrVal finish ();
  ErrVal uninit ();

  ErrVal getSymbol            ( UInt& ruiSymbol, CabacContextModel& rcCCModel );
  ErrVal getEpSymbol          ( UInt& ruiSymbol );
  ErrVal getEpExGolomb        ( UInt& ruiSymbol, UInt uiCount );
  ErrVal getExGolombLevel     ( UInt& ruiSymbol, CabacContextModel& rcCCModel  );
  ErrVal getExGolombMvd       ( UInt& ruiSymbol, CabacContextModel* pcCCModel, UInt uiMaxBin );
  ErrVal getTerminateBufferBit( UInt& ruiBit );
  ErrVal getUnaryMaxSymbol    ( UInt& ruiSymbol, CabacContextModel* pcCCModel, Int iOffset, UInt uiMaxSymbol );
  ErrVal getUnarySymbol       ( UInt& ruiSymbol, CabacContextModel* pcCCModel, Int iOffset );

private:
  __inline ErrVal xReadBit( UInt& ruiValue );

protected:
  BitReadBuffer*  m_pcBitReadBuffer;
  UInt            m_uiRange;
  UInt            m_uiValue;
  UInt            m_uiWord;
  UInt            m_uiBitsLeft;
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_CABADECODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
