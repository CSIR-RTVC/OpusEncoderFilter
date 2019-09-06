#pragma once

#include <DirectShowExt/FilterPropertiesBase.h>
#include "Conversion.h"
#include <cassert>
#include <climits>
#include "resource.h"

#include "FilterParameters.h"

// {17A6C974-20A9-4292-8CEE-F52BADB81C36}
static const GUID CLSID_OpusProperties =
{ 0x17a6c974, 0x20a9, 0x4292, { 0x8c, 0xee, 0xf5, 0x2b, 0xad, 0xb8, 0x1c, 0x36 } };


#define BUFFER_SIZE 256

const int RADIO_BUTTON_IDS[] = { IDC_RADIO_OPUS_APPLICATION_VOIP, IDC_RADIO_OPUS_APPLICATION_AUDIO, IDC_RADIO_OPUS_APPLICATION_RESTRICTED_LOWDELAY };

class OpusEncoderProperties : public FilterPropertiesBase
{
public:

  static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* pHr)
  {
    OpusEncoderProperties* pNewObject = new OpusEncoderProperties(pUnk);
    if (pNewObject == NULL)
    {
      *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
  }

  OpusEncoderProperties::OpusEncoderProperties(IUnknown* pUnk) :
    FilterPropertiesBase(NAME("Opus Properties"), pUnk, IDD_OPUSPROP_DIALOG, IDD_OPUSPROP_CAPTION)
  {
    ;
  }

  BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    // Let the parent class handle the message.
    return FilterPropertiesBase::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
  }

  HRESULT ReadSettings()
  {
    initialiseControls();

    HRESULT hr = setEditTextFromIntFilterParameter(CODEC_PARAM_TARGET_BITRATE_KBPS, IDC_EDIT_BITRATE_LIMIT);
    if (FAILED(hr)) return hr;

    // opus application type
    int nLength = 0;
    char szBuffer[BUFFER_SIZE];
    hr = m_pSettingsInterface->GetParameter(CODEC_PARAM_OPUS_APPLICATION, sizeof(szBuffer), szBuffer, &nLength);
    if (SUCCEEDED(hr))
    {
      int nH264Type = atoi(szBuffer);
      int nRadioID = RADIO_BUTTON_IDS[nH264Type];
      long lResult = SendMessage(				// returns LRESULT in lResult
        GetDlgItem(m_Dlg, nRadioID),		// handle to destination control
        (UINT)BM_SETCHECK,					// message ID
        (WPARAM)1,							// = 0; not used, must be zero
        (LPARAM)0							// = (LPARAM) MAKELONG ((short) nUpper, (short) nLower)
      );
      return S_OK;
    }
    else
    {
      return E_FAIL;
    }

    return hr;
  }

  HRESULT OnApplyChanges(void)
  {
    // mode of operation
    HRESULT hr = setIntFilterParameterFromEditText(CODEC_PARAM_TARGET_BITRATE_KBPS, IDC_EDIT_BITRATE_LIMIT);
    if (FAILED(hr)) return hr;

    for (int i = 0; i <= 2; ++i)
    {
      int nRadioID = RADIO_BUTTON_IDS[i];
      int iCheck = SendMessage(GetDlgItem(m_Dlg, nRadioID), (UINT)BM_GETCHECK, 0, 0);
      if (iCheck != 0)
      {
        std::string sID = artist::toString(i);
        HRESULT hr = m_pSettingsInterface->SetParameter(CODEC_PARAM_OPUS_APPLICATION, sID.c_str());
        break;
      }
    }
    return hr;
  }

private:

  void initialiseControls()
  {
    InitCommonControls();

    // set spin box ranges
    // Frame bit limit
    setSpinBoxRange(IDC_SPIN1, 0, UINT_MAX);
  }
};

