#ifndef ASSETWRITER_H
#define ASSETWRITER_H

#include "Chunk.h"
#include "IO.h"
#include <fstream>
#include <string>

namespace rw {

    class AssetWriter {
    public:
        explicit AssetWriter(const std::string& path);
        ~AssetWriter();

        // begin a chunk: writes a header with size=0 and returns file position of the header start
        std::streampos begin_chunk(uint32_t type, uint32_t version = 1);

        // end chunk: calculate payload size and patch header
        void end_chunk(std::streampos startPos);

        // convenient write helpers
        void write_string(const std::string& s);
        void write_bytes(const void* data, size_t size);

        template<typename T> void write_pod(const T& v) { io::write_pod(fs, v); }

        bool good() const { return fs.good(); }

    private:
        std::fstream fs;
    };

} // namespace rw

#endif //ASSETWRITER_H
