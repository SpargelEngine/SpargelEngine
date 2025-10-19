#include "spargel/resource/directory.h"

#include "spargel/base/const.h"
#include "spargel/base/platform.h"
#include "spargel/logging/logging.h"
#include "spargel/util/path.h"

#if SPARGEL_USE_FILE_MMAP

#if SPARGEL_IS_POSIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#elif SPARGEL_IS_WINDOWS
#include <windows.h>
#endif

#endif  // SPARGEL_USE_FILE_MMAP

#if SPARGEL_IS_POSIX
#include <unistd.h>
#endif

// libc
#include <stdio.h>
#include <string.h>

namespace spargel::resource {

    class ResourceDirectory : public Resource {
        friend class ResourceManagerDirectory;

    public:
        ~ResourceDirectory() override;

        usize size() override { return _size; }

        void getData(void* buf) override;

        void* mapData() override;

        usize _size;
#if SPARGEL_USE_FILE_MMAP
        void* _mapped;

#if SPARGEL_IS_POSIX
        int _fd;
        ResourceDirectory(usize size, int fd)
            : _size(size), _mapped(nullptr), _fd(fd) {}
#elif SPARGEL_IS_WINDOWS
        HANDLE _file_handle;
        HANDLE _mapping_handle;
        ResourceDirectory(usize size, HANDLE file_handle)
            : _size(size),
              _mapped(nullptr),
              _file_handle(file_handle),
              _mapping_handle(nullptr) {}
#else
#error unimplemented
#endif

        void* _map();
        void _unmap(void* ptr, usize size);
#else
        FILE* _fp;
        ResourceDirectory(usize size, FILE* fp) : _size(size), _fp(fp) {}
#endif
    };

#if SPARGEL_USE_FILE_MMAP

    ResourceDirectory::~ResourceDirectory() {
        if (_mapped) {
            _unmap(_mapped, _size);
        }
#if SPARGEL_IS_POSIX
        ::close(_fd);
#elif SPARGEL_IS_WINDOWS
        if (_mapping_handle) {
            CloseHandle(_mapping_handle);
        }
        CloseHandle(_file_handle);
#else
#error unimplemented
#endif  // SPARGEL_IS_POSIX
    }

    void ResourceDirectory::getData(void* buf) {
        if (!_mapped) {
            _mapped = _map();
        }
        memcpy(buf, _mapped, _size);
    }

    void* ResourceDirectory::mapData() {
        if (!_mapped) {
            _mapped = _map();
        }
        return _mapped;
    }

#if SPARGEL_IS_POSIX

    void* ResourceDirectory::_map() {
        return mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, _fd, 0);
    }

    void ResourceDirectory::_unmap(void* ptr, usize size) { munmap(ptr, size); }

    base::Optional<base::unique_ptr<Resource>> ResourceManagerDirectory::open(
        const ResourceId& id) {
        auto real_path = base::CString(_real_path(id));
        int fd = ::open(real_path.data(), O_RDONLY);
        if (fd < 0) {
            // One should log in the caller.
            //
            // spargel_log_error("requested resource \"%s\" not found (%s)",
            //                   base::CString(id.path()).data(),
            //                   real_path.data());
            return base::nullopt;
        }
        struct stat sb;
        if (fstat(fd, &sb) < 0) {
            ::close(fd);
            return base::nullopt;
        }

        return base::makeOptional<base::unique_ptr<ResourceDirectory>>(
            base::make_unique<ResourceDirectory>(static_cast<usize>(sb.st_size),
                                                 fd));
    }

#elif SPARGEL_IS_WINDOWS

    void* ResourceDirectory::_map() {
        if (!_mapping_handle) {
            if (_file_handle == INVALID_HANDLE_VALUE) {
                spargel_log_error("invalid file handle");
                return nullptr;
            }
            _mapping_handle = CreateFileMappingA(
                _file_handle, nullptr, PAGE_READONLY,
                (unsigned long long)_size >> 32, _size & 0xffffffff, nullptr);
            if (!_mapping_handle) {
                spargel_log_error("cannot create file mapping");
                return nullptr;
            }
        }
        return MapViewOfFile(_mapping_handle, FILE_MAP_READ, 0, 0, _size);
    }

    void ResourceDirectory::_unmap(void* ptr, usize size) {
        UnmapViewOfFile(ptr);
    }

    base::Optional<base::unique_ptr<Resource>> ResourceManagerDirectory::open(
        const ResourceId& id) {
        auto real_path = base::CString(_real_path(id));
        HANDLE file_handle =
            CreateFileA(real_path.data(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_READONLY, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) {
            spargel_log_error("requested resource \"%s:%s\" not found (%s)",
                              base::CString(id.ns()).data(),
                              base::CString(id.path()).data(),
                              real_path.data());
            return base::nullopt;
        }

        LARGE_INTEGER size;
        if (!GetFileSizeEx(file_handle, &size)) {
            spargel_log_error("cannot get the size of resource \"%s:%s\" (%s)",
                              base::CString(id.ns()).data(),
                              base::CString(id.path()).data(),
                              real_path.data());
            CloseHandle(file_handle);
            return base::nullopt;
        }

        return base::makeOptional<base::unique_ptr<ResourceDirectory>>(
            base::make_unique<ResourceDirectory>(size.QuadPart, file_handle));
    }

#else
#error unimplemented
#endif  // SPARGEL_IS_POSIX

#else   // SPARGEL_USE_FILE_MMAP

    ResourceDirectory::~ResourceDirectory() { fclose(_fp); }

    void ResourceDirectory::getData(void* buf) {
        fseek(_fp, 0, SEEK_SET);
        fread(buf, _size, 1, _fp);
    }

    void* ResourceDirectory::mapData() { return Resource::mapData(); }

    base::Optional<base::unique_ptr<Resource>> ResourceManagerDirectory::open(
        const ResourceId& id) {
        auto real_path = base::CString(_real_path(id));
        FILE* fp = fopen(real_path.data(), "rb");
        if (!fp) {
            spargel_log_error("cannot open file for resource \"%s:%s\" (%s)",
                              base::CString(id.ns()).data(),
                              base::CString(id.path()).data(),
                              real_path.data());
            return base::nullopt;
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        return base::makeOptional<base::unique_ptr<ResourceDirectory>>(
            base::make_unique<ResourceDirectory>(size, fp));
    }
#endif  // SPARGEL_USE_FILE_MMAP

#if SPARGEL_IS_POSIX
    bool ResourceManagerDirectory::has(const ResourceId& id) {
        auto real_path = base::CString(_real_path(id));
        return access(real_path.data(), F_OK) == 0;
    }
#else
    bool ResourceManagerDirectory::has(const ResourceId& id) {
        auto real_path = base::CString(_real_path(id));
        FILE* fp = fopen(real_path.data(), "rb");
        if (fp) {
            fclose(fp);
            return true;
        } else {
            return false;
        }
    }
#endif

    base::String ResourceManagerDirectory::_real_path(const ResourceId& id) {
        if (_root_path.length() == 0) {
            return base::String(id.path());
        } else {
            return _root_path + PATH_SPLIT + id.path();
        }
    }

    base::unique_ptr<ResourceManagerDirectory> makeRelativeManager(
        const base::String& resources_dir) {
        base::String root_path =
            util::dirname(base::get_executable_path().view());
        if (resources_dir.length() > 0)
            root_path = root_path + PATH_SPLIT + base::String(resources_dir);
        return base::make_unique<ResourceManagerDirectory>(root_path.view());
    }

}  // namespace spargel::resource
