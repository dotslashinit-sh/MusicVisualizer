#include "AudioStream.hpp"

#include <Audioclient.h>
#include <audiopolicy.h>

#define RETURN_ERROR(x, code, msg) \
    if(FAILED(x)) \
        return GenError(code, msg);

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

Status_t WindowsAudioStream_t::LoopbackCapture(WindowsAudioStream_t* stream) {
    IMMDeviceEnumerator * _pEnumerator;
    WAVEFORMATEX* pWf;

    RETURN_ERROR(CoInitialize(NULL),
        ERROR_CODE_UNKNOWN,
        "An unknown error occurred when running CoInitialize()!"
    );
    
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
        stream->m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&stream->m_pAudioClient),
        ERROR_CODE_DEVICE_INIT_FAIL,
        "Failed to activate audio device!"
    );

    RETURN_ERROR(stream->m_pAudioClient->GetMixFormat(&pWf),
        ERROR_CODE_DATA_FETCH_FAIL,
        "Failed to get the audio format!"
    );

    RETURN_ERROR(stream->m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, (REFERENCE_TIME)REF_TIME_PER_SEC, 0, pWf, NULL),
        ERROR_CODE_INSTANCE_INIT_FAIL,
        "Failed to initialize audio client!"
    );

    RETURN_ERROR(stream->m_pAudioClient->GetBufferSize(&stream->m_config.sampleCount),
        ERROR_CODE_DATA_FETCH_FAIL,
        "Failed to fetch audio data!"
    );

    // Populate stream format data
    stream->m_config.sampleRate = pWf->nSamplesPerSec;
    stream->m_config.channels = pWf->nChannels;
    stream->m_config.bitRate = pWf->nAvgBytesPerSec * 8;

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
    UINT32 pktSz;
    UINT32 totalRead;
    BYTE *pBuffer;
    BOOL bDone = FALSE;
    DWORD dwFlags;
    size_t i = 0;
    const size_t dstBufSize = buffer->GetSize();

    while(i < dstBufSize) {
        if(buffer == nullptr) {
            return GenError(
                ERROR_CODE_INVALID_ARG,
                "Attempted to write stream buffer to nullptr!"
            );
        }
    
        RETURN_ERROR(m_pCaptureClient->GetBuffer(&pBuffer, &pktSz, &dwFlags, NULL, NULL),
            ERROR_CODE_DATA_FETCH_FAIL,
            "Failed to fetch data from buffer!"
        );

        if(pktSz > 0) {
            buffer->WriteBuffer(pBuffer, pktSz, i);
            i += pktSz;
        }

        RETURN_ERROR(m_pCaptureClient->ReleaseBuffer(pktSz),
            ERROR_CODE_UNKNOWN,
            "An unknown error occurred when trying to free audio buffer"
        );
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
}