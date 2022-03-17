
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>



#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CFMO.h"

H264AVC_NAMESPACE_BEGIN

//#define PRINT_FMO_MAPS

//#define gMin(a,b) ((a)>(b))?b:a
//#define gMax(a,b) ((a)<(b))?b:a

//#define PRINT_FMO_MAPS


Bool FMO::m_siSGId[Max_Num_Slice_Groups];
int FMO::m_iPOC = -1;
int FMO::m_iFrame = -1;

/*!
 ************************************************************************
 * \brief
 *    Generates MapUnitToSliceGroupMap_
 *    Has to be called every time a new Picture Parameter Set is used
 *
 * \param pps
 *    Picture Parameter set to be used for map generation
 * \param sps
 *    Sequence Parameter set to be used for map generation
 *
 ************************************************************************
 */
int FMO::GenerateMapUnitToSliceGroupMap()
{

  if (initMapUnitToSliceGroupMap() == 0) return 0;

  switch (pps_.slice_group_map_type)
  {
  case 0:
    GenerateType0MapUnitMap ();
    break;
  case 1:
    GenerateType1MapUnitMap ();
    break;
  case 2:
    GenerateType2MapUnitMap ();
    break;
  case 3:
    GenerateType3MapUnitMap ();
    break;
  case 4:
    GenerateType4MapUnitMap ();
    break;
  case 5:
    GenerateType5MapUnitMap ();
    break;
  case 6:
    GenerateType6MapUnitMap ();
    break;
  default:
    printf ("Illegal slice_group_map_type %d , exit \n", pps_.slice_group_map_type);
    exit (-1);
  }
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    Generates MbToSliceGroupMap_ from MapUnitToSliceGroupMap
 *
 * \param pps
 *    Picture Parameter set to be used for map generation
 * \param sps
 *    Sequence Parameter set to be used for map generation
 *
 ************************************************************************
 */
int FMO::GenerateMbToSliceGroupMap()
{


  // allocate memory for MbToSliceGroupMap_
  mallocMbToSliceGroupMap();

  if (sps_.frame_mbs_only_flag|| img_.field_pic_flag)
  {
    for (unsigned i=0; i<img_.PicSizeInMbs; i++)
    {
      MbToSliceGroupMap_[i] = MapUnitToSliceGroupMap_[i];
    }
  }
  else
    if (sps_.mb_adaptive_frame_field_flag  &&  (!img_.field_pic_flag))
    {
      for (unsigned i=0; i<img_.PicSizeInMbs; i++)
      {
        MbToSliceGroupMap_[i] = MapUnitToSliceGroupMap_[i/2];
      }
    }
    else
    {
      for (unsigned i=0; i<img_.PicSizeInMbs; i++)
      {
        MbToSliceGroupMap_[i] = MapUnitToSliceGroupMap_[(i/(2*img_.PicWidthInMbs))*img_.PicWidthInMbs+(i%img_.PicWidthInMbs)];
      }
    }
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    FMO initialization: Generates MapUnitToSliceGroupMap and MbToSliceGroupMap_.
 *
 * \param pps
 *    Picture Parameter set to be used for map generation
 * \param sps
 *    Sequence Parameter set to be used for map generation
 ************************************************************************
 */
Void FMO::printFmoMaps()
{
#if 1 // NO_DEBUG
  return;
#endif

  unsigned i,j;

  printf("\n");
  printf("FMO Map (Units):\n");

  for (j=0; j<img_.PicHeightInMapUnits; j++)
  {
    for (i=0; i<img_.PicWidthInMbs; i++)
    {
      printf("%c",48+MapUnitToSliceGroupMap_[i+j*img_.PicWidthInMbs]);
    }
    printf("\n");
  }
  printf("\n");
  printf("FMO Map (Mb):\n");

  for (j=0; j<=sps_.pic_height_in_map_units_minus1; j++)
  {
    for (i=0; i<=sps_.pic_width_in_mbs_minus1; i++)
    {
      printf("%c",48+MbToSliceGroupMap_[i+j*img_.PicWidthInMbs]);
    }
    printf("\n");
  }
  printf("\n");
}

int FMO::init()
{
  if(MbToSliceGroupMap_)
  {
    delete[] MbToSliceGroupMap_;
  MbToSliceGroupMap_ = NULL;
  }
  if (MapUnitToSliceGroupMap_)
  {
    delete[] MapUnitToSliceGroupMap_;
  MapUnitToSliceGroupMap_ = NULL;
  }
  if( numMbInSliceGroup_ )        // fix HS
  {                               // fix HS
    delete[] numMbInSliceGroup_;  // fix HS
    numMbInSliceGroup_ = NULL;
  }

  InitFirstMBsInSlices();


  GenerateMapUnitToSliceGroupMap();
  GenerateMbToSliceGroupMap();

  NumberOfSliceGroups_ = pps_.num_slice_groups_minus1+1;

  calcMbNumInSliceGroup();

  printFmoMaps();
  m_bInitialized = true;
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    Free memory allocated by FMO functions
 ************************************************************************
 */
int FMO::finit()
{
  if (MbToSliceGroupMap_)
  {
    delete[] MbToSliceGroupMap_;
    MbToSliceGroupMap_ = NULL;
  }
  if (MapUnitToSliceGroupMap_)
  {
    delete[] MapUnitToSliceGroupMap_;
    MapUnitToSliceGroupMap_ = NULL;
  }
  if( numMbInSliceGroup_ )        // fix HS
  {                               // fix HS
    delete[] numMbInSliceGroup_;  // fix HS
    numMbInSliceGroup_ = NULL;
  }
  m_bInitialized = false;
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetNumberOfSliceGroup()
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FMO::getNumberOfSliceGroup()
{
  return NumberOfSliceGroups_;
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetLastMBOfPicture()
 *    returns the macroblock number of the last MB in a picture.  This
 *    mb happens to be the last macroblock of the picture if there is only
 *    one slice group
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FMO::getLastMBOfPicture()
{
  return getLastMBInSliceGroup (getNumberOfSliceGroup()-1);
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetLastMBInSliceGroup: Returns MB number of last MB in SG
 *
 * \par Input:
 *    SliceGroupID (0 to 7)
 ************************************************************************
 */

int FMO::getLastMBInSliceGroup (int SliceGroup)
{
  for (int i=img_.PicSizeInMbs-1; i>=0; i--)
    if (getSliceGroupId (i) == SliceGroup)
      return i;
  return -1;

};


/*!
 ************************************************************************
 * \brief
 *    Returns SliceGroupID for a given MB
 *
 * \param mb
 *    Macroblock number (in scan order)
 ************************************************************************
 */
int FMO::getSliceGroupId (int mb) const
{
  assert (mb < (int)img_.PicSizeInMbs);
  assert (MbToSliceGroupMap_ != NULL);
  return MbToSliceGroupMap_[mb];
}



Void FMO::calcMbNumInSliceGroup()
{
  if( numMbInSliceGroup_ != NULL)
    delete[] numMbInSliceGroup_;

  numMbInSliceGroup_ = new int[NumberOfSliceGroups_];

  int i;
  for( i=0; i<NumberOfSliceGroups_; i++)
    numMbInSliceGroup_[i] = 0;

  for( i=0; i<(Int)PicSizeInMapUnits_; i++)
    numMbInSliceGroup_[getSliceGroupId(i)]++;
}


int FMO::getNumMbInSliceGroup(int sliceGroupID)
{
  return numMbInSliceGroup_[sliceGroupID];
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetNextMBBr: Returns the MB-Nr (in scan order) of the next
 *    MB in the (scattered) Slice, -1 if the slice is finished
 *
 * \param CurrentMbNr
 *    number of the current macroblock
 ************************************************************************
 */
int FMO::getNextMBNr (int CurrentMbNr)
{
  int SliceGroup = getSliceGroupId (CurrentMbNr);

  while (++CurrentMbNr<(int)img_.PicSizeInMbs && MbToSliceGroupMap_ [CurrentMbNr] != SliceGroup)
    ;

  if (CurrentMbNr >= (int)img_.PicSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
}


/*!
 ************************************************************************
 * \brief
 *    Generate interleaved slice group map type MapUnit map (type 0)
 *
 ************************************************************************
 */
Void FMO::GenerateType0MapUnitMap ()
{

  unsigned iGroup, j;
  unsigned i = 0;
  do
  {
    for( iGroup = 0;
         (iGroup <= pps_.num_slice_groups_minus1) && (i < PicSizeInMapUnits_);
         i += pps_.run_length_minus1[iGroup++] + 1 )
    {
      for( j = 0; j <= pps_.run_length_minus1[ iGroup ] && i + j < PicSizeInMapUnits_; j++ )
        MapUnitToSliceGroupMap_[i+j] = iGroup;
    }
  }
  while( i < PicSizeInMapUnits_);
}


/*!
 ************************************************************************
 * \brief
 *    Generate dispersed slice group map type MapUnit map (type 1)
 *
 ************************************************************************
 */
Void FMO::GenerateType1MapUnitMap()
{

  unsigned i;
  for( i = 0; i < PicSizeInMapUnits_; i++ )
  {
    MapUnitToSliceGroupMap_[i] = ((i%img_.PicWidthInMbs)+(((i/img_.PicWidthInMbs)*(pps_.num_slice_groups_minus1+1))/2))
                                %(pps_.num_slice_groups_minus1+1);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Generate foreground with left-over slice group map type MapUnit map (type 2)
 *
 ************************************************************************
 */
Void FMO::GenerateType2MapUnitMap ()
{

  int iGroup;
  unsigned i, x, y;
  unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for( i = 0; i < PicSizeInMapUnits_; i++ )
    MapUnitToSliceGroupMap_[ i ] = pps_.num_slice_groups_minus1;


  for( iGroup = pps_.num_slice_groups_minus1 - 1 ; iGroup >= 0; iGroup-- )
  {
  yTopLeft = GetYTopLeft(iGroup);
  xTopLeft = GetXTopLeft(iGroup);

  yBottomRight = GetYBottomRight(iGroup);
  xBottomRight = GetXBottomRight(iGroup);

  assert(xBottomRight >=xTopLeft);
  assert(yBottomRight >=yTopLeft);

    for( y = yTopLeft; y <= yBottomRight; y++ )
      for( x = xTopLeft; x <= xBottomRight; x++ )
        MapUnitToSliceGroupMap_[ y * img_.PicWidthInMbs + x ] = iGroup;
 }
}


/*!
 ************************************************************************
 * \brief
 *    Generate box-out slice group map type MapUnit map (type 3)
 *
 ************************************************************************
 */
Void FMO::GenerateType3MapUnitMap()
{

  unsigned i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  unsigned mapUnitsInSliceGroup0 = gMin((pps_.slice_group_change_rate_minus1 + 1) * img_.slice_group_change_cycle, PicSizeInMapUnits_);

  for( i = 0; i < PicSizeInMapUnits_; i++ )
    MapUnitToSliceGroupMap_[ i ] = 2;

  x = ( img_.PicWidthInMbs - pps_.slice_group_change_direction_flag ) / 2;
  y = ( img_.PicHeightInMapUnits - pps_.slice_group_change_direction_flag ) / 2;

  leftBound   = x;
  topBound    = y;
  rightBound  = x;
  bottomBound = y;

  xDir =  pps_.slice_group_change_direction_flag - 1;
  yDir =  pps_.slice_group_change_direction_flag;

  for( k = 0; k < PicSizeInMapUnits_; k += mapUnitVacant )
  {
    mapUnitVacant = ( MapUnitToSliceGroupMap_[ y * img_.PicWidthInMbs + x ]  ==  2 );
    if( mapUnitVacant )
       MapUnitToSliceGroupMap_[ y * img_.PicWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound )
    {
      leftBound = gMax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps_.slice_group_change_direction_flag - 1;
    }
    else
      if( xDir  ==  1  &&  x  ==  rightBound )
      {
        rightBound = gMin( rightBound + 1, (int)img_.PicWidthInMbs - 1 );
        x = rightBound;
        xDir = 0;
        yDir = 1 - 2 * pps_.slice_group_change_direction_flag;
      }
      else
        if( yDir  ==  -1  &&  y  ==  topBound )
        {
          topBound = gMax( topBound - 1, 0 );
          y = topBound;
          xDir = 1 - 2 * pps_.slice_group_change_direction_flag;
          yDir = 0;
         }
        else
          if( yDir  ==  1  &&  y  ==  bottomBound )
          {
            bottomBound = gMin( bottomBound + 1, (int)img_.PicHeightInMapUnits - 1 );
            y = bottomBound;
            xDir = 2 * pps_.slice_group_change_direction_flag - 1;
            yDir = 0;
          }
          else
          {
            x = x + xDir;
            y = y + yDir;
          }
  }

}

/*!
 ************************************************************************
 * \brief
 *    Generate raster scan slice group map type MapUnit map (type 4)
 *
 ************************************************************************
 */
Void FMO::GenerateType4MapUnitMap()
{

  unsigned mapUnitsInSliceGroup0 = gMin((pps_.slice_group_change_rate_minus1 + 1) * img_.slice_group_change_cycle, PicSizeInMapUnits_);
  unsigned sizeOfUpperLeftGroup = pps_.slice_group_change_direction_flag ? ( PicSizeInMapUnits_ - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i;

  for( i = 0; i < PicSizeInMapUnits_; i++ )
    if( i < sizeOfUpperLeftGroup )
        MapUnitToSliceGroupMap_[ i ] = pps_.slice_group_change_direction_flag;
    else
        MapUnitToSliceGroupMap_[ i ] = 1 - pps_.slice_group_change_direction_flag;

}

/*!
 ************************************************************************
 * \brief
 *    Generate wipe slice group map type MapUnit map (type 5)
 *
 ************************************************************************
 */
Void FMO::GenerateType5MapUnitMap ()
{

  unsigned mapUnitsInSliceGroup0 = gMin((pps_.slice_group_change_rate_minus1 + 1) * img_.slice_group_change_cycle, PicSizeInMapUnits_);
  unsigned sizeOfUpperLeftGroup = pps_.slice_group_change_direction_flag ? ( PicSizeInMapUnits_ - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i,j, k = 0;

  for( j = 0; j < img_.PicWidthInMbs; j++ )
    for( i = 0; i < img_.PicHeightInMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            MapUnitToSliceGroupMap_[ i * img_.PicWidthInMbs + j ] = 1 - pps_.slice_group_change_direction_flag;
        else
            MapUnitToSliceGroupMap_[ i * img_.PicWidthInMbs + j ] = pps_.slice_group_change_direction_flag;

}

/*!
 ************************************************************************
 * \brief
 *    Generate explicit slice group map type MapUnit map (type 6)
 *
 ************************************************************************
 */
Void FMO::GenerateType6MapUnitMap()
{
  unsigned i;
  for (i=0; i<PicSizeInMapUnits_; i++)
  {
    MapUnitToSliceGroupMap_[i] = pps_.slice_group_id_map[i];
  }
}

Void FMO::mallocMbToSliceGroupMap()
{
  if (MbToSliceGroupMap_ != NULL)
    delete[] MbToSliceGroupMap_;

  MbToSliceGroupMap_ = new int [img_.PicSizeInMbs ];

  if (MbToSliceGroupMap_ == NULL)
  {
    printf ("cannot allocated %u bytes for MbToSliceGroupMap_, exit\n", static_cast<unsigned int>((img_.PicSizeInMbs) * sizeof(int)) );
    exit (-1);
  }

}

int FMO::initMapUnitToSliceGroupMap()
{
  // allocate memory for MapUnitToSliceGroupMap
  if (MapUnitToSliceGroupMap_)
    delete[] MapUnitToSliceGroupMap_;
  if ((MapUnitToSliceGroupMap_ = new int[GetNumSliceGroupMapUnits()]) == NULL)
  {
    printf ("cannot allocated %u bytes for MapUnitToSliceGroupMap, exit\n", static_cast<unsigned int>((pps_.num_slice_group_map_units_minus1+1) * sizeof (int)) );
    exit (-1);
  }

  if (pps_.num_slice_groups_minus1 == 0)    // only one slice group
  {
    memset (MapUnitToSliceGroupMap_, 0, GetNumSliceGroupMapUnits() * sizeof (int));
    return 0;
  }
  return 1;
}

unsigned FMO::GetNumSliceGroupMapUnits()
{

  PicSizeInMapUnits_= (sps_.pic_height_in_map_units_minus1+1)* (sps_.pic_width_in_mbs_minus1+1);

  if (pps_.slice_group_map_type == 6)
  {
    if ((pps_.num_slice_group_map_units_minus1+1) != PicSizeInMapUnits_)
    {
      //error ("wrong pps_.num_slice_group_map_units_minus1 for used SPS and FMO type 6", 500);
      printf("wrong pps_.num_slice_group_map_units_minus1 for used SPS and FMO type 6");
    }
  }
  return PicSizeInMapUnits_;

}

unsigned int FMO::GetYTopLeft(int iGroup)
{
  return pps_.top_left[ iGroup ] / img_.PicWidthInMbs;
}

unsigned int FMO::GetXTopLeft(int iGroup)
{
  return pps_.top_left[ iGroup ] % img_.PicWidthInMbs;
}

unsigned int FMO::GetYBottomRight(int iGroup)
{
  return pps_.bottom_right[ iGroup ] / img_.PicWidthInMbs;
}

unsigned int FMO::GetXBottomRight(int iGroup)
{
  return pps_.bottom_right[ iGroup ] % img_.PicWidthInMbs;
}

/*!
 ************************************************************************
 * \brief
 *    FmoStartPicture: initializes FMO at the begin of each new picture
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FMO::StartPicture ()
{
  int i;

  assert (MbToSliceGroupMap_ != NULL);

  for (i=0; i<Max_Num_Slice_Groups; i++)
    FirstMBInSlice[i] = getFirstMBOfSliceGroup (i);
  return 0;
}

/*!
 ************************************************************************
 * \brief
 *    FmoEndPicture: Ends the Scattered Slices Module (called once
 *    per picture).
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FMO::EndPicture ()
{
  // Do nothing
  return 0;
}



/*!
 ************************************************************************
 * \brief
 *    FmoGetNextMBBr: Returns the MB-Nr (in scan order) of the next
 *    MB in the (FMO) Slice, -1 if the SliceGroup is finished
 *
 * \par Input:
 *    CurrentMbNr
 ************************************************************************
 */
int FMO::getPreviousMBNr (int CurrentMbNr)
{

  int  SliceGroupID = getSliceGroupId (CurrentMbNr);
  CurrentMbNr--;
  while (CurrentMbNr>=0 &&  MbToSliceGroupMap_[CurrentMbNr] != SliceGroupID)
    CurrentMbNr--;

  if (CurrentMbNr < 0)
    return -1;    // No previous MB in this slice
  else
    return CurrentMbNr;
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetFirstMBOfSliceGroup: Returns the MB-Nr (in scan order) of the
 *    next first MB of the Slice group, -1 if no such MB exists
 *
 * \par Input:
 *    SliceGroupID: Id of SliceGroup
 ************************************************************************
 */
int FMO::getFirstMBOfSliceGroup (int SliceGroupID)
{
  int i = 0;
  while ((i<(int)img_.PicSizeInMbs) && (getSliceGroupId (i) != SliceGroupID))
    i++;

  if (i < (int)img_.PicSizeInMbs)
    return i;
  else
    return -1;
}


/*!
 ************************************************************************
 * \brief
 *    FmoGetLastCodedMBOfSlice: Returns the MB-Nr (in scan order) of
 *    the last MB of the slice group
 *
 * \par Input:
 *    SliceGroupID
 * \par Return
 *    MB Nr in case of success (is always >= 0)
 *    -1 if the SliceGroup doesn't exist
 ************************************************************************
 */
int FMO::getLastCodedMBOfSliceGroup (int SliceGroupID)
{
  int i;
  int LastMB = -1;

  for (i=0; i<(int)img_.PicSizeInMbs; i++)
    if (getSliceGroupId (i) == SliceGroupID)
      LastMB = i;
  return LastMB;
}


Void FMO::setLastMacroblockInSlice ( int mb)
{
  // called by terminate_slice(), writes the last processed MB into the
  // FirstMBInSlice[MAXnum_slice_groups_minus1] array.  FmoGetFirstMacroblockInSlice()
  // uses this info to identify the first uncoded MB in each slice group

  int currSliceGroup = getSliceGroupId (mb);
  assert (mb >= 0);
  mb = getNextMBNr (mb);   // The next (still uncoded) MB, or -1 if SG is finished
  FirstMBInSlice[currSliceGroup] = mb;
}

int FMO::getFirstMacroblockInSlice ( int SliceGroup)
{
  return FirstMBInSlice[SliceGroup];
  // returns the first uncoded MB in each slice group, -1 if there is no
  // more to do in this slice group
}


int FMO::SliceGroupCompletelyCoded( int SliceGroupID)
{
  if (getFirstMacroblockInSlice (SliceGroupID) < 0)  // slice group completelty coded or not present
    return 1;//TRUE;
  else
    return 0;//FALSE;
}

Int
FMO::getFirstSliceGroupId()
{
  Int iID = 0;
  for( ; iID < Max_Num_Slice_Groups; iID++ )
  {
    if( getFirstMBOfSliceGroup( iID ) >= 0 )
    {
      break;
    }
  }
  return iID;
}

Int
FMO::getNextSliceGroupId( Int iPrevSliceGroupId )
{
  Int iID = iPrevSliceGroupId + 1;
  for( ; iID < Max_Num_Slice_Groups; iID++ )
  {
    if( getFirstMBOfSliceGroup( iID ) >= 0 )
    {
      break;
    }
  }
  return iID;
}

Void FMO::InitFirstMBsInSlices()
{
  for( Int k = 0; k <= Max_Num_Slice_Groups; k++ )
    FirstMBInSlice[ k ] = -1;
}

// JVT-S054 (2) (ADD) ->
UInt FMO::getNumMbsInSlice(UInt uiFirstMbInSlice, UInt uiLastMbInSlice)
{
  UInt uiNumMBsInSlice = 0;
  UInt uiMbAddress = uiFirstMbInSlice;
  while (uiMbAddress <= uiLastMbInSlice)
  {
    uiNumMBsInSlice++;
    uiMbAddress = getNextMBNr(uiMbAddress );
  }
  return uiNumMBsInSlice;
}

int FMO::getLastMbInSlice(UInt uiFirstMbInSlice, UInt uiNumMbsInSlice)
{
  int iCurrMb, iNextMb;
  iCurrMb = uiFirstMbInSlice;
  iNextMb = uiFirstMbInSlice;
  for (UInt uiMBCount = 0; uiMBCount < uiNumMbsInSlice; uiMBCount++)
  {
    iCurrMb = iNextMb;
    iNextMb = getNextMBNr(iCurrMb);
    if (iNextMb == -1)
      break;
  }
  return iCurrMb;
}
// JVT-S054 (2) (ADD) <-

H264AVC_NAMESPACE_END
