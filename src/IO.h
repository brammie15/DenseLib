#ifndef IO_H
#define IO_H

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rw {
    namespace io {
        // write raw bytes
        void write_bytes(std::ostream& os, const void* data, std::size_t size);

        // write POD (trivially copyable)
        template<typename T>
        void write_pod(std::ostream& os, const T& v) {
            static_assert(std::is_trivially_copyable<T>::value, "write_pod requires trivially copyable type");
            write_bytes(os, &v, sizeof(T));
        }

        // write length-prefixed string (uint32 length, then bytes)
        void write_string(std::ostream& os, const std::string& s);

        // read raw bytes
        void read_bytes(std::istream& is, void* out, std::size_t size);

        // read POD
        template<typename T>
        void read_pod(std::istream& is, T& out) {
            static_assert(std::is_trivially_copyable<T>::value, "read_pod requires trivially copyable type");
            read_bytes(is, &out, sizeof(T));
        }

        // read length-prefixed string
        std::string read_string(std::istream& is);

        // read with a check; returns true if the full sizeof(T) was read
        template<typename T>
        bool read_pod_checked(std::istream& is, T& out) {
            static_assert(std::is_trivially_copyable<T>::value, "read_pod_checked requires trivially copyable type");
            is.read(reinterpret_cast<char*>(&out), static_cast<std::streamsize>(sizeof(T)));
            return static_cast<bool>(is) && is.gcount() == static_cast<std::streamsize>(sizeof(T));
        }
    } // namespace io
} // namespace rw

#endif //IO_H
