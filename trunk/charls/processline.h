// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 
#ifndef CHARLS_PROCESSLINE
#define CHARLS_PROCESSLINE

#include "colortransform.h"
#include <iostream>
#pragma warning(disable: 4996)
//
// This file defines the ProcessLine base class, its derivitives and helper functions.
// During coding/decoding, CharLS process one line at a time. The different Processline implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.
// This mechanism could be used to encode/decode images as they are received.
//

class ProcessLine
{
public:
	virtual ~ProcessLine() {}
	virtual void NewLineDecoded(const void* pSrc, int pixelCount, int bytesperline) = 0;
	virtual void NewLineRequested(void* pDest, int pixelCount, int bytesperline) = 0;
};


class PostProcesSingleComponent : public ProcessLine
{
public:
	PostProcesSingleComponent(void* rawData, const JlsParameters& info, int bytesPerPixel) :
		_rawData((BYTE*)rawData), 
		_bytesPerPixel(bytesPerPixel),
		_bytesPerLine(info.bytesperline)
	{
	}

	void NewLineRequested(void* dest, int pixelCount, int /*byteStride*/)
	{
		::memcpy(dest, _rawData, pixelCount * _bytesPerPixel);
		_rawData += _bytesPerLine;
	}

	void NewLineDecoded(const void* pSrc, int pixelCount, int /*byteStride*/)
	{
		::memcpy(_rawData, pSrc, pixelCount * _bytesPerPixel);
		_rawData += _bytesPerLine;		
	}

private:
	BYTE* _rawData;
	int _bytesPerPixel;
	int _bytesPerLine;
};


class PostProcesSingleStream : public ProcessLine
{
public:
	PostProcesSingleStream(byteStream* rawData, const JlsParameters& info, int bytesPerPixel) :
		_rawData(rawData), 
		_bytesPerPixel(bytesPerPixel),
		_bytesPerLine(info.bytesperline)
	{
	}

	void NewLineRequested(void* dest, int pixelCount, int /*byteStride*/)
	{
		int bytesToRead = pixelCount * _bytesPerPixel;
		while(bytesToRead != 0)
		{
			int read = _rawData->sgetn((char*)dest, bytesToRead);
			if (read == 0)
				throw new JlsException(UncompressedBufferTooSmall);

			bytesToRead -= read;
		}
		//_rawData->pubseekoff(_bytesPerLine - bytesToRead, std::ios_base::cur);
	}

	void NewLineDecoded(const void* /*pSrc*/, int /*pixelCount*/, int /*byteStride*/)
	{
		 	
	}

private:
	byteStream* _rawData;
	int _bytesPerPixel;
	int _bytesPerLine;
	
};

template<class TRANSFORM, class SAMPLE> 
void TransformLineToQuad(const SAMPLE* ptypeInput, LONG pixelStrideIn, Quad<SAMPLE>* pbyteBuffer, LONG pixelStride, TRANSFORM& transform)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	Quad<SAMPLE>* ptypeBuffer = (Quad<SAMPLE>*)pbyteBuffer;

	for (int x = 0; x < cpixel; ++x)
	{
		Quad<SAMPLE> pixel(transform(ptypeInput[x], ptypeInput[x + pixelStrideIn], ptypeInput[x + 2*pixelStrideIn]),ptypeInput[x + 3*pixelStrideIn]) ;
		
		ptypeBuffer[x] = pixel;
	}
}


template<class TRANSFORM, class SAMPLE> 
void TransformQuadToLine(const Quad<SAMPLE>* pbyteInput, LONG pixelStrideIn, SAMPLE* ptypeBuffer, LONG pixelStride, TRANSFORM& transform)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	const Quad<SAMPLE>* ptypeBufferIn = (Quad<SAMPLE>*)pbyteInput;

	for (int x = 0; x < cpixel; ++x)
	{
		Quad<SAMPLE> color = ptypeBufferIn[x];
		Quad<SAMPLE> colorTranformed(transform(color.v1, color.v2, color.v3), color.v4);

		ptypeBuffer[x] = colorTranformed.v1;
		ptypeBuffer[x + pixelStride] = colorTranformed.v2;
		ptypeBuffer[x + 2 *pixelStride] = colorTranformed.v3;
		ptypeBuffer[x + 3 *pixelStride] = colorTranformed.v4;
	}
}


template<class SAMPLE> 
void TransformRgbToBgr(SAMPLE* pDest, int samplesPerPixel, int pixelCount)
{
	for (int i = 0; i < pixelCount; ++i)
	{
		std::swap(pDest[0], pDest[2]);		
		pDest += samplesPerPixel;
	}
}


template<class TRANSFORM, class SAMPLE> 
void TransformLine(Triplet<SAMPLE>* pDest, const Triplet<SAMPLE>* pSrc, int pixelCount, TRANSFORM& transform) 
{	
	for (int i = 0; i < pixelCount; ++i)
	{
		pDest[i] = transform(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3);
	}
}


