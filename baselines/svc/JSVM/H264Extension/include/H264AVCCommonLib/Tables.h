
#if !defined(AFX_TABLES_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_)
#define AFX_TABLES_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

H264AVCCOMMONLIB_API extern const UChar g_aucInvFrameScan     [16];
H264AVCCOMMONLIB_API extern const UChar g_aucFrameScan        [16];
H264AVCCOMMONLIB_API extern const UChar g_aucIndexChromaDCScan[4];
H264AVCCOMMONLIB_API extern const UChar g_aucLumaFrameDCScan  [16];
H264AVCCOMMONLIB_API extern const UChar g_aucFieldScan        [16];
H264AVCCOMMONLIB_API extern const UChar g_aucLumaFieldDCScan  [16];
H264AVCCOMMONLIB_API extern const Int   g_aaiQuantCoef     [6][16];
H264AVCCOMMONLIB_API extern const Int   g_aaiDequantCoef   [6][16];
H264AVCCOMMONLIB_API extern const UChar g_aucChromaScale      [52];


H264AVCCOMMONLIB_API extern const UChar g_aucInvFrameScan64   [64];
H264AVCCOMMONLIB_API extern const UChar g_aucFrameScan64      [64];
H264AVCCOMMONLIB_API extern const UChar g_aucFieldScan64      [64];
H264AVCCOMMONLIB_API extern const Int   g_aaiDequantCoef64 [6][64];
H264AVCCOMMONLIB_API extern const Int   g_aaiQuantCoef64   [6][64];

H264AVCCOMMONLIB_API extern const UChar g_aucScalingMatrixDefault4x4Intra[16];
H264AVCCOMMONLIB_API extern const UChar g_aucScalingMatrixDefault4x4Inter[16];
H264AVCCOMMONLIB_API extern const UChar g_aucScalingMatrixDefault8x8Intra[64];
H264AVCCOMMONLIB_API extern const UChar g_aucScalingMatrixDefault8x8Inter[64];


H264AVC_NAMESPACE_END


#endif //AFX_TABLES_H__C2625057_4318_4267_8A7A_8185BB75AA7F__INCLUDED_

