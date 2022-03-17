
#if !defined(AFX_WRITEYUVTOFILE_H__B25F5BA6_A016_4457_A617_0FE895AA14EC__INCLUDED_)
#define AFX_WRITEYUVTOFILE_H__B25F5BA6_A016_4457_A617_0FE895AA14EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "WriteYuvIf.h"
#include "LargeFile.h"

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


class H264AVCVIDEOIOLIB_API WriteYuvToFile : public WriteYuvIf
{
protected:
  WriteYuvToFile();
  virtual ~WriteYuvToFile();

public:
  static ErrVal create    ( WriteYuvToFile*&    rpcWriteYuv );
  ErrVal        destroy   ();

  ErrVal        init      ( const std::string&  rcFileName );
  ErrVal        uninit    ();

  ErrVal        writeFrame( const UChar*  pLum,
                            const UChar*  pCb,
                            const UChar*  pCr,
                            UInt          uiLumHeight,
                            UInt          uiLumWidth,
                            UInt          uiLumStride,
                            const UInt    rauiCropping[] );

protected:
  LargeFile m_cFile;
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

#endif // !defined(AFX_WRITEYUVTOFILE_H__B25F5BA6_A016_4457_A617_0FE895AA14EC__INCLUDED_)
