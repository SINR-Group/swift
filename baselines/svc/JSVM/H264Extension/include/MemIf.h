
#if !defined(AFX_MEMIF_H__3B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_)
#define AFX_MEMIF_H__3B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>

enum MemType
{
  MEM_CONT =      0,
  MEM_LIST =      1
};

template< class T >
class MemIf
{
public:
  MemIf() {}
  virtual ~MemIf() {}

public:
  virtual MemType getMemType() const = 0;

  virtual Void set( MemIf< T >& rcMemIf ) = 0;
  virtual Void set( T* pcT, UInt uiSize, T* pcDeleteT=NULL, UInt uiUsableSize=0 ) = 0;

  virtual Void release( T*& rpcT, UInt& ruiSize ) = 0;
  virtual Void release( T*& rpcT, UInt& ruiSize, T*& rpcDeleteT, UInt& ruiUsableSize ) = 0;

  virtual Void deleteData() = 0;

  virtual UInt size() const = 0;
  virtual UInt byteSize() const = 0;
};


#endif // !defined(AFX_MEMIF_H__3B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_)
