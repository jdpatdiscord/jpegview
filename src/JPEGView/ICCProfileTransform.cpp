#include "stdafx.h"

#include "ICCProfileTransform.h"
#include "SettingsProvider.h"


#ifndef WINXP

// This define is necessary for 32-bit builds to work, for some reason
#define CMS_DLL
#include "lcms2.h"


void* ICCProfileTransform::sRGBProfile = NULL;

void* ICCProfileTransform::CreateTransform(const void* profile, unsigned int size, PixelFormat format)
{
	if (profile == NULL || size == 0)
		return NULL; // No ICC Profile
	if (sRGBProfile == NULL) {
		try {
			sRGBProfile = cmsCreate_sRGBProfile();
		} catch (...) {}
	}
	if (sRGBProfile == NULL)
		return NULL; // lcms2.dll not found or out of memory
	cmsHPROFILE hInProfile = cmsOpenProfileFromMem(profile, size);
	if (hInProfile == NULL)
		return NULL; // Invalid profile

	// Create transform from embedded profile to sRGB
	cmsUInt32Number flags = cmsFLAGS_BLACKPOINTCOMPENSATION | cmsFLAGS_COPY_ALPHA;
	cmsUInt32Number inFormat, outFormat;
	switch (format) {
		case FORMAT_BGRA:
			inFormat = TYPE_BGRA_8;
			outFormat = TYPE_BGRA_8;
			break;
		case FORMAT_RGBA:
			inFormat = TYPE_RGBA_8;
			outFormat = TYPE_BGRA_8;
			break;
		case FORMAT_BGR:
			inFormat = TYPE_BGR_8;
			outFormat = TYPE_BGR_8;
			break;
		case FORMAT_RGB:
			inFormat = TYPE_RGB_8;
			outFormat = TYPE_BGR_8;
			break;
		default:
			cmsCloseProfile(hInProfile);
			return NULL;
	}
	cmsHTRANSFORM transform = cmsCreateTransform(hInProfile, inFormat, sRGBProfile, outFormat, INTENT_RELATIVE_COLORIMETRIC, flags);
	cmsCloseProfile(hInProfile);
	return transform;
}

bool ICCProfileTransform::DoTransform(void* transform, const void* inputBuffer, void* outputBuffer, unsigned int width, unsigned int height, unsigned int stride)
{
	unsigned int numPixels = width * height;
	if (transform == NULL || inputBuffer == NULL || outputBuffer == NULL || numPixels == 0 || !CSettingsProvider::This().UseEmbeddedColorProfiles())
		return false;
	cmsUInt32Number inFormat = cmsGetTransformInputFormat(transform);
	int nchannels;
	if (inFormat == TYPE_BGRA_8 || inFormat == TYPE_RGBA_8) {
		nchannels = 4;
	} else {
		nchannels = 3;
	}
	if (stride == 0)
		stride = width * nchannels;
	cmsDoTransformLineStride(transform, inputBuffer, outputBuffer, width, height, stride, Helpers::DoPadding(width * nchannels, 4), stride * height, Helpers::DoPadding(width * nchannels, 4) * height);
	return true;
}

void ICCProfileTransform::DeleteTransform(void* transform)
{
	if (transform != NULL)
		cmsDeleteTransform(transform);
}


#else

// stub out lcms2 methods in an elegant way in XP build, as per suggestion https://github.com/sylikc/jpegview/commit/4b62f07e2a147a04a5014a5711d159670162e799#commitcomment-102738193

void* ICCProfileTransform::CreateTransform(const void* /* profile */, unsigned int /* size */, PixelFormat /* format */) {
	return NULL;
}

bool ICCProfileTransform::DoTransform(void* /* transform */, const void* /* inputBuffer */, void* /* outputBuffer */, unsigned int /* width */, unsigned int /* height */, unsigned int /* stride */) {
	return false;
}

void ICCProfileTransform::DeleteTransform(void* /* transform */) { }

#endif