/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2014, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>


enum NalUnitType
{
  NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
  NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

  NAL_UNIT_CODED_SLICE_TSA_N,     // 2
  NAL_UNIT_CODED_SLICE_TSA_R,       // 3

  NAL_UNIT_CODED_SLICE_STSA_N,    // 4
  NAL_UNIT_CODED_SLICE_STSA_R,    // 5

  NAL_UNIT_CODED_SLICE_RADL_N,    // 6
  NAL_UNIT_CODED_SLICE_RADL_R,      // 7

  NAL_UNIT_CODED_SLICE_RASL_N,    // 8
  NAL_UNIT_CODED_SLICE_RASL_R,      // 9

  NAL_UNIT_RESERVED_VCL_N10,
  NAL_UNIT_RESERVED_VCL_R11,
  NAL_UNIT_RESERVED_VCL_N12,
  NAL_UNIT_RESERVED_VCL_R13,
  NAL_UNIT_RESERVED_VCL_N14,
  NAL_UNIT_RESERVED_VCL_R15,

  NAL_UNIT_CODED_SLICE_BLA_W_LP,    // 16
  NAL_UNIT_CODED_SLICE_BLA_W_RADL,  // 17
  NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
  NAL_UNIT_CODED_SLICE_IDR_W_RADL,  // 19
  NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
  NAL_UNIT_CODED_SLICE_CRA,       // 21
  NAL_UNIT_RESERVED_IRAP_VCL22,
  NAL_UNIT_RESERVED_IRAP_VCL23,

  NAL_UNIT_RESERVED_VCL24,
  NAL_UNIT_RESERVED_VCL25,
  NAL_UNIT_RESERVED_VCL26,
  NAL_UNIT_RESERVED_VCL27,
  NAL_UNIT_RESERVED_VCL28,
  NAL_UNIT_RESERVED_VCL29,
  NAL_UNIT_RESERVED_VCL30,
  NAL_UNIT_RESERVED_VCL31,

  NAL_UNIT_VPS,                   // 32
  NAL_UNIT_SPS,                   // 33
  NAL_UNIT_PPS,                   // 34
  NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
  NAL_UNIT_EOS,                   // 36
  NAL_UNIT_EOB,                   // 37
  NAL_UNIT_FILLER_DATA,           // 38
  NAL_UNIT_PREFIX_SEI,              // 39
  NAL_UNIT_SUFFIX_SEI,              // 40
  NAL_UNIT_RESERVED_NVCL41,
  NAL_UNIT_RESERVED_NVCL42,
  NAL_UNIT_RESERVED_NVCL43,
  NAL_UNIT_RESERVED_NVCL44,
  NAL_UNIT_RESERVED_NVCL45,
  NAL_UNIT_RESERVED_NVCL46,
  NAL_UNIT_RESERVED_NVCL47,
  NAL_UNIT_UNSPECIFIED_48,
  NAL_UNIT_UNSPECIFIED_49,
  NAL_UNIT_UNSPECIFIED_50,
  NAL_UNIT_UNSPECIFIED_51,
  NAL_UNIT_UNSPECIFIED_52,
  NAL_UNIT_UNSPECIFIED_53,
  NAL_UNIT_UNSPECIFIED_54,
  NAL_UNIT_UNSPECIFIED_55,
  NAL_UNIT_UNSPECIFIED_56,
  NAL_UNIT_UNSPECIFIED_57,
  NAL_UNIT_UNSPECIFIED_58,
  NAL_UNIT_UNSPECIFIED_59,
  NAL_UNIT_UNSPECIFIED_60,
  NAL_UNIT_UNSPECIFIED_61,
  NAL_UNIT_UNSPECIFIED_62,
  NAL_UNIT_UNSPECIFIED_63,
  NAL_UNIT_INVALID,
};

typedef struct NalUnitHeader_s
{
  int nalUnitType;
  int nuhLayerId;
  int nuhTemporalIdPlus1;
} NalUnitHeader;


int findStartCodePrefix(FILE *inFile, int *numStartCodeZeros)
{
  int numZeros = 0;
  int currByte;
  
  while (1)
  {
    currByte = fgetc(inFile);
    if (currByte == EOF)
    {
      return 0;
    }

    if (currByte == 0x01 && numZeros > 1)
    {
      *numStartCodeZeros = numZeros;
      return 1;
    }
    else if (currByte == 0)
    {
      numZeros++;
    }
    else
    {
      numZeros = 0;
    }
  }
}

