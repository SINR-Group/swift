
#if !defined(AFX_NALUNITPARSER_H__D5B74729_6F04_42E9_91AE_2E28937F9F3A__INCLUDED_)
#define AFX_NALUNITPARSER_H__D5B74729_6F04_42E9_91AE_2E28937F9F3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



H264AVC_NAMESPACE_BEGIN


class BitReadBuffer;


class NalUnitParser : public NalUnitHeader
{
protected:
  NalUnitParser                   ();
  virtual ~NalUnitParser          ();

public:
  static ErrVal   create          ( NalUnitParser*&     rpcNalUnitParser  );
  ErrVal          destroy         ();
  ErrVal          init            ( BitReadBuffer*      pcBitReadBuffer,
                                    HeaderSymbolReadIf* pcHeaderSymbolReadIf );
  ErrVal          uninit          ();

  ErrVal          initNalUnit     ( BinDataAccessor&    rcBinDataAccessor,
                                    Int                 iDQId = MSYS_INT_MIN );
  ErrVal          closeNalUnit    ( Bool                bCheckEndOfNalUnit = true );

private:
  ErrVal          xInitSODB       ( BinDataAccessor&    rcBinDataAccessor,
                                    Bool                bTrailingBits );

private:
  Bool                m_bInitialized;
  Bool                m_bNalUnitInitialized;
  BitReadBuffer*      m_pcBitReadBuffer;
  HeaderSymbolReadIf* m_pcHeaderSymbolReadIf;
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_NALUNITPARSER_H__D5B74729_6F04_42E9_91AE_2E28937F9F3A__INCLUDED_)
