
#ifndef __MSYS_MACROS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __MSYS_MACROS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79

#if defined( _DEBUG ) || defined( DEBUG )
  #if !defined( _DEBUG )
    #define _DEBUG
  #endif
  #if !defined( DEBUG )
    #define DEBUG
  #endif
#endif

#if !defined( ASSERT )
#define ASSERT assert
#endif

#define VOID_TYPE Void
#define ERR_CLASS Err
#define ERR_VAL   ErrVal


#define RERR( )               \
{                             \
    ASSERT( 0 );              \
    return ERR_CLASS::m_nERR; \
}

#define ROF( exp )            \
{                             \
  if( !( exp ) )              \
  {                           \
    RERR();                   \
  }                           \
}

#define ROT( exp )            \
{                             \
  if( ( exp ) )               \
  {                           \
   RERR();                    \
  }                           \
}

#define RERRS( )               \
{                             \
return ERR_CLASS::m_nERR;     \
}


#define ROFS( exp )           \
{                             \
  if( !( exp ) )              \
  {                           \
    RERRS();                  \
  }                           \
}

#define ROTS( exp )           \
{                             \
  if( ( exp ) )               \
  {                           \
    RERRS();                  \
  }                           \
}


#define RVAL( retVal )        \
{                             \
    ASSERT( 0 );              \
    return retVal;            \
}


#define ROFR( exp, retVal )   \
{                             \
  if( !( exp ) )              \
  {                           \
    RVAL( retVal );           \
  }                           \
}


#define ROTR( exp, retVal )   \
{                             \
  if( ( exp ) )               \
  {                           \
    RVAL( retVal );             \
  }                           \
}



#define ROFRS( exp, retVal )  \
{                             \
  if( !( exp ) )              \
  {                           \
    return retVal;            \
  }                           \
}

#define ROTRS( exp, retVal )  \
{                             \
  if( ( exp ) )               \
  {                           \
    return retVal;            \
  }                           \
}

#define ROFV( exp )           \
{                             \
  if( !( exp ) )              \
  {                           \
    ASSERT( 0 );              \
    return;                   \
  }                           \
}

#define ROTV( exp )           \
{                             \
  if( ( exp ) )               \
  {                           \
    ASSERT( 0 );              \
    return;                   \
  }                           \
}

#define ROFVS( exp )          \
{                             \
  if( !( exp ) )              \
  {                           \
    return;                   \
  }                           \
}

#define ROTVS( exp )          \
{                             \
  if( ( exp ) )               \
  {                           \
    return;                   \
  }                           \
}

#define RNOK( exp )               \
{                                 \
  const ERR_VAL nMSysRetVal = ( exp );   \
  if( ERR_CLASS::m_nOK != nMSysRetVal )  \
  {                               \
    ASSERT( 0 );                  \
    return nMSysRetVal;                  \
  }                               \
}

#define RNOKR( exp, retVal )        \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    ASSERT( 0 );                    \
    return retVal;                  \
  }                                 \
}

#define RNOKS( exp )                \
{                                   \
  const ERR_VAL nMSysRetVal = ( exp );     \
  if( ERR_CLASS::m_nOK != nMSysRetVal )    \
  {                                 \
    return nMSysRetVal;             \
  }                                 \
}

#define RNOKRS( exp, retVal )       \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    return retVal;                  \
  }                                 \
}

#define RNOKV( exp )                \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    ASSERT( 0 );                    \
    return;                         \
  }                                 \
}

#define RNOKVS( exp )               \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    return;                         \
  }                                 \
}


#define AF( )                 \
{                             \
    ASSERT( 0 );              \
}


#define ANOK( exp )                 \
{                                   \
  if( ERR_CLASS::m_nOK != ( exp ) ) \
  {                                 \
    ASSERT( 0 );                    \
  }                                 \
}

#define AOF( exp )                  \
{                                   \
  if( !( exp ) )                    \
  {                                 \
    ASSERT( 0 );                    \
  }                                 \
}

#define AOT( exp )            \
{                             \
  if( ( exp ) )               \
  {                           \
    ASSERT( 0 );              \
  }                           \
}





#if defined( _DEBUG ) || defined( DEBUG )
  #define CHECK( exp )      ASSERT( exp )
  #define AOT_DBG( exp )    AOT( exp )
  #define AOF_DBG( exp )    AOF( exp )
  #define ANOK_DBG( exp )   ANOK( exp )
  #define DO_DBG( exp )     ( exp )
#else  // _DEBUG

  #define CHECK( exp )      ((VOID_TYPE)( exp ))
  #define AOT_DBG( exp )    ((VOID_TYPE)0)
  #define AOF_DBG( exp )    ((VOID_TYPE)0)
  #define ANOK_DBG( exp )   ((VOID_TYPE)0)
  #define DO_DBG( exp )     ((VOID_TYPE)0)
#endif // _DEBUG

#endif //__MACROS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
