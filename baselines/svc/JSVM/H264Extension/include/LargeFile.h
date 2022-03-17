
#if !defined(AFX_LARGEFILE_H__7E94650C_CCB2_4248_AEF5_74966C842261__INCLUDED_)
#define AFX_LARGEFILE_H__7E94650C_CCB2_4248_AEF5_74966C842261__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined( MSYS_LINUX )
# if (!defined( MSYS_UNIX_LARGEFILE )) || (!defined(_LARGEFILE64_SOURCE) )
#  error Large file support requires MSYS_UNIX_LARGEFILE and _LARGEFILE64_SOURCE defined
# endif
#endif

class H264AVCVIDEOIOLIB_API LargeFile
{
public:
  enum OpenMode
  {
    OM_READONLY,
    OM_WRITEONLY,
    OM_APPEND,
    OM_READWRITE
  };

public:
  LargeFile();
  ~LargeFile();

  ErrVal open( const std::string& rcFilename, enum OpenMode eOpenMode, int iPermMode=0777 );

  bool is_open() const { return -1 != m_iFileHandle; }

  ErrVal close();
  ErrVal seek( Int64 iOffset, int iOrigin );
  Int64 tell();

  ErrVal read( Void *pvBuffer, UInt32 uiCount, UInt32& ruiBytesRead );
  ErrVal write( const Void *pvBuffer, UInt32 uiCount );

  Int getFileHandle() { return m_iFileHandle; }

private:
  Int m_iFileHandle;
};



#endif // !defined(AFX_LARGEFILE_H__7E94650C_CCB2_4248_AEF5_74966C842261__INCLUDED_)
