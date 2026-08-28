#pragma once

#include "../AudioStream.hpp"
#include "../Error.hpp"

#include <audioclient.h>
#include <mmdeviceapi.h>

#define REF_TIME_PER_SEC 10000000

class WindowsAudioStream_t : public AudioStream_t {
public:
    WindowsAudioStream_t() = default;
    virtual ~WindowsAudioStream_t();
    static Status_t LoopbackCapture(WindowsAudioStream_t* stream);
    virtual Status_t UpdateAudioBuffer(AudioBuffer_t* buffer);

    virtual const size_t GetSampleRate() const;

    void ReleaseStream();

private:
    IMMDevice* m_pDevice = nullptr;
    IAudioClient* m_pAudioClient = nullptr;
    IAudioCaptureClient* m_pCaptureClient = nullptr;
};