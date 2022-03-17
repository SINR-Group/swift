
#if !defined(AFX_QUANTIZER_H__84CC26BC_52EB_45C4_A92E_C5A97D96BF64__INCLUDED_)
#define AFX_QUANTIZER_H__84CC26BC_52EB_45C4_A92E_C5A97D96BF64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif

// h264 namepace begin
H264AVC_NAMESPACE_BEGIN


class QpParameter
{
public :
  const QpParameter& operator= (const QpParameter& rcQp )
  {
    m_iPer  = rcQp.m_iPer;
    m_iRem  = rcQp.m_iRem;
    m_iBits = rcQp.m_iBits;
    m_iAdd  = rcQp.m_iAdd;
    return *this;
  }

  Void setQp( Int iQp, Bool bIntra )
  {
    m_iPer  = (iQp)/6;
    m_iRem  = (iQp)%6;
    m_iBits = QP_BITS + m_iPer;
    m_iAdd  = ( 1 << m_iBits) / (( bIntra ) ?  3 : 6);
  }

  const Int per()   const { return m_iPer; }
  const Int rem()   const { return m_iRem; }
  const Int bits()  const { return m_iBits; }
  const Int add()   const { return m_iAdd; }
  const Int mode()  const { return m_iMode; }
  const Int value() const { return 6*m_iPer+m_iRem; }

private:
  Int m_iPer;
  Int m_iRem;
  Int m_iBits;
  Int m_iAdd;
  Int m_iMode;
};



class H264AVCCOMMONLIB_API Quantizer
{
public:
  Quantizer();
  virtual ~Quantizer();


  Void setQp( const MbDataAccess& rcMbDataAccess, Bool bIntraMb )
  {
    Int   iLumaQp   = rcMbDataAccess.getMbData().getQp();
    Int   iCbQp     = rcMbDataAccess.getSH().getCbQp( iLumaQp );
    Int   iCrQp     = rcMbDataAccess.getSH().getCrQp( iLumaQp );
    Bool  bRefPic   = rcMbDataAccess.getSH().getNalRefIdc () != NAL_REF_IDC_PRIORITY_LOWEST;
    Bool  bKeyPic   = rcMbDataAccess.getSH().getTemporalId() == 0;
    Bool  bOneThird = ( bKeyPic || ( bRefPic && bIntraMb ) );

    m_cLumaQp.setQp( iLumaQp, bOneThird );
    m_cCbQp  .setQp( iCbQp,   bOneThird );
    m_cCrQp  .setQp( iCrQp,   bOneThird );
  }

  Void setDecompositionStages( Int iDStages )
  {
    m_iDStages = iDStages;
  }


  const QpParameter&  getCbQp     ()              const { return m_cCbQp; }
  const QpParameter&  getCrQp     ()              const { return m_cCrQp; }
  const QpParameter&  getChromaQp ( UInt uiIdx )  const { AOT( uiIdx > 1 ); return ( uiIdx ? getCrQp() : getCbQp() ); }
  const QpParameter&  getLumaQp   ()  const { return m_cLumaQp;   }

protected:
  QpParameter m_cLumaQp;
  QpParameter m_cCbQp;
  QpParameter m_cCrQp;
  Int         m_iDStages;
};


H264AVC_NAMESPACE_END

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

#endif // !defined(AFX_QUANTIZER_H__84CC26BC_52EB_45C4_A92E_C5A97D96BF64__INCLUDED_)
