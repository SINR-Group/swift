
#ifndef _SCALABLE_MODIFY_CODE_
#define _SCALABLE_MODIFY_CODE_

#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/Sei.h"

class ScalableSEIModifyCode
{
protected:
	ScalableSEIModifyCode();
	~ScalableSEIModifyCode()	{}

public:
  static ErrVal Create         ( ScalableSEIModifyCode*& rpcScalableSEIModifyCode );
	ErrVal Destroy               ();
	ErrVal init                  ( UInt* pulStream, UInt uiBytes );
	ErrVal Uninit                ();
  ErrVal Write                 ( UInt uiBits, UInt uiNumberOfBits );
	ErrVal WriteUVLC             ( UInt uiValue );
	ErrVal WriteFlag             ( Bool bFlag );
	ErrVal WriteCode             ( UInt uiValue, UInt uiLength );
	ErrVal WriteAlignZero        ();
	ErrVal flushBuffer           ();
	UInt	 getNumberOfWrittenBits() { return m_uiBitsWritten; }
	ErrVal WritePayloadHeader    ( enum h264::SEI::MessageType eType, UInt uiSize );
	ErrVal WriteTrailingBits     ();
	ErrVal ConvertRBSPToPayload  ( UChar* m_pucBuffer, UChar pucStreamPacket[], UInt& uiBits, UInt uiHeaderBytes );

	ErrVal SEICode	( h264::SEI::ScalableSei* pcScalableSei, ScalableSEIModifyCode *pcScalableCodeIf );
	ErrVal SEICode	( h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent, ScalableSEIModifyCode *pcScalableCodeIf );
	ErrVal SEICode	( h264::SEI::ScalableSeiDependencyChange* pcScalableSeiDependencyChange, ScalableSEIModifyCode *pcScalableCodeIf );

protected:
	UInt  xSwap( UInt ul )
	{
		// heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
#ifdef MSYS_BIG_ENDIAN
		return ul;
#else
		UInt ul2;

		ul2  = ul>>24;
		ul2 |= (ul>>8) & 0x0000ff00;
		ul2 |= (ul<<8) & 0x00ff0000;
		ul2 |= ul<<24;

		return ul2;
#endif
	}

private:
	BinData *m_pcBinData;
	UInt *m_pulStreamPacket;
	UInt m_uiBitCounter;
	UInt m_uiPosCounter;
	UInt m_uiDWordsLeft;
	UInt m_uiBitsWritten;
	UInt m_iValidBits;
	UInt m_ulCurrentBits;
	UInt m_uiCoeffCost;
};

#endif




