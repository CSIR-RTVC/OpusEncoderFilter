/** @file

MODULE				: OpusEncoderFilter

FILE NAME			: OpusEncoderFilter.cpp

DESCRIPTION			:

LICENSE: Software License Agreement (BSD License)

Copyright (c) 2014, CSIR
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the CSIR nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
#include "OpusEncoderFilter.h"

//Codec classes
#include <OpusCodec/OpusFactory.h>
#include <CodecUtils/ICodecv2.h>
// #include "Conversion.h"
#include <Mmreg.h>

OpusEncoderFilter::OpusEncoderFilter()
	: CCustomBaseFilter(NAME("CSIR VPP Opus Encoder"), 0, CLSID_VPP_OpusEncoder),
  m_pCodec(NULL), 
  has_start(false), rtStart(0),
  m_uiSamplesPerSecond(0),
  m_uiChannels(0),
  m_uiBitsPerSample(0),
  m_uiMaxCompressedSize(0)
{
  //Call the initialise input method to load all acceptable input types for this filter
  InitialiseInputTypes();
  initParameters();
  OpusFactory factory;
  m_pCodec = factory.GetCodecInstance();
  // Set default codec properties 
  if (!m_pCodec)
  {
    SetLastError("Unable to create Opus Encoder from Factory.", true);
  }

//#ifdef TEST_OPUS_ENCODE_DECODE
//  m_pDecoder = factory.GetCodecInstance();
//  ASSERT(m_pDecoder);
//#endif
}

OpusEncoderFilter::~OpusEncoderFilter()
{
  if (m_pCodec)
  {
    m_pCodec->Close();
    OpusFactory factory;
    factory.ReleaseCodecInstance(m_pCodec);
  }

//#ifdef TEST_OPUS_ENCODE_DECODE
//  if (m_pDecoder)
//  {
//    m_pDecoder->Close();
//    OpusFactory factory;
//    factory.ReleaseCodecInstance(m_pDecoder);
//  }
//
//#endif
}

CUnknown * WINAPI OpusEncoderFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT *pHr )
{
	OpusEncoderFilter *pFilter = new OpusEncoderFilter();
	if (pFilter== NULL) 
	{
		*pHr = E_OUTOFMEMORY;
	}
	return pFilter;
}

void OpusEncoderFilter::InitialiseInputTypes()
{
	AddInputType(&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM, &FORMAT_WaveFormatEx);
}

HRESULT OpusEncoderFilter::SetMediaType( PIN_DIRECTION direction, const CMediaType *pmt )
{
	HRESULT hr = CCustomBaseFilter::SetMediaType(direction, pmt);
	if (direction == PINDIR_INPUT)
	{
    if (pmt->majortype != MEDIATYPE_Audio)
      return E_FAIL;
    if (pmt->subtype != MEDIASUBTYPE_PCM)
      return E_FAIL;
    if (pmt->formattype != FORMAT_WaveFormatEx)
      return E_FAIL;

    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pmt->pbFormat;

    // only accept raw audio
    if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
    {
      return E_UNEXPECTED;
    }
    if (pwfx->wBitsPerSample != 16)
    {
      return E_UNEXPECTED;
    }
    if (pwfx->nChannels < 0 || pwfx->nChannels > 2)
    {
      return E_UNEXPECTED;
    }

    switch (pwfx->nSamplesPerSec)
    {
    case 48000:
    case 24000:
    case 16000:
    case 12000:
    case 8000:
      break;
    default:
      return E_UNEXPECTED;
    }

    WAVEFORMATEX *pWfx = (WAVEFORMATEX*)pmt->pbFormat;
    // remember this for later
    m_uiSamplesPerSecond = pWfx->nSamplesPerSec;
    m_uiChannels = pWfx->nChannels;
    m_uiBitsPerSample = pWfx->wBitsPerSample;
    m_pAudioBuffer = std::make_unique<AudioBuffer>(m_uiSamplesPerSecond, m_uiChannels, m_uiBitsPerSample);

    m_pCodec->SetParameter("samples_per_second", std::to_string(m_uiSamplesPerSecond).c_str());
    m_pCodec->SetParameter("channels", std::to_string(m_uiChannels).c_str());
    m_pCodec->SetParameter("bits_per_sample", std::to_string(m_uiBitsPerSample).c_str());
    m_pCodec->SetParameter("target_bitrate_kbps", std::to_string(m_uiTargetBitrateKbps).c_str());

    if (!m_pCodec->Open())
    {
      //Houston: we have a failure
      char* szErrorStr = m_pCodec->GetErrorStr();
      printf("%s\n", szErrorStr);
      SetLastError(szErrorStr, true);
    }

//#ifdef TEST_OPUS_ENCODE_DECODE
//    m_pDecoder->SetParameter("samples_per_second", toString(m_uiSamplesPerSecond).c_str());
//    m_pDecoder->SetParameter("channels", toString(m_uiChannels).c_str());
//    m_pDecoder->SetParameter("bits_per_sample", toString(m_uiBitsPerSample).c_str());
//
//    if (!m_pDecoder->Open())
//    {
//      //Houston: we have a failure
//      char* szErrorStr = m_pDecoder->GetErrorStr();
//      printf("%s\n", szErrorStr);
//      SetLastError(szErrorStr, true);
//    }
//#endif
	}

	return hr;
}

HRESULT OpusEncoderFilter::GetMediaType( int iPosition, CMediaType *pMediaType )
{
	if (iPosition < 0)
	{
		return E_INVALIDARG;
	}
	else if (iPosition == 0)
	{
#ifdef TEST_OPUS_ENCODE_DECODE
    // Get the input pin's media type and return this as the output media type - we want to retain
    // all the information about the image
    HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
    if (FAILED(hr))
    {
      return hr;
    }
#else
		// there is only one type we can produce
		pMediaType->majortype = MEDIATYPE_Audio;
		pMediaType->subtype = MEDIASUBTYPE_OPUS;
    pMediaType->bFixedSizeSamples = FALSE;
    pMediaType->bTemporalCompression = FALSE;
    pMediaType->formattype = FORMAT_WaveFormatEx;
		pMediaType->lSampleSize = 0;
    pMediaType->pUnk = NULL;

    WAVEFORMATEX *pWfx = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
    memset(pWfx, 0, sizeof(*pWfx));
    pWfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    pWfx->wBitsPerSample = m_audioInHeader.wBitsPerSample;
		pWfx->nChannels = m_audioInHeader.nChannels;
		pWfx->nSamplesPerSec = m_audioInHeader.nSamplesPerSec;
    pWfx->nBlockAlign = (pWfx->wBitsPerSample * pWfx->nChannels) / 8;
    pWfx->nAvgBytesPerSec = pWfx->nSamplesPerSec * pWfx->nBlockAlign;
    pWfx->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
#endif
		return NOERROR;
	}
	return VFW_S_NO_MORE_ITEMS;
}

HRESULT OpusEncoderFilter::DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp )
{
	AM_MEDIA_TYPE mt;
	HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
	if (FAILED(hr))
	{
		return hr;
	}
	//Make sure that the format type is our custom format
	ASSERT(mt.formattype == FORMAT_WaveFormatEx);
  WAVEFORMATEX* pwfx = (WAVEFORMATEX*)mt.pbFormat;

  // decide the size
  // Use the buffer to be a seconds worth of data
  pProp->cBuffers = 5;
  pProp->cbBuffer = pwfx->nSamplesPerSec * pwfx->wBitsPerSample * pwfx->nChannels / 8;
  // configure max compressed size
  m_pCodec->SetParameter("max_compr_size", std::to_string(pProp->cbBuffer).c_str());

  pProp->cbAlign = pwfx->nBlockAlign;
  ASSERT(pProp->cbBuffer);

	// Release the format block.
	FreeMediaType(mt);

	// Set allocator properties.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}
	// Even when it succeeds, check the actual result.
	if (pProp->cbBuffer > Actual.cbBuffer) 
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT OpusEncoderFilter::ApplyTransform( BYTE* ptr, long lInBufferSize, long size, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength )
{
  // dummy since we're overriding receive
  return E_NOTIMPL;
}

HRESULT OpusEncoderFilter::StartStreaming()
{
	has_start = false;

	return __super::StartStreaming();
}

HRESULT OpusEncoderFilter::EndFlush()
{
	has_start = false;

	return __super::EndFlush();
}

HRESULT OpusEncoderFilter::Receive(IMediaSample *pSample)
{
  /*  Check for other streams and pass them on */
  AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
  if (pProps->dwStreamId != AM_STREAM_MEDIA) {
    return m_pOutput->Deliver(pSample);
  }
  HRESULT hr;

  IMediaSample *pSource = pSample;
  BYTE *pSourceBuffer;
  long lSourceSize = pSource->GetActualDataLength();
  pSource->GetPointer(&pSourceBuffer);

  AM_MEDIA_TYPE mt;
  hr = m_pOutput->ConnectionMediaType(&mt);
  if (FAILED(hr))
  {
    return hr;
  }
  //Make sure that the format type is our custom format
  ASSERT(mt.formattype == FORMAT_WaveFormatEx);
  WAVEFORMATEX* pwfx = (WAVEFORMATEX*)mt.pbFormat;

  int iCurrentFrame = 0;
  int iProcessedBytes = 0;
  int iCompressedSize = 0;
  int iCompressedBytes = 0;

  const LONGLONG TWENTY_MILLISECONDS = 200000;
  REFERENCE_TIME tStart, tStop;
  hr = pSample->GetTime(&tStart, &tStop);

  int res = m_pAudioBuffer->addAudioData(pSourceBuffer, lSourceSize, tStart, tStop);

  ASSERT(SUCCEEDED(hr));
  ASSERT (m_pCodec && m_pCodec->Ready());
  ASSERT(res != - 1);

  REFERENCE_TIME tStartSample, tStopSample;
  // while there is data send it downstream
  uint8_t* pStartOfSample = nullptr;
  while (m_pAudioBuffer->readNextAudioFrame(tStartSample, tStopSample, pStartOfSample))
  {
    ASSERT(pSample);
    IMediaSample * pOutSample;
    // If no output to deliver to then no point sending us data
    ASSERT(m_pOutput != NULL);
    // Set up the output sample
    hr = InitializeOutputSample(pSample, &pOutSample);
    if (FAILED(hr)) {
      return hr;
    }
    hr = pOutSample->SetTime(&tStartSample, &tStopSample);
    ASSERT(SUCCEEDED(hr));

    // Start timing the transform (if PERF is defined)
    MSR_START(m_idTransform);
    // have the derived class transform the data

    // hr = Transform(pSample, pOutSample);
    IMediaSample *pDest = pOutSample;

    BYTE *pDestBuffer;
    long lDestSize = pDest->GetSize();

#ifdef DEBUG
    ASSERT(lDestSize >= lSourceSize);
#endif
    pDest->GetPointer(&pDestBuffer);

    //////////////////////
    m_pCodec->SetParameter("max_compr_size", std::to_string(lDestSize).c_str());
    int nResult = m_pCodec->Code(pStartOfSample, pDestBuffer, m_pAudioBuffer->getBytesPerFrame());
    if (nResult)
    {
      //Encoding was successful
      iCompressedSize = m_pCodec->GetCompressedByteLength();
      DbgLog((LOG_TRACE, 5, TEXT("Compressed %d %d %d"), lSourceSize, iCompressedSize, m_pAudioBuffer->getBytesPerFrame()));
      pOutSample->SetActualDataLength(iCompressedSize);
    }
    else
    {
      //An error has occurred
      DbgLog((LOG_TRACE, 0, TEXT("Opus Codec Error: %s"), m_pCodec->GetErrorStr()));
      std::string sError = m_pCodec->GetErrorStr();
      return E_FAIL;
    }

    if (iCompressedSize > 1)
    {
      // a size of 1 means that it doesn't have to be transmitted
      iCompressedBytes += iCompressedSize;

      // Stop the clock and log it (if PERF is defined)
      MSR_STOP(m_idTransform);

      if (FAILED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("Error from transform")));
      }
      else {
        // the Transform() function can return S_FALSE to indicate that the
        // sample should not be delivered; we only deliver the sample if it's
        // really S_OK (same as NOERROR, of course.)
        if (hr == NOERROR) {
          hr = m_pOutput->Deliver(pOutSample);
          m_bSampleSkipped = FALSE;	// last thing no longer dropped
        }
        else {
          // S_FALSE returned from Transform is a PRIVATE agreement
          // We should return NOERROR from Receive() in this cause because returning S_FALSE
          // from Receive() means that this is the end of the stream and no more data should
          // be sent.
          if (S_FALSE == hr) {

            //  Release the sample before calling notify to avoid
            //  deadlocks if the sample holds a lock on the system
            //  such as DirectDraw buffers do
            pOutSample->Release();
            m_bSampleSkipped = TRUE;
            if (!m_bQualityChanged) {
              NotifyEvent(EC_QUALITY_CHANGE, 0, 0);
              m_bQualityChanged = TRUE;
            }
            return NOERROR;
          }
        }
      }

      // release the output buffer. If the connected pin still needs it,
      // it will have addrefed it itself.
      pOutSample->Release();
    }
    // iProcessedBytes += iBytesPerFrame;
    
    // end of transform
    ++iCurrentFrame;
  } // while
  return hr;
}

HRESULT OpusEncoderFilter::CheckTransform( const CMediaType *mtIn, const CMediaType *mtOut )
{
	// Check the major type.
	if (mtOut->majortype != MEDIATYPE_Audio)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	if (mtOut->subtype != MEDIASUBTYPE_OPUS)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	if (mtOut->formattype != FORMAT_WaveFormatEx)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	// Everything is good.
	return S_OK;
}

STDMETHODIMP OpusEncoderFilter::GetParameter( const char* szParamName, int nBufferSize, char* szValue, int* pLength )
{
	if (SUCCEEDED(CCustomBaseFilter::GetParameter(szParamName, nBufferSize, szValue, pLength)))
	{
		return S_OK;
	}
	else
	{
		// Check if its a codec parameter
		if (m_pCodec && m_pCodec->GetParameter(szParamName, pLength, szValue))
		{
			return S_OK;
		}
		return E_FAIL;
	}
}

STDMETHODIMP OpusEncoderFilter::SetParameter( const char* type, const char* value )
{
  if (SUCCEEDED(CCustomBaseFilter::SetParameter(type, value)))
	{
		return S_OK;
	}
	else
	{
		// Check if it's a codec parameter
		if (m_pCodec && m_pCodec->SetParameter(type, value))
		{
      // re-open codec if certain parameters have changed
      if ( _strnicmp(type, "target_bitrate_kbps", strlen(type)) == 0 )
      {
        /**
        if (!m_pCodec->Open())
        {
          //Houston: we have a failure
          char* szErrorStr = m_pCodec->GetErrorStr();
          printf("%s\n", szErrorStr);
          SetLastError(szErrorStr, true);
          return E_FAIL;
        }*/
      }
      return S_OK;
		}
		return E_FAIL;
	}
}

STDMETHODIMP OpusEncoderFilter::GetParameterSettings( char* szResult, int nSize )
{
  if (SUCCEEDED(CCustomBaseFilter::GetParameterSettings(szResult, nSize)))
	{
		// Now add the codec parameters to the output:
		int nLen = strlen(szResult);
		char szValue[10];
		int nParamLength = 10;
		std::string sCodecParams("Codec Parameters:\r\n");
		if( m_pCodec->GetParameter("parameters", &nParamLength, szValue))
		{
			int nParamCount = atoi(szValue);
			char szParamValue[256];
			memset(szParamValue, 0, 256);

			int nLenName = 0, nLenValue = 256;
			for (int i = 0; i < nParamCount; i++)
			{
				// Get parameter name
				const char* szParamName;
				m_pCodec->GetParameterName(i, &szParamName, &nLenName);
				// Now get the value
				m_pCodec->GetParameter(szParamName, &nLenValue, szParamValue);
				sCodecParams += "Parameter: " + std::string(szParamName) + " : Value:" + std::string(szParamValue) + "\r\n";
			}
			// now check if the buffer is big enough:
			int nTotalSize = sCodecParams.length() + nLen;
			if (nTotalSize < nSize)
			{
				memcpy(szResult + nLen, sCodecParams.c_str(), sCodecParams.length());
				// Set null terminator
				szResult[nTotalSize] = 0;
				return S_OK;
			}
			else
			{
				return E_FAIL;
			}
		}
		else
		{
			return E_FAIL;
		}
	}
	else
	{
		return E_FAIL;
	}
}

