
#if !defined(AFX_HEADERSYMBOLWRITEIF_H__6CF9086D_DB71_40C0_91BD_6F205082CB8C__INCLUDED_)
#define AFX_HEADERSYMBOLWRITEIF_H__6CF9086D_DB71_40C0_91BD_6F205082CB8C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class HeaderSymbolWriteIf
{
protected:
  HeaderSymbolWriteIf()  {}
  virtual ~HeaderSymbolWriteIf() {}

public:
  virtual HeaderSymbolWriteIf* getHeaderSymbolWriteIfNextSlice( Bool bStartNewBitstream ) = 0;

  virtual ErrVal writeUvlc ( UInt uiCode,                const Char* pcTraceString ) = 0;
  virtual ErrVal writeSvlc ( Int iCode,                  const Char* pcTraceString ) = 0;
  virtual ErrVal writeCode ( UInt uiCode, UInt uiLength, const Char* pcTraceString ) = 0;
  virtual ErrVal writeSCode( Int iCode,   UInt uiLength, const Char* pcTraceString ) = 0;
  virtual ErrVal writeFlag ( Bool bFlag,                 const Char* pcTraceString ) = 0;
  virtual UInt   getNumberOfWrittenBits () = 0;
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_HEADERSYMBOLWRITEIF_H__6CF9086D_DB71_40C0_91BD_6F205082CB8C__INCLUDED_)
