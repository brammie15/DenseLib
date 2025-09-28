#include "AssetReader.h"
#include <stdexcept>

using namespace rw;

AssetReader::AssetReader(const std::string& path)
    : fs(path, std::ios::binary | std::ios::in)
{
    if (!fs) throw std::runtime_error("Failed to open file for reading");
}

AssetReader::~AssetReader() = default;

bool AssetReader::next_chunk(chunk::ChunkHeader& outHdr, std::streampos& payloadPos) {
    if (!fs.good() || fs.eof()) return false;
    std::streampos pos = fs.tellg();
    if (!io::read_pod_checked(fs, outHdr)) return false;
    payloadPos = fs.tellg();
    return true;
}

void AssetReader::skip_payload(const chunk::ChunkHeader& hdr) {
    fs.seekg(static_cast<std::streamoff>(hdr.size), std::ios::cur);
}

std::string AssetReader::read_string() { return io::read_string(fs); }
void AssetReader::read_bytes(void* out, size_t size) { io::read_bytes(fs, out, size); }
std::streampos AssetReader::tell() { return fs.tellg(); }
void AssetReader::seek(std::streampos p) { fs.seekg(p); }
bool AssetReader::good() const { return fs.good(); }
