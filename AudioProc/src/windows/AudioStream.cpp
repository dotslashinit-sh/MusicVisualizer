#include "AudioStream.hpp"

#include <Audioclient.h>
#include <audiopolicy.h>
#include <print>


#define RETURN_ERROR(x, code, msg) \
    if(FAILED(x)) \
        return GenError(code, msg);

#ifndef MAX_WAIT_LIMIT
#define MAX_WAIT_LIMIT 1000
#endif

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient3 = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

Status_t WindowsAudioStream_t::LoopbackCapture(WindowsAudioStream_t* stream) {
    IMMDeviceEnumerator * _pEnumerator;
    WAVEFORMATEX* pWf, *pWfClose, wfRequired;

    // RETURN_ERROR(CoInitialize(NULL),
    //     ERROR_CODE_UNKNOWN,
    //     "An unknown error occurred when running CoInitialize()!"
    // );
    DWORD s = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (s != S_OK && s != S_FALSE) {
        return GenError(ERROR_CODE_UNKNOWN, std::format("Unknown error occurred when setting as STA! Result: {}", s));
    }
    
    RETURN_ERROR(CoCreateInstance(CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)&_pEnumerator
    ),
        ERROR_CODE_INSTANCE_INIT_FAIL,
        "Failed to create device enumrator instance!"
    );

    RETURN_ERROR(
        _pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &stream->m_pDevice),
        ERROR_CODE_DEVICE_FETCH_FAIL,
        "Failed to get default loopback audio device!"
    );

    _pEnumerator->Release();

    RETURN_ERROR(
        stream->m_pDevice->Activate(IID_IAudioClient3, CLSCTX_ALL, NULL, (void**)&stream->m_pAudioClient),
        ERROR_CODE_DEVICE_INIT_FAIL,
        "Failed to activate audio device!"
    );

    RETURN_ERROR(stream->m_pAudioClient->GetMixFormat(&pWf),
        ERROR_CODE_DATA_FETCH_FAIL,
        "Failed to get the audio format!"
    );

    auto pbWf = reinterpret_cast<uint8_t*>(pWf);
    std::copy(pbWf, pbWf + sizeof(WAVEFORMATEX), reinterpret_cast<uint8_t*>(&wfRequired));
    wfRequired.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfRequired.cbSize = 0;
    s = stream->m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfRequired, &pWfClose);
    auto pbWfClose = reinterpret_cast<uint8_t*>(pWfClose);
    if (s == AUDCLNT_E_UNSUPPORTED_FORMAT) {
        return GenError(ERROR_CODE_UNKNOWN,
            std::format("Audio client does not support IEEE float format! 0x{:x}", pWfClose->wFormatTag));
    } else if (s == S_FALSE) {
        if (pWfClose->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            auto* pWfEx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pWfClose);
            if (!IsEqualGUID(pWfEx->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
                return GenError(ERROR_CODE_UNKNOWN,
                    std::format("Audio client does not support IEEE float format! 0x{:x}", pWfClose->wFormatTag));
            }
            std::copy(pbWfClose, pbWfClose + sizeof(WAVEFORMATEX), pbWf);
        }
    }

    stream->m_hEventBufferReady = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (stream->m_hEventBufferReady == NULL) {
        return GenError(ERROR_CODE_UNKNOWN, "Failed to create audio buffer ready event handle!");
    }

    RETURN_ERROR(stream->m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, (REFERENCE_TIME)REF_TIME_PER_SEC, 0, pWf, NULL),
        ERROR_CODE_INSTANCE_INIT_FAIL,
        "Failed to initialize audio client!"
    );

    stream->m_pAudioClient->SetEventHandle(stream->m_hEventBufferReady);

    RETURN_ERROR(stream->m_pAudioClient->GetBufferSize(&stream->m_config.sampleCount),
        ERROR_CODE_DATA_FETCH_FAIL,
        "Failed to fetch audio data!"
    );

    // Populate stream format data
    stream->m_config.sampleRate = pWf->nSamplesPerSec;
    stream->m_config.channels = pWf->nChannels;
    stream->m_config.bitRate = pWf->nAvgBytesPerSec * 8;
    stream->m_pktStride = sizeof(Sample_t) * stream->m_config.channels;
    CoTaskMemFree(pWf);

    RETURN_ERROR(stream->m_pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&stream->m_pCaptureClient),
        ERROR_CODE_INSTANCE_INIT_FAIL,
        "Failed to get audio capture client!"
    );

    RETURN_ERROR(stream->m_pAudioClient->Start(),
        ERROR_CODE_AUDIO_CAPTURE_START_FAIL,
        "Failed to start capturing loopback audio!"
    );

    return STATUS_OK;
}

