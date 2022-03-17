
#if !defined(AFX_DECERROR_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
#define AFX_DECERROR_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if defined (FAULT_TOLERANT)
 #define DECASSERT(x)
#else
 #define DECASSERT(x) ASSERT(x)
#endif


#define DECROF( exp )         \
{                             \
  if( !( exp ) )              \
  {                           \
    DECASSERT( 0 );           \
    return ERR_CLASS::m_nERR; \
  }                           \
}

#define DECROT( exp )         \
{                             \
  if( ( exp ) )               \
  {                           \
    DECASSERT( 0 );           \
    return ERR_CLASS::m_nERR; \
  }                           \
}


#define DECROFR( exp, retVal )   \
{                             \
  if( !( exp ) )              \
  {                           \
    DECASSERT( 0 );           \
    return retVal;            \
  }                           \
}

#define DECROTR( exp, retVal )   \
{                             \
  if( ( exp ) )               \
  {                           \
    DECASSERT( 0 );           \
    return retVal;            \
  }                           \
}


#define DECROFV( exp )        \
{                             \
  if( !( exp ) )              \
  {                           \
    DECASSERT( 0 );           \
    return;                   \
  }                           \
}

#define DECROTV( exp )        \
{                             \
  if( ( exp ) )               \
  {                           \
    DECASSERT( 0 );           \
    return;                   \
  }                           \
}

#if JVT_U125
#define DECRNOK( exp )            \
{                                 \
  const ERR_VAL nMSysRetVal = ( exp );   \
  if( ERR_CLASS::m_nOK != nMSysRetVal )  \
  {                               \
    return nMSysRetVal;           \
  }                               \
}
#else
#define DECRNOK( exp )            \
{                                 \
  const ERR_VAL nMSysRetVal = ( exp );   \
  if( ERR_CLASS::m_nOK != nMSysRetVal )  \
  {                               \
    DECASSERT( 0 );               \
    return nMSysRetVal;           \
  }                               \
}
#endif

#define DECRNOKR( exp, retVal )     \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    DECASSERT( 0 );                 \
    return retVal;                  \
  }                                 \
}


#define DECRNOKV( exp )             \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    DECASSERT( 0 );                 \
    return;                         \
  }                                 \
}


#endif // !defined(AFX_DECERROR_H__19223BF1_DA9C_475F_9AEE_D1A55237EB1E__INCLUDED_)
