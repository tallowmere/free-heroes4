#include "h4/io/mapped_file.hpp"

#include <format>
#include <fstream>
#include <utility>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define H4_HAVE_MMAP 1
#endif

namespace h4::io {

namespace {

#if defined(H4_HAVE_MMAP)
class FileDescriptor {
public:
    explicit FileDescriptor(int fd) noexcept : fd_{fd} {
    }
    FileDescriptor(const FileDescriptor&)            = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    ~FileDescriptor() {
        if (fd_ >= 0)
            ::close(fd_);
    }
    [[nodiscard]] int get() const noexcept {
        return fd_;
    }

private:
    int fd_;
};
#endif

}

core::Result<MappedFile> MappedFile::open(const std::filesystem::path& path) {
    MappedFile file;
#if defined(H4_HAVE_MMAP)
    const FileDescriptor fd{::open(path.c_str(), O_RDONLY)};
    if (fd.get() < 0)
        return core::fail(core::ErrorCode::io, std::format("cannot open {}", path.string()));
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0)
        return core::fail(core::ErrorCode::io, std::format("cannot stat {}", path.string()));
    file.size_ = static_cast<std::size_t>(st.st_size);
    if (file.size_ == 0)
        return file;
    void* p = ::mmap(nullptr, file.size_, PROT_READ, MAP_PRIVATE, fd.get(), 0);
    if (p == MAP_FAILED)
        return core::fail(core::ErrorCode::io, std::format("cannot map {}", path.string()));
    file.data_   = static_cast<const std::byte*>(p);
    file.mapped_ = true;
    return file;
#else
    std::ifstream in{path, std::ios::binary | std::ios::ate};
    if (!in)
        return core::fail(core::ErrorCode::io, std::format("cannot open {}", path.string()));
    file.buffer_.resize(static_cast<std::size_t>(in.tellg()));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(file.buffer_.data()), static_cast<std::streamsize>(file.buffer_.size()));
    if (!in)
        return core::fail(core::ErrorCode::io, std::format("cannot read {}", path.string()));
    file.data_ = file.buffer_.data();
    file.size_ = file.buffer_.size();
    return file;
#endif
}

MappedFile::MappedFile(MappedFile&& other) noexcept
: data_{std::exchange(other.data_, nullptr)}, size_{std::exchange(other.size_, 0)},
  mapped_{std::exchange(other.mapped_, false)}, buffer_{std::move(other.buffer_)} {
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        release();
        data_   = std::exchange(other.data_, nullptr);
        size_   = std::exchange(other.size_, 0);
        mapped_ = std::exchange(other.mapped_, false);
        buffer_ = std::move(other.buffer_);
    }
    return *this;
}

MappedFile::~MappedFile() {
    release();
}

void MappedFile::release() noexcept {
#if defined(H4_HAVE_MMAP)
    if (mapped_ && data_ != nullptr)
        ::munmap(const_cast<void*>(static_cast<const void*>(data_)), size_);
#endif
    data_   = nullptr;
    size_   = 0;
    mapped_ = false;
    buffer_.clear();
}

}
