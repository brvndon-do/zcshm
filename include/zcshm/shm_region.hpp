#pragma once

#include <cstddef>
#include <string>
#include <sys/mman.h>

namespace zcshm {
    class ShmRegion {
    private:
        std::byte* data_;
        std::string name_;
        std::size_t size_;
        bool owned_;

        ShmRegion(std::byte* data, std::string name, std::size_t size, bool owned);

        void reset() {
            if (data_)
                munmap(data_, size_);

            if (owned_)
                shm_unlink(name_.c_str());

            data_ = nullptr;
            size_ = 0;
            owned_ = false;
        }

    public:
        static ShmRegion create(const std::string& name, std::size_t size);
        static ShmRegion attach(const std::string& name);

        ~ShmRegion();

        // move
        ShmRegion(ShmRegion&& other) noexcept;
        ShmRegion& operator=(ShmRegion&& other) noexcept;

        // copy (do not generate)
        ShmRegion(const ShmRegion&)=delete;
        ShmRegion& operator=(const ShmRegion&)=delete;

        std::byte* data() noexcept;
        const std::byte* data() const noexcept;
        const std::string& name() const noexcept;
        std::size_t size() const noexcept;
        bool owned() const noexcept;
    };
}
