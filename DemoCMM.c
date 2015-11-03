/*
	File:		DemoCMM.c

	Contains:	Demo CMM Component for ColorSync 2.x
				
				This is a very simple CMM - so simple in fact that the only thing
				it uses from a profile is its colorspace.
				
				If the first (source) profile is RGB and the last (dest) profile
				is CMYK, then this CMM uses the simple "one minus" formula
				to convert from from RGB to CMYK.
				
				If the first (source) profile is CMYK and the last (dest) profile
				is RGB, then this CMM uses the inverse of the "one minus" formula
				to convert from from CMYK to RGB.

				If the first and the last profile are RGB or if the first and the 
				last profile are CMYK, then this CMM leaves the colors unchanged.
				
	Version:	ColorSync 2 or later

	Copyright:	2002 by Apple Computer, Inc., all rights reserved.
*/


#ifndef FLAT_INCLUDES
#if defined(__MRC__) || defined(__MWERKS__) || (defined(__SC__))
#define FLAT_INCLUDES	1
#else
#define FLAT_INCLUDES	0
#endif
#endif // FLAT_INCLUDES


#if FLAT_INCLUDES
#include <ConditionalMacros.h>
#include <MacTypes.h>
#include <Errors.h>
#include <Endian.h>
#include <MacMemory.h>
#include <Gestalt.h>
#include <CMApplication.h>
#include <CMMComponent.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#endif


#include <math.h>


#define CMM_ENTRY		pascal

// Component version
#define 	kCMCodeVersion		1
#define		kCMMVersion			((CMMInterfaceVersion << 16) | kCMCodeVersion)

typedef void (*MatchOneProc) (UInt16* chan);


// Component storage
typedef struct
{
	OSType				srcSpace;
	OSType				srcClass;
	OSType				dstSpace;
	OSType				dstClass;
	MatchOneProc		proc;
} CMMStorageRec, *CMMStoragePtr, **CMMStorageHdl;


// Match stuff
typedef struct
{
	CMMStorageHdl		storage;
	
	UInt32				height;
	UInt32				width;
	
	OSType				srcSpace;
	UInt8*				srcBuf[4];
	UInt32				srcChanBits;
	UInt32				srcRowBytes;
	UInt32				srcColBytes;
	Boolean				srcSwap;
	
	OSType				dstSpace;
	UInt8*				dstBuf[4];
	UInt32				dstChanBits;
	UInt32				dstRowBytes;
	UInt32				dstColBytes;
	Boolean				dstSwap;
	
} CMMMatchRec, *CMMMatchPtr, **CMMMatchHdl;





// function prototypes
static CMError DoNCMMInit			(CMMStorageHdl storage, CMProfileRef srcProfile, CMProfileRef dstProfile);
static CMError DoCMMConcatInit		(CMMStorageHdl storage, CMConcatProfileSet* profileSet);
static CMError DoNCMMConcatInit		(CMMStorageHdl storage, NCMConcatProfileSet* profileSet, CMConcatCallBackUPP proc, void* refCon);
static CMError DoCMMMatchColors		(CMMStorageHdl storage, CMColor *colorBuf, UInt32 count);
static CMError DoCMMCheckColors		(CMMStorageHdl storage, CMColor *colorBuf, UInt32 count, UInt32 *gamutResult);
static CMError DoCMMMatchBitmap		(CMMStorageHdl storage, const CMBitmap * srcMap, CMBitmapCallBackUPP progressProc, void* refCon, CMBitmap* dstMap);
static CMError DoCMMCheckBitmap		(CMMStorageHdl storage, const CMBitmap * srcMap, CMBitmapCallBackUPP progressProc, void* refCon, CMBitmap* chkMap);
static CMError CheckStorage			(CMMStorageHdl storage);
static void    MatchAll				(CMMMatchPtr pMatchInfo);
static void MatchOne_RGB_CMYK	(UInt16* chan);
static void MatchOne_CMYK_RGB	(UInt16* chan);
static void MatchOne_RGB_XYZ	(UInt16* chan);
static void MatchOne_XYZ_RGB	(UInt16* chan);
static void MatchOne_RGB_LAB	(UInt16* chan);
static void MatchOne_LAB_RGB	(UInt16* chan);
static void MatchOne_XYZ_LAB	(UInt16* chan);
static void MatchOne_LAB_XYZ	(UInt16* chan);
static void MatchOne_XYZ_Gray	(UInt16* chan);
static void MatchOne_Gray_XYZ	(UInt16* chan);
static void MatchOne_CMYK_LAB	(UInt16* chan);
static void MatchOne_LAB_CMYK	(UInt16* chan);
static void MatchOne_CMYK_XYZ	(UInt16* chan);
static void MatchOne_XYZ_CMYK	(UInt16* chan);
static void MatchOne_RGB_Gray	(UInt16* chan);
static void MatchOne_Gray_RGB	(UInt16* chan);
static void MatchOne_LAB_Gray	(UInt16* chan);
static void MatchOne_Gray_LAB	(UInt16* chan);
static void MatchOne_CMYK_Gray	(UInt16* chan);
static void MatchOne_Gray_CMYK	(UInt16* chan);





#if TARGET_API_MAC_OSX
#pragma mark -
#pragma mark ----- X entry points -----


//--------------------------------------------------------------------- CMMOpen

CMM_ENTRY CMError
CMMOpen ( UInt32 *cmmRefcon,
		  void* hInstance) 
{ 
#pragma unused (hInstance)
	*cmmRefcon = (UInt32)calloc(1,sizeof(CMMStorageRec));
	//{
	//	CFBundleRef ref = nil;
	//	ref = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ColorSync.DemoCMM"));
	//}
	if (*cmmRefcon)
		return noErr;
	else
		return memFullErr;
}


//--------------------------------------------------------------------- CMMClose

CMM_ENTRY CMError
CMMClose ( UInt32 *cmmRefcon ) 
{
	if (*cmmRefcon)
	free((void*)*cmmRefcon);
	return noErr;
}


CMM_ENTRY CMError  
NCMMInit (UInt32 *cmmRefcon, CMProfileRef srcProfile, CMProfileRef dstProfile)
{
	return DoNCMMInit ((CMMStorageHdl)cmmRefcon, srcProfile, dstProfile);
}

CMM_ENTRY CMError  
CMMConcatInit (UInt32 *cmmRefcon, CMConcatProfileSet* profileSet)
{
	return DoCMMConcatInit ((CMMStorageHdl)cmmRefcon, profileSet);
}

CMM_ENTRY CMError  
NCMMConcatInit (UInt32 *cmmRefcon, NCMConcatProfileSet* profileSet, CMConcatCallBackUPP proc, void* refCon)
{
	return DoNCMMConcatInit ((CMMStorageHdl)cmmRefcon, profileSet, proc, refCon);
}

CMM_ENTRY CMError  
CMMMatchColors (UInt32 *cmmRefcon, CMColor *colorBuf, UInt32 count)
{
	return DoCMMMatchColors ((CMMStorageHdl)cmmRefcon, colorBuf, count);
}
							
CMM_ENTRY CMError  
CMMCheckColors (UInt32 *cmmRefcon, CMColor *colorBuf, UInt32 count, UInt32* gamutResult)
{
	return DoCMMCheckColors ((CMMStorageHdl)cmmRefcon, colorBuf, count, gamutResult);
}

CMM_ENTRY CMError  
CMMMatchBitmap (UInt32 *cmmRefcon, CMBitmap * srcMap,
				 CMBitmapCallBackUPP progressProc, void * refCon,
				 CMBitmap* dstMap)
{
	return DoCMMMatchBitmap ((CMMStorageHdl)cmmRefcon, srcMap, progressProc, refCon, dstMap);
}

CMM_ENTRY CMError  
CMMCheckBitmap (UInt32 *cmmRefcon,  const CMBitmap * srcMap,
				 CMBitmapCallBackUPP progressProc, void * refCon,
				 CMBitmap* chkMap)
{
	return DoCMMCheckBitmap ((CMMStorageHdl)cmmRefcon,  srcMap, progressProc, refCon, chkMap);
}

#endif // TARGET_API_MAC_OSX


#pragma mark -
#pragma mark ----- implimentations -----


//--------------------------------------------------------------------- DoNCMMInit

typedef struct CMConcatProfileSet2 {
	unsigned short 		keyIndex;		/* Zero-based */
	unsigned short 		count;			/* Min 1, Max 2 */
	CMProfileRef 		profileSet[2];	/* Fixed. Source and Dest */
} CMConcatProfileSet2;

static CMError
DoNCMMInit (CMMStorageHdl storage, CMProfileRef srcProfile, CMProfileRef dstProfile)
{
	CMConcatProfileSet2		set;
	
	set.keyIndex = 0;
	set.count = 2;
	set.profileSet[0] = srcProfile;
	set.profileSet[1] = dstProfile;
		
	return DoCMMConcatInit(storage, (CMConcatProfileSet*)&set);
}


//--------------------------------------------------------------------- DoCMMConcatInit

static CMError
DoCMMConcatInit (CMMStorageHdl storage, CMConcatProfileSet* profileSet)
{
	ComponentResult			result = noErr;
	CMAppleProfileHeader	srcHdr;
	CMAppleProfileHeader	dstHdr;
	CMProfileRef			srcProfile;
	CMProfileRef			dstProfile;
	
	// Check params
	if (profileSet==nil)
		return paramErr;
	
	srcProfile = profileSet->profileSet[0];
	dstProfile = profileSet->profileSet[profileSet->count-1];
	if ((srcProfile == nil) || (dstProfile == nil))
		return paramErr;
	
	if (result == noErr)
		result = CMGetProfileHeader(srcProfile, &srcHdr);
	
	if (result == noErr)
		result = CMGetProfileHeader(dstProfile, &dstHdr);
	
	if (result == noErr)
	{
		(**storage).srcSpace = srcHdr.cm2.dataColorSpace;
		(**storage).srcClass = srcHdr.cm2.profileClass;
		
		(**storage).dstSpace = dstHdr.cm2.dataColorSpace;
		(**storage).dstClass = dstHdr.cm2.profileClass;
		
		result = CheckStorage(storage);
	}
		
	return result ;
}


//--------------------------------------------------------------------- DoNCMMConcatInit

static CMError
DoNCMMConcatInit (CMMStorageHdl storage, NCMConcatProfileSet* profileSet, CMConcatCallBackUPP proc, void* refcon)
{
#pragma unused (proc,refcon)
	ComponentResult			result = noErr;
	CMAppleProfileHeader	srcHdr;
	CMAppleProfileHeader	dstHdr;
	CMProfileRef			srcProfile;
	CMProfileRef			dstProfile;
	UInt32 					srcTransform;
	UInt32 					dstTransform;
	
	// Check params
	if (profileSet==nil)
		return paramErr;
	
	srcProfile = profileSet->profileSpecs[0].profile;
	dstProfile = profileSet->profileSpecs[profileSet->profileCount-1].profile;
	if ((srcProfile == nil) || (dstProfile == nil))
		return paramErr;
	
	srcTransform = profileSet->profileSpecs[0].transformTag;
	dstTransform = profileSet->profileSpecs[profileSet->profileCount-1].transformTag;
	
	if (result == noErr)
		result = CMGetProfileHeader(srcProfile, &srcHdr);
	
	if (result == noErr)
		result = CMGetProfileHeader(dstProfile, &dstHdr);
	
	if (result == noErr)
	{
		(**storage).srcSpace = (kDeviceToPCS) ? srcHdr.cm2.dataColorSpace : srcHdr.cm2.profileConnectionSpace;
		(**storage).srcClass = srcHdr.cm2.profileClass;
		
		(**storage).dstSpace = (kPCSToDevice) ? dstHdr.cm2.dataColorSpace : dstHdr.cm2.profileConnectionSpace;
		(**storage).dstClass = dstHdr.cm2.profileClass;
		
		result = CheckStorage(storage);
	}
		
	return result ;
}


//--------------------------------------------------------------------- DoCMMMatchColors

static CMError
DoCMMMatchColors (CMMStorageHdl storage, CMColor *colorBuf, UInt32 count)
{
	CMMMatchRec			matchInfo;
	
	matchInfo.storage		= storage;
	matchInfo.height		= count;
	matchInfo.width			= 1;
	matchInfo.srcSpace		= (**storage).srcSpace;
	matchInfo.srcBuf[0]		= ((UInt8*)colorBuf) + 0;
	matchInfo.srcBuf[1]		= ((UInt8*)colorBuf) + 2;
	matchInfo.srcBuf[2]		= ((UInt8*)colorBuf) + 4;
	matchInfo.srcBuf[3]		= ((UInt8*)colorBuf) + 6;
	matchInfo.srcChanBits	= 16;
	matchInfo.srcRowBytes	= sizeof(CMColor);
	matchInfo.srcColBytes	= sizeof(CMColor);
	matchInfo.srcSwap		= false;
	
	matchInfo.dstSpace		= (**storage).dstSpace;
	matchInfo.dstBuf[0]		= ((UInt8*)colorBuf) + 0;
	matchInfo.dstBuf[1]		= ((UInt8*)colorBuf) + 2;
	matchInfo.dstBuf[2]		= ((UInt8*)colorBuf) + 4;
	matchInfo.dstBuf[3]		= ((UInt8*)colorBuf) + 6;
	matchInfo.dstChanBits	= 16;
	matchInfo.dstRowBytes	= sizeof(CMColor);
	matchInfo.dstColBytes	= sizeof(CMColor);
	matchInfo.dstSwap		= false;
	
	MatchAll(&matchInfo);
	
	return noErr;
}
								

//--------------------------------------------------------------------- DoCMMCheckColors

static CMError
DoCMMCheckColors (CMMStorageHdl storage,  CMColor *colorBuf, UInt32 count, UInt32* gamutResult)
{
#pragma unused (storage, colorBuf)
	
	UInt32		longCount;
	
	/* Everything is in gamut. This is just sample code. */
	
	longCount = (count + 31) / 32;
	
	while (longCount--)
		*gamutResult++ = 0xFFFFFFFF;
	
	return noErr;
}


//--------------------------------------------------------------------- DoCMMMatchBitmap

static CMError
DoCMMMatchBitmap (CMMStorageHdl storage,  const CMBitmap * srcMap,
				 CMBitmapCallBackUPP progressProc, void * refCon,
				 CMBitmap* dstMap)
{
#pragma unused (progressProc, refCon)
	
	CMMMatchRec			matchInfo;
	
	// Check params
	if (srcMap==nil || dstMap==nil)
		return paramErr;
	
	matchInfo.storage		= storage;
	matchInfo.height		= srcMap->height;
	matchInfo.width			= srcMap->width;
	matchInfo.srcRowBytes	= srcMap->rowBytes;
	matchInfo.dstRowBytes	= dstMap->rowBytes;
	
	switch (srcMap->space)
	{
		case cmGray8Space:
		matchInfo.srcSpace		= cmGrayData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= nil;
		matchInfo.srcBuf[2]		= nil;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 1;
		break;
		
		case cmGray16Space:
		case cmGray16LSpace:
		matchInfo.srcSpace		= cmGrayData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= nil;
		matchInfo.srcBuf[2]		= nil;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 16;
		matchInfo.srcColBytes	= 2;
		break;
	
		case cmRGB24Space:
		matchInfo.srcSpace		= cmRGBData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 1;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 3;
		break;
		
		case cmRGB32Space:
		matchInfo.srcSpace		= cmRGBData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 1;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 3;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 4;
		break;
		
		case cmRGB48Space:
		case cmRGB48LSpace:
		matchInfo.srcSpace		= cmRGBData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 4;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 16;
		matchInfo.srcColBytes	= 6;
		break;
		
		case cmCMYK32Space:
		matchInfo.srcSpace		= cmCMYKData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 1;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[3]		= (UInt8*)srcMap->image + 3;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 4;
		break;
		
		case cmCMYK64Space:
		case cmCMYK64LSpace:
		matchInfo.srcSpace		= cmCMYKData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 4;
		matchInfo.srcBuf[3]		= (UInt8*)srcMap->image + 6;
		matchInfo.srcChanBits	= 16;
		matchInfo.srcColBytes	= 8;
		break;
		
		case cmLAB24Space:
		matchInfo.srcSpace		= cmLabData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 1;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 3;
		break;
		
		case cmLAB48Space:
		case cmLAB48LSpace:
		matchInfo.srcSpace		= cmLabData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 4;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 16;
		matchInfo.srcColBytes	= 6;
		break;
		
		case cmXYZ24Space:
		matchInfo.srcSpace		= cmXYZData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 1;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 8;
		matchInfo.srcColBytes	= 3;
		break;
		
		case cmXYZ48Space:
		case cmXYZ48LSpace:
		matchInfo.srcSpace		= cmXYZData;
		matchInfo.srcBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.srcBuf[1]		= (UInt8*)srcMap->image + 2;
		matchInfo.srcBuf[2]		= (UInt8*)srcMap->image + 4;
		matchInfo.srcBuf[3]		= nil;
		matchInfo.srcChanBits	= 16;
		matchInfo.srcColBytes	= 6;
		break;
		
		default:
		return cmInvalidSrcMap;
		break;
	}
	
	#if TARGET_RT_LITTLE_ENDIAN
		matchInfo.srcSwap = ((srcMap->space & cmLittleEndianPacking) == 0);
	#else
		matchInfo.srcSwap = ((srcMap->space & cmLittleEndianPacking) == cmLittleEndianPacking);
	#endif
	
	
	switch (dstMap->space)
	{
		case cmGray8Space:
		matchInfo.dstSpace		= cmGrayData;
		matchInfo.dstBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.dstBuf[1]		= nil;
		matchInfo.dstBuf[2]		= nil;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 1;
		break;
		
		case cmGray16Space:
		case cmGray16LSpace:
		matchInfo.dstSpace		= cmGrayData;
		matchInfo.dstBuf[0]		= (UInt8*)srcMap->image + 0;
		matchInfo.dstBuf[1]		= nil;
		matchInfo.dstBuf[2]		= nil;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 16;
		matchInfo.dstColBytes	= 2;
		break;
	
		case cmRGB24Space:
		matchInfo.dstSpace		= cmRGBData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 1;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 3;
		break;
		
		case cmRGB32Space:
		matchInfo.dstSpace		= cmRGBData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 1;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 3;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 4;
		break;
		
		case cmRGB48Space:
		case cmRGB48LSpace:
		matchInfo.dstSpace		= cmRGBData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 4;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 16;
		matchInfo.dstColBytes	= 6;
		break;
		
		case cmCMYK32Space:
		matchInfo.dstSpace		= cmCMYKData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 1;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[3]		= (UInt8*)dstMap->image + 3;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 4;
		break;
		
		case cmCMYK64Space:
		case cmCMYK64LSpace:
		matchInfo.dstSpace		= cmCMYKData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 4;
		matchInfo.dstBuf[3]		= (UInt8*)dstMap->image + 6;
		matchInfo.dstChanBits	= 16;
		matchInfo.dstColBytes	= 8;
		break;
		
		case cmLAB24Space:
		matchInfo.dstSpace		= cmLabData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 1;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 3;
		break;
		
		case cmLAB48Space:
		case cmLAB48LSpace:
		matchInfo.dstSpace		= cmLabData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 4;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 16;
		matchInfo.dstColBytes	= 6;
		break;
		
		case cmXYZ24Space:
		matchInfo.dstSpace		= cmXYZData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 1;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 8;
		matchInfo.dstColBytes	= 3;
		break;
		
		case cmXYZ48Space:
		case cmXYZ48LSpace:
		matchInfo.dstSpace		= cmXYZData;
		matchInfo.dstBuf[0]		= (UInt8*)dstMap->image + 0;
		matchInfo.dstBuf[1]		= (UInt8*)dstMap->image + 2;
		matchInfo.dstBuf[2]		= (UInt8*)dstMap->image + 4;
		matchInfo.dstBuf[3]		= nil;
		matchInfo.dstChanBits	= 16;
		matchInfo.dstColBytes	= 6;
		break;
		
		default:
		return cmInvalidSrcMap;
		break;
	}
	
	#if TARGET_RT_LITTLE_ENDIAN
		matchInfo.dstSwap = ((dstMap->space & cmLittleEndianPacking) == 0);
	#else
		matchInfo.dstSwap = ((dstMap->space & cmLittleEndianPacking) == cmLittleEndianPacking);
	#endif
	
	
	if ((**storage).srcSpace != matchInfo.srcSpace)
		return cmInvalidSrcMap;
	
	
	if ((**storage).dstSpace != matchInfo.dstSpace)
		return cmInvalidDstMap;
	
	MatchAll(&matchInfo);
	
	return noErr;
}


//--------------------------------------------------------------------- DoCMMCheckBitmap

static CMError
DoCMMCheckBitmap (CMMStorageHdl storage,  const CMBitmap * srcMap,
				 CMBitmapCallBackUPP progressProc, void * refCon,
				 CMBitmap* chkMap)
{
#pragma unused (storage, srcMap, progressProc, refCon, chkMap)
	return paramErr;
}


#pragma mark -
#pragma mark ----- utilities -----


//--------------------------------------------------------------------- CheckStorage

static CMError
CheckStorage (CMMStorageHdl storage)
{
	OSType			srcSpace = (**storage).srcSpace;
	OSType			dstSpace = (**storage).dstSpace;
	
	(**storage).proc = nil;
	
	if (srcSpace == dstSpace)
		return noErr;
	
	if      (srcSpace==cmRGBData  && dstSpace==cmCMYKData)		(**storage).proc = &MatchOne_RGB_CMYK;
	else if (srcSpace==cmRGBData  && dstSpace==cmXYZData)		(**storage).proc = &MatchOne_RGB_XYZ;
	else if (srcSpace==cmRGBData  && dstSpace==cmLabData)		(**storage).proc = &MatchOne_RGB_LAB;
	else if (srcSpace==cmRGBData  && dstSpace==cmGrayData)		(**storage).proc = &MatchOne_RGB_Gray;
	else if (srcSpace==cmCMYKData && dstSpace==cmRGBData)		(**storage).proc = &MatchOne_CMYK_RGB;
	else if (srcSpace==cmCMYKData && dstSpace==cmLabData)		(**storage).proc = &MatchOne_CMYK_LAB;
	else if (srcSpace==cmCMYKData && dstSpace==cmXYZData)		(**storage).proc = &MatchOne_CMYK_XYZ;
	else if (srcSpace==cmCMYKData && dstSpace==cmGrayData)		(**storage).proc = &MatchOne_CMYK_Gray;
	else if (srcSpace==cmXYZData  && dstSpace==cmRGBData)		(**storage).proc = &MatchOne_XYZ_RGB;
	else if (srcSpace==cmXYZData  && dstSpace==cmLabData)		(**storage).proc = &MatchOne_XYZ_LAB;
	else if (srcSpace==cmXYZData  && dstSpace==cmCMYKData)		(**storage).proc = &MatchOne_XYZ_CMYK;
	else if (srcSpace==cmXYZData  && dstSpace==cmGrayData)		(**storage).proc = &MatchOne_XYZ_Gray;
	else if (srcSpace==cmLabData  && dstSpace==cmRGBData)		(**storage).proc = &MatchOne_LAB_RGB;
	else if (srcSpace==cmLabData  && dstSpace==cmXYZData)		(**storage).proc = &MatchOne_LAB_XYZ;
	else if (srcSpace==cmLabData  && dstSpace==cmCMYKData)		(**storage).proc = &MatchOne_LAB_CMYK;
	else if (srcSpace==cmLabData  && dstSpace==cmGrayData)		(**storage).proc = &MatchOne_LAB_Gray;
	else if (srcSpace==cmGrayData && dstSpace==cmRGBData)		(**storage).proc = &MatchOne_Gray_RGB;
	else if (srcSpace==cmGrayData && dstSpace==cmCMYKData)		(**storage).proc = &MatchOne_Gray_CMYK;
	else if (srcSpace==cmGrayData && dstSpace==cmXYZData)		(**storage).proc = &MatchOne_Gray_XYZ;
	else if (srcSpace==cmGrayData && dstSpace==cmLabData)		(**storage).proc = &MatchOne_Gray_LAB;
	else
		return cmInvalidProfile;
	
	return noErr;
}


//--------------------------------------------------------------------- DebugColor4

#if DO_DEBUGCOLOR
static void
DebugColor4( UInt16* color)
{
	char			s[] = " 0xXXXX, 0xXXXX, 0xXXXX, 0xXXXX\n";
	char*			c = &(s[3]);
	int				i;
	int				v;
	
	for (i=0; i<4; i++)
	{
		v = (((*color)>>12) & 0x000F);	*c++ = (v<10) ? (v+'0') : (v+'A'-10);
		v = (((*color)>>8) & 0x000F);	*c++ = (v<10) ? (v+'0') : (v+'A'-10);
		v = (((*color)>>4) & 0x000F);	*c++ = (v<10) ? (v+'0') : (v+'A'-10);
		v = (((*color)) & 0x000F);		*c++ = (v<10) ? (v+'0') : (v+'A'-10);
		c += 4;
		color++;
	}
	
	fprintf(stderr, s);
}
#endif


//---------------------------------------------------------------------	MatchAll				
//	Simple conversion of a bunch or colors.
//---------------------------------------------------------------------

static void
MatchAll (CMMMatchPtr pMatchInfo)
{
	UInt32				r,c;
	UInt16				chan[4];
	UInt8**				sBuf;
	UInt8**				dBuf;
	
	sBuf = pMatchInfo->srcBuf;
	dBuf = pMatchInfo->dstBuf;
	
	for (r=0; r < pMatchInfo->height; r++)
	{
		for (c=0; c < pMatchInfo->width; c++)
		{
			// read color in from source buffer
			if (pMatchInfo->srcChanBits==16)
			{
				if (sBuf[0]) chan[0] = *(UInt16*)(sBuf[0] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[1]) chan[1] = *(UInt16*)(sBuf[1] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[2]) chan[2] = *(UInt16*)(sBuf[2] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[3]) chan[3] = *(UInt16*)(sBuf[3] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
			}
			else
			{
				if (sBuf[0]) chan[0] = *(UInt8*)(sBuf[0] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[1]) chan[1] = *(UInt8*)(sBuf[1] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[2]) chan[2] = *(UInt8*)(sBuf[2] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				if (sBuf[3]) chan[3] = *(UInt8*)(sBuf[3] + (r * pMatchInfo->srcRowBytes) + (c * pMatchInfo->srcColBytes));
				chan[0] = (chan[0] << 8) | chan[0];
				chan[1] = (chan[1] << 8) | chan[1];
				chan[2] = (chan[2] << 8) | chan[2];
				chan[3] = (chan[3] << 8) | chan[3];
			}
			
			if (pMatchInfo->srcSwap)
			{
				chan[0] = Endian16_Swap(chan[0]);
				chan[1] = Endian16_Swap(chan[1]);
				chan[2] = Endian16_Swap(chan[2]);
				chan[3] = Endian16_Swap(chan[3]);
			}

#if DO_DEBUGCOLOR
			DebugColor4(chan);
#endif			
			// Match the color
			if ((**(pMatchInfo->storage)).proc)
				((**(pMatchInfo->storage)).proc)(chan);

#if DO_DEBUGCOLOR
			DebugColor4(chan);
#endif
			
			if (pMatchInfo->dstSwap)
			{
				chan[0] = Endian16_Swap(chan[0]);
				chan[1] = Endian16_Swap(chan[1]);
				chan[2] = Endian16_Swap(chan[2]);
				chan[3] = Endian16_Swap(chan[3]);
			}
			
			// Write color to destination buffer
			if (pMatchInfo->dstChanBits==16)
			{
				if (dBuf[0]) *(UInt16*)(dBuf[0] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[0];
				if (dBuf[1]) *(UInt16*)(dBuf[1] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[1];
				if (dBuf[2]) *(UInt16*)(dBuf[2] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[2];
				if (dBuf[3]) *(UInt16*)(dBuf[3] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[3];
			}
			else
			{
				if (dBuf[0]) *(UInt8*)(dBuf[0] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[0] >> 8;
				if (dBuf[1]) *(UInt8*)(dBuf[1] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[1] >> 8;
				if (dBuf[2]) *(UInt8*)(dBuf[2] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[2] >> 8;
				if (dBuf[3]) *(UInt8*)(dBuf[3] + (r * pMatchInfo->dstRowBytes) + (c * pMatchInfo->dstColBytes)) = chan[3] >> 8;
			}
		}
	}
}


//---------------------------------------------------------------------					
//	Simple conversions of one color with 16 bits-per-channel.
//---------------------------------------------------------------------


static void
MatchOne_RGB_CMYK (UInt16* chan)
{
	chan[0] = 0xFFFF - chan[0];
	chan[1] = 0xFFFF - chan[1];
	chan[2] = 0xFFFF - chan[2];
	chan[3] = (chan[0] < chan[1]) ?
				( (chan[0] < chan[2]) ? (chan[0]) : (chan[2]) ) :
				( (chan[1] < chan[2]) ? (chan[1]) : (chan[2]) );
	chan[0] -= chan[3];
	chan[1] -= chan[3];
	chan[2] -= chan[3];
}

static void
MatchOne_CMYK_RGB (UInt16* chan)
{
	chan[0] = 0xFFFF - chan[0];
	chan[1] = 0xFFFF - chan[1];
	chan[2] = 0xFFFF - chan[2];
	chan[0] = (chan[0] > chan[3]) ? (chan[0] - chan[3]) : 0;
	chan[1] = (chan[1] > chan[3]) ? (chan[1] - chan[3]) : 0;
	chan[2] = (chan[2] > chan[3]) ? (chan[2] - chan[3]) : 0;
	chan[3] = 0;
}

#define DoubToUInt16(x)		(((x)<=0.0)?(0):(((x)>=1.0)?(65535):((x)*65535.0 + 0.5)))
#define UInt16ToDoub(x)		((double)(x)/65535.0)
#define DoubToFract(x)		(((x)<=0.0)?(0):(((x)>=2.0)?(65535):((x)*32768.0 + 0.5)))
#define FractToDoub(x)		((double)(x)/32768.0)
#define UInt16ToFract(x)	((x)>>1)
#define FractToUInt16(x)	((x)<<1)

static void
MatchOne_RGB_XYZ (UInt16* chan)
{
	double r,g,b;
	double X,Y,Z;
	
	r = UInt16ToDoub(chan[0]);
	g = UInt16ToDoub(chan[1]);
	b = UInt16ToDoub(chan[2]);
	
	// map through 2.2 gamma
	// r = pow( r, 2.2)
	// g = pow( g, 2.2)
	// b = pow( b, 2.2)
	
	// sRGB phosphors matrix
	X = (0.418 * r) + (0.363 * g) + (0.183 * b);
	Y = (0.213 * r) + (0.715 * g) + (0.072 * b);
	Z = (0.015 * r) + (0.090 * g) + (0.720 * b);
	
	chan[0] = DoubToFract(X);
	chan[1] = DoubToFract(Y);
	chan[2] = DoubToFract(Z);
}

static void
MatchOne_XYZ_RGB (UInt16* chan)
{
	double r,g,b;
	double X,Y,Z;
	
	X = FractToDoub(chan[0]);
	Y = FractToDoub(chan[1]);
	Z = FractToDoub(chan[2]);
	
	// sRGB phosphors inverse matrix
	r = ( 3.202 * X) + (-1.543 * Y) + (-0.660 * Z);
	g = (-0.959 * X) + ( 1.879 * Y) + ( 0.056 * Z);
	b = ( 0.053 * X) + (-0.203 * Y) + ( 1.396 * Z);
	
	// map through inverse of 2.2 gamma
	// r = pow( r, 1.0 / 2.2)
	// g = pow( g, 1.0 / 2.2)
	// b = pow( b, 1.0 / 2.2)
	
	chan[0] = DoubToUInt16(r);
	chan[1] = DoubToUInt16(g);
	chan[2] = DoubToUInt16(b);
}

static void
MatchOne_RGB_LAB (UInt16* chan)
{
	MatchOne_RGB_XYZ(chan);
	MatchOne_XYZ_LAB(chan);
}

static void
MatchOne_LAB_RGB (UInt16* chan)
{
	MatchOne_LAB_XYZ(chan);
	MatchOne_XYZ_RGB(chan);
}

static void
MatchOne_XYZ_LAB (UInt16* chan)
{
#if 0
	CMXYZColor white;
	white.X = 31594;
	white.Y = 32768;
	white.Z = 27030;
	CMConvertXYZToLab( (CMColor*)chan, &white, (CMColor*)chan,1);
#else
	double				X, Y, Z;
	double				L, a, b;
	double				fx, fy, fz;

	X = FractToDoub(chan[0]);
	Y = FractToDoub(chan[1]);
	Z = FractToDoub(chan[2]);
	
	// Assume XYZ white is D50
	X /= 0.9642;
	Z /= 0.8249;

	if (X > 0.008856)
		fx = pow(X, 0.3333);
	else
		fx = 7.787 * X + 16.0 / 116.0;
	
	if (Y > 0.008856)
		fy = pow(Y, 0.3333);
	else
		fy = 7.787 * Y + 16.0 / 116.0;
	
	if (Z > 0.008856)
		fz = pow(Z, 0.3333);
	else
		fz = 7.787 * Z + 16.0 / 116.0;

	L = 116.0 * fy - 16;
	a = 500.0 * (fx - fy);
	b = 200.0 * (fy - fz);
	
	L = L / 100.0;
	a = (a + 128.0) / 256.0;
	b = (b + 128.0) / 256.0;
	
	chan[0] = DoubToUInt16(L);
	chan[1] = DoubToUInt16(a);
	chan[2] = DoubToUInt16(b);
#endif
}

static void
MatchOne_LAB_XYZ (UInt16* chan)
{
#if 0
	CMXYZColor white;
	white.X = 31594;
	white.Y = 32768;
	white.Z = 27030;
	CMConvertLabToXYZ( (CMColor*)chan, &white, (CMColor*)chan,1);
#else
	double				X, Y, Z;
	double				L, a, b;
	double				fx, fy, fz;

	L = UInt16ToDoub(chan[0]) * 100.0;
	a = UInt16ToDoub(chan[1]) * 256.0 - 128.0;
	b = UInt16ToDoub(chan[2]) * 256.0 - 128.0;
	
	fy = (L + 16.0) / 116.0;
	fx = a / 500.0 + fy;
	fz = fy - b / 200.0;
	
	if (fx > 0.20696) 
		X = pow(fx, 3);
	else
		X = (fx - 16.0 / 116.0) / 7.787;
	
	if (fy > 0.20696) 
		Y = pow(fy, 3);
	else
		Y = (fy - 16.0 / 116.0) / 7.787;
	
	if (fz > 0.20696) 
		Z = pow(fz, 3);
	else
		Z = (fz - 16.0 / 116.0) / 7.787;
	
	X *= 0.9642;
	Z *= 0.8249;

	chan[0] = DoubToFract(X);
	chan[1] = DoubToFract(Y);
	chan[2] = DoubToFract(Z);
#endif
}

static void
MatchOne_CMYK_LAB (UInt16* chan)
{
	MatchOne_CMYK_RGB(chan);
	MatchOne_RGB_LAB(chan);
}

static void
MatchOne_LAB_CMYK (UInt16* chan)
{
	MatchOne_LAB_RGB(chan);
	MatchOne_RGB_CMYK(chan);
}

static void
MatchOne_CMYK_XYZ (UInt16* chan)
{
	MatchOne_CMYK_RGB(chan);
	MatchOne_RGB_XYZ(chan);
}

static void
MatchOne_XYZ_CMYK (UInt16* chan)
{
	MatchOne_XYZ_RGB(chan);
	MatchOne_RGB_CMYK(chan);
}

static void
MatchOne_RGB_Gray (UInt16* chan)
{
	UInt16 alpha;
	alpha = chan[3]; // preserve alpha
	MatchOne_RGB_XYZ(chan);
	MatchOne_XYZ_Gray(chan);
	chan[1] = alpha; // preserve alpha
}

static void
MatchOne_Gray_RGB (UInt16* chan)
{
	chan[3] = chan[1]; // preserve alpha
	chan[1] = chan[2] = chan[0];
}

static void
MatchOne_LAB_Gray (UInt16* chan)
{
#pragma unused (chan)
	// nothing to do gray = L
}

static void
MatchOne_Gray_LAB (UInt16* chan)
{
	chan[1] = chan[2] = 0;
}

static void
MatchOne_CMYK_Gray (UInt16* chan)
{
	MatchOne_CMYK_XYZ(chan);
	MatchOne_XYZ_Gray(chan);
}

static void
MatchOne_Gray_CMYK (UInt16* chan)
{
	chan[3] = chan[0]; // K = gray
	chan[0] = chan[1] = chan[2] = 0; // CMY = 0
}

static void
MatchOne_XYZ_Gray (UInt16* chan)
{
	chan[0] = FractToUInt16(chan[1]); // gray = Y
}

static void
MatchOne_Gray_XYZ (UInt16* chan)
{
	double X,Y,Z;
	
	Y = UInt16ToDoub(chan[0]);
	X = Y * 0.96417;
	Z = Y * 0.82489;
	
	chan[0] = DoubToFract(X);
	chan[1] = DoubToFract(Y);
	chan[2] = DoubToFract(Z);
}

