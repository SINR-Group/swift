
#if !defined(AFX_SEQUENCESTRUCTURE_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)
#define AFX_SEQUENCESTRUCTURE_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef std::string String;


H264AVC_NAMESPACE_BEGIN


class FrameSpec
{
public:
  FrameSpec  ();
  ~FrameSpec ();

  Void              uninit            ();
  ErrVal            init              ( UChar       ucType,
                                        UInt        uiContFrameNum,
                                        Bool        bAnchor,
                                        UInt        uiFramesSkipped,
                                        Bool        bUseBaseRep,
                                        UInt        uiLayer,
                                        DecRefPicMarking* pcMmcoBuf,
                                        RefPicListReOrdering* pcRplrBufL0,
                                        RefPicListReOrdering* pcRplrBufL1 );

  Bool              isInitialized     ()                  const;
  UInt              getContFrameNumber()                  const;
  SliceType         getSliceType      ()                  const;
  NalUnitType       getNalUnitType    ()                  const;
  NalRefIdc         getNalRefIdc      ()                  const;
  Bool              isSkipped         ()                  const;
  Bool              isBaseRep         ()                  const;
  Bool              isAnchor          ()                  const;
  UInt              getFramesSkipped  ()                  const;
  UInt              getTemporalLayer  ()                  const;
  const DecRefPicMarking* getMmcoBuf        ()                  const;
  const RefPicListReOrdering* getRplrBuf        ( ListIdx eLstIdx)  const;

private:
  Bool          m_bInit;
  UInt          m_uiContFrameNumber;
  SliceType     m_eSliceType;
  NalUnitType   m_eNalUnitType;
  NalRefIdc     m_eNalRefIdc;
  Bool          m_bUseBaseRep;
  Bool          m_bAnchor;
  UInt          m_uiFramesSkipped;
  UInt          m_uiTemporalLayer;
  DecRefPicMarking*   m_pcMmcoBuf;
  RefPicListReOrdering*   m_apcRplrBuf[2];
};



class  FormattedStringParser
{
public:
  static  ErrVal  separatString               ( const String&   rcString,
                                                String&         rcFDString,
                                                String&         rcMmcoString,
                                                String&         rcRplrStringL0,
                                                String&         rcRplrStringL1 );
  static  ErrVal  extractRplr                 ( const String&   rcString,
                                                RefPicListReOrdering&     rcRplrBuf);
  static  ErrVal  extractMmco                 ( const String&   rcString,
                                                DecRefPicMarking&     rcMmcoBuf );
  static  ErrVal  extractSingleRplrCommand   ( const String&   rcString,
                                                RplrCommand&           rcRplr );
  static  ErrVal  extractSingleMmcoCommand    ( const String&   rcString,
                                                MmcoCommand&           rcMmco );
  static  ErrVal  extractFrameDescription     ( const String&   rcString,
                                                UChar&          rucType,
                                                UInt&           ruiIncrement,
                                                Bool&           rbUseBaseRep,
                                                UInt&           ruiLayer );
  static  Bool    isFrameSequencePart         ( const String&   rcString );
  static  ErrVal  extractRepetitions          ( const String&   rcString,
                                                String&         rcNoRepString,
                                                UInt&           ruiNumberOfRepetitions );
  static  ErrVal  getNumberOfFrames           ( const String&   rcString,
                                                UInt&           ruiNumberOfFrames );
  static  ErrVal  extractNextFrameDescription ( const String&   rcString,
                                                String&         rcFDString,
                                                UInt&           ruiStartPos );
  static  ErrVal  getNumberOfParts            ( const String&   rcString,
                                                UInt&           ruiNumberOfParts );
  static  ErrVal  extractPart                 ( const String&   rcString,
                                                String&         rcPartString,
                                                UInt&           ruiStartPos );

private:
  static const String sm_cSetOfTypes;
  static const String sm_cSetOfDigits;
  static const String sm_cSetOfPartStart;
};




class SequenceStructure
{
private:

  class FrameDescriptor
  {
  public:
    FrameDescriptor             ();
    ~FrameDescriptor            ();

    Void    uninit              ();
    ErrVal  init                ( const String&   rcString,
                                  UInt            uiLastAnchorFrameNumIncrement );
    ErrVal  reduceFramesSkipped ( UInt            uiNotCoded );

    ErrVal  check               ()  const;
    Bool    isIDR               ()  const;
    Bool    isSkipped           ()  const;
    Bool    isCoded             ()  const;
    Bool    isReference         ()  const;
    Bool    isAnchor            ()  const;
    Bool    isBaseRep           ()  const;
    UInt    getIncrement        ()  const;
    UInt    getFramesSkipped    ()  const;

    ErrVal  setFrameSpec        ( FrameSpec&      rcFrameSpec,
                                  UInt            uiFrameNumOffset ) const;

  private:
    Bool        m_bInit;
    UChar       m_ucType;
    UInt        m_uiFrameNumIncrement;
    Bool        m_bAnchor;
    UInt        m_uiFramesSkipped;
    Bool        m_bUseBaseRep;
    UInt        m_uiLayer;
    DecRefPicMarking* m_pcMmcoBuf;
    RefPicListReOrdering* m_apcRplrBuf[2];
  };


  class SequencePart
  {
  public:
    SequencePart                        ()  {}
    virtual ~SequencePart               ()  {}

    virtual Void    uninit              ()                                      = 0;
    virtual ErrVal  init                ( const String& rcString)               = 0;
    virtual Void    reset               ()                                      = 0;
    virtual ErrVal  check               ()                                      = 0;

    virtual Bool    isFirstIDR          ()  const                               = 0;
    virtual UInt    getMinDPBSizeRef    ()  const                               = 0;
    virtual UInt    getMinDPBSizeNonRef ()  const                               = 0;

    virtual Bool    getNextFrameSpec    ( FrameSpec&    rcFrameSpec,
                                          UInt&         uiFrameNumPartOffset )  = 0;
  };


  class FrameSequencePart : public SequencePart
  {
  public:
    FrameSequencePart             ();
    virtual ~FrameSequencePart    ();

    Void    uninit                ();
    ErrVal  init                  ( const String& rcString );
    Void    reset                 ();
    ErrVal  check                 ();

    Bool    isFirstIDR            ()  const;
    UInt    getMinDPBSizeRef      ()  const;
    UInt    getMinDPBSizeNonRef   ()  const;

    Bool    getNextFrameSpec      ( FrameSpec&    rcFrameSpec,
                                    UInt&         uiFrameNumPartOffset );

  private:
    Bool              m_bInit;
    FrameDescriptor*  m_pacFrameDescriptor;
    UInt              m_uiNumberOfFrames;
    UInt              m_uiCurrentFrame;
    UInt              m_uiNumberOfRepetitions;
    UInt              m_uiCurrentRepetition;
    UInt              m_uiMinDPBSizeRef;
    UInt              m_uiMinDPBSizeNonRef;
  };


  class GeneralSequencePart : public SequencePart
  {
  public:
    GeneralSequencePart           ();
    virtual ~GeneralSequencePart  ();

    Void    uninit                ();
    ErrVal  init                  ( const String& rcString );
    Void    reset                 ();
    ErrVal  check                 ();

    Bool    isFirstIDR            ()  const;
    UInt    getMinDPBSizeRef      ()  const;
    UInt    getMinDPBSizeNonRef   ()  const;

    Bool    getNextFrameSpec      ( FrameSpec&    rcFrameSpec,
                                    UInt&         uiFrameNumPartOffset );

  private:
    Bool              m_bInit;
    SequencePart**    m_papcSequencePart;
    UInt              m_uiNumberOfParts;
    UInt              m_uiCurrentPart;
    UInt              m_uiNumberOfRepetitions;
    UInt              m_uiCurrentRepetition;
    UInt              m_uiMinDPBSizeRef;
    UInt              m_uiMinDPBSizeNonRef;
  };


protected:
  SequenceStructure                 ();
  virtual ~SequenceStructure        ();

public:
  static ErrVal     create          ( SequenceStructure*& rpcSequenceStructure );
  ErrVal            destroy         ();
  ErrVal            uninit          ();
  ErrVal            init            ( const String&       rcString,
                                      UInt                uiNumberOfFrames );
  Void              reset           ();
  ErrVal            check           ();

  const FrameSpec&  getFrameSpec    ();
  const FrameSpec&  getNextFrameSpec();

  static  Bool      checkString     ( const String&       rcString );
  static  ErrVal    debugOutput     ( const String&       rcString,
                                      UInt                uiNumberOfFrames,
                                      FILE*               pcFile );


  UInt    getNumberOfTotalFrames    ()  const;
  UInt    getNumberOfIDRFrames      ()  const;
  UInt    getNumberOfIntraFrames    ()  const;
  UInt    getNumberOfInterFrames    ()  const;
  UInt    getNumberOfRefFrames      ()  const;
  UInt    getNumberOfCodedFrames    ()  const;
  UInt    getMaxAbsFrameDiffRef     ()  const;
  UInt    getMinDelay               ()  const;
  UInt    getNumTemporalLayers      ()  const;


private:
  Bool    xIsFirstIDR               ();
  Void    xInitParameters           ();
  ErrVal  xGetNextValidFrameSpec    ( FrameSpec&          rcFrameSpec,
                                      UInt                uiTotalFrames );

private:
  Bool                m_bInit;
  UInt                m_uiNumberOfTotalFrames;
  UInt                m_uiNumberOfFramesProcessed;
  UInt                m_uiFrameNumPartOffset;
  SequencePart*       m_pcSequencePart;
  FrameSpec           m_cFrameSpec;
  UInt                m_uiNumIDR;
  UInt                m_uiNumIntra;
  UInt                m_uiNumRef;
  UInt                m_uiNumCoded;
  UInt                m_uiMaxAbsFrameDiffRef;
  UInt                m_uiMinDelay;
  UInt                m_uiMaxLayer;
};



H264AVC_NAMESPACE_END

#endif // !defined(AFX_SEQUENCESTRUCTURE_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)


