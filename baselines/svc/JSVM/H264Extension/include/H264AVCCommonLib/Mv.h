
#if !defined(AFX_MV_H__91E1E1F3_0D6C_4222_9D71_EAB2E415688C__INCLUDED_)
#define AFX_MV_H__91E1E1F3_0D6C_4222_9D71_EAB2E415688C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



H264AVC_NAMESPACE_BEGIN


class H264AVCCOMMONLIB_API Mv
{
public:
	__inline const UInt xGetFilter() const;

  Mv()
    : m_sHor( 0 ),
      m_sVer( 0 )
  {}
  Mv( Short sHor, Short sVer )
    : m_sHor( sHor ),
      m_sVer( sVer )
  {}

  Short getHor()                const           { return m_sHor; }
  Short getVer()                const           { return m_sVer; }
  Short getAbsHor()             const           { return abs( m_sHor ); }
  Short getAbsVer()             const           { return abs( m_sVer ); }

  Void  setZero()                               { m_sHor = m_sVer = 0; }
  Void  setHor( Short sHor )                    { m_sHor = sHor; }
  Void  setVer( Short sVer )                    { m_sVer = sVer; }
  Void  set( Short sHor, Short sVer )           { m_sHor = sHor; m_sVer = sVer; }

  Short getAbsHorDiff( const Mv& rcMv )  const  { return abs( m_sHor - rcMv.m_sHor ); }
  Short getAbsVerDiff( const Mv& rcMv )  const  { return abs( m_sVer - rcMv.m_sVer ); }

  Short getAbsMvDiff( const Mv& rcMv) const {return abs( m_sHor - rcMv.m_sHor )+ abs( m_sVer - rcMv.m_sVer ); }

  const UInt  getFilter() const { return (m_sHor & 3) + (m_sVer & 3)*4;}
  const Mv operator + ( const Mv& rcMv ) const
  {
    return Mv( m_sHor + rcMv.m_sHor, m_sVer + rcMv.m_sVer );
  }
  const Mv operator - ( const Mv& rcMv ) const
  {
    return Mv( m_sHor - rcMv.m_sHor, m_sVer - rcMv.m_sVer );
  }
  const Mv& operator += ( const Mv& rcMv )
  {
    m_sHor += rcMv.m_sHor;
    m_sVer += rcMv.m_sVer;
    return *this;
  }
  const Mv& operator -= ( const Mv& rcMv )
  {
    m_sHor -= rcMv.m_sHor;
    m_sVer -= rcMv.m_sVer;
    return *this;
  }

  const Mv operator << ( Short sShift ) const
  {
    return Mv( m_sHor << sShift, m_sVer << sShift );
  }
  const Mv operator >> ( Short sShift ) const
  {
    return Mv( m_sHor >> sShift, m_sVer >> sShift );
  }

  const Mv& operator <<= ( Short sShift )
  {
    m_sHor <<= sShift;
    m_sVer <<= sShift;
    return *this;
  }
  const Mv& operator >>= ( Short sShift )
  {
    m_sHor >>= sShift;
    m_sVer >>= sShift;
    return *this;
  }

  Bool operator == ( const Mv& rcMv ) const
  {
    return ( m_sHor == rcMv.m_sHor && m_sVer == rcMv.m_sVer );
  }
  Bool operator != ( const Mv& rcMv ) const
  {
    return ( m_sHor != rcMv.m_sHor || m_sVer != rcMv.m_sVer );
  }

  const Mv& setFrameToFieldPredictor()
  {
    m_sVer /= 2;
    return *this;
  }
  const Mv& setFieldToFramePredictor()
  {
    m_sVer *= 2;
    return *this;
  }

  Void limitComponents( const Mv& rcMvMin, const Mv& rcMvMax )
  {
    m_sHor = gMin( rcMvMax.m_sHor, gMax( rcMvMin.m_sHor, m_sHor ) );
    m_sVer = gMin( rcMvMax.m_sVer, gMax( rcMvMin.m_sVer, m_sVer ) );
  }

  static const Mv& ZeroMv() { return m_cMvZero; }

  const Mv scaleMv( Int iScale )
  {
    return Mv( (iScale * getHor() + 128) >> 8, (iScale * getVer() + 128) >> 8);
  }

public:
	static const Mv* m_cMvZero1;
  static const Mv m_cMvZero;

  Short m_sHor;
  Short m_sVer;
};




enum RefIdxValues
{
  BLOCK_NOT_AVAILABLE =  0,
  BLOCK_NOT_PREDICTED = -1
};



class H264AVCCOMMONLIB_API Mv3D :
public Mv
{
public:
  Mv3D()
    : Mv     (),
      m_scRef( BLOCK_NOT_AVAILABLE )
  {}
  Mv3D( const Mv& rcMv, SChar scRef )
    : Mv     ( rcMv ),
      m_scRef( scRef )
  {}
  Mv3D( Short sHor, Short sVer, SChar scRef )
    : Mv     ( sHor, sVer ),
      m_scRef( scRef )
  {}

  SChar getRef()                const               { return m_scRef; }

  Void  setZero()                                   { Mv::setZero(); m_scRef = BLOCK_NOT_AVAILABLE; }
  Void  setRef( SChar scRef )                       { m_scRef = scRef; }
  Void  setMv( const Mv& rcMv )                     { Mv::operator= ( rcMv); }
  Void  set( Short sHor, Short sVer, SChar scRef )  { Mv::set( sHor, sVer); m_scRef = scRef; }
  Void  set( const Mv& rcMv, SChar scRef )          { Mv::operator= ( rcMv ); m_scRef = scRef; }

  const Mv3D& setFrameToFieldPredictor()
  {
    Mv::setFrameToFieldPredictor();
    if( m_scRef > BLOCK_NOT_AVAILABLE)
    {
      m_scRef = ((m_scRef-1)<<1)+1;
    }
    return *this;
  }
  const Mv3D& setFieldToFramePredictor()
  {
    Mv::setFieldToFramePredictor();
    if( m_scRef > BLOCK_NOT_AVAILABLE)
    {
      m_scRef = ((m_scRef-1)>>1)+1;
    }
    return *this;
  }
  Mv3D& minRefIdx( Mv3D& rcMv3D )  { return (((UChar)(rcMv3D.getRef()-1)) < ((UChar)(getRef()-1)) ? rcMv3D : *this); }
  Bool operator== ( const RefIdxValues eRefIdxValues )  { return ( eRefIdxValues == m_scRef ); }
  Bool operator!= ( const RefIdxValues eRefIdxValues )  { return ( eRefIdxValues != m_scRef ); }

  operator const SChar()                                { return m_scRef; }

  const Mv3D& operator= (const Mv& rcMv)  { set( rcMv.getHor(), rcMv.getVer(), BLOCK_NOT_AVAILABLE); return *this; }

private:
  SChar m_scRef;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MV_H__91E1E1F3_0D6C_4222_9D71_EAB2E415688C__INCLUDED_)