int parseNalUnitHeader(FILE *inFile, NalUnitHeader *nalu)
{
  int byte0, byte1;

  byte0 = fgetc(inFile);
  byte1 = fgetc(inFile);

  if (byte0 == EOF || byte1 == EOF)
  {
    return 0;
  }

  nalu->nalUnitType = (byte0 >> 1) & 0x3f;
  nalu->nuhLayerId  = (((byte0 << 8) | byte1) >> 3) & 0x3f;
  nalu->nuhTemporalIdPlus1 = byte1 & 0x07;

  return 1;
}

void writeStartCodePrefixAndNUH(FILE *outFile, int numStartCodeZeros, NalUnitHeader *nalu)
{
  int byte0, byte1;
  int i;

  /* Start code prefix */
  if (numStartCodeZeros > 3)
  {
    numStartCodeZeros = 3;
  }
  for (i = 0; i < numStartCodeZeros; i++)
  {
    fputc(0, outFile);
  }
  fputc(0x01, outFile);

  /* NAL unit header */
  byte0 = ((nalu->nalUnitType << 6) | nalu->nuhLayerId) >> 5;
  byte1 = ((nalu->nuhLayerId << 3) | nalu->nuhTemporalIdPlus1) & 0xff;
  fputc(byte0, outFile);
  fputc(byte1, outFile);
}

int main(int argc, char **argv)
{
  FILE *inFile;
  FILE *outFile;
  int assignedBaseLayerId;
  NalUnitHeader nalu;
  int numStartCodeZeros;
  int isSpsPpsEosEob;

  if (argc < 3)
  {
    printf("Usage: Splicer <infile> <outfile> <assigned base layer ID>\n", argv[0]);
  }

  inFile = fopen(argv[1], "rb");
  if (inFile == NULL)
  {
    fprintf(stderr, "Cannot open input file %s\n", argv[1]);
    exit(1);
  }

  outFile = fopen(argv[2], "wb");
  if (outFile == NULL)
  {
    fprintf(stderr, "Cannot open output file %s\n", argv[2]);
    exit(1);
  }

  assignedBaseLayerId = atoi(argv[3]);

  while (1)
  {
    if (!findStartCodePrefix(inFile, &numStartCodeZeros))
    {
      break;
    }
    if (!parseNalUnitHeader(inFile, &nalu))
    {
      break;
    }

    printf("NAL unit type: %i,  NUH layer ID: %i,  NUH Temporal ID: %i\n", nalu.nalUnitType, nalu.nuhLayerId, nalu.nuhTemporalIdPlus1 - 1);

    isSpsPpsEosEob = (nalu.nalUnitType == NAL_UNIT_SPS || nalu.nalUnitType == NAL_UNIT_PPS || nalu.nalUnitType == NAL_UNIT_EOS || nalu.nalUnitType == NAL_UNIT_EOB);

    if ((isSpsPpsEosEob && nalu.nuhLayerId == 0) || (!isSpsPpsEosEob && nalu.nuhLayerId == assignedBaseLayerId))
    {
      /* Write current NAL unit to output bitstream */

      long naluBytesStartPos;
      long numNaluBytes;
      long i;

      nalu.nuhLayerId = 0;
      writeStartCodePrefixAndNUH(outFile, numStartCodeZeros, &nalu);

      naluBytesStartPos = ftell(inFile);
      /* Find beginning of the next NAL unit to calculate length of the current unit */
      if (findStartCodePrefix(inFile, &numStartCodeZeros))
      {
        numNaluBytes = ftell(inFile) - naluBytesStartPos - numStartCodeZeros - 1;
      }
      else
      {
        numNaluBytes = ftell(inFile) - naluBytesStartPos;
      }
      fseek(inFile, naluBytesStartPos, SEEK_SET);

      for (i = 0; i < numNaluBytes; i++)
      {
        fputc(fgetc(inFile), outFile);
      }
    }
  }

  fclose(inFile);
  fclose(outFile);


  return 0;
}