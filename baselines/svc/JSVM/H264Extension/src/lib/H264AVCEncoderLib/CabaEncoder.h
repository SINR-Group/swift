
#if !defined(AFX_CABAENCODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
#define AFX_CABAENCODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BitWriteBufferIf.h"


H264AVC_NAMESPACE_BEGIN


class CabacContextModel;

class CabaEncoder
{
protected:
  CabaEncoder();
  virtual ~CabaEncoder();

public:
  ErrVal start();
  ErrVal getLastByte(UChar &uiLastByte, UInt &uiLastBitPos); //FIX_FRAG_CAVLC
  ErrVal setFirstBits(UChar ucByte,UInt uiLastBitPos); //FIX_FRAG_CAVLC
  ErrVal init( BitWriteBufferIf* pcBitWriteBufferIf );
  ErrVal uninit();

  ErrVal writeEPSymbol( UInt uiSymbol );
  ErrVal writeSymbol( UInt uiSymbol, CabacContextModel& rcCCModel );
  ErrVal writeUnaryMaxSymbol( UInt uiSymbol, CabacContextModel* pcCCModel, Int iOffset, UInt uiMaxSymbol );
  ErrVal writeUnarySymbol( UInt uiSymbol, CabacContextModel* pcCCModel, Int iOffset );

  ErrVal writeExGolombLevel( UInt uiSymbol, CabacContextModel& rcCCModel  );
  ErrVal writeEpExGolomb( UInt uiSymbol, UInt uiCount );
  ErrVal writeExGolombMvd( UInt uiSymbol, CabacContextModel* pcCCModel, UInt uiMaxBin );
  ErrVal writeTerminatingBit( UInt uiBit );
  ErrVal finish();
  UInt   getWrittenBits()  { return m_pcBitWriteBufferIf->getNumberOfWrittenBits() + 8 + m_uiBitsToFollow - m_uiBitsLeft + 1; } //JVT-P031

  Void   setStates  (CabaEncoder* pcExtEncoder )
  {
    m_pcBitWriteBufferIf  = pcExtEncoder->m_pcBitWriteBufferIf;
    m_uiRange             = pcExtEncoder->m_uiRange;
    m_uiLow               = pcExtEncoder->m_uiLow;
    m_uiByte              = pcExtEncoder->m_uiByte;
    m_uiBitsLeft          = pcExtEncoder->m_uiBitsLeft;
    m_uiBitsToFollow      = pcExtEncoder->m_uiBitsToFollow;
  }
  Void   getStates  (CabaEncoder* pcExtEncoder )
  {
    pcExtEncoder->m_pcBitWriteBufferIf  = m_pcBitWriteBufferIf;
    pcExtEncoder->m_uiRange             = m_uiRange;
    pcExtEncoder->m_uiLow               = m_uiLow;
    pcExtEncoder->m_uiByte              = m_uiByte;
    pcExtEncoder->m_uiBitsLeft          = m_uiBitsLeft;
    pcExtEncoder->m_uiBitsToFollow      = m_uiBitsToFollow;
  }
	BitWriteBufferIf* getBitWriteBufferIf(void){return m_pcBitWriteBufferIf;}//JVT-X046

private:
  __inline ErrVal xWriteBit( UInt uiBit);
  __inline ErrVal xWriteBitAndBitsToFollow( UInt uiBit);

protected:
  BitWriteBufferIf* m_pcBitWriteBufferIf;

  UInt m_uiRange;
  UInt m_uiLow;
  UInt m_uiByte;
  UInt m_uiBitsLeft;
  UInt m_uiBitsToFollow;
  Bool m_bTraceEnable;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_CABAENCODER_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