template<class TRANSFORM, class SAMPLE> 
void TransformLineToTriplet(const SAMPLE* ptypeInput, LONG pixelStrideIn, Triplet<SAMPLE>* pbyteBuffer, LONG pixelStride, TRANSFORM& transform)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	Triplet<SAMPLE>* ptypeBuffer = (Triplet<SAMPLE>*)pbyteBuffer;

	for (int x = 0; x < cpixel; ++x)
	{
		ptypeBuffer[x] = transform(ptypeInput[x], ptypeInput[x + pixelStrideIn], ptypeInput[x + 2*pixelStrideIn]);
	}
}


template<class TRANSFORM, class SAMPLE> 
void TransformTripletToLine(const Triplet<SAMPLE>* pbyteInput, LONG pixelStrideIn, SAMPLE* ptypeBuffer, LONG pixelStride, TRANSFORM& transform)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	const Triplet<SAMPLE>* ptypeBufferIn = (Triplet<SAMPLE>*)pbyteInput;

	for (int x = 0; x < cpixel; ++x)
	{
		Triplet<SAMPLE> color = ptypeBufferIn[x];
		Triplet<SAMPLE> colorTranformed = transform(color.v1, color.v2, color.v3);

		ptypeBuffer[x] = colorTranformed.v1;
		ptypeBuffer[x + pixelStride] = colorTranformed.v2;
		ptypeBuffer[x + 2 *pixelStride] = colorTranformed.v3;
	}
}


template<class TRANSFORM> 
class ProcessTransformed : public ProcessLine
{
	typedef typename TRANSFORM::SAMPLE SAMPLE;

	ProcessTransformed(const ProcessTransformed&) {}
public:
	ProcessTransformed(void* rawData, byteStream* rawStream, const JlsParameters& info, TRANSFORM transform) :
		_rawData((BYTE*)rawData),
		_info(info),
		_templine(info.width *  info.components),
		_transform(transform),
		_inverseTransform(transform),
		_rawStream(rawStream)
	{
//		ASSERT(_info.components == sizeof(TRIPLET)/sizeof(TRIPLET::SAMPLE));
	}
		

	void NewLineRequested(void* dest, int pixelCount, int stride)
	{		
		if (_rawStream != NULL)
		{
			int bytesToRead = pixelCount * _info.components * sizeof(SAMPLE);	
			std::vector<BYTE> buffer(bytesToRead);
			while(bytesToRead != 0)
			{
				int read = _rawStream->sgetn((char*)&buffer[0], bytesToRead);
				if (read == 0)
					throw new JlsException(UncompressedBufferTooSmall);

				bytesToRead -= read;
			}
			Transform(&buffer[0], dest, pixelCount, stride);		
			return;
		}

		Transform(_rawData, dest, pixelCount, stride);
		_rawData += _info.bytesperline;
	}

	void Transform(const void* source, void* dest, int pixelCount, int stride)
	{
		if (_info.outputBgr)
		{
			memcpy(&_templine[0], source, sizeof(Triplet<SAMPLE>)*pixelCount);
			TransformRgbToBgr((SAMPLE*)&_templine[0], _info.components, pixelCount);
			source = &_templine[0]; 			
		}

		if (_info.components == 3)
		{
			if (_info.ilv == ILV_SAMPLE)
			{
				TransformLine((Triplet<SAMPLE>*)dest, (const Triplet<SAMPLE>*)source, pixelCount, _transform);
			}
			else
			{
				TransformTripletToLine((const Triplet<SAMPLE>*)source, pixelCount, (SAMPLE*)dest, stride, _transform);
			}
		}
		else if (_info.components == 4 && _info.ilv == ILV_LINE)
		{
			TransformQuadToLine((const Quad<SAMPLE>*)source, pixelCount, (SAMPLE*)dest, stride, _transform);
		}
	}


	void DecodeTransform(const void* pSrc, void* rawData, int pixelCount, int byteStride)
	{
		if (_info.components == 3)
		{	
			if (_info.ilv == ILV_SAMPLE)
			{
				TransformLine((Triplet<SAMPLE>*)rawData, (const Triplet<SAMPLE>*)pSrc, pixelCount, _inverseTransform);
			}
			else
			{
				TransformLineToTriplet((const SAMPLE*)pSrc, byteStride, (Triplet<SAMPLE>*)rawData, pixelCount, _inverseTransform);
			}
		}
		else if (_info.components == 4 && _info.ilv == ILV_LINE)
		{
			TransformLineToQuad((const SAMPLE*)pSrc, byteStride, (Quad<SAMPLE>*)rawData, pixelCount, _inverseTransform);
		}

		if (_info.outputBgr)
		{
			TransformRgbToBgr((SAMPLE*)rawData, _info.components, pixelCount);
		}
	}

	void NewLineDecoded(const void* pSrc, int pixelCount, int byteStride)
	{
		DecodeTransform(pSrc, _rawData, pixelCount, byteStride);
		
		_rawData += _info.bytesperline;		
	}


private:
	BYTE* _rawData;
	const JlsParameters& _info;	
	std::vector<SAMPLE> _templine;
	TRANSFORM _transform;	
	typename TRANSFORM::INVERSE _inverseTransform;
	byteStream* _rawStream;
};



#endif
