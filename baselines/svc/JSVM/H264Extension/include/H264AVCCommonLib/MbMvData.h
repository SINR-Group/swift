
#if !defined(AFX_MBMVDATA_H__06960F25_0FB8_4A65_935D_B06282FFDF6E__INCLUDED_)
#define AFX_MBMVDATA_H__06960F25_0FB8_4A65_935D_B06282FFDF6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class Frame;

class H264AVCCOMMONLIB_API MbMvData
{
public:
  Void copyFrom( const MbMvData& rcMbMvData, const ParIdx8x8 eParIdx );
  Void copyFrom( const MbMvData& rcMbMvData );

  MbMvData()
    : m_bFieldFlag( false )
  {
    clear();
  }

  Void clear()
  {
    m_acMv[ 0 ].setZero();
    m_acMv[ 1 ].setZero();
    m_acMv[ 2 ].setZero();
    m_acMv[ 3 ].setZero();
    m_acMv[ 4 ].setZero();
    m_acMv[ 5 ].setZero();
    m_acMv[ 6 ].setZero();
    m_acMv[ 7 ].setZero();
    m_acMv[ 8 ].setZero();
    m_acMv[ 9 ].setZero();
    m_acMv[10 ].setZero();
    m_acMv[11 ].setZero();
    m_acMv[12 ].setZero();
    m_acMv[13 ].setZero();
    m_acMv[14 ].setZero();
    m_acMv[15 ].setZero();
  }

  Void clear( ParIdx8x8 eParIdx )
  {
    Mv* pcMv = &m_acMv[ eParIdx ];
    pcMv[ 0 ].setZero();
    pcMv[ 1 ].setZero();
    pcMv[ 4 ].setZero();
    pcMv[ 5 ].setZero();
  }

  Void  setFirstMv( const Mv& rcMv )                                          { m_acMv[ 0 ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx16x8 eParIdx  )                     { m_acMv[ eParIdx ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx8x16 eParIdx  )                     { m_acMv[ eParIdx ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx8x8  eParIdx  )                     { m_acMv[ eParIdx ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx ) { m_acMv[ eParIdx+eSParIdx ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx ) { m_acMv[ eParIdx+eSParIdx ] = rcMv; }
  Void  setFirstMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx ) { m_acMv[ eParIdx+eSParIdx ] = rcMv; }

  Void  setAllMv( const Mv& rcMv );
  Void  setAllMv( const Mv& rcMv, ParIdx16x8 eParIdx  );
  Void  setAllMv( const Mv& rcMv, ParIdx8x16 eParIdx  );
  Void  setAllMv( const Mv& rcMv, ParIdx8x8  eParIdx  );
  Void  setAllMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx );
  Void  setAllMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx );
  Void  setAllMv( const Mv& rcMv, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx );

  Void  setMv( const Mv& rcMv, LumaIdx cIdx );

  const Mv& getMv()                                           const { return m_acMv[ 0 ]; }
  const Mv& getMv( ParIdx16x8 eParIdx   )                     const { return m_acMv[ eParIdx ]; }
  const Mv& getMv( ParIdx8x16 eParIdx   )                     const { return m_acMv[ eParIdx ]; }
  const Mv& getMv( ParIdx8x8  eParIdx   )                     const { return m_acMv[ eParIdx ]; }
  const Mv& getMv( ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx )  const { return m_acMv[ eParIdx + eSParIdx ]; }
  const Mv& getMv( ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx )  const { return m_acMv[ eParIdx + eSParIdx ]; }
  const Mv& getMv( ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx )  const { return m_acMv[ eParIdx + eSParIdx ]; }
  const Mv& getMv( LumaIdx cIdx )                             const { return m_acMv[ cIdx.b4x4() ]; }

  ErrVal  save( FILE* pFile );
  ErrVal  load( FILE* pFile );

protected:
  Mv    m_acMv[16];
  Bool  m_bFieldFlag;
};


#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


class H264AVCCOMMONLIB_API RefPicIdc
{
public:
  RefPicIdc() : m_iPoc(0), m_ePicType(NOT_SPECIFIED), m_pcFrame(0) {}

  Void  set( const Frame* pcFrame );

  Bool          isValid ()  const;
  const Frame*  getPic  ()  const;

  Bool  operator==( const RefPicIdc& rcRefPicIdc )  const { return ( ( m_iPoc == rcRefPicIdc.m_iPoc ) && ( m_ePicType == rcRefPicIdc.m_ePicType ) ); }
  Bool  operator!=( const RefPicIdc& rcRefPicIdc )  const { return ( ( m_iPoc != rcRefPicIdc.m_iPoc ) || ( m_ePicType != rcRefPicIdc.m_ePicType ) ); }

  const RefPicIdc& operator=( const RefPicIdc& rcRefPicIdc ) { m_iPoc = rcRefPicIdc.m_iPoc; m_ePicType = rcRefPicIdc.m_ePicType; m_pcFrame = rcRefPicIdc.m_pcFrame; return *this; }

private:
  Int           m_iPoc;
  PicType       m_ePicType;
  const Frame*  m_pcFrame;
};


class H264AVCCOMMONLIB_API MbMotionData : public MbMvData
{
public:
  Void copyFrom( const MbMotionData& rcMbMotionData, const ParIdx8x8  eParIdx  );
  Void copyFrom( const MbMotionData& rcMbMotionData );

  BlkMode getBlkMode( const ParIdx8x8 eParIdx, BlkMode eBlkMode );

  MbMotionData()
    : MbMvData        ()
    , m_usMotPredFlags( 0x0000 )
  {
    m_ascRefIdx[ 0 ] = m_ascRefIdx[ 1 ] = m_ascRefIdx[ 2 ] = m_ascRefIdx[ 3 ] = BLOCK_NOT_AVAILABLE;
    m_usMotPredFlags = 0;
  }

  Void reset()
  {
    clear( BLOCK_NOT_AVAILABLE );
  }

  Void clear( RefIdxValues eRefIdxValues )
  {
    MbMvData::clear();
    m_usMotPredFlags = 0x0000;
    m_bFieldFlag = false;
    m_ascRefIdx  [ 0 ] = m_ascRefIdx  [ 1 ] = m_ascRefIdx  [ 2 ] = m_ascRefIdx  [ 3 ] = eRefIdxValues;
    m_acRefPicIdc[ 0 ] = m_acRefPicIdc[ 1 ] = m_acRefPicIdc[ 2 ] = m_acRefPicIdc[ 3 ] = RefPicIdc();
  }

	Void  setFieldMode( Bool b )       { m_bFieldFlag = b; }
  Bool  getFieldMode()         const { return m_bFieldFlag; }

  Void  setRefIdx( SChar scRefIdx );
  Void  setRefIdx( SChar scRefIdx, ParIdx16x8 eParIdx  );
  Void  setRefIdx( SChar scRefIdx, ParIdx8x16 eParIdx  );
  Void  setRefIdx( SChar scRefIdx, ParIdx8x8  eParIdx  );

  SChar getRefIdx()                      const  { return m_ascRefIdx[ 0         ]; }
  SChar getRefIdx( ParIdx16x8 eParIdx  ) const  { return m_ascRefIdx[ m_auiBlk2Part[ eParIdx ] ]; }
  SChar getRefIdx( ParIdx8x16 eParIdx  ) const  { return m_ascRefIdx[ m_auiBlk2Part[ eParIdx ] ]; }
  SChar getRefIdx( ParIdx8x8  eParIdx  ) const  { return m_ascRefIdx[ m_auiBlk2Part[ eParIdx ] ]; }
  SChar getRefIdx( LumaIdx cIdx )        const  { return m_ascRefIdx[ m_auiBlk2Part[ cIdx.b4x4() ] ]; }
  SChar getRefIdx( Par8x8 ePar8x8 )      const  { return m_ascRefIdx[ ePar8x8]; }

  const RefPicIdc&  getRefPicIdc  ()                      const  { return m_acRefPicIdc[ 0         ]; }
  const RefPicIdc&  getRefPicIdc  ( ParIdx16x8 eParIdx  ) const  { return m_acRefPicIdc[ m_auiBlk2Part[ eParIdx ] ]; }
  const RefPicIdc&  getRefPicIdc  ( ParIdx8x16 eParIdx  ) const  { return m_acRefPicIdc[ m_auiBlk2Part[ eParIdx ] ]; }
  const RefPicIdc&  getRefPicIdc  ( ParIdx8x8  eParIdx  ) const  { return m_acRefPicIdc[ m_auiBlk2Part[ eParIdx ] ]; }
  const RefPicIdc&  getRefPicIdc  ( LumaIdx cIdx )        const  { return m_acRefPicIdc[ m_auiBlk2Part[ cIdx.b4x4() ] ]; }
  const RefPicIdc&  getRefPicIdc  ( Par8x8 ePar8x8 )      const  { return m_acRefPicIdc[ ePar8x8]; }

  ErrVal  setRefPicIdcs ( RefFrameList* pcRefFrameList );

  Bool  getMotPredFlag()                     const  { return xGetMotPredFlag( 0 ); }
  Bool  getMotPredFlag( ParIdx16x8 eParIdx ) const  { return xGetMotPredFlag( eParIdx ); }
  Bool  getMotPredFlag( ParIdx8x16 eParIdx ) const  { return xGetMotPredFlag( eParIdx ); }
  Bool  getMotPredFlag( ParIdx8x8  eParIdx ) const  { return xGetMotPredFlag( eParIdx ); }
  Bool  getMotPredFlag( LumaIdx    cIdx    ) const  { return xGetMotPredFlag( cIdx.b4x4() ); }

  Void  setMotPredFlag( Bool bFlag );
  Void  setMotPredFlag( Bool bFlag, ParIdx16x8 eParIdx );
  Void  setMotPredFlag( Bool bFlag, ParIdx8x16 eParIdx );
  Void  setMotPredFlag( Bool bFlag, ParIdx8x8  eParIdx );
  Void  setMotPredFlag( Bool bFlag, LumaIdx    cIdx    );

  Void  getMvRef         ( Mv& rcMv, SChar& rscRef, LumaIdx cIdx )                            const;
  Void  getMv3D          ( Mv3D& rcMv3D,            LumaIdx cIdx )                            const;
  Void  getMvRefNeighbour( Mv& rcMv, SChar& rscRef, LumaIdx cIdx )    const;
	Void  getMv3DNeighbour ( Mv3D& rcMv3D,            LumaIdx cIdx, Bool bCurrentFieldFlag )    const;

  ErrVal  save( FILE* pFile );
  ErrVal  load( FILE* pFile );

  Bool  xGetMotPredFlag ( UInt  uiPos )  const
  {
    return ( ( m_usMotPredFlags & (1 << uiPos) ) >> uiPos ? true : false );
  }
  Void  xSetMotPredFlag ( Bool  bFlag, UInt  uiPos )
  {
    AOF(uiPos<16);
    m_usMotPredFlags &= ~(UShort)(1<<uiPos);
    ROFVS( bFlag );
    m_usMotPredFlags += (1<<uiPos);
  }

private:
  static const UInt m_auiBlk2Part[16];
  SChar             m_ascRefIdx   [4];
  RefPicIdc         m_acRefPicIdc [4];
  UShort            m_usMotPredFlags;
};





#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif



__inline
Void MbMotionData::setRefIdx( SChar scRefIdx )
{
  m_ascRefIdx[ 0 ] = m_ascRefIdx[ 1 ] = m_ascRefIdx[ 2 ] = m_ascRefIdx[ 3 ] = scRefIdx;
}

__inline
Void MbMotionData::setRefIdx( SChar scRefIdx, ParIdx16x8 eParIdx )
{
  m_ascRefIdx[ m_auiBlk2Part[eParIdx]   ] = scRefIdx;
  m_ascRefIdx[ m_auiBlk2Part[eParIdx]+1 ] = scRefIdx;
}

__inline
Void MbMotionData::setRefIdx( SChar scRefIdx, ParIdx8x16 eParIdx )
{
  m_ascRefIdx[ m_auiBlk2Part[eParIdx]   ] = scRefIdx;
  m_ascRefIdx[ m_auiBlk2Part[eParIdx]+2 ] = scRefIdx;
}

__inline Void MbMotionData::setRefIdx( SChar scRefIdx, ParIdx8x8 eParIdx )
{
  m_ascRefIdx[ m_auiBlk2Part[eParIdx]   ] = scRefIdx;
}


__inline Void MbMvData::setAllMv( const Mv& rcMv )
{
  const register Mv cMv = rcMv;
  m_acMv[ 0 ] = cMv;
  m_acMv[ 1 ] = cMv;
  m_acMv[ 2 ] = cMv;
  m_acMv[ 3 ] = cMv;
  m_acMv[ 4 ] = cMv;
  m_acMv[ 5 ] = cMv;
  m_acMv[ 6 ] = cMv;
  m_acMv[ 7 ] = cMv;
  m_acMv[ 8 ] = cMv;
  m_acMv[ 9 ] = cMv;
  m_acMv[ 10 ] = cMv;
  m_acMv[ 11 ] = cMv;
  m_acMv[ 12 ] = cMv;
  m_acMv[ 13 ] = cMv;
  m_acMv[ 14 ] = cMv;
  m_acMv[ 15 ] = cMv;
}

__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx16x8 eParIdx  )
{
  Mv* pcMv = m_acMv + eParIdx;
  const register Mv cMv = rcMv;
  pcMv[ 0 ] = cMv;
  pcMv[ 1 ] = cMv;
  pcMv[ 2 ] = cMv;
  pcMv[ 3 ] = cMv;
  pcMv[ 4 ] = cMv;
  pcMv[ 5 ] = cMv;
  pcMv[ 6 ] = cMv;
  pcMv[ 7 ] = cMv;
}


__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx8x16 eParIdx  )
{
  Mv* pcMv = m_acMv + eParIdx;
  const register Mv cMv = rcMv;
  pcMv[ 0 ] = cMv;
  pcMv[ 1 ] = cMv;
  pcMv[ 4 ] = cMv;
  pcMv[ 5 ] = cMv;
  pcMv[ 8 ] = cMv;
  pcMv[ 9 ] = cMv;
  pcMv[ 12 ] = cMv;
  pcMv[ 13 ] = cMv;
}

__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx8x8 eParIdx  )
{
  Mv* pcMv = m_acMv + eParIdx;
  const register Mv cMv = rcMv;
  pcMv[ 0 ] = cMv;
  pcMv[ 1 ] = cMv;
  pcMv[ 4 ] = cMv;
  pcMv[ 5 ] = cMv;
}

__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx8x8 eParIdx, SParIdx8x4 eSParIdx )
{
  Mv* pcMv = m_acMv + eParIdx + eSParIdx;
  const register Mv cMv = rcMv;
  pcMv[ 0 ] = cMv;
  pcMv[ 1 ] = cMv;
}

__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx8x8 eParIdx, SParIdx4x8 eSParIdx )
{
  Mv* pcMv = m_acMv + eParIdx + eSParIdx;
  const register Mv cMv = rcMv;
  pcMv[ 0 ] = cMv;
  pcMv[ 4 ] = cMv;
}

__inline Void MbMvData::setAllMv( const Mv& rcMv, ParIdx8x8 eParIdx, SParIdx4x4 eSParIdx )
{
  Mv* pcMv = m_acMv + eParIdx + eSParIdx;
  pcMv[ 0 ] = rcMv;
}




__inline  Void  MbMotionData::setMotPredFlag( Bool bFlag )
{
  xSetMotPredFlag( bFlag,  0 );
  xSetMotPredFlag( bFlag,  1 );
  xSetMotPredFlag( bFlag,  2 );
  xSetMotPredFlag( bFlag,  3 );
  xSetMotPredFlag( bFlag,  4 );
  xSetMotPredFlag( bFlag,  5 );
  xSetMotPredFlag( bFlag,  6 );
  xSetMotPredFlag( bFlag,  7 );
  xSetMotPredFlag( bFlag,  8 );
  xSetMotPredFlag( bFlag,  9 );
  xSetMotPredFlag( bFlag, 10 );
  xSetMotPredFlag( bFlag, 11 );
  xSetMotPredFlag( bFlag, 12 );
  xSetMotPredFlag( bFlag, 13 );
  xSetMotPredFlag( bFlag, 14 );
  xSetMotPredFlag( bFlag, 15 );
}

__inline  Void  MbMotionData::setMotPredFlag( Bool bFlag, ParIdx16x8 eParIdx )
{
  UInt ui = eParIdx;

  xSetMotPredFlag( bFlag, ui+ 0 );
  xSetMotPredFlag( bFlag, ui+ 1 );
  xSetMotPredFlag( bFlag, ui+ 2 );
  xSetMotPredFlag( bFlag, ui+ 3 );
  xSetMotPredFlag( bFlag, ui+ 4 );
  xSetMotPredFlag( bFlag, ui+ 5 );
  xSetMotPredFlag( bFlag, ui+ 6 );
  xSetMotPredFlag( bFlag, ui+ 7 );
}

__inline  Void  MbMotionData::setMotPredFlag( Bool bFlag, ParIdx8x16 eParIdx )
{
  UInt ui = eParIdx;

  xSetMotPredFlag( bFlag, ui+ 0 );
  xSetMotPredFlag( bFlag, ui+ 1 );
  xSetMotPredFlag( bFlag, ui+ 4 );
  xSetMotPredFlag( bFlag, ui+ 5 );
  xSetMotPredFlag( bFlag, ui+ 8 );
  xSetMotPredFlag( bFlag, ui+ 9 );
  xSetMotPredFlag( bFlag, ui+12 );
  xSetMotPredFlag( bFlag, ui+13 );
}

__inline  Void  MbMotionData::setMotPredFlag( Bool bFlag, ParIdx8x8  eParIdx )
{
  UInt ui = eParIdx;

  xSetMotPredFlag( bFlag, ui+ 0 );
  xSetMotPredFlag( bFlag, ui+ 1 );
  xSetMotPredFlag( bFlag, ui+ 4 );
  xSetMotPredFlag( bFlag, ui+ 5 );
}

__inline  Void  MbMotionData::setMotPredFlag( Bool bFlag, LumaIdx    cIdx    )
{
  xSetMotPredFlag( bFlag, cIdx.b4x4() );
}





__inline Void MbMvData::setMv( const Mv& rcMv, LumaIdx cIdx )
{
  m_acMv[ cIdx.b4x4() ] = rcMv;
}

__inline Void MbMotionData::getMvRef( Mv& rcMv, SChar& rscRef, LumaIdx cIdx ) const
{
  rcMv   = m_acMv[ cIdx.b4x4() ];
  rscRef = m_ascRefIdx[ m_auiBlk2Part[ cIdx.b4x4() ] ];
}

__inline Void MbMotionData::getMv3D( Mv3D& rcMv3D, LumaIdx cIdx ) const
{
  rcMv3D.set( m_acMv[ cIdx.b4x4() ], m_ascRefIdx[ m_auiBlk2Part[ cIdx.b4x4()] ] );
}

__inline Void MbMotionData::getMvRefNeighbour( Mv& rcMv, SChar& rscRef, LumaIdx cIdx ) const
{
  rcMv   = m_acMv[ cIdx.b4x4() ];
  rscRef = m_ascRefIdx[ m_auiBlk2Part[ cIdx.b4x4()] ];
}

__inline Void MbMotionData::getMv3DNeighbour( Mv3D& rcMv3D, LumaIdx cIdx, Bool bCurrentFieldFlag ) const
{
  rcMv3D.set( m_acMv[ cIdx.b4x4() ], m_ascRefIdx[ m_auiBlk2Part[ cIdx.b4x4()] ] );

  if( bCurrentFieldFlag && ! m_bFieldFlag )
  {
    rcMv3D.setFrameToFieldPredictor();
  }
  else if ( ! bCurrentFieldFlag && m_bFieldFlag )
  {
    rcMv3D.setFieldToFramePredictor();
  }
}

H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBMVDATA_H__06960F25_0FB8_4A65_935D_B06282FFDF6E__INCLUDED_)
