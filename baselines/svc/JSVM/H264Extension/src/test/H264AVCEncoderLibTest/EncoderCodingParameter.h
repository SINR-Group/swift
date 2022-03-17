
#if !defined(AFX_ENCODERCODINGPARAMETER_H__145580A5_E0D6_4E9C_820F_EA4EF1E1B793__INCLUDED_)
#define AFX_ENCODERCODINGPARAMETER_H__145580A5_E0D6_4E9C_820F_EA4EF1E1B793__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <string>
#include "CodingParameter.h"

using namespace h264;

#define ROTREPORT(x,t) {if(x) {::printf("\n%s\n",t); assert(0); return Err::m_nInvalidParameter;} }

class EncoderConfigLineStr : public h264::EncoderConfigLineBase
{
public:
  EncoderConfigLineStr( const Char* pcTag, std::string* pcPar, const Char* pcDefault ) : EncoderConfigLineBase( pcTag, 1 ), m_pcPar( pcPar )
  {
    *m_pcPar = pcDefault;
  };
  Void setVar( std::string& pvValue )
  {
    *m_pcPar = pvValue;
  };
protected:
  std::string* m_pcPar;
};

class EncoderConfigLineDbl : public h264::EncoderConfigLineBase
{
public:
  EncoderConfigLineDbl( const Char* pcTag, Double* pdPar, Double pdDefault ) :  EncoderConfigLineBase( pcTag, 2 ), m_pdPar( pdPar )
  {
    *m_pdPar = pdDefault;
  };
  Void setVar( std::string& pvValue )
  {
    *m_pdPar = atof( pvValue.c_str() );
  };
protected:
  Double* m_pdPar;
};

class EncoderConfigLineInt : public h264::EncoderConfigLineBase
{
public:
  EncoderConfigLineInt( const Char* pcTag, Int* piPar, Int piDefault ) : EncoderConfigLineBase( pcTag, 3 ), m_piPar( piPar )
  {
    *m_piPar = piDefault;
  };
  Void setVar( std::string& pvValue)
  {
    *m_piPar = atoi( pvValue.c_str() );
  };
protected:
  Int* m_piPar;
};

class EncoderConfigLineUInt : public h264::EncoderConfigLineBase
{
public:
  EncoderConfigLineUInt( const Char* pcTag, UInt* puiPar, UInt puiDefault ) : EncoderConfigLineBase( pcTag, 4 ), m_puiPar( puiPar )
  {
    *m_puiPar = puiDefault;
  };
  Void setVar( std::string& pvValue)
  {
    *m_puiPar = atoi( pvValue.c_str() );
  };
protected:
  UInt* m_puiPar;
};

class EncoderConfigLineChar : public h264::EncoderConfigLineBase
{
public:
  EncoderConfigLineChar( const Char* pcTag, Char* pcPar, Char pcDefault ) : EncoderConfigLineBase( pcTag, 5 ), m_pcPar( pcPar )
  {
    *m_pcPar = pcDefault;
  };
  Void setVar( std::string& pvValue )
  {
    *m_pcPar = (Char)atoi( pvValue.c_str() );
  };
protected:
  Char* m_pcPar;
};



class EncoderCodingParameter :
public h264::CodingParameter
{
#if DOLBY_ENCMUX_ENABLE
private:
  Int                       m_iMuxMethod;
  Int                       m_iMuxFilter;
  Int                       m_iMuxOffset[MAX_LAYERS];
#endif

protected:
  EncoderCodingParameter          (){}
  virtual ~EncoderCodingParameter (){}

public:
  static ErrVal create    ( EncoderCodingParameter*& rpcEncoderCodingParameter );
  ErrVal        destroy   ();
  ErrVal        init      ( Int     argc,
                            Char**  argv,
                            std::string&               rcBitstreamFile );

  Void          printHelp ();

#if DOLBY_ENCMUX_ENABLE
  Int  getMuxMethod() { return m_iMuxMethod;}
  Int  getMuxFilter() { return m_iMuxFilter;}
  Int  getMuxOffset(Int iLayerIdx) {return m_iMuxOffset[iLayerIdx];}
#endif

protected:
  Bool    equals( const Char* str1, const Char* str2, UInt nLetter ) { return 0 == ::strncmp( str1, str2, nLetter); }

  ErrVal  xReadFromFile      ( std::string&            rcFilename,
                               std::string&            rcBitstreamFile  );
  ErrVal  xReadLayerFromFile ( std::string&            rcFilename,
                               h264::LayerParameters&  rcLayer );
  ErrVal  xReadLine          ( FILE*                   hFile,
                               std::string*            pacTag );
  ErrVal  xReadSliceGroupCfg(h264::LayerParameters&  rcLayer );
  ErrVal  xReadROICfg(h264::LayerParameters&  rcLayer );
  ErrVal  xReadScalMat( UChar* pBuffer, Int iCnt, std::string& rcFileName );
};




ErrVal EncoderCodingParameter::create( EncoderCodingParameter*& rpcEncoderCodingParameter )
{
  rpcEncoderCodingParameter = new EncoderCodingParameter;

  ROT( NULL == rpcEncoderCodingParameter );

  return Err::m_nOK;
}


