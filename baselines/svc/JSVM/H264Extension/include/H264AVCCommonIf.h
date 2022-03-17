
#if !defined(AFX_H264AVCCOMMONIF_H__625AA7B6_0241_4166_8D3A_BC831985BE5F__INCLUDED_)
#define AFX_H264AVCCOMMONIF_H__625AA7B6_0241_4166_8D3A_BC831985BE5F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if defined( WIN32 )
# if !defined( MSYS_WIN32 )
#   define MSYS_WIN32
# endif
#endif

#if defined( _DEBUG ) || defined( DEBUG )
# if !defined( _DEBUG )
#   define _DEBUG
# endif
# if !defined( DEBUG )
#   define DEBUG
# endif
#endif


typedef int ErrVal;

class Err
{
public:
  static const ErrVal m_nOK;
  static const ErrVal m_nERR;
  static const ErrVal m_nEndOfStream;
  static const ErrVal m_nEndOfFile;
  static const ErrVal m_nEndOfBuffer;
  static const ErrVal m_nInvalidParameter;
  static const ErrVal m_nDataNotAvailable;
};

#include <assert.h>
#include <iostream>
#include "Typedefs.h"
#include "Macros.h"
#include "MemList.h"


typedef MemCont< UChar > BinData;
typedef MemAccessor< UChar > BinDataAccessor;

#include <list>
#include <algorithm>

#define gMin(x,y) ((x)<(y)?(x):(y))
#define gMax(x,y) ((x)>(y)?(x):(y))

typedef UChar   Pel;
typedef Short   XPel;
typedef Int     XXPel;

class TCoeff
{
public:
  TCoeff( Int iVal = 0 ) : m_iCoeffValue( iVal ), m_iLevelValue ( iVal ), m_sPred( 0 ) {}
  ~TCoeff()                                                                            {}

  operator Int() const { return m_iCoeffValue; }

  TCoeff &operator=  ( const Int iVal )      { m_iCoeffValue =  iVal; return *this; }
  TCoeff &operator+= ( const Int iVal )      { m_iCoeffValue += iVal; return *this; }
  TCoeff &operator-= ( const Int iVal )      { m_iCoeffValue -= iVal; return *this; }
  TCoeff &operator*= ( const Int iVal )      { m_iCoeffValue *= iVal; return *this; }
  TCoeff &operator/= ( const Int iVal )      { m_iCoeffValue /= iVal; return *this; }

  Int   getLevel()            const          { return m_iLevelValue; }
  Int   getCoeff()            const          { return m_iCoeffValue; }
  Short getSPred()            const          { return m_sPred;       }
  Void  setLevel( Int  iVal )                { m_iLevelValue = iVal; }
  Void  setCoeff( Int  iVal )                { m_iCoeffValue = iVal; }
  Void  setSPred( XPel sVal )                { m_sPred       = sVal; }

private:
  Int   m_iCoeffValue; // more than 16 bit required for chroma DC
  Int   m_iLevelValue; // more than 16 bit required for chroma DC
  XPel  m_sPred;
};

template< class T >
class MyList : public std::list< T >
{
public:
  typedef typename std::list<T>::iterator MyIterator;

  MyList& operator += ( const MyList& rcMyList) { if( ! rcMyList.empty() ) { insert( this->end(), rcMyList.begin(), rcMyList.end());} return *this; } // leszek
  T popBack()                           { T cT = this->back(); this->pop_back(); return cT;  }
  T popFront()                          { T cT = this->front(); this->pop_front(); return cT; }
  Void pushBack( const T& rcT )         { if( sizeof(T) == sizeof(void*) ) { if( rcT != NULL ){ push_back( rcT);} } }
  Void pushFront( const T& rcT )        { if( sizeof(T) == sizeof(void*) ) { if( rcT != NULL ){ push_front( rcT);} } }
  MyIterator find( const T& rcT ) {  return std::find( this->begin(), this->end(), rcT ); } // leszek
};

class ExtBinDataAccessor : public BinDataAccessor
{
public:
  ExtBinDataAccessor() : BinDataAccessor() , m_pcMediaPacket (NULL ){}
  ExtBinDataAccessor( BinDataAccessor& rcAccessor, Void* pcMediaPacket = NULL )
    :  BinDataAccessor( rcAccessor )
    ,  m_pcMediaPacket (pcMediaPacket ){}

  Void* getMediaPacket() { return m_pcMediaPacket; }
private:
  Void* m_pcMediaPacket;
};


typedef MyList< ExtBinDataAccessor* > ExtBinDataAccessorList;
typedef MyList< ExtBinDataAccessorList* > ExtBinDataAccessorListList;

enum PicStruct
{
  PS_NOT_SPECIFIED = -1,
  PS_FRAME         =  0, // frame	field_pic_flag shall be 0	1
  PS_TOP           =  1, // top field	field_pic_flag shall be 1, bottom_field_flag shall be 0 1
  PS_BOT           =  2, // bottom field field_pic_flag shall be 1, bottom_field_flag shall be 1 1
  PS_TOP_BOT       =  3, // top field, bottom field, in that order field_pic_flag shall be 0 2
  PS_BOT_TOP       =  4, // bottom field, top field, in that order field_pic_flag shall be 0 2
  PS_TOP_BOT_TOP   =  5,
  PS_BOT_TOP_BOT   =  6,
  PS_FRM_DOUBLING  =  7,
  PS_FRM_TRIPLING  =  8,
};

class PicBuffer
{
public:
	PicBuffer( Pel* pcBuffer = NULL, Void* pcMediaPacket  = NULL, Int64 i64Cts = 0)
		: m_pcMediaPacket( pcMediaPacket )
		, m_pcBuffer     ( pcBuffer )
		, m_iInUseCout   ( 0 )
		, m_i64Cts       ( i64Cts )
    , m_ePicStruct   ( PS_NOT_SPECIFIED )
    , m_iTopPoc      ( 0 )
    , m_iBotPoc      ( 0 )
    , m_iFramePoc    ( 0 )
    , m_uiIdrPicId   ( 0 )
    , m_bFieldCoded  ( false )
	{}

  Pel*  switchBuffer( Pel* pcBuffer ) { Pel* pcTmp = m_pcBuffer; m_pcBuffer = pcBuffer; return pcTmp; }

  Void setUnused() { m_iInUseCout--; }
  Void setUsed()   { m_iInUseCout++; }
  Bool isUsed()    { return 0 != m_iInUseCout; }
  Pel* getBuffer() { return m_pcBuffer; }
  operator Pel*()  { return m_pcBuffer; }
  Void* getMediaPacket() { return m_pcMediaPacket; }
  Int64& getCts()            { return m_i64Cts; }
  Void setCts( Int64 i64 )   { m_i64Cts = i64; } // HS: decoder robustness

	Void setPicStruct     ( PicStruct e  ) { m_ePicStruct  = e;  }
	Void setFieldCoding   ( Bool      b  ) { m_bFieldCoded = b;  }
  Void setIdrPicId      ( UInt      ui ) { m_uiIdrPicId  = ui; }
  Void setTopPoc        ( Int       i  ) { m_iTopPoc     = i;  }
  Void setBotPoc        ( Int       i  ) { m_iBotPoc     = i;  }
  Void setFramePoc      ( Int       i  ) { m_iFramePoc   = i;  }

	PicStruct getPicStruct() const  { return m_ePicStruct; }
  Bool isFieldCoded     () const  { return m_bFieldCoded; }
  UInt getIdrPicId      () const  { return m_uiIdrPicId; }
  Int  getTopPoc        () const  { return m_iTopPoc; }
  Int  getBotPoc        () const  { return m_iBotPoc; }
  Int  getFramePoc      () const  { return m_iFramePoc; }

private:
  Void*     m_pcMediaPacket;
  Pel*      m_pcBuffer;
  Int       m_iInUseCout;
	Int64			m_i64Cts;
	PicStruct m_ePicStruct;
  Int       m_iTopPoc;
  Int       m_iBotPoc;
  Int       m_iFramePoc;
  UInt      m_uiIdrPicId;
  Bool      m_bFieldCoded;
};

typedef MyList< PicBuffer* > PicBufferList;
typedef MyList< BinData*   > BinDataList;


#endif // !defined(AFX_H264AVCCOMMONIF_H__625AA7B6_0241_4166_8D3A_BC831985BE5F__INCLUDED_)
