/** @file

MODULE				: OpusEncoderFilter

FILE NAME			: OpusEncoderFilter.h

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
#pragma once
#include <DirectShowExt/CustomBaseFilter.h>
#include <DirectShowExt/CustomMediaTypes.h>
#include "VersionInfo.h"
#include "AudioBuffer.h"
#include "OpusEncoderProperties.h"

// #define TEST_OPUS_ENCODE_DECODE

#define FILTER_PARAM_TARGET_BITRATE_KBPS      "target_bitrate_kbps"

// Forward
class ICodecv2;

// {15065576-E28F-43AA-B719-87E83BA09410}
static const GUID CLSID_VPP_OpusEncoder =
{ 0x15065576, 0xe28f, 0x43aa, { 0xb7, 0x19, 0x87, 0xe8, 0x3b, 0xa0, 0x94, 0x10 } };

class OpusEncoderFilter : public CCustomBaseFilter,
                          public ISpecifyPropertyPages
{
public:
  DECLARE_IUNKNOWN

	/**
   * @brief Constructor
   */
	OpusEncoderFilter();
	/**
   * @brief Destructor
   */
	~OpusEncoderFilter();
  /**
   * @brief Static object-creation method (for the class factory)
   */
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr); 
  /**
 	 * @brief Overridden from CTransformFilter
	 */
	HRESULT Receive(IMediaSample *pSample);
	/**
 	 * Overriding this to set outgoing media type
	 */
	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	/**
 	 * Used for Media Type Negotiation 
	 * Returns an HRESULT value. Possible values include those shown in the following table.
	 * <table border="0" cols="2"><tr valign="top"><td><b>Value</b></td><td><b>Description</b></td></TR><TR><TD>S_OK</TD><TD>Success</TD></TR><TR><TD>VFW_S_NO_MORE_ITEMS</TD><TD>Index out of range</TD></TR><TR><TD>E_INVALIDARG</TD><TD>Index less than zero</TD></TR></TABLE>
	 * The output pin's CTransformOutputPin::GetMediaType method calls this method. The derived class must implement this method. For more information, see CBasePin::GetMediaType.
	 * To use custom media types, the media type can be manipulated in this method.
	 */
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	/**
	 * The output pin's CTransformOutputPin::DecideBufferSize method calls this method. The derived class must implement this method. For more information, see CBaseOutputPin::DecideBufferSize. 
	 * @param pAlloc Pointer to the IMemAllocator interface on the output pin's allocator.
	 * @param pProp Pointer to an ALLOCATOR_PROPERTIES structure that contains buffer requirements from the downstream input pin.
	 * @return Value: Returns S_OK or another HRESULT value.
	 */
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	/**
 	 * The CheckTransform method checks whether an input media type is compatible with an output media type.
	 * <table border="0" cols="2"> <tr valign="top"> <td  width="50%"><b>Value</b></td> <td width="50%"><b>Description</b></td> </tr> <tr valign="top"> <td width="50%">S_OK</td> <td width="50%">The media types are compatible.</td> </tr> <tr valign="top"> <td width="50%">VFW_E_TYPE_NOT_ACCEPTED</td> <td width="50%">The media types are not compatible.</td> </tr> </table>
	 */
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
   
  virtual HRESULT StartStreaming();
	virtual HRESULT EndFlush();

  virtual void doGetVersion(std::string& sVersion)
  {
    sVersion = VersionInfo::toString();
  }

	/// Interface methods
	///Overridden from CSettingsInterface
	virtual void initParameters()
	{
    addParameter("target_bitrate_kbps", &m_uiTargetBitrateKbps, 128);
	}

	/// Overridden from SettingsInterface
	STDMETHODIMP GetParameter( const char* szParamName, int nBufferSize, char* szValue, int* pLength );
	STDMETHODIMP SetParameter( const char* type, const char* value);
	STDMETHODIMP GetParameterSettings( char* szResult, int nSize );

	/// Overridden from CCustomBaseFilter
	virtual void InitialiseInputTypes();

  STDMETHODIMP GetPages(CAUUID* pPages)
  {
    if (pPages == NULL) return E_POINTER;
    pPages->cElems = 1;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
      return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_OpusProperties;
    return S_OK;
  }

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    if (riid == IID_ISpecifyPropertyPages)
    {
      return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    }
    else
    {
      // Call the parent class.
      return CCustomBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
  }

private:
	/**
	* This method converts the input buffer from RGB24 | 32 to YUV420P
	* @param pSource The source buffer
	* @param pDest The destination buffer
	*/
	virtual HRESULT ApplyTransform(BYTE* pBufferIn, long lInBufferSize, long lActualDataLength, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength);

  /// Codec
	ICodecv2* m_pCodec;

//#ifdef TEST_OPUS_ENCODE_DECODE
//  ICodecv2* m_pDecoder;
//#endif

  /// samples per second of source
  unsigned int m_uiSamplesPerSecond;
  /// number of channels of source
  unsigned int m_uiChannels;
  /// bits per sample of source
  unsigned int m_uiBitsPerSample;
  /// the maximum number of bytes to be used when compressing
  unsigned int m_uiMaxCompressedSize;
  /// kbps
  uint32_t m_uiTargetBitrateKbps;

	REFERENCE_TIME		rtStart;
	REFERENCE_TIME		rtInput;
	bool				has_start;

  std::unique_ptr<AudioBuffer> m_pAudioBuffer;
};
