#ifndef ASSETREADER_H
#define ASSETREADER_H

#include <fstream>
#include "Chunk.h"
#include "IO.h"

namespace rw {

    class AssetReader {
    public:
        explicit AssetReader(const std::string& path);
        ~AssetReader();

        // read next chunk header; returns false at EOF
        // payloadPos will be set to the file position right after the header (start of payload)
        bool next_chunk(chunk::ChunkHeader& outHdr, std::streampos& payloadPos);

        // skip the payload of the last-read header
        void skip_payload(const chunk::ChunkHeader& hdr);

        // helpers forwarded from IO
        std::string read_string();
        template<typename T> void read_pod(T& out) { io::read_pod(fs, out); }
        void read_bytes(void* out, size_t size);

        std::streampos tell();
        void seek(std::streampos p);
        bool good() const;

    private:
        std::fstream fs;
    };

} // namespace rw
#endif //ASSETREADER_H