Status_t WindowsAudioStream_t::UpdateAudioBuffer(AudioBuffer_t* buffer) {
    UINT32 pktCount;
    UINT32 totalRead;
    BYTE *pBuffer;
    BOOL bDone = FALSE;
    DWORD dwFlagsOrRes; // Works as the result for WaitSingleObject and flag for GetBuffer
    size_t currentSamplePos = 0, pktsRead = 0, samplesRead = 0;
    double sample = 0;
    const size_t& samplesPerFrame = m_config.channels;

    if (buffer == nullptr) {
        return GenError(
            ERROR_CODE_INVALID_ARG,
            "Attempted to write stream buffer to nullptr!"
        );
    }

    const size_t dstBufSize = buffer->GetSize();

    while(currentSamplePos < dstBufSize) {
        dwFlagsOrRes = WaitForSingleObject(m_hEventBufferReady, MAX_WAIT_LIMIT);
        if (dwFlagsOrRes == WAIT_TIMEOUT) {
            buffer->FillBuffer(currentSamplePos, dstBufSize - currentSamplePos, 0.0f);
            break;
        }
        else if (dwFlagsOrRes != WAIT_OBJECT_0) {
            return GenError(ERROR_CODE_UNKNOWN, "An unknown error occurred when waiting for buffer ready event!");
        }

        RETURN_ERROR(m_pCaptureClient->GetBuffer(&pBuffer, &pktCount, &dwFlagsOrRes, NULL, NULL),
            ERROR_CODE_DATA_FETCH_FAIL,
            "Failed to fetch data from buffer!"
        );

        if (dwFlagsOrRes & AUDCLNT_BUFFERFLAGS_SILENT) {
            buffer->FillBuffer(0, dstBufSize, 0.0f);
        }
        else if(pktCount > 0) {
            while (pktsRead < pktCount) {
                samplesRead = 0;
                sample = 0.0;
                while (samplesRead < samplesPerFrame) {
                    sample += static_cast<double>(reinterpret_cast<Sample_t*>(pBuffer)[samplesRead++]);
                }
                sample /= samplesRead;
                buffer->WriteSample(static_cast<float>(sample), currentSamplePos + pktsRead);
                pktsRead++;
                pBuffer += m_pktStride;
            }
            pktsRead = 0;
        }

        RETURN_ERROR(m_pCaptureClient->ReleaseBuffer(pktCount),
            ERROR_CODE_UNKNOWN,
            "An unknown error occurred when trying to free audio buffer"
        );
        currentSamplePos += pktCount;
    }
    return STATUS_OK;
}

const size_t WindowsAudioStream_t::GetSampleRate() const {
    return m_config.sampleRate;
}

void WindowsAudioStream_t::ReleaseStream() {
    if(m_pCaptureClient != nullptr) {
        m_pCaptureClient->Release();
    }
    if(m_pAudioClient != nullptr) {
        m_pAudioClient->Stop();
        m_pAudioClient->Release();
    }
    if(m_pDevice != NULL) {
        m_pDevice->Release();
    }
}

WindowsAudioStream_t::~WindowsAudioStream_t() {
    this->ReleaseStream();
    CloseHandle(m_hEventBufferReady);
}