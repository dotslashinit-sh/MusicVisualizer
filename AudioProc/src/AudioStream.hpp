#pragma once

#include "AudioBuffer.hpp"
#include "Error.hpp"

struct AudioStreamConfig_t {
    uint16_t sampleRate;
    size_t bitRate;
    uint8_t channels;
    uint32_t sampleCount;
};

class AudioStream_t {
public:
    AudioStream_t() = default;
    virtual ~AudioStream_t() = default;

    /// @brief Updates the contents of `*buffer` with the current audio samples.
    /// @param buffer Pointer to an `AudioBuffer_t` object.
    /// @return Returns `STATUS_OK` if successful.
    virtual Status_t UpdateAudioBuffer(AudioBuffer_t* buffer) = 0;

    virtual const size_t GetSampleRate() const = 0;

protected:
    AudioStreamConfig_t m_config;
};

/// @brief Allocates a new audio stream object based on the operating system. For example, on
///  windows, this function returns a pointer to a newly created `WindowsAudioStream_t` object.
///  Allocated objects must be manually deleted using `delete`.
/// @param status Pointer to a `Status_t` object to store the details about success/failure of creation.
/// @return Returns a pointer to the newly created audio stream.
AudioStream_t* AllocAudioStream(Status_t* status);