ErrVal EncoderCodingParameter::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal EncoderCodingParameter::init( Int     argc,
                                     Char**  argv,
                                     std::string& rcBitstreamFile  )
{
  Char* pcCom;

  rcBitstreamFile = "";

  ROTS( argc < 2 )

  for( Int n = 1; n < argc; n++ )
  {
    pcCom = argv[n++];
    if( equals( pcCom, "-abt", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt  uiLayer = atoi( argv[n  ] );
      UInt  uiValue = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setEnable8x8Trafo( uiValue );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-smat", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt  uiLayer = atoi( argv[n  ] );
      UInt  uiValue = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setScalingMatricesPresent( uiValue );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-arr", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt  uiLayer = atoi( argv[n  ] );
      UInt  uiValue = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setAVCRewrite( uiValue );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-kpm", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      UInt  uiMode = atoi( argv[n] );
      CodingParameter::setEncodeKeyPictures( uiMode );
      continue;
    }
    if( equals( pcCom, "-mgsctrl", 5 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiMode = atoi( argv[n] );
      CodingParameter::setMGSKeyPictureControl( uiMode );
      continue;
    }
    if( equals( pcCom, "-eqpc", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt  uiLayer = atoi( argv[n  ] );
      UInt  uiValue = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setExplicitQPCascading( uiValue );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-dqp", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      ROTS( NULL == argv[n+2] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiLevel = atoi( argv[n+1] );
      Double  dValue  = atof( argv[n+2] );
      CodingParameter::getLayerParameters( uiLayer ).setDeltaQPTLevel( uiLevel, dValue );
      n += 2;
      continue;
    }
    if( equals( pcCom, "-aeqpc", 6 ) )
    {
      ROTS( NULL == argv[n  ] );
      UInt  uiValue = atoi( argv[n] );
      for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setExplicitQPCascading( uiValue );
      }
      continue;
    }
    if( equals( pcCom, "-adqp", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLevel = atoi( argv[n  ] );
      Double  dValue  = atof( argv[n+1] );
      for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setDeltaQPTLevel( uiLevel, dValue );
      }
      n += 1;
      continue;
    }
    if( equals( pcCom, "-xdqp", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      ROTS( NULL == argv[n+2] );
      Double  dDQP0   = atof( argv[n  ] );
      Double  dDDQP1  = atof( argv[n+1] );
      Double  dDDQPN  = atof( argv[n+2] );
      for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setExplicitQPCascading( 1 );
        for( UInt uiLevel = 0; uiLevel <= MAX_DSTAGES; uiLevel++ )
        {
          Double  dDQP = dDQP0;
          if( uiLevel > 0 )
          {
            dDQP += dDDQP1 + (Double)( uiLevel - 1 ) * dDDQPN;
          }
          CodingParameter::getLayerParameters( uiLayer ).setDeltaQPTLevel( uiLevel, dDQP );
        }
      }
      n += 2;
      continue;
    }
    if( equals( pcCom, "-bf", 3 ) )
    {
      ROTS( NULL == argv[n] );
      rcBitstreamFile = argv[n];
      continue;
    }
    if( equals( pcCom, "-numl", 5 ) )
    {
      ROTS( NULL == argv[n] );
      UInt  uiNumLayers = atoi( argv[n] );
      CodingParameter::setNumberOfLayers( uiNumLayers );
      continue;
    }
    if( equals( pcCom, "-rqp", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      Double  dResQp  = atof( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setBaseQpResidual( dResQp );
      printf("\n********** layer %1d - rqp = %f **********\n\n",uiLayer,dResQp);
      n += 1;
      continue;
    }
    if( equals( pcCom, "-mbaff", 6 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiMbAff = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setMbAff( uiMbAff );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-paff", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiPAff  = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setPAff( uiPAff );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-mqp", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      ROTS( NULL == argv[n+2] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiStage = atoi( argv[n+1] );
      Double  dMotQp  = atof( argv[n+2] );
      CodingParameter::getLayerParameters( uiLayer ).setQpModeDecision( uiStage, dMotQp );
      n += 2;
      continue;
    }
    if( equals( pcCom, "-lqp", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      Double  dQp     = atof( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setBaseQpResidual( dQp );
      for( UInt uiStage = 0; uiStage < MAX_DSTAGES; uiStage++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setQpModeDecision( uiStage, dQp );
      }
      CodingParameter::getLayerParameters( uiLayer ).setQpModeDecisionLP( dQp );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-meqplp", 7 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      Double  dQp     = atof( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setQpModeDecisionLP( dQp );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-ilpred", 7 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiBLRes = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setInterLayerPredictionMode( uiBLRes );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-gop", 4 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiGOPSize = atoi( argv[n] );
      CodingParameter::setGOPSize( uiGOPSize );
      continue;
    }
		if( equals( pcCom, "-iper", 5 ) )
    {
      ROTS( NULL == argv[n] );
      Int iIntraPeriod = atoi( argv[n] );
      CodingParameter::setIntraPeriod( iIntraPeriod );
      continue;
    }
    if( equals( pcCom, "-blid", 5 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      UInt    uiBlId  = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setBaseLayerId( uiBlId );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-frms", 5 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiFrms = atoi( argv[n] );
      CodingParameter::setTotalFrames( uiFrms );
      continue;
    }
    if( equals( pcCom, "-bcip", 5 ) )
    {
      n--;
      ROTS( NULL == argv[n] );
      CodingParameter::getLayerParameters(0).setConstrainedIntraPred( 1 );
      continue;
    }
    if( equals( pcCom, "-pf", 3) )
    {
      ROTS( NULL == argv[n] );
      std::string cFilename = argv[n];
      RNOKS( xReadFromFile( cFilename, rcBitstreamFile ) );
      continue;
    }

	//S051{
	if( equals( pcCom, "-encsip", 7 ) )
    {
		ROTS( NULL == argv[n  ] );
		ROTS( NULL == argv[n+1] );

		UInt    uiLayer = atoi( argv[n  ] );
		CodingParameter::getLayerParameters( uiLayer ).setEncSIP(true);
		CodingParameter::getLayerParameters( uiLayer ).setInSIPFileName(argv[n+1]);
		n += 1;
		continue;
    }
	if( equals( pcCom, "-anasip", 7 ) )
    {
		ROTS( NULL == argv[n  ] );
		ROTS( NULL == argv[n+1] );
		ROTS( NULL == argv[n+2] );

		UInt    uiLayer = atoi( argv[n  ] );
		UInt	uiMode = atoi( argv[n+1] );

		if(uiMode!=0)
			CodingParameter::getLayerParameters( uiLayer ).setAnaSIP(2);
		else
			CodingParameter::getLayerParameters( uiLayer ).setAnaSIP(1);

		CodingParameter::getLayerParameters( uiLayer ).setOutSIPFileName(argv[n+2]);
		n += 2;
		continue;
    }
	//S051}

    //JVT-W052 bug_fixed
    if( equals( pcCom, "-icsei", 6 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiIntegrityCheckSei = atoi( argv[n] );
      CodingParameter::setIntegrityCheckSEIEnable( uiIntegrityCheckSei );
      continue;
    }
    //JVT-W052 bug_fixed

    // JVT-U116 LMI {
    if( equals( pcCom, "-tlidx", 6 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiTl0DepRepIdxSeiEnable = atoi( argv[n] );
      CodingParameter::setTl0DepRepIdxSeiEnable( uiTl0DepRepIdxSeiEnable );
      continue;
    }
    // JVT-U116 LMI }

    // JVT-U085 LMI {
    if( equals( pcCom, "-tlnest", 7 ) )
    {
      ROTS( NULL == argv[n] );
      UInt uiTlNestFlag = atoi( argv[n] );
      CodingParameter::setTlevelNestingFlag( uiTlNestFlag );
      continue;
    }
    // JVT-U085 LMI }

    if( equals( pcCom, "-org", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      ROF(    uiLayer < MAX_LAYERS );
      CodingParameter::getLayerParameters( uiLayer ).setInputFilename( argv[n+1] );
      n += 1;
      continue;
    }
		//JVT-W049 {
    if( equals( pcCom, "-plr", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      ROF(    uiLayer < MAX_LAYERS );
			UInt    uiPLR = atoi( argv[n+1  ] );
      CodingParameter::getLayerParameters( uiLayer ).setPLR( uiPLR );
      n += 1;
      continue;
    }
		if( equals( pcCom, "-redu", 5 ) )
		{
			ROTS( NULL == argv[n  ] );
			ROTS( NULL == argv[n+1] );
			UInt    uiLayer = atoi( argv[n  ] );
			ROF(    uiLayer < MAX_LAYERS );
			Bool    bReduFlag = (atoi( argv[n+1  ] ) > 0 ? true:false);
			CodingParameter::getLayerParameters( uiLayer ).setUseRedundantSliceFlag( bReduFlag );
			n += 1;
			continue;
		}

		if( equals( pcCom, "-kpredu", 7 ) )
		{
			ROTS( NULL == argv[n  ] );
			ROTS( NULL == argv[n+1] );
			UInt    uiLayer = atoi( argv[n  ] );
			ROF(    uiLayer < MAX_LAYERS );
			Bool    bReduKeyFlag = (atoi( argv[n+1  ] ) > 0 ? true:false);
			CodingParameter::getLayerParameters( uiLayer ).setUseRedundantKeySliceFlag( bReduKeyFlag );
			n += 1;
			continue;
    }
    //JVT-W049 }
    if( equals( pcCom, "-rec", 4 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer = atoi( argv[n  ] );
      ROF(    uiLayer < MAX_LAYERS );
      CodingParameter::getLayerParameters( uiLayer ).setOutputFilename( argv[n+1] );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-ec", 3 ) )
    {
      ROTS( NULL == argv[n  ] );
      ROTS( NULL == argv[n+1] );
      UInt    uiLayer  = atoi( argv[n  ] );
      ROF(    uiLayer < MAX_LAYERS );
      UInt    uiECmode = atoi( argv[n+1] );
      CodingParameter::getLayerParameters( uiLayer ).setEntropyCodingModeFlag( uiECmode != 0 );
      n += 1;
      continue;
    }
    if( equals( pcCom, "-vlc", 4 ) )
    {
      n--;
      ROTS( NULL == argv[n] );
      for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setEntropyCodingModeFlag( false );
      }
      continue;
    }
    if( equals( pcCom, "-cabac", 6 ) )
    {
      n--;
      ROTS( NULL == argv[n] );
      for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
      {
        CodingParameter::getLayerParameters( uiLayer ).setEntropyCodingModeFlag( true );
      }
      continue;
    }
    //JVT-U106 Behaviour at slice boundaries{
    if( equals( pcCom, "-ciu", 3 ) )
    {
	    ROTS( NULL == argv[n] );
	    UInt flag = atoi( argv[n] );
	    CodingParameter::setCIUFlag( flag );
	    continue;
    }
    //JVT-U106 Behaviour at slice boundaries}

	// JVT-AD021 {
	if( equals( pcCom, "-ml", 3 ) )
	{
		ROTS( NULL == argv[n] );
		UInt flag = atoi( argv[n] );
		CodingParameter::setMultiLayerLambda( flag );
		continue;
	}
	// JVT-AD021 }

	if( equals( pcCom, "-h", 2) )
    {
      printHelp();
      return Err::m_nOK;
    }

    return Err::m_nERR;
  }


  RNOKS( check() );

  return Err::m_nOK;
}


Void EncoderCodingParameter::printHelp()
{
  printf("\n supported options:\n\n");
  printf("  -pf     Parameter File Name\n\n");

  printf("  -bf     BitStreamFile\n");
  printf("  -frms   Number of total frames\n");
	printf("  -gop    GOPSize - GOP size (2,4,8,16,32,64, default: 1)\n");
	printf("  -iper   Intra period (default: -1) : must be a power of 2 of GOP size (or -1)\n");
  printf("  -numl   Number Of Layers\n");
  printf("  -cabac  CABAC for all layers as entropy coding mode\n");
  printf("  -vlc    VLC for all layers as entropy coding mode\n");
  printf("  -ecmf   (Layer) (entropy_coding_mod_flag)\n");
  printf("  -org    (Layer) (original file)\n");
  printf("  -rec    (Layer) (reconstructed file)\n");
  printf("  -ec     (Layer) (entropy coding mode)\n");
  printf("  -rqp    (Layer) (ResidualQP)\n");
  printf("  -mqp    (Layer) (Stage) (MotionQP)\n");
  printf("  -lqp    (Layer) (ResidualAndMotionQP)\n");
  printf("  -meqplp (Layer) (MotionQPLowpass)\n");
  printf("  -ilpred (Layer) (InterLayerPredictionMode)\n");
  printf("  -blid   (Layer) (BaseLayerId)\n");
  printf("  -bcip   Constrained intra prediction for base layer (needed for single-loop) in scripts\n");
  //S051{
  printf("  -anasip (Layer) (SIP Analysis Mode)[0: persists all inter-predictions, 1: forbids all inter-prediction.] (File for storing bits information)\n");
  printf("  -encsip (Layer) (File with stored SIP information)\n");
  //S051}
  //JVT-W052 bug_fixed
  printf("  -icsei   (IntegrityCheckSEIEnableFlag)[0: IntegrityCheckSEI is not applied, 1: IntegrityCheckSEI is applied.]\n");
  //JVT-W052 bug_fixed
   //JVT-U085 LMI
  printf("  -tlnest (TlevelNestingFlag)[0: temporal level nesting constraint is not applied, 1: the nesting constraint is applied.]\n");
  //JVT-U116 JVT-V088 JVT-W062 LMI
  printf("  -tlidx (Tl0DepRepIdxSeiEnable)[0: tl0_dep_rep_idx is not present, 1: tl0_dep_rep_idx is present.]\n");
  //JVT-U106 Behaviour at slice boundaries{
  printf("  -ciu    (Constrained intra upsampling)[0: no, 1: yes]\n");
  //JVT-U106 Behaviour at slice boundaries}
  printf("  -kpm       (mode) [0:only for FGS(default), 1:FGS&MGS, 2:always]\n");
  printf("  -mgsctrl   (mode) [0:normal encoding(default), 1:EL ME, 2:EL ME+MC]\n");

  printf("  -eqpc   (layer) (value)         sets explicit QP cascading mode for given layer [0: no, 1: yes]\n");
  printf("  -dqp    (layer) (level) (value) sets delta QP for given layer and temporal level (in explicit mode)\n");
  printf("  -aeqpc  (value)                 sets explicit QP cascading mode for all layers  [0: no, 1: yes]\n");
  printf("  -adqp   (level) (value)         sets delta QP for all layers and given temporal level (in explicit mode)\n");
  printf("  -xdqp   (DQP0) (DDQP1) (DDQPN)  sets delta QP for all layers (in explicit mode)\n");

  printf("  -mbaff  (layer) (Mb Adaptive Frame Field Coding)  \n");
  printf("  -paff   (layer) (Picture Adadptive Frame Field Coding)   \n");

  // JVT-AD021 {
  printf("  -ml   (mode) [0:disabled(default), 1:multi-layer lambda selection, 2:mode1x0.8]   \n");
  // JVT-AD021 }
  printf("  -h       Print Option List \n");
  printf("\n");
}


ErrVal EncoderCodingParameter::xReadLine( FILE* hFile, std::string* pacTag )
  {
  ROF( pacTag );

    Int  n;
    UInt uiTagNum = 0;
  Bool          bComment  = false;
  std::string*  pcTag     = &pacTag[0];

    for( n = 0; n < 4; n++ )
    {
      pacTag[n] = "";
    }

  for( n = 0; ; n++ )
    {
      Char cChar = (Char) fgetc( hFile );
    ROTRS( cChar == '\n' || feof( hFile ), Err::m_nOK );  // end of line
    if   ( cChar == '#' )
      {
      bComment = true;
      }
    if( ! bComment )
      {
      if ( cChar == '\t' || cChar == ' ' ) // white space
        {
          ROTR( uiTagNum == 3, Err::m_nERR );
        if( ! pcTag->empty() )
          {
            uiTagNum++;
          pcTag = &pacTag[uiTagNum];
          }
  }
      else
  {
        *pcTag += cChar;
  }
}
  }

 }

ErrVal EncoderCodingParameter::xReadFromFile( std::string& rcFilename, std::string& rcBitstreamFile )
{
  std::string acLayerConfigName[MAX_LAYERS];
  std::string acTags[4];
  UInt        uiLayerCnt   = 0;
  UInt        uiParLnCount = 0;

  FILE *f = fopen( rcFilename.c_str(), "r");
  if( NULL == f )
  {
    printf( "failed to open %s parameter file\n", rcFilename.c_str() );
    return Err::m_nERR;
  }

  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineStr ("OutputFile",              &rcBitstreamFile,                                      "test.264");
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("FrameRate",               &m_dMaximumFrameRate,                                  60.0      );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("MaxDelay",                &m_dMaximumDelay,                                      1200.0    );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("FramesToBeEncoded",       &m_uiTotalFrames,                                      1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("GOPSize",                 &m_uiGOPSize,                                          1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("IntraPeriod",             &m_uiIntraPeriod,                                      MSYS_UINT_MAX );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("NumberReferenceFrames",   &m_uiNumRefFrames,                                     1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("BaseLayerMode",           &m_uiBaseLayerMode,                                    0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("NumLayers",               &m_uiNumberOfLayers,                                   1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SearchRange",             &(m_cMotionVectorSearchParams.m_uiSearchRange),        96);
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("ELSearchRange",           &(m_cMotionVectorSearchParams.m_uiELSearchRange),      0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("FastBiSearch",            &(m_cMotionVectorSearchParams.m_uiFastBiSearch),       0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("BiPredIter",              &(m_cMotionVectorSearchParams.m_uiNumMaxIter),         4 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("IterSearchRange",         &(m_cMotionVectorSearchParams.m_uiIterSearchRange),    8 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("LoopFilterDisable",       &(m_cLoopFilterParams.m_uiFilterIdc),                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt ("LoopFilterAlphaC0Offset", &(m_cLoopFilterParams.m_iAlphaOffset),                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt ("LoopFilterBetaOffset",    &(m_cLoopFilterParams.m_iBetaOffset),                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("InterLayerLoopFilterDisable",       &(m_cInterLayerLoopFilterParams.m_uiFilterIdc),                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt ("InterLayerLoopFilterAlphaC0Offset", &(m_cInterLayerLoopFilterParams.m_iAlphaOffset),                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt ("InterLayerLoopFilterBetaOffset",    &(m_cInterLayerLoopFilterParams.m_iBetaOffset),                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SearchMode",              &(m_cMotionVectorSearchParams.m_uiSearchMode),         0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SearchFuncFullPel",       &(m_cMotionVectorSearchParams.m_uiFullPelDFunc),       0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SearchFuncSubPel",        &(m_cMotionVectorSearchParams.m_uiSubPelDFunc),        0 );

//TMM_WP
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("WeightedPrediction",         &m_uiIPMode,                                     0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("WeightedBiprediction",       &m_uiBMode,                                      0 );
//TMM_WP
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt("NonRequiredEnable",			&m_bNonRequiredEnable,							 0 );  //NonRequired JVT-Q066
  std::string cInputFile, cReconFile;
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("AVCMode",                 &m_uiAVCmode,                                          0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineStr ("InputFile",               &cInputFile,                                           "in.yuv");
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineStr ("ReconFile",               &cReconFile,                                           "rec.yuv");
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SourceWidth",             &m_uiFrameWidth,                                       0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SourceHeight",            &m_uiFrameHeight,                                      0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SymbolMode",              &m_uiSymbolMode,                                       1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("Enable8x8Transform",      &m_uiEnable8x8Trafo,                                   0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("ConstrainedIntraPred",    &m_uiConstrainedIntraPred,                             0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("ScalingMatricesPresent",  &m_uiScalingMatricesPresent,                           0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("BasisQP",                 &m_dBasisQp,                                          26 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("DPBSize",                 &m_uiDPBSize,                                           1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("NumRefFrames",            &m_uiNumDPBRefFrames,                                  1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("Log2MaxFrameNum",         &m_uiLog2MaxFrameNum,                                  4 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("Log2MaxPocLsb",           &m_uiLog2MaxPocLsb,                                    4 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineStr ("SequenceFormatString",    &m_cSequenceFormatString,                              "A0*n{P0}" );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer0Quant",        &m_adDeltaQpLayer[0],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer1Quant",        &m_adDeltaQpLayer[1],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer2Quant",        &m_adDeltaQpLayer[2],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer3Quant",        &m_adDeltaQpLayer[3],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer4Quant",        &m_adDeltaQpLayer[4],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineDbl ("DeltaLayer5Quant",        &m_adDeltaQpLayer[5],                                  0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MaxRefIdxActiveBL0",      &m_uiMaxRefIdxActiveBL0,                               1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MaxRefIdxActiveBL1",      &m_uiMaxRefIdxActiveBL1,                               1 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MaxRefIdxActiveP",        &m_uiMaxRefIdxActiveP,                                 1 );

  //JVT-R057 LA-RDO{
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("LARDO",                   &m_uiLARDOEnable,                                      0 );
  //JVT-R057 LA-RDO}

  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("RPEncCheck",							 &m_uiEssRPChkEnable,																					0  );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MVDiffThreshold",				 &m_uiMVThres,																					20 );

	//JVT-T073 {
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("NestingSEI",              &m_uiNestingSEIEnable,                                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("SceneInfo",               &m_uiSceneInfoEnable,                                  0 );
  //JVT-T073 }

	//JVT-W052
	m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("IntegrityCheckSEI",          &m_uiIntegrityCheckSEIEnable,                          false ); // Disabled due to buggy behaviour. mwi 070803, bug_fixed
	//JVT-W052

//JVT-S036 lsj start  //bug-fix suffix{{
//PreAndSuffixUnitEnable shall always be on in SVC contexts (i.e. when there are FGS/CGS/spatial enhancement layers)
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("PreAndSuffixUnitEnable",  &m_uiPreAndSuffixUnitEnable,                           1 ); //prefix unit
//JVT-S036 lsj end //bug-fix suffix}}
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("CgsSnrRefinement",        &m_uiCGSSNRRefinementFlag,                             0 );  //JVT-T054
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("TLNestingFlag",           &m_uiTlevelNestingFlag,                                0 );  //JVT-U085
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("TL0DepRepIdxSeiEnable",    &m_uiTl0DepRepIdxSeiEnable,                           0 );  //JVT-U116,JVT-W062

  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("EncodeKeyPictures",       &m_uiEncodeKeyPictures,                                0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSControl",              &m_uiMGSKeyPictureControl,                             0 );

// JVT-V068 HRD {
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("EnableNalHRD",            &m_uiNalHRD,                                           0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("EnableVclHRD",            &m_uiVclHRD,                                           0 );
// JVT-V068 HRD }
//JVT-W049 {
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("EnableRedundantKeyPic",   &m_uiRedundantKeyPic,                                  0 );
//JVT-W049 }

  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("ConstrainedIntraUps",     &m_uiCIUFlag,                                          0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("BiPredLT8x8Disable",      &m_uiBiPred8x8Disable,                                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MCBlocksLT8x8Disable",    &m_uiMCBlks8x8Disable,                                 0 );

  //JVT-W043 {
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("RCMinQP",                 &m_uiRCMinQP,                                         12 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("RCMaxQP",                 &m_uiRCMaxQP,                                         40 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MaxQpChange",             &m_uiMaxQpChange,                                      2 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("InitialQp",               &m_uiInitialQp,                                       30 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("BasicUnit",               &m_uiBasicUnit,                                       99 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("BitRate",                 &m_uiBitRate,                                      64000 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("RateControlEnable",       &m_uiRateControlEnable,                                0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("AdaptInitialQP",          &m_uiAdaptInitialQP,                                   0 );
  // JVT-W043 }
  //JVT-AD021 {
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineUInt("MultiLayerLambdaSel",     &m_uiMultiLayerLambda,                                 0 );
  //JVT-AD021 }
#if DOLBY_ENCMUX_ENABLE
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt("MuxMethod",     &m_iMuxMethod,                                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt("MuxFilter",     &m_iMuxFilter,                                 0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt("MuxOffset0",    &(m_iMuxOffset[0]),                              0 );
  m_pEncoderLines[uiParLnCount++] = new EncoderConfigLineInt("MuxOffset1",    &(m_iMuxOffset[1]),                              0 );
#endif
  m_pEncoderLines[uiParLnCount] = NULL;

  while (!feof(f))
  {
    RNOK( xReadLine( f, acTags ) );
    if ( acTags[0].empty() )
    {
      continue;
    }
    for (UInt ui=0; m_pEncoderLines[ui] != NULL; ui++)
    {
      if( acTags[0] == m_pEncoderLines[ui]->getTag() )
      {
        m_pEncoderLines[ui]->setVar( acTags[1] );
        break;
      }
    }
    if( acTags[0] == "LayerCfg" )
    {
      acLayerConfigName[uiLayerCnt++] = acTags[1];
      continue;
    }
  }

  uiParLnCount = 0;
  while (m_pEncoderLines[uiParLnCount] != NULL)
  {
    delete m_pEncoderLines[uiParLnCount];
    m_pEncoderLines[uiParLnCount] = NULL;
    uiParLnCount++;
  }

  if( m_uiAVCmode )
  {
    m_uiNumberOfLayers = 0;
    getLayerParameters(0).setInputFilename        ( (Char*)cInputFile.c_str() );
    getLayerParameters(0).setOutputFilename       ( (Char*)cReconFile.c_str() );
    getLayerParameters(0).setFrameWidthInSamples  ( m_uiFrameWidth );
    getLayerParameters(0).setFrameHeightInSamples ( m_uiFrameHeight );
    fclose( f );
    return Err::m_nOK;
  }

  if ( uiLayerCnt != m_uiNumberOfLayers )
  {
    fprintf(stderr, "Could not locate all layer config files: check config file syntax\n");
    AF();
  }

  fclose( f );
//JVT-T054{
  UInt uiPrevLayer        = 0;
  Double  dPrevTemp       = 0.0;
  UInt uiPrevWidth        = 0;
  UInt uiPrevHeight       = 0;
  UInt uiLastLayer        = 0;
//JVT-T054}
  Bool bInterlaced        = false;
  UInt ui                 = 0;

  for( ui = 0; ui < m_uiNumberOfLayers; ui++ )
  {
    Bool  bFrameMbsOnly = true;
    getLayerParameters(ui).setDependencyId(ui);
    RNOK( xReadLayerFromFile( acLayerConfigName[ui], getLayerParameters(ui) ) );
		if ( getLayerParameters(ui).m_uiMbAff || getLayerParameters(ui).m_uiPAff )
    {
      bFrameMbsOnly = false;
      bInterlaced   = true;
    }

    // HS: set base layer id
    UInt uiBaseLayerId = getLayerParameters(ui).getBaseLayerId();
    if( ui && uiBaseLayerId == MSYS_UINT_MAX )
    {
      uiBaseLayerId = ui - 1; // default value
    }
    getLayerParameters(ui).setBaseLayerId(uiBaseLayerId);
    // HS: set base layer id

    if( m_uiCGSSNRRefinementFlag )
    {
      if(ui == 0)
      {
        uiPrevLayer  = ui;
        dPrevTemp    = getLayerParameters(ui).getOutputFrameRate();
        uiPrevWidth  = getLayerParameters(ui).getFrameWidthInSamples ();
        uiPrevHeight = getLayerParameters(ui).getFrameHeightInSamples();
        getLayerParameters(ui).setLayerCGSSNR(ui);
        getLayerParameters(ui).setQualityLevelCGSSNR(0);
        uiLastLayer = uiPrevLayer;
        getLayerParameters(ui).setBaseLayerCGSSNR( MSYS_UINT_MAX );
        getLayerParameters(ui).setBaseQualityLevelCGSSNR( 0 );
      }
      else
      {
        if( dPrevTemp    == getLayerParameters(ui).getOutputFrameRate     () &&
            uiPrevWidth  == getLayerParameters(ui).getFrameWidthInSamples () &&
            uiPrevHeight == getLayerParameters(ui).getFrameHeightInSamples() )
        {
          // layer can be considered as a CGS refinement
          UInt uiLayerTemp = getLayerParameters(ui-1).getLayerCGSSNR();
          getLayerParameters(ui).setLayerCGSSNR(uiLayerTemp);
          UInt uiQualityLevelTemp = getLayerParameters(ui-1).getQualityLevelCGSSNR();
          getLayerParameters(ui).setQualityLevelCGSSNR( uiQualityLevelTemp + getLayerParameters(ui-1).getNumberOfQualityLevelsCGSSNR() );
          ROTREPORT( uiBaseLayerId != ui - 1, "BaseLayerId cannot be arbitrarily set for MGS enhancement layers!");
        }
        else
        {
          //layer is not a refinement from previous CGS layer
          uiLastLayer++;
          uiPrevLayer  = uiLastLayer;
          dPrevTemp    = getLayerParameters(ui).getOutputFrameRate();
          uiPrevWidth  = getLayerParameters(ui).getFrameWidthInSamples ();
          uiPrevHeight = getLayerParameters(ui).getFrameHeightInSamples();
          getLayerParameters(ui).setLayerCGSSNR(uiLastLayer);
          getLayerParameters(ui).setQualityLevelCGSSNR(0);
        }
        getLayerParameters(ui).setBaseLayerCGSSNR( getLayerParameters(uiBaseLayerId).getLayerCGSSNR() );
        getLayerParameters(ui).setBaseQualityLevelCGSSNR( getLayerParameters(uiBaseLayerId).getQualityLevelCGSSNR() + getLayerParameters(uiBaseLayerId).getNumberOfQualityLevelsCGSSNR() - 1 );
      }
    }
    else
    {
      getLayerParameters(ui).setLayerCGSSNR(ui);
      getLayerParameters(ui).setQualityLevelCGSSNR(0);
      if( ui )
      {
        getLayerParameters(ui).setBaseLayerCGSSNR( getLayerParameters(uiBaseLayerId).getLayerCGSSNR() );
        getLayerParameters(ui).setBaseQualityLevelCGSSNR( getLayerParameters(uiBaseLayerId).getQualityLevelCGSSNR() + getLayerParameters(uiBaseLayerId).getNumberOfQualityLevelsCGSSNR() - 1 );
      }
      else
      {
        getLayerParameters(ui).setBaseLayerCGSSNR( MSYS_UINT_MAX );
        getLayerParameters(ui).setBaseQualityLevelCGSSNR( 0 );
      }
    }

//DS_FIX_FT_09_2007
  //uiBaseLayerId is no more discardable
    if(uiBaseLayerId != MSYS_UINT_MAX)
    {
      getLayerParameters(uiBaseLayerId).setNonDiscardable();
    }
//~DS_FIX_FT_09_2007

    if( uiBaseLayerId != MSYS_UINT_MAX )
    {
      LayerParameters&  rcCurrLayer       = getLayerParameters( ui );
      LayerParameters&  rcBaseLayer       = getLayerParameters( uiBaseLayerId );
      ResizeParameters& rcCurrRP          = rcCurrLayer.getResizeParameters();
      ResizeParameters& rcBaseRP          = rcBaseLayer.getResizeParameters();
      rcCurrRP.m_iRefLayerFrmWidth        = rcBaseRP.m_iFrameWidth;
      rcCurrRP.m_iRefLayerFrmHeight       = rcBaseRP.m_iFrameHeight;
      rcCurrRP.m_iRefLayerWidthInSamples  = rcBaseRP.m_iWidthInSamples;
      rcCurrRP.m_iRefLayerHeightInSamples = rcBaseRP.m_iHeightInSamples;

      if( rcCurrRP.m_iRefLayerFrmWidth != rcCurrRP.m_iRefLayerWidthInSamples )
      {
        Int iShift  = 1;
        Int iDiv    = rcCurrRP.m_iRefLayerWidthInSamples << iShift;
        Int iScalW  = ( ( rcCurrRP.m_iScaledRefFrmWidth * rcCurrRP.m_iRefLayerFrmWidth + ( iDiv >> 1 ) ) / iDiv ) << iShift;
        rcCurrRP.m_iScaledRefFrmWidth = iScalW;
      }
      if( rcCurrRP.m_iRefLayerFrmHeight != rcCurrRP.m_iRefLayerHeightInSamples )
      {
        Int iShift  = ( rcCurrLayer.isInterlaced() ? 2 : 1 );
        Int iDiv    = rcCurrRP.m_iRefLayerHeightInSamples << iShift;
        Int iScalH  = ( ( rcCurrRP.m_iScaledRefFrmHeight * rcCurrRP.m_iRefLayerFrmHeight + ( iDiv >> 1 ) ) / iDiv ) << iShift;
        rcCurrRP.m_iScaledRefFrmHeight = iScalH;
      }

      if( rcCurrRP.m_iExtendedSpatialScalability == ESS_NONE )
      {
        if( rcCurrRP.m_iLeftFrmOffset ||
            rcCurrRP.m_iTopFrmOffset  ||
            rcCurrRP.m_iFrameWidth   != rcCurrRP.m_iScaledRefFrmWidth    ||
            rcCurrRP.m_iFrameHeight  != rcCurrRP.m_iScaledRefFrmHeight   ||
            rcCurrRP.m_iChromaPhaseX != rcCurrRP.m_iRefLayerChromaPhaseX ||
            rcCurrRP.m_iChromaPhaseY != rcCurrRP.m_iRefLayerChromaPhaseY )
        {
          rcCurrRP.m_iExtendedSpatialScalability = ESS_SEQ;
        }
      }
    }
    else
    {
      ResizeParameters& rcCurrRP              = getLayerParameters(ui).getResizeParameters();
      rcCurrRP.m_iExtendedSpatialScalability  = ESS_NONE;
      rcCurrRP.m_iRefLayerFrmWidth            = rcCurrRP.m_iFrameWidth;
      rcCurrRP.m_iRefLayerFrmHeight           = rcCurrRP.m_iFrameHeight;
      rcCurrRP.m_iRefLayerWidthInSamples      = rcCurrRP.m_iWidthInSamples;
      rcCurrRP.m_iRefLayerHeightInSamples     = rcCurrRP.m_iHeightInSamples;
    }
  }
  //>>> zhangxd_20101220 >>>
  for( ui = 0; ui < m_uiNumberOfLayers; ui++ )
  {
	  UInt uiBaseLayerId   = getLayerParameters( ui ).getBaseLayerId();
	  UInt maxDependencyId = getLayerParameters( m_uiNumberOfLayers - 1 ).getLayerCGSSNR();

	  if( ui && uiBaseLayerId == MSYS_UINT_MAX )
	  {
		  uiBaseLayerId = ui - 1; 
	  }

	  if( uiBaseLayerId != MSYS_UINT_MAX && uiBaseLayerId != ui - 1 )
	  {
		  for( UInt i = uiBaseLayerId + 1; i < ui; i++ )
      {
				getLayerParameters(i).setDiscardable();
      }
	  }

	  if( uiBaseLayerId != MSYS_UINT_MAX && getLayerParameters(ui).getLayerCGSSNR() == maxDependencyId && getLayerParameters(ui).getQualityLevelCGSSNR() != 0 )
	  {
		  getLayerParameters(ui).setDiscardable();
	  }
  }
  //<<< zhangxd_20101220 <<<

#if DOLBY_ENCMUX_ENABLE
  //check parameter integrity;
  if(m_iMuxMethod)
  {
    if(m_uiNumberOfLayers<2 || m_iMuxFilter>16 || m_iMuxFilter <0)
    {
      fprintf(stderr, "The parameters for MuxMethod(%d) is not correct!\n", m_iMuxMethod);
      return Err::m_nInvalidParameter;
    }
    if(m_iMuxMethod <0 || m_iMuxMethod >2)
    {
      fprintf(stderr, "MuxMethod(%d) is not supported yet!\n", m_iMuxMethod);
      return Err::m_nInvalidParameter;
    }
    //check the resolution;
    LayerParameters& cpLayer0 = getLayerParameters(0);
    LayerParameters& cpLayer1 = getLayerParameters(1);
    if(m_iMuxMethod == 1)
    {
      if(cpLayer0.getFrameWidthInSamples()*2 != cpLayer1.getFrameWidthInSamples() || cpLayer0.getFrameHeightInSamples() != cpLayer1.getFrameHeightInSamples())
      {
        fprintf(stderr, "The resolution for layer 0 and layer 1 is not correct for side by side muxing!\n");
        return Err::m_nInvalidParameter;
      }
    }
    else
    {
      if(cpLayer0.getFrameWidthInSamples() != cpLayer1.getFrameWidthInSamples() || cpLayer0.getFrameHeightInSamples()*2 != cpLayer1.getFrameHeightInSamples())
      {
        fprintf(stderr, "The resolution for layer 0 and layer 1 is not correct for top and bottom muxing!\n");
        return Err::m_nInvalidParameter;
      }
    }
  }
#endif

  return Err::m_nOK;
}



ErrVal EncoderCodingParameter::xReadLayerFromFile ( std::string&            rcFilename,
                                                    h264::LayerParameters&  rcLayer )
{
  std::string acTags[4];
  std::string cInputFilename, cOutputFilename, cMotionFilename;

  //S051{
  std::string cEncSIPFilename;
  //S051}

  UInt        uiParLnCount = 0;

  FILE *f = fopen( rcFilename.c_str(), "r");
  if( NULL == f )
  {
    printf( "failed to open %s layer config file\n", rcFilename.c_str() );
    return Err::m_nERR;
  }

  //--ICU/ETRI FMO Implementation
  UInt bSliceGroupChangeDirection_flag=0;

  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SourceWidth",            &(rcLayer.m_uiFrameWidthInSamples),      352       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SourceHeight",           &(rcLayer.m_uiFrameHeightInSamples),     288       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("FrameRateIn",            &(rcLayer.m_dInputFrameRate),            30        );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("FrameRateOut",           &(rcLayer.m_dOutputFrameRate),           30        );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("InputFile",              &cInputFilename,                         "test.yuv");
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ReconFile",              &cOutputFilename,                        "rec.yuv" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MbAff",                  &(rcLayer.m_uiMbAff),                    0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("PAff",                   &(rcLayer.m_uiPAff),                     0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SymbolMode",             &(rcLayer.m_uiEntropyCodingModeFlag),    1         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("Enable8x8Transform",     &(rcLayer.m_uiEnable8x8Trafo),           0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ConstrainedIntraPred",   &(rcLayer.m_uiConstrainedIntraPred),     0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ScalingMatricesPresent", &(rcLayer.m_uiScalingMatricesPresent),   0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MaxDeltaQP",             &(rcLayer.m_uiMaxAbsDeltaQP),            1         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("QP",                     &(rcLayer.m_dBaseQpResidual),            32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQPLP",                 &(rcLayer.m_dQpModeDecisionLP),          -1.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP0",                  &(rcLayer.m_adQpModeDecision[0]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP1",                  &(rcLayer.m_adQpModeDecision[1]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP2",                  &(rcLayer.m_adQpModeDecision[2]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP3",                  &(rcLayer.m_adQpModeDecision[3]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP4",                  &(rcLayer.m_adQpModeDecision[4]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("MeQP5",                  &(rcLayer.m_adQpModeDecision[5]),        32.0      );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("InterLayerPred",         &(rcLayer.m_uiInterLayerPredictionMode), 0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ILModePred",             &(rcLayer.m_uiILPredMode),           MSYS_UINT_MAX );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ILMotionPred",           &(rcLayer.m_uiILPredMotion),         MSYS_UINT_MAX );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ILResidualPred",         &(rcLayer.m_uiILPredResidual),       MSYS_UINT_MAX );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("CbQPIndexOffset",        &(rcLayer.m_iChromaQPIndexOffset),       0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("CrQPIndexOffset",        &(rcLayer.m_i2ndChromaQPIndexOffset),    0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("LowComplexityMbMode",    &(rcLayer.m_uiLowComplexMbEnable), 0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("UseESS",                 &(rcLayer.m_cResizeParameters.m_iExtendedSpatialScalability), 0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ESSPicParamFile",        &(rcLayer.m_cESSFilename),                                              "ess.dat" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSCropWidth",           &(rcLayer.m_cResizeParameters.m_iScaledRefFrmWidth),                   0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSCropHeight",          &(rcLayer.m_cResizeParameters.m_iScaledRefFrmHeight),                  0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSOriginX",             &(rcLayer.m_cResizeParameters.m_iLeftFrmOffset),                       0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSOriginY",             &(rcLayer.m_cResizeParameters.m_iTopFrmOffset),                       0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSChromaPhaseX",        &(rcLayer.m_cResizeParameters.m_iChromaPhaseX),              -1         );  // SSUN, Nov2005
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSChromaPhaseY",        &(rcLayer.m_cResizeParameters.m_iChromaPhaseY),               0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSBaseChromaPhaseX",    &(rcLayer.m_cResizeParameters.m_iRefLayerChromaPhaseX),      -1         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("ESSBaseChromaPhaseY",    &(rcLayer.m_cResizeParameters.m_iRefLayerChromaPhaseY),       0         );  // SSUN, Nov2005
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ForceReOrdering",        &(rcLayer.m_uiForceReorderingCommands),  0         );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("BaseLayerId",            &(rcLayer.m_uiBaseLayerId),              MSYS_UINT_MAX );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SliceMode",              &(rcLayer.m_uiSliceMode),                             0       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SliceArgument",          &(rcLayer.m_uiSliceArgument),            MSYS_UINT_MAX       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("NumSlicGrpMns1",         &(rcLayer.m_uiNumSliceGroupsMinus1),                  0       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SlcGrpMapType",          &(rcLayer.m_uiSliceGroupMapType),                     2       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SlcGrpChgDrFlag",        &(bSliceGroupChangeDirection_flag),         0       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("SlcGrpChgRtMus1",        &(rcLayer.m_uiSliceGroupChangeRateMinus1),           85       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("SlcGrpCfgFileNm",        &rcLayer.m_cSliceGroupConfigFileName,             "sgcfg.cfg" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("NumROI",                 &(rcLayer.m_uiNumROI),                  0       );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ROICfgFileNm",           &rcLayer.m_cROIConfigFileName,             "roicfg.cfg" );
// JVT-Q065 EIDR{
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("IDRPeriod",	            &(rcLayer.m_iIDRPeriod),								0		);
// JVT-Q065 EIDR}
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineInt ("IntraPeriod",	          &(rcLayer.m_iLayerIntraPeriod),							0		);
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("PLR",	                  &(rcLayer.m_uiPLR),								0		); //JVT-R057 LA-RDO
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("UseRedundantSlc",        &(rcLayer.m_uiUseRedundantSlice), 0   );  //JVT-Q054 Red. Picture
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("UseRedundantKeySlc",     &(rcLayer.m_uiUseRedundantKeySlice), 0   );  //JVT-W049
  //S051{
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr( "EncSIPFile", &cEncSIPFilename, "");
  //S051}

  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVectorMode", &(rcLayer.m_uiMGSVectorMode), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector0", &(rcLayer.m_uiMGSVect[0]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector1", &(rcLayer.m_uiMGSVect[1]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector2", &(rcLayer.m_uiMGSVect[2]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector3", &(rcLayer.m_uiMGSVect[3]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector4", &(rcLayer.m_uiMGSVect[4]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector5", &(rcLayer.m_uiMGSVect[5]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector6", &(rcLayer.m_uiMGSVect[6]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector7", &(rcLayer.m_uiMGSVect[7]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector8", &(rcLayer.m_uiMGSVect[8]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector9", &(rcLayer.m_uiMGSVect[9]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector10", &(rcLayer.m_uiMGSVect[10]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector11", &(rcLayer.m_uiMGSVect[11]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector12", &(rcLayer.m_uiMGSVect[12]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector13", &(rcLayer.m_uiMGSVect[13]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector14", &(rcLayer.m_uiMGSVect[14]), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MGSVector15", &(rcLayer.m_uiMGSVect[15]), 0 );

  // JVT-V035
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt ("AvcRewriteFlag",          &(rcLayer.m_bAVCRewriteFlag),                            0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt ("AvcAdaptiveRewriteFlag",  &(rcLayer.m_bAVCAdaptiveRewriteFlag),                    0 );

  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt ("SliceSkip",          &(rcLayer.m_uiSliceSkip),                            0 );

  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ExplicitQPCascading", &(rcLayer.m_uiExplicitQPCascading), 0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel0",         &(rcLayer.m_adDeltaQPTLevel[0]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel1",         &(rcLayer.m_adDeltaQPTLevel[1]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel2",         &(rcLayer.m_adDeltaQPTLevel[2]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel3",         &(rcLayer.m_adDeltaQPTLevel[3]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel4",         &(rcLayer.m_adDeltaQPTLevel[4]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel5",         &(rcLayer.m_adDeltaQPTLevel[5]),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineDbl ("DQP4TLevel6",         &(rcLayer.m_adDeltaQPTLevel[6]),    0 );

  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("IPCMRate",            &(rcLayer.m_uiIPCMRate),            0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatIntra4x4Y",    &(rcLayer.m_acScalMatFiles[0]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatIntra4x4U",    &(rcLayer.m_acScalMatFiles[1]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatIntra4x4V",    &(rcLayer.m_acScalMatFiles[2]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatInter4x4Y",    &(rcLayer.m_acScalMatFiles[3]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatInter4x4U",    &(rcLayer.m_acScalMatFiles[4]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatInter4x4V",    &(rcLayer.m_acScalMatFiles[5]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatIntra8x8Y",    &(rcLayer.m_acScalMatFiles[6]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineStr ("ScalMatInter8x8Y",    &(rcLayer.m_acScalMatFiles[7]),     "" );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("BiPredLT8x8Disable",  &(rcLayer.m_uiBiPred8x8Disable),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MCBlocksLT8x8Disable",&(rcLayer.m_uiMCBlks8x8Disable),    0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("BottomFieldFirst",    &(rcLayer.m_uiBotFieldFirst),       0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("UseLongTerm",         &(rcLayer.m_uiUseLongTerm),         0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("DisableBSlices",      &(rcLayer.m_uiPicCodingType),       0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("ProfileIdc",          &(rcLayer.m_uiProfileIdc),          0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MinLevelIdc",         &(rcLayer.m_uiLevelIdc),            0 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MMCOBaseEnable",			 &(rcLayer.m_uiMMCOBaseEnable),      1 );
  m_pLayerLines[uiParLnCount++] = new EncoderConfigLineUInt("MMCOEnable",          &(rcLayer.m_uiMMCOEnable),          1 );

  m_pLayerLines[uiParLnCount] = NULL;

  while (!feof(f))
  {
    RNOK( xReadLine( f, acTags ) );
    if ( acTags[0].empty() )
    {
      continue;
    }
    for (UInt ui=0; m_pLayerLines[ui] != NULL; ui++)
    {
      if( acTags[0] == m_pLayerLines[ui]->getTag() )
      {
        m_pLayerLines[ui]->setVar( acTags[1] );
        break;
      }
    }
  }

  for( UInt uiSMIdx = 0; uiSMIdx < 8; uiSMIdx++ )
  {
    if( rcLayer.m_acScalMatFiles[uiSMIdx].empty() )
    {
      ::memset( rcLayer.m_aaucScalingMatrices[uiSMIdx], 0x00, 64*sizeof(UChar) );
    }
    else
    {
      RNOK( xReadScalMat( rcLayer.m_aaucScalingMatrices[uiSMIdx], ( uiSMIdx < 6 ? 16 : 64 ), rcLayer.m_acScalMatFiles[uiSMIdx] ) );
    }
  }

  //S051{
  if(cEncSIPFilename.length())
  {
    rcLayer.setEncSIP(true);
    rcLayer.setInSIPFileName( (char*) cEncSIPFilename.c_str());
  }
  //S051}

  rcLayer.setInputFilename     ( (Char*)cInputFilename.c_str() );
  rcLayer.setOutputFilename    ( (Char*)cOutputFilename.c_str() );

  uiParLnCount = 0;
  while (m_pLayerLines[uiParLnCount] != NULL)
  {
    delete m_pLayerLines[uiParLnCount];
    m_pLayerLines[uiParLnCount] = NULL;
    uiParLnCount++;
  }

  rcLayer.m_cResizeParameters.m_iFrameWidth               = (Int)rcLayer.getFrameWidthInMbs () << 4;
  rcLayer.m_cResizeParameters.m_iFrameHeight              = (Int)rcLayer.getFrameHeightInMbs() << 4;
  rcLayer.m_cResizeParameters.m_iWidthInSamples           = (Int)rcLayer.getFrameWidthInSamples ();
  rcLayer.m_cResizeParameters.m_iHeightInSamples          = (Int)rcLayer.getFrameHeightInSamples();
  rcLayer.m_cResizeParameters.m_iRefLayerFrmWidth         = 0; // is set later
  rcLayer.m_cResizeParameters.m_iRefLayerFrmHeight        = 0; // is set later
  rcLayer.m_cResizeParameters.m_iRefLayerWidthInSamples   = 0; // is set later
  rcLayer.m_cResizeParameters.m_iRefLayerHeightInSamples  = 0; // is set later

  if( ! rcLayer.m_cResizeParameters.m_iExtendedSpatialScalability )
  {
    rcLayer.m_cResizeParameters.m_iScaledRefFrmWidth      = rcLayer.m_cResizeParameters.m_iWidthInSamples;
    rcLayer.m_cResizeParameters.m_iScaledRefFrmHeight     = rcLayer.m_cResizeParameters.m_iHeightInSamples;
    rcLayer.m_cResizeParameters.m_iLeftFrmOffset          = 0;
    rcLayer.m_cResizeParameters.m_iTopFrmOffset           = 0;
    rcLayer.m_cResizeParameters.m_iRefLayerChromaPhaseX   = rcLayer.m_cResizeParameters.m_iChromaPhaseX;
    rcLayer.m_cResizeParameters.m_iRefLayerChromaPhaseY   = rcLayer.m_cResizeParameters.m_iChromaPhaseY;
  }

  //--ICU/ETRI FMO Implementation : FMO stuff start
  rcLayer.m_bSliceGroupChangeDirection_flag = ( bSliceGroupChangeDirection_flag != 0 );
  RNOK( xReadSliceGroupCfg( rcLayer)); //Slice group configuration file
  //--ICU/ETRI FMO Implementation : FMO stuff end

  // ROI Config ICU/ETRI
  RNOK( xReadROICfg( rcLayer));

  ::fclose(f);

  return Err::m_nOK;
}

ErrVal
EncoderCodingParameter::xReadScalMat( UChar* pBuffer, Int iCnt, std::string& rcFileName )
{
  FILE* pFile = ::fopen( rcFileName.c_str(), "rt" );
  if( ! pFile )
  {
    fprintf( stderr, "Cannot open file \"%s\"!", rcFileName.c_str() );
    ROT(1);
  }
  for( Int iIdx = 0; iIdx < iCnt; iIdx++ )
  {
    Int iValue = 0;
    Int iRead  = fscanf( pFile, " %d", &iValue );
    if( iRead != 1 )
    {
      fprintf( stderr, "Error while reading from file \"%s\"!", rcFileName.c_str() );
      ROT(1);
    }
    if( iValue < 1 || iValue > 255 )
    {
      fprintf( stderr, "Values in file \"%s\" are outside the range [1;255]!", rcFileName.c_str() );
      ROT(1);
    }
    pBuffer[iIdx] = (UChar)iValue;
  }
  ::fclose( pFile );
  return Err::m_nOK;
}

ErrVal EncoderCodingParameter::xReadSliceGroupCfg( h264::LayerParameters&  rcLayer )
{
	UInt mapunit_height;
	UInt mb_height;
	UInt i;
	UInt mb_width;
 	FILE* sgfile=NULL;

	if( (rcLayer.getNumSliceGroupsMinus1()!=0)&&
		((rcLayer.getSliceGroupMapType() == 0) || (rcLayer.getSliceGroupMapType() == 2) || (rcLayer.getSliceGroupMapType() == 6)) )
	{
    if ( ! rcLayer.getSliceGroupConfigFileName().empty() &&
         ( sgfile = fopen( rcLayer.getSliceGroupConfigFileName().c_str(), "r" ) ) == NULL )
		{
      printf("Error open file %s", rcLayer.getSliceGroupConfigFileName().c_str() );
		}
		else
		{
			if (rcLayer.getSliceGroupMapType() == 0)
			{
				for(i=0;i<=rcLayer.getNumSliceGroupsMinus1();i++)
				{
					ROF( 1   == fscanf(sgfile,"%d",(rcLayer.getArrayRunLengthMinus1()+i)) );
					ROF( EOF != fscanf(sgfile,"%*[^\n]") );
				}
			}
			else if (rcLayer.getSliceGroupMapType() == 2)
			{
				// every two lines contain 'top_left' and 'bottom_right' value
				for(i=0;i<rcLayer.getNumSliceGroupsMinus1();i++)
				{
					ROF( 1   == fscanf(sgfile,"%d",(rcLayer.getArrayTopLeft()+i)) );
					ROF( EOF != fscanf(sgfile,"%*[^\n]") );
					ROF( 1   == fscanf(sgfile,"%d",(rcLayer.getArrayBottomRight()+i)) );
					ROF( EOF != fscanf(sgfile,"%*[^\n]") );
				}

			}
			else if (rcLayer.getSliceGroupMapType()== 6)
			{
				Int tmp;
        mb_width        = rcLayer.getFrameWidthInMbs      ();
        mb_height       = rcLayer.getFrameHeightInMbs     ();
        mapunit_height  = rcLayer.getFrameHeightInMapUnits();
        UInt  uiNumMapU = mapunit_height * mb_width;
        rcLayer.initSliceGroupIdArray( uiNumMapU );
				// each line contains slice_group_id for one Macroblock
				for (i=0;i<uiNumMapU;i++)
				{
					ROF( 1 == fscanf(sgfile," %d", &tmp) );
					rcLayer.setSliceGroupId(i,(UInt)tmp);
          ROF( tmp <= (Int)rcLayer.getNumSliceGroupsMinus1() );
				}

			}
			fclose(sgfile);
		}
	}
	return Err::m_nOK;

}


// ROI Config Read ICU/ETRI
ErrVal EncoderCodingParameter::xReadROICfg( h264::LayerParameters&  rcLayer )
{
	UInt i;
 	FILE* roifile=NULL;

	if ( (0 < rcLayer.getNumROI()) )
	{
		if ( ! rcLayer.getROIConfigFileName().empty() &&
         ( roifile = fopen( rcLayer.getROIConfigFileName().c_str(), "r" ) ) == NULL )
		{
			printf("Error open file %s", rcLayer.getROIConfigFileName().c_str() );
		}

		else
		{
			// every two lines contain 'top_left' and 'bottom_right' value
			for(i=0;i<rcLayer.getNumROI(); i++)
			{
				ROF( 1   == fscanf(roifile, "%d",(rcLayer.getROIID()+i)) );
				ROF( EOF != fscanf(roifile, "%*[^\n]") );
				ROF( 1   == fscanf(roifile, "%d",(rcLayer.getSGID()+i)) );
				ROF( EOF != fscanf(roifile, "%*[^\n]") );
				ROF( 1   == fscanf(roifile, "%d",(rcLayer.getSLID()+i)) );
				ROF( EOF != fscanf(roifile, "%*[^\n]") );
			}

			fclose(roifile);
		}

	}


	return Err::m_nOK;
}

#endif // !defined(AFX_ENCODERCODINGPARAMETER_H__145580A5_E0D6_4E9C_820F_EA4EF1E1B793__INCLUDED_)
