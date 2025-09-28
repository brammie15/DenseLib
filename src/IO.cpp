#include "IO.h"

namespace rw::io {
    void write_bytes(std::ostream& os, const void* data, std::size_t size) {
        os.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    void write_string(std::ostream& os, const std::string& s) {
        const auto len = static_cast<uint32_t>(s.size());
        write_pod(os, len);
        if (len) write_bytes(os, s.data(), len);
    }

    void read_bytes(std::istream& is, void* out, std::size_t size) {
        is.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(size));
    }

    std::string read_string(std::istream& is) {
        uint32_t len = 0;
        read_pod(is, len);
        std::string s;
        if (len) {
            s.resize(len);
            read_bytes(is, &s[0], len);
        }
        return s;
    }
}
