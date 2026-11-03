#pragma once

#include <vector>
#include <algorithm>
#include <exception>
#include <string>
#include <format>

typedef float Sample_t;

class AudioBuffer_t {
    friend class Visualizer;
public:
    AudioBuffer_t() = delete;
    AudioBuffer_t(size_t size) : m_data(size) {}

    inline void ReadSamples(size_t start, size_t length, Sample_t* dest) {
        auto s = m_data.begin() + start;
        auto e = length;
        if(start >= m_data.size()) {
            std::fill(dest, dest+length, 0.0f);
        } else if(start+length > m_data.size()) {
            e = m_data.size() - start;
            std::fill(dest + e, dest + length, 0.0f);
        }
        copy(s, s + e, dest);
    }

    /// @brief Writes the contents of raw bytes `src` into the samples.
    /// NOTE: If size is not in boundary of `sizeof(Sample_t)`, the size is converted to
    /// the next smallest boundary of size and contents of buffer till there is considered.
    /// If the buffer overflows because the source is too large or the start + length is
    /// past the end, then the last bytes are discarded.
    /// @param src Source buffer 
    /// @param size Size of the buffer
    /// @param start Starting position of the sample to be written
    inline void WriteBuffer(const uint8_t* src, size_t size, size_t start = 0) {
        const Sample_t* src_s = reinterpret_cast<const Sample_t*>(src);
        const size_t src_sz = size / sizeof(Sample_t);
        this->WriteSamples(src_s, src_sz, start);
    }

    inline void WriteSamples(const Sample_t* src, size_t length, size_t start = 0) {
        const size_t bfSz = m_data.size();
        const size_t end = (start + length > bfSz) ? (bfSz - start) : length;
        if(start > bfSz) {
            return;
        }
        std::copy(src, src + end, m_data.begin() + start);
    }

    inline void WriteSample(const Sample_t value, size_t pos) {
        if(pos < m_data.size())
            m_data[pos] = value;
    }

    inline void FillBuffer(const size_t start, const size_t len, const Sample_t value) {
        auto s = m_data.begin() + start;
        std::fill(s, s + len, value);
    }

    inline size_t GetSize() const noexcept {
        return m_data.size();
    }

    AudioBuffer_t(const AudioBuffer_t&) = delete;
    AudioBuffer_t(AudioBuffer_t&& ab) : m_data(std::move(ab.m_data)){}

    AudioBuffer_t& operator=(const AudioBuffer_t&) = delete;
    AudioBuffer_t& operator=(AudioBuffer_t&& ab) {
        m_data = std::move(ab.m_data);
        return *this;
    }

    Sample_t& operator[](size_t i) {
        return m_data[i];
    }

private:
    std::vector<Sample_t> m_data;
};