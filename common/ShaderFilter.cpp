// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------------------

// project header files
#include "ShaderFilter.h"
#include "FilterBigDocument.h"
#include <time.h>
#include "Logger.h"
#include "Timer.h"

#include <iostream>
#include <math.h>
#include <algorithm>
#include <thread>
#include "renderer/renderer.h"
#include "math/CommonMath.h"
#include "math/vec2.h"
#include "color/color.h"

//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
// parameters passed into PluginMain that need to be global to the project
FilterRecord * gFilterRecord = NULL;
intptr_t * gDataHandle = NULL;
int16 * gResult = NULL;		// all errors go here
SPBasicSuite * sSPBasic = NULL;

// pointers to our data and parameters defined in Dissolve.h
Data * gData = NULL;
Parameters * gParams = NULL;

//-------------------------------------------------------------------------------
// local routines
//-------------------------------------------------------------------------------
// the six main routines of the plug in
void DoParameters(void);
void DoPrepare(void);
void DoStart(void);
void DoContinue(void);
void DoFinish(void);

void DoFilter(void);

void ConvertRGBColorToMode(const int16 imageMode, FilterColor& color);
void ScaleRect(VRect& destination, const int16 num, const int16 den);
void ShrinkRect(VRect& destination, const int16 width, const int16 height);
void CopyRect(VRect& destination, const VRect& source);
void LockHandles(void);
void UnlockHandles(void);
void CreateParametersHandle(void);
void InitParameters(void);
void CreateDataHandle(void);
void InitData(void);
void CopyRenderedImageToPhotoshop(float* srcPixels, int width, int height, int bytesPerPixel);

void kernel(const unsigned int& x,
	const unsigned int& y,
	const unsigned int& width,
	const unsigned int& height,
	Color& outputColor)
{
	static float aspectRatio = (float)width / height;
	Vec2 uv = Vec2(float(x) / width, float(y) / height) * 2.0f - 1.0f;
	uv.X() *= aspectRatio;

	float t = float(pow(abs(1.0f / (((uv.X() * 300.0) + sin(uv.Y() * 5.0f)*50.0f))), 0.75f));
	t = clamp01(t);
	outputColor.SetValues(t * 2.0f, t * 4.0f, t * 8.0f, 1.0f);
	Color::Clamp(outputColor, 0.0f, 1.0f);
}

//-------------------------------------------------------------------------------
//
//	PluginMain
//	
//	All calls to the plug in module come through this routine.
//
//	Inputs:
//		const int16 selector		Host provides selector indicating what
//									command to do.
//
//	Inputs and Outputs:
//		FilterRecord *filterRecord	Host provides a pointer to parameter block
//									containing pertinent data and callbacks.
//									See PIFilter.h
//
//		intptr_t *data				Use this to store a handle or pointer to our global
//									data structure, which is maintained by the
//									host between calls to the plug in.
//
//	Outputs:
//		int16 *result				Returns error result. Some errors are handled
//									by the host, some are silent, and some you
//									must handle. See PIGeneral.h.
//
//-------------------------------------------------------------------------------
DLLExport MACPASCAL void PluginMain(const int16 selector,
								    FilterRecordPtr filterRecord,
								    intptr_t * data,
								    int16 * result)
{
	
	try {


	Logger logIt("ShaderFilter");
	Timer timeIt;

	logIt.Write( "Selector: ", false );
	logIt.Write( selector, false );
	logIt.Write( " ", false );

	// update our global parameters
	gFilterRecord = filterRecord;
	gDataHandle = data;
	gResult = result;

	if (selector == filterSelectorAbout)
	{
		sSPBasic = ((AboutRecord*)gFilterRecord)->sSPBasic;
	}
	else
	{
		sSPBasic = gFilterRecord->sSPBasic;

		if (gFilterRecord->bigDocumentData != NULL)
			gFilterRecord->bigDocumentData->PluginUsing32BitCoordinates = true;
	}

	// do the command according to the selector
	switch (selector)
	{
		case filterSelectorAbout:
			
			break;
		case filterSelectorParameters:
			DoParameters();
			break;
		case filterSelectorPrepare:
			DoPrepare();
			break;
		case filterSelectorStart:
			DoStart();
			break;
		case filterSelectorContinue:
			DoContinue();
			break;
		case filterSelectorFinish:
			DoFinish();
			break;
		default:
			break;
	}
	
	// unlock our handles used by gData and gParams
	if (selector != filterSelectorAbout)
		UnlockHandles();

	logIt.Write( timeIt.GetElapsed(), true );

	} // end try

	catch (...)
	{
		if (NULL != result)
			*result = -1;
	}

}

//-------------------------------------------------------------------------------
//
// DoParameters
//
// Makes sure we have valid Data and Parameters handle(s). Locks and initializes
// these items.
// 
// NOTE:
// This routine is NOT guaranteed to be called by Photoshop. If a user enters the
// CTRL-F keyboard shortcut to invoke the last filter command this routine will
// NOT be called. If the filter is ran by the actions pallete or an automation
// plug in this routine will NOT be called.
//
// NOTE:
// The fields in the gFilterRecord are not all valid at this stage.
//-------------------------------------------------------------------------------
void DoParameters(void)
{
	if (gFilterRecord->parameters == NULL)
		CreateParametersHandle();
	if ((*gDataHandle) == 0)
		CreateDataHandle();
	if (*gResult == noErr)
	{
		LockHandles();
		InitParameters();
		InitData();
	}
}

//-------------------------------------------------------------------------------
//
// DoPrepare
//
// Almost identical to DoParameters. Make sure we have valid Data and Parameters
// handle(s) and lock and initialize as necessary. Sets the bufferSpace and 
// maxSpace variables in the gFilterRecord so memory is used at an optimum.
//
// NOTE:
// The fields in the gFilterRecord are not all valid at this stage. We will take a
// guess at the actual tile size information.
// 
//-------------------------------------------------------------------------------
void DoPrepare(void)
{
	if (gFilterRecord->parameters != NULL && (*gDataHandle) != 0)
		LockHandles();
	else
	{
		if (gFilterRecord->parameters == NULL)
			CreateParametersHandle();
		if ((*gDataHandle) == 0)
			CreateDataHandle();
		if (*gResult == noErr)
		{
			LockHandles();
			InitParameters();
			InitData();
		}
	}
	
	// we don't need any buffer space
	gFilterRecord->bufferSpace = 0; 

	// give as much memory back to Photoshop as you can
	// we only need a tile per plane plus the maskData
	// inTileHeight and inTileWidth are invalid at this
	// point. Assume the tile size is 256 max.
	VRect filterRect = GetFilterRect();
	int32 tileHeight = filterRect.bottom - filterRect.top;
	int32 tileWidth = filterRect.right - filterRect.left;
	if (tileHeight > 256)
		tileHeight = 256;
	if (tileWidth > 256)
		tileWidth = 256;

	int32 tileSize = tileHeight * tileWidth;
	int32 planes = gFilterRecord->planes;
	if (gFilterRecord->maskData != NULL)
		planes++;
	// duplicate because we have two copies, inData and outData
	planes *= 2;

	int32 totalSize = tileSize * planes;
	// this is worst case and can be dropped considerably
	if (gFilterRecord->maxSpace > totalSize)
		gFilterRecord->maxSpace = totalSize;
}

//-------------------------------------------------------------------------------
//
// DoStart
//
// The main filtering routine for this plug in. See if we have any registry
// parameters from the last time we ran. Determine if the UI needs to be
// displayed by reading the script parameters. Save the last dialog parameters
// in case something goes wrong or the user cancels.
//
//-------------------------------------------------------------------------------
void DoStart(void)
{
	LockHandles();

	// save parameters
	int16 lastDisposition = gParams->disposition;
	int16 lastPercent = gParams->percent;
	Boolean lastIgnoreSelection = gParams->ignoreSelection;


	// we know we have enough information to run without next time
	gData->queryForParameters = false;

	// the main processing routine
	DoFilter();
}

//-------------------------------------------------------------------------------
//
// DoContinue
//
// If we get here we probably did something wrong. This selector was needed
// before advanceState() was in the FilterRecord*. Now that we use advanceState()
// there is nothing for us to do but set all the rectangles to 0 and return.
//
//-------------------------------------------------------------------------------
void DoContinue(void)
{
	VRect zeroRect = { 0, 0, 0, 0 };

	SetInRect(zeroRect);
	SetOutRect(zeroRect);
	SetMaskRect(zeroRect);
}

//-------------------------------------------------------------------------------
//
// DoFinish
//
// Everything went as planned and the pixels have been modified. Now record
// scripting parameters and put our information in the Photoshop Registry for the
// next time we get called. The Registry saves us from keeping a preferences file.
//
//-------------------------------------------------------------------------------
void DoFinish(void)
{
	LockHandles();
}

//-------------------------------------------------------------------------------
//
// DoFilter
//
// Randomly change the pixel values based on the parameters the user gave us from
// our dialog box or scripting. We do this a tile at a time making sure the rect.
// we ask for is in the bounds of the filterRect.
//
//-------------------------------------------------------------------------------
void DoFilter(void)
{
	// Fixed numbers are 16.16 values 
	// the first 16 bits represent the whole number
	// the last 16 bits represent the fraction
	gFilterRecord->inputRate = (int32)1 << 16;
	gFilterRecord->maskRate = (int32)1 << 16;

	VRect filterRect = GetFilterRect();
	VRect inRect = GetInRect();

	inRect.top = filterRect.top;
	inRect.left = filterRect.left;
	inRect.bottom = filterRect.bottom;
	inRect.right = filterRect.right;

	SetInRect(inRect);

	// duplicate what's in the inData with the outData
	SetOutRect(inRect);

	int bytesPerPixel = 4;
	Renderer renderer(kernel, inRect.right, inRect.bottom, bytesPerPixel);
	renderer.Render();
	float *pixels = renderer.GetPixels();

	CopyRenderedImageToPhotoshop(pixels, inRect.right, inRect.bottom, bytesPerPixel);
}

void CopyRenderedImageToPhotoshop(float* srcPixels, int width, int height, int bytesPerPixel)
{
	for (int16 plane = 0; plane < gFilterRecord->planes; plane++)
	{
		// we want one plane at a time, small memory foot print is good
		gFilterRecord->outLoPlane = gFilterRecord->inLoPlane = plane;
		gFilterRecord->outHiPlane = gFilterRecord->inHiPlane = plane;
	
		// update the gFilterRecord with our latest request
		*gResult = gFilterRecord->advanceState();
		if (*gResult != noErr) return;

		int pixelIndex = 0;
		for (int i = plane; i < width * height * bytesPerPixel; i += bytesPerPixel)
		{
			float *fPixel = (float*)gFilterRecord->outData;
			fPixel[pixelIndex] = srcPixels[i];
			++pixelIndex;
		}
	}
}

//-------------------------------------------------------------------------------
//
// CreateParametersHandle
//
// Create a handle to our Parameters structure. Photoshop will take ownership of
// this handle and delete it when necessary.
//-------------------------------------------------------------------------------
void CreateParametersHandle(void)
{
	gFilterRecord->parameters = gFilterRecord->handleProcs->newProc
											(sizeof(Parameters));
	if (gFilterRecord->parameters == NULL)
		*gResult = memFullErr;
}

//-------------------------------------------------------------------------------
//
// InitParameters
//
// Initialize our UI parameters. gParams is guaranteed to point at something
//-------------------------------------------------------------------------------
void InitParameters(void)
{
	gParams->disposition = 1;
	gParams->ignoreSelection = false;
	gParams->percent = 50;
}

//-------------------------------------------------------------------------------
//
// CreateDataHandle
//
// Create a pointer to our Data structure. Photoshop will take ownership of this
// and give it back to use on any future calls.
//-------------------------------------------------------------------------------
void CreateDataHandle(void)
{
	Handle h = gFilterRecord->handleProcs->newProc(sizeof(Data));
	if (h != NULL)
		*gDataHandle = (intptr_t)h;
	else
		*gResult = memFullErr;
}

//-------------------------------------------------------------------------------
//
// InitData
//
// Initialize the gData pointer
//-------------------------------------------------------------------------------
void InitData(void)
{
	CopyColor(gData->colorArray[0], gFilterRecord->backColor);
	SetColor(gData->colorArray[1], 0, 0, 255, 0);
	SetColor(gData->colorArray[2], 255, 0, 0, 0);
	SetColor(gData->colorArray[3], 0, 255, 0, 0);
	for(int a = 1; a < 4; a++)
		ConvertRGBColorToMode(gFilterRecord->imageMode, gData->colorArray[a]);
	CopyColor(gData->color, gData->colorArray[gParams->disposition]);
	gData->proxyRect.left = 0;
	gData->proxyRect.right = 0;
	gData->proxyRect.top = 0;
	gData->proxyRect.bottom = 0;
	gData->scaleFactor = 1.0;
	gData->queryForParameters = true;
	gData->dissolveBufferID = NULL;
	gData->dissolveBuffer = NULL;
	gData->proxyBufferID = NULL;
	gData->proxyBuffer = NULL;
	gData->proxyWidth = 0;
	gData->proxyHeight = 0;
	gData->proxyPlaneSize = 0;
}

//-------------------------------------------------------------------------------
//
// ConvertRGBColorToMode
//
// Convert the FilterColor from RGB mode to the imageMode using the color
// services call backs.
//
// Inputs:
//		int16 imageMode			Mode to convert the color to
// Inputs and Outputs:
//		FilterColor& color		RGB color to convert
//
//-------------------------------------------------------------------------------
void ConvertRGBColorToMode(const int16 imageMode, FilterColor& color)
{
	if (imageMode != plugInModeRGBColor)
	{
		ColorServicesInfo	csInfo;

		csInfo.selector = plugIncolorServicesConvertColor;
		csInfo.sourceSpace = plugIncolorServicesRGBSpace;
		csInfo.reservedSourceSpaceInfo = NULL;
		csInfo.reservedResultSpaceInfo = NULL;
		csInfo.reserved = NULL;
		csInfo.selectorParameter.pickerPrompt = NULL;
		csInfo.infoSize = sizeof(csInfo);

		csInfo.resultSpace = CSModeToSpace(gFilterRecord->imageMode);
		for (int16 a = 0; a < 4; a++)
			csInfo.colorComponents[a] = color[a];

		if (!(gFilterRecord->colorServices(&csInfo)))
			for (int16 b = 0; b < 4; b++)
				color[b] = (int8)csInfo.colorComponents[b];
	}				   
}

//-------------------------------------------------------------------------------
//
// LockHandles
//
// Lock the handles and get the pointers for gData and gParams
// Set the global error, *gResult, if there is trouble
//
//-------------------------------------------------------------------------------
void LockHandles(void)
{
	if (gFilterRecord->parameters == NULL || (*gDataHandle) == 0)
	{
		*gResult = filterBadParameters;
		return;
	}
	gParams = (Parameters*)gFilterRecord->handleProcs->lockProc
				(gFilterRecord->parameters, TRUE);
	gData = (Data*)gFilterRecord->handleProcs->lockProc
		        ((Handle)*gDataHandle, TRUE);
	if (gParams == NULL || gData == NULL)
	{
		*gResult = memFullErr;
		return;
	}
}

//-------------------------------------------------------------------------------
//
// UnlockHandles
//
// Unlock the handles used by the data and params pointers
//
//-------------------------------------------------------------------------------
void UnlockHandles(void)
{
	if ((*gDataHandle) != 0)
		gFilterRecord->handleProcs->unlockProc((Handle)*gDataHandle);
	if (gFilterRecord->parameters != NULL)
		gFilterRecord->handleProcs->unlockProc(gFilterRecord->parameters);
}

//-------------------------------------------------------------------------------
//
// ScaleRect
//
// Utility routine for scaling a rectangle by a rational
//
//-------------------------------------------------------------------------------
void ScaleRect(VRect& destination, const int16 num, const int16 den)
{
	if (den != 0)
	{
		destination.left = (int16)((destination.left * num) / den);
		destination.top = (int16)((destination.top * num) / den);
		destination.right = (int16)((destination.right * num) / den);
		destination.bottom = (int16)((destination.bottom * num) / den);
	}
}

//-------------------------------------------------------------------------------
//
// ShrinkRect
//
// Utility routine for shrinking a Rect by a width and height
//
//-------------------------------------------------------------------------------
void ShrinkRect(VRect& destination, const int16 width, const int16 height)
{
	destination.left = (int16)(destination.left + width);
	destination.top = (int16)(destination.top + height);
	destination.right = (int16)(destination.right - width);
	destination.bottom = (int16)(destination.bottom - height);
}

//-------------------------------------------------------------------------------
//
// CopyRect
//
// Utility routine for setting a Rect from a VRect
//
//-------------------------------------------------------------------------------
void CopyRect(VRect& destination, const VRect& source)
{
	destination.left = source.left;
	destination.top = source.top;
	destination.right = source.right;
	destination.bottom = source.bottom;
}

//-------------------------------------------------------------------------------
//
// CopyColor
//
// Utility routine for setting a FilterColor array from a FilterColor
//
//-------------------------------------------------------------------------------
void CopyColor(FilterColor& destination, const FilterColor& source)
{
	for (int a = 0; a < sizeof(FilterColor); a++)
		destination[a] = source[a];
}

//-------------------------------------------------------------------------------
//
// SetColor
//
// Utility routine for setting a FilterColor array from 4 color components
//
//-------------------------------------------------------------------------------
void SetColor(FilterColor& destination, 
			  const uint8 a, 
			  const uint8 b, 
			  const uint8 c, 
			  const uint8 d)
{
	destination[0] = a;
	destination[1] = b;
	destination[2] = c;
	destination[3] = d;
}

//-------------------------------------------------------------------------------
//
// DisplayPixelsMode
//
// Convert the imageMode into a display mode so we can use displayPixels.
// All of the 16 bit data is converted to 8 bit.
//
//-------------------------------------------------------------------------------
int32 DisplayPixelsMode(int16 mode)
{
	int32 returnMode = mode;
	switch (mode)
	{
		case plugInModeGray16:
		case plugInModeGray32:
			returnMode = plugInModeGrayScale;
			break;
		case plugInModeRGB96:
		case plugInModeRGB48:
			returnMode = plugInModeRGBColor;
			break;
		case plugInModeLab48:
			returnMode = plugInModeLabColor;
			break;
		case plugInModeCMYK64:
			returnMode = plugInModeCMYKColor;
			break;
		case plugInModeDeepMultichannel:
			returnMode = plugInModeMultichannel;
			break;
		case plugInModeDuotone16:
			returnMode = plugInModeDuotone;
			break;
	}
	return (returnMode);
}
// end Dissolve.cpp