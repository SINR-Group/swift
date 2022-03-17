
#if !defined _INPUT_PIC_BUFFER_INCLUDED_
#define      _INPUT_PIC_BUFFER_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN


class InputAccessUnit
{
public:
  InputAccessUnit   ( UInt        uiContFrameNumber,
                      PicBuffer*  pcInputPicBuffer );
  ~InputAccessUnit  ();

  UInt        getContFrameNumber  ()  const   { return m_uiContFrameNumber; }
  PicBuffer*  getInputPicBuffer   ()          { return m_pcInputPicBuffer; }
private:
  UInt        m_uiContFrameNumber;
  PicBuffer*  m_pcInputPicBuffer;
};



class InputPicBuffer
{
public:
  InputPicBuffer            ();
  virtual ~InputPicBuffer   ();

  static ErrVal     create  ( InputPicBuffer*&  rpcInputPicBuffer );
  ErrVal            destroy ();

  ErrVal            init    ();
  ErrVal            uninit  ();

  Bool              empty   ();
  ErrVal            add     ( PicBuffer*        pcInputPicBuffer );
  InputAccessUnit*  remove  ( UInt              uiContFrameNumber );

private:
  Bool                        m_bInit;
  std::list<InputAccessUnit*> m_cInputUnitList;
  UInt                        m_uiContFrameNumber;
};

H264AVC_NAMESPACE_END

#endif  // _INPUT_PIC_BUFFER_INCLUDED_

