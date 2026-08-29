#include <cerrno>
#include <chrono>
#include <cstddef>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>

#include "zcshm/shm_region.hpp"

namespace zcshm {

namespace {
    void throwIfFailed(int rc, int* fd, const std::string* name, const std::string& msg) {
        if (rc == -1) {
            int err = errno;
            if (fd)
                close(*fd);

            if (name)
                shm_unlink(name->c_str());

            throw std::system_error(err, std::system_category(), msg);
        }
    }

    void throwIfFailed(int* fd, const std::string* name, const std::string& msg) {
        throwIfFailed(-1, fd, name, msg);
    }
}

    ShmRegion::ShmRegion(std::byte* data, std::string name, std::size_t size, bool owned)
        : data_(data), name_(std::move(name)), size_(size), owned_(owned) {}

    ShmRegion ShmRegion::create(const std::string& name, std::size_t size) {
        int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        throwIfFailed(fd, nullptr, nullptr, "shmopen");

        int rc = ftruncate(fd, size);
        throwIfFailed(rc, &fd, &name, "ftruncate");

        auto* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED)
            throwIfFailed(&fd, &name, "mmap");

        close(fd);

        return ShmRegion{static_cast<std::byte*>(ptr), name, size, true};
    }

    ShmRegion ShmRegion::attach(const std::string& name) {
        int fd = shm_open(name.c_str(), O_RDWR, 0); // mode is ignored entirely unless O_CREAT is in the flags; 0 is ok here
        throwIfFailed(fd, nullptr, nullptr, "shmopen");

        struct stat out;
        int rc = fstat(fd, &out);
        throwIfFailed(rc, &fd, nullptr, "fstat");

        std::size_t size = out.st_size;
        int retryCount = 0;
        while (size == 0 && retryCount < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            rc = fstat(fd, &out);
            throwIfFailed(rc, &fd, nullptr, "fstat");
            size = out.st_size;

            ++retryCount;
        }
        // TODO: gracefully error out once retry is done.
        if (size == 0) {
            // cleanup and throw?
        }

        auto* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED)
            throwIfFailed(&fd, &name, "mmap");

        close(fd);

        return ShmRegion{static_cast<std::byte*>(ptr), name, size, false};
    }

    ShmRegion::~ShmRegion() {
        reset();
    }

    ShmRegion::ShmRegion(ShmRegion&& other) noexcept
        : data_(other.data_), name_(std::move(other.name_)), size_(other.size_), owned_(other.owned_) {
            other.data_ = nullptr;
            other.owned_ = false;
        }

    ShmRegion& ShmRegion::operator=(ShmRegion&& other) noexcept {
        if (this != &other) {
            reset();

            data_ = other.data_;
            name_ = std::move(other.name_);
            size_ = other.size_;
            owned_ = other.owned_;

            other.data_ = nullptr;
            other.owned_ = false;
        }

        return *this;
    }

    std::byte* ShmRegion::data() noexcept {
        return data_;
    }

    const std::byte* ShmRegion::data() const noexcept {
        return data_;
    }

    const std::string& ShmRegion::name() const noexcept {
        return name_;
    }

    std::size_t ShmRegion::size() const noexcept {
        return size_;
    }

    bool ShmRegion::owned() const noexcept {
        return owned_;
    }
}
