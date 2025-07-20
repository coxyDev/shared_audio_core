#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <memory>

namespace SharedAudio {

    // Lock-free single producer, single consumer FIFO
    // Essential for real-time audio to avoid priority inversion
    template<typename T, size_t Size>
    class LockFreeFIFO {
    public:
        LockFreeFIFO() : write_pos_(0), read_pos_(0) {
            static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
        }

        // Called from producer thread (UI/main thread)
        bool push(const T& item) {
            const size_t current_write = write_pos_.load(std::memory_order_relaxed);
            const size_t next_write = (current_write + 1) & (Size - 1);

            if (next_write == read_pos_.load(std::memory_order_acquire)) {
                return false; // Buffer full
            }

            buffer_[current_write] = item;
            write_pos_.store(next_write, std::memory_order_release);
            return true;
        }

        // Called from consumer thread (audio thread)
        bool pop(T& item) {
            const size_t current_read = read_pos_.load(std::memory_order_relaxed);

            if (current_read == write_pos_.load(std::memory_order_acquire)) {
                return false; // Buffer empty
            }

            item = buffer_[current_read];
            read_pos_.store((current_read + 1) & (Size - 1), std::memory_order_release);
            return true;
        }

        // Non-blocking check if data available
        bool available() const {
            return read_pos_.load(std::memory_order_relaxed) !=
                write_pos_.load(std::memory_order_acquire);
        }

        // Get number of items in queue (approximation)
        size_t size() const {
            const size_t write = write_pos_.load(std::memory_order_acquire);
            const size_t read = read_pos_.load(std::memory_order_relaxed);
            return (write - read) & (Size - 1);
        }

        void clear() {
            read_pos_.store(write_pos_.load(std::memory_order_acquire),
                std::memory_order_release);
        }

    private:
        std::array<T, Size> buffer_;
        alignas(64) std::atomic<size_t> write_pos_; // Cache line aligned
        alignas(64) std::atomic<size_t> read_pos_;  // Prevent false sharing
    };

    // Lock-free message passing for audio thread communication
    struct AudioThreadMessage {
        enum Type {
            NONE = 0,
            START_CUE,
            STOP_CUE,
            SET_VOLUME,
            SET_PAN,
            CROSSFADE,
            LOAD_BUFFER,
            SEEK
        };

        Type type = NONE;
        char cue_id[64] = { 0 };
        union {
            float float_value;
            int int_value;
            double double_value;
        } param1 = { 0 };
        union {
            float float_value;
            int int_value;
        } param2 = { 0 };
    };

    using AudioMessageQueue = LockFreeFIFO<AudioThreadMessage, 256>;

} // namespace SharedAudio