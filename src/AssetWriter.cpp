#include "AssetWriter.h"
#include <iostream>
#include <stdexcept>
#include <cassert>

using namespace rw;

AssetWriter::AssetWriter(const std::string& path)
    : fs(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc)
{
    if (!fs) throw std::runtime_error("Failed to open file for writing");
}

AssetWriter::~AssetWriter() {
    // destructor will close fstream automatically
}

std::streampos AssetWriter::begin_chunk(uint32_t type, uint32_t version) {
    chunk::ChunkHeader hdr{ type, 0u, version };
    std::streampos pos = fs.tellp();
    io::write_pod(fs, hdr);
    return pos;
}

void AssetWriter::end_chunk(std::streampos startPos) {
    std::streampos endPos = fs.tellp();
    if (!(endPos > startPos)) {
        std::cerr << "AssetWriter::end_chunk: end <= start\n";
        return;
    }
    std::streamoff payloadSize = endPos - startPos - static_cast<std::streamoff>(sizeof(chunk::ChunkHeader));
    // read the header we originally wrote (type/version) then patch size
    chunk::ChunkHeader hdr{};
    // read header using seekg/read
    fs.seekg(startPos);
    if (!io::read_pod_checked(fs, hdr)) {
        throw std::runtime_error("Failed to read header for patching");
    }
    hdr.size = static_cast<uint32_t>(payloadSize);
    // write patched header back
    fs.seekp(startPos);
    io::write_pod(fs, hdr);
    // restore write position
    fs.seekp(endPos);
}

void AssetWriter::write_string(const std::string& s) { io::write_string(fs, s); }
void AssetWriter::write_bytes(const void* data, size_t size) { io::write_bytes(fs, data, size); }
