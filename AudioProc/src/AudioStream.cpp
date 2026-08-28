#include "AudioStream.hpp"

#ifdef _WIN64

#include "windows/AudioStream.hpp"

AudioStream_t* AllocAudioStream(Status_t* status) {
    WindowsAudioStream_t* stream = new WindowsAudioStream_t();
    *status = WindowsAudioStream_t::LoopbackCapture(stream);
    if(*status != STATUS_OK) {
        return nullptr;
    }
    return dynamic_cast<AudioStream_t*>(stream);
}

#endif