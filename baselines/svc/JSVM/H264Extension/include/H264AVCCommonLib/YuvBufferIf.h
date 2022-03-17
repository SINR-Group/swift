
#if !defined(AFX_YUVBUFFERRWIF_H__E94AE8DA_A40D_4411_8226_A471358FDCDD__INCLUDED_)
#define AFX_YUVBUFFERRWIF_H__E94AE8DA_A40D_4411_8226_A471358FDCDD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN

class H264AVCCOMMONLIB_API YuvBufferIf
{
protected:
  YuvBufferIf() {}
  virtual ~YuvBufferIf() {}
public:
  virtual const Int getCStride()   const  = 0;
  virtual Pel* getLumBlk()   = 0;
  virtual Pel* getYBlk( LumaIdx cIdx ) = 0;
  virtual Pel* getUBlk( LumaIdx cIdx ) = 0;
  virtual Pel* getVBlk( LumaIdx cIdx ) = 0;
  virtual const Int getLStride()    const = 0;

  virtual Pel* getMbLumAddr() = 0;
  virtual Pel* getMbCbAddr()  = 0;
  virtual Pel* getMbCrAddr()  = 0;

  virtual Void set4x4Block( LumaIdx cIdx ) = 0;

  virtual const Int getLWidth()     const = 0;
  virtual const Int getLHeight()    const = 0;
  virtual const Int getLXMargin()   const = 0;
  virtual const Int getLYMargin()   const = 0;
  virtual const Int getCWidth()     const = 0;
  virtual const Int getCHeight()    const = 0;
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_YUVBUFFERRWIF_H__E94AE8DA_A40D_4411_8226_A471358FDCDD__INCLUDED_)
