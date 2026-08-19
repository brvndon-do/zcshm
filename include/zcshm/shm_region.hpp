#pragma once

#include <cstddef>
#include <string>

namespace zcshm {
    class ShmRegion {
    private:
        std::byte* data_;
        std::string name_;
        std::size_t size_;
        bool owned_;

        ShmRegion(std::byte* data, std::string name, std::size_t size, bool owned);

    public:
        static ShmRegion create(const std::string& name, std::size_t size);
        static ShmRegion attach(const std::string& name);

        ~ShmRegion();

        // move
        ShmRegion(ShmRegion&&) noexcept;
        ShmRegion& operator=(ShmRegion&&) noexcept;

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
