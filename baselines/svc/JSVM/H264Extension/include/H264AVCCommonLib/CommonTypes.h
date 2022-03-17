
#if !defined(AFX_COMMONTYPES_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
#define AFX_COMMONTYPES_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_



H264AVC_NAMESPACE_BEGIN


class CostData
{
public:
  CostData( Double fRdCost = 0.0, UInt uiAddBits = 0, UInt uiBits = 0, UInt uiYDist = 0, UInt uiUDist = 0, UInt uiVDist = 0):  m_fRdCost(fRdCost),  m_uiAddBits(uiAddBits), m_uiBits(uiBits),  m_uiYDist(uiYDist),  m_uiUDist(uiUDist),  m_uiVDist(uiVDist) {}
  virtual ~CostData() {}

  CostData( const CostData& rcCostData)  { CostData( rcCostData.m_fRdCost, rcCostData.m_uiBits, rcCostData.m_uiYDist, rcCostData.m_uiUDist, rcCostData.m_uiVDist); }

  Void add( const CostData& rcCostData ) const
  {
    const_cast<CostData*>(this)->m_uiYDist += rcCostData.m_uiYDist;
    const_cast<CostData*>(this)->m_uiUDist += rcCostData.m_uiUDist;
    const_cast<CostData*>(this)->m_uiVDist += rcCostData.m_uiVDist;
    const_cast<CostData*>(this)->m_uiBits  += rcCostData.m_uiBits;
    const_cast<CostData*>(this)->m_fRdCost += rcCostData.m_fRdCost;
  }
  const UInt   distY()      const { return m_uiYDist; }
  const UInt   distU()      const { return m_uiUDist; }
  const UInt   distV()      const { return m_uiVDist; }
  const UInt   bits()       const { return m_uiBits; }
  const UInt   coeffCost()  const { return m_uiCoeffCost; }
  const Double rdCost()     const { return m_fRdCost; }
  const UInt&  addBits()    const { return m_uiAddBits; }

  UInt&   distY()       { return m_uiYDist; }
  UInt&   distU()       { return m_uiUDist; }
  UInt&   distV()       { return m_uiVDist; }
  UInt&   bits()        { return m_uiBits; }
  UInt&   addBits()     { return m_uiAddBits; }
  UInt&   coeffCost()   { return m_uiCoeffCost; }
  Double& rdCost()      { return m_fRdCost; }

  Bool operator < ( const CostData& rcCostData) { return m_fRdCost < rcCostData.m_fRdCost; }
  Bool operator > ( const CostData& rcCostData) { return m_fRdCost > rcCostData.m_fRdCost; }
  CostData& operator=( const CostData& rcCostData)
  {
    m_fRdCost       = rcCostData.m_fRdCost;
    m_uiCoeffCost   = rcCostData.m_uiCoeffCost;
    m_uiAddBits     = rcCostData.m_uiAddBits;
    m_uiBits        = rcCostData.m_uiBits;
    m_uiVDist       = rcCostData.m_uiVDist;
    m_uiUDist       = rcCostData.m_uiUDist;
    m_uiYDist       = rcCostData.m_uiYDist;
    return *this;
  }
  Void clear()
  {
    m_fRdCost      = DOUBLE_MAX;
    m_uiAddBits    = 0;
    m_uiBits       = 0;
    m_uiYDist      = 0;
    m_uiUDist      = 0;
    m_uiVDist      = 0;
    m_uiCoeffCost  = 0;
  }
protected:
  Double m_fRdCost;
  UInt   m_uiAddBits;
  UInt   m_uiBits;
  UInt   m_uiYDist;
  UInt   m_uiUDist;
  UInt   m_uiVDist;
  UInt   m_uiCoeffCost;
};


const UChar g_aucConvertBlockOrder[17] =
{
   0,  1,  4,  5,
   2,  3,  6,  7,
   8,  9, 12, 13,
  10, 11, 14, 15, 0xff,
};

// TMM_ESS {
const UChar g_aucConvertTo8x8Idx[16]=
{
    0 ,0 ,1 , 1,
    0 ,0 ,1 , 1,
    2 ,2 ,3 , 3,
    2 ,2 ,3 ,3
};

const UChar g_aucConvertTo4x4Idx[4]=
{  0,2,8,10 };
// TMM_ESS }

class LumaIdx
{
protected:
  LumaIdx( Int iIdx = 0 )          : m_iIdx( iIdx )       {}

public:
  __inline LumaIdx   operator + ( SParIdx4x4     eSParIdx )  const { return LumaIdx( m_iIdx + eSParIdx ); }
  __inline LumaIdx   operator + ( NeighbourBlock eBlock )    const { return LumaIdx( m_iIdx + eBlock ); }
  __inline LumaIdx   operator + ( ParIdx8x8      eParIdx )   const { return LumaIdx( m_iIdx + eParIdx ); }

  operator Int()                                    const { return m_iIdx;    }
  Int x()                                           const { return m_iIdx&3;  }
  Int y()                                           const { return m_iIdx>>2; }
  Int b4x4()                                        const { return m_iIdx;    }
  Par8x8 getPar8x8()                                const { return Par8x8(g_aucConvertBlockOrder[m_iIdx]>>2); }
  UInt   getS4x4()                                  const { return Par8x8(g_aucConvertBlockOrder[m_iIdx]& 3); }

protected:
  Int m_iIdx;
};


class ChromaIdx
{
protected:
  ChromaIdx( Int iIdx = 0 ) : m_iIdx( iIdx )              {}
public:
  operator Int()                                    const { return m_iIdx;        }
  Int x()                                           const { return m_iIdx&1;      }
  Int y()                                           const { return (m_iIdx>>1)&1; }
  Int plane()                                       const { return m_iIdx>>2;     }

protected:
  Int m_iIdx;
};

class B8x8Idx :
public LumaIdx
{
public:
  B8x8Idx( Par8x8 ePar8x8 ) : m_iSIdx( ePar8x8<<2 )       { convert(); }
  B8x8Idx()  : m_iSIdx( 0 )                               { convert(); }
  B8x8Idx  operator + (Int i)                       const { return B8x8Idx( (m_iSIdx>>2) + i ); }
  B8x8Idx& operator +=(Int i)                             { m_iSIdx+=i<<2; convert(); return *this; }
  B8x8Idx& operator ++(Int)                               { m_iSIdx+=4;    convert(); return *this; }
  B8x8Idx& operator --(Int)                               { m_iSIdx-=4;    convert(); return *this; }
  Bool      isLegal()                               const { return m_iSIdx < 16; }
  const Int xxxgetSIdx()                            const { return m_iSIdx; }
  ParIdx8x8 b8x8()                                  const { return ParIdx8x8(m_iIdx); }
  Par8x8    b8x8Index()                             const { return Par8x8(m_iSIdx>>2); }
  B8x8Idx( LumaIdx cIdx )
  {
    UInt    uiB8x8  = (cIdx.y() / 2) * 2 + (cIdx.x() / 2);
    Par8x8  ePar8x8 = Par8x8(uiB8x8);
    m_iSIdx =  ePar8x8 << 2;
    convert();
  }

protected:
  Int convert()                                           { return m_iIdx = g_aucConvertBlockOrder[m_iSIdx]; }
  B8x8Idx(Int i) : m_iSIdx( i<<2 )                        { convert(); }

protected:
  Int m_iSIdx;
};


class B4x4Idx :
public LumaIdx
{
public:
  B4x4Idx( UInt ui4x4Idx = 0)  : LumaIdx( ui4x4Idx )      {}
  B4x4Idx& operator ++(Int)                               { m_iIdx++; return *this; }
  Bool isLegal()                                    const { return m_iIdx < 16; }
};


class S4x4Idx :
public LumaIdx
{
private:
  S4x4Idx( UInt ui ) : m_iSIdx( ui )                                { convert(); }

public:
  S4x4Idx( const B8x8Idx& rcB8x8Idx ) : m_iSIdx( rcB8x8Idx.xxxgetSIdx() )  { convert(); }
  S4x4Idx()                    : m_iSIdx( 0 )                       { convert(); }

  S4x4Idx operator+(Int i )                                   const { return S4x4Idx( m_iSIdx + i ); }
  S4x4Idx operator+(UInt ui )                                 const { return S4x4Idx( m_iSIdx + ui ); }
  Int s4x4()                                                  const { return m_iSIdx; } // this is for s-only-ordered buffers
  Par8x8 getContainingPar8x8()                                const { return Par8x8(m_iSIdx>>2); }

  S4x4Idx& operator ++(Int)                                         { m_iSIdx++; convert(); return *this; }
  Bool isLegal()                                              const { return m_iSIdx <16; }
  Bool isLegal( const B8x8Idx& rcB8x8Idx )                    const { return m_iSIdx <(4+rcB8x8Idx.xxxgetSIdx()); }
protected:
  Void convert()                                                    { m_iIdx = g_aucConvertBlockOrder[m_iSIdx]; }

protected:
  Int m_iSIdx;
};

class CPlaneIdx
{
public:
  CPlaneIdx( UInt uiIdx = 0 ) : m_iIdx( uiIdx ) {}
  CPlaneIdx& operator ++(Int)    { m_iIdx++; return *this; }
  Bool       isLegal()     const { return m_iIdx < 2; }
  operator   Int()         const { return m_iIdx;     }
private:
  UInt m_iIdx;
};


class CIdx :
public ChromaIdx
{
public:
  CIdx( UInt uiIdx = 0)     : ChromaIdx( uiIdx )       {}

  CIdx( const CPlaneIdx &rcCPlIdx )                 : ChromaIdx( rcCPlIdx<<2 )             {}
  CIdx( const CPlaneIdx &rcCPlIdx, Par8x8 ePar8x8 ) : ChromaIdx( (rcCPlIdx<<2) + ePar8x8 ) {}

  CIdx   operator+ ( UInt ui )                  const { return CIdx( m_iIdx + ui ); }
  CIdx   operator+ ( Int  i  )                  const { return CIdx( m_iIdx + i  ); }

  CIdx& operator ++(Int)                              { m_iIdx++; return *this; }
  Bool isLegal()                                const { return m_iIdx < 8; }

  Bool isLegal( CPlaneIdx cCPlIdx )             const { return m_iIdx < ( ( cCPlIdx + 1 ) << 2 ); }
  Bool isLegal( Int i)                          const { return m_iIdx < i; }
};



H264AVC_NAMESPACE_END


#endif //!defined(AFX_COMMONTYPES_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
