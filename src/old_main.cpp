#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

constexpr uint32_t MAKE_FOURCC(char a, char b, char c, char d) {
    return (uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24);
}

namespace ChunkType {
    constexpr uint32_t ROOT = MAKE_FOURCC('R','O','O','T'); // root container
    constexpr uint32_t MESH = MAKE_FOURCC('M','E','S','H');
    constexpr uint32_t GEOM = MAKE_FOURCC('G','E','O','M');
    constexpr uint32_t MATS = MAKE_FOURCC('M','A','T','S');
    constexpr uint32_t SKEL = MAKE_FOURCC('S','K','E','L');
    constexpr uint32_t ANIM = MAKE_FOURCC('A','N','I','M');
    constexpr uint32_t TEXD = MAKE_FOURCC('T','E','X','D'); // texture dictionary
    constexpr uint32_t TEXR = MAKE_FOURCC('T','E','X','R'); // texture
    constexpr uint32_t META = MAKE_FOURCC('M','E','T','A'); // JSON/MsgPack metadata
}

// Chunk header: 12 bytes
#pragma pack(push,1)
struct ChunkHeader {
    uint32_t type;    // FourCC
    uint32_t size;    // size of payload (not including this header)
    uint32_t version; // chunk-specific version
};
#pragma pack(pop)

static_assert(sizeof(ChunkHeader) == 12, "ChunkHeader must be 12 bytes");

void write_bytes(std::ostream& os, const void* data, std::size_t size) {
    os.write(reinterpret_cast<const char*>(data), size);
}

template<typename T>
void write_pod(std::ostream& os, const T& v) {
    write_bytes(os, &v, sizeof(T));
}

inline void write_string(std::ostream& os, const std::string& s) {
    const auto len = static_cast<uint32_t>(s.size());
    write_pod(os, len);
    if (len) write_bytes(os, s.data(), len);
}

inline void read_bytes(std::istream& is, void* out, std::size_t size) {
    is.read(reinterpret_cast<char*>(out), size);
}

template<typename T>
inline void read_pod(std::istream& is, T& out) {
    read_bytes(is, &out, sizeof(T));
}

inline std::string read_string(std::istream& is) {
    uint32_t len = 0;
    read_pod(is, len);
    std::string s;
    if (len) {
        s.resize(len);
        read_bytes(is, s.data(), len);
    }
    return s;
}

class ChunkWriter {
public:
    explicit ChunkWriter(std::fstream& out)
        : os(out) {}

    // Begin a chunk: write header with zero size for now and return position
    std::streampos begin_chunk(uint32_t type, uint32_t version = 1) {
        ChunkHeader hdr{type, 0u, version};
        std::streampos pos = os.tellp();
        write_pod(os, hdr);
        return pos;
    }

    // End chunk: calculate payload size and patch header
    void end_chunk(std::streampos startPos) {
        std::streampos endPos = os.tellp();
        assert(endPos > startPos);
        std::streamoff payloadSize = endPos - startPos - static_cast<std::streamoff>(sizeof(ChunkHeader));

        // Patch size field
        os.seekp(startPos);
        ChunkHeader hdr{};
        read_pod_from_streampos(startPos, hdr); // read existing header (type/version kept)
        hdr.size = static_cast<uint32_t>(payloadSize);
        os.seekp(startPos);
        write_pod(os, hdr);
        os.seekp(endPos);
    }

private:
    std::fstream& os;

    // utility: read header from file position (requires underlying stream supports tellg/seekg)
    void read_pod_from_streampos(std::streampos pos, ChunkHeader& out) {
        // This helper temporarily flushes and uses ifstream not available here; instead, we keep what's currently in the stream
        // We will assume the header we wrote earlier is unchanged except size. We'll read it by seeking and reading.
        std::streampos curpos = os.tellp();
        os.seekp(pos);
        // read header into buffer
        char buf[sizeof(ChunkHeader)];
        os.read(buf, sizeof(ChunkHeader));
        // Copy into out
        std::memcpy(&out, buf, sizeof(ChunkHeader));
        os.seekp(curpos);
    }
};

class AssetWriter {
public:
    explicit AssetWriter(const std::string& path)
        : fs(path,  std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc) {
        if (!fs) throw std::runtime_error("Failed to open file for writing");
    }

    std::streampos begin_chunk(uint32_t type, uint32_t version = 1) {
        ChunkHeader hdr{type, 0u, version};
        std::streampos pos = fs.tellp();
        write_pod(hdr);
        std::printf("begin_chunk: type=%c%c%c%c version=%u at %lld\n",
            (char)(type & 0xFF), (char)((type >> 8) & 0xFF), (char)((type >> 16) & 0xFF), (char)((type >> 24) & 0xFF),
            version, static_cast<long long>(pos));
        return pos;
    }

    void end_chunk(std::streampos startPos) {
        std::streampos endPos = fs.tellp();
        std::cout << "end_chunk: start=" << startPos << " end=" << endPos << "\n";
        if (!(endPos > startPos)) {
            std::cerr << "Invalid chunk positions: end <= start\n";
            // throw std::runtime_error("Invalid chunk positions");
        }
        uint32_t payloadSize = static_cast<uint32_t>(endPos - startPos - sizeof(ChunkHeader));
        std::cout << "  payload size: " << payloadSize << "\n";
        // patch
        fs.seekp(startPos);
        ChunkHeader hdr;
        read_pod(fs, hdr); // read header we wrote earlier
        hdr.size = payloadSize;
        fs.seekp(startPos);
        write_pod(hdr);
        fs.seekp(endPos);
    }

    void write_string(const std::string& s) { ::write_string(fs, s); }
    void write_bytes(const void* data, size_t size) { ::write_bytes(fs, data, size); }
    template<typename T> void write_pod(const T& v) { ::write_pod(fs, v); }

    bool good() const { return fs.good(); }

private:
    std::fstream fs;
};

class AssetReader {
public:
    explicit AssetReader(const std::string& path)
        : fs(path, std::ios::binary | std::ios::in) {
        if (!fs) throw std::runtime_error("Failed to open file for reading");
    }

    // read next chunk header; returns false at EOF
    bool next_chunk(ChunkHeader& outHdr, std::streampos& payloadPos) {
        if (!fs.good() || fs.eof()) return false;
        std::streampos pos = fs.tellg();
        if (!read_pod_checked(fs, outHdr)) return false;
        payloadPos = fs.tellg();
        return true;
    }

    void skip_payload(const ChunkHeader& hdr) {
        fs.seekg(static_cast<std::streamoff>(hdr.size), std::ios::cur);
    }

    // helpers forwarded from global
    std::string read_string() { return ::read_string(fs); }
    template<typename T> void read_pod(T& out) { ::read_pod(fs, out); }
    void read_bytes(void* out, size_t size) { ::read_bytes(fs, out, size); }

    std::streampos tell() { return fs.tellg(); }
    void seek(std::streampos p) { fs.seekg(p); }
    bool good() const { return fs.good(); }

private:
    std::fstream fs;

    static bool read_pod_checked(std::istream& is, ChunkHeader& out) {
        if (!is.good()) return false;
        is.read(reinterpret_cast<char*>(&out), sizeof(ChunkHeader));
        return is.gcount() == sizeof(ChunkHeader);
    }
};

// Example payload writers/readers for common chunk types

// GEOM payload layout (simple):
// uint32_t vertexCount
// float[3]*vertexCount (positions)
// uint32_t indexCount
// uint32_t[indexCount] (indices)

void write_geom(AssetWriter& w, const std::vector<float>& positions /* x,y,z x,y,z ... */, const std::vector<uint32_t>& indices) {
    std::streampos start = w.begin_chunk(ChunkType::GEOM, 1);
    uint32_t vcount = static_cast<uint32_t>(positions.size() / 3);
    w.write_pod(vcount);

    if (!positions.empty()) w.write_bytes(positions.data(), positions.size() * sizeof(float));
    uint32_t icount = static_cast<uint32_t>(indices.size());
    w.write_pod(icount);

    if (!indices.empty()) w.write_bytes(indices.data(), indices.size() * sizeof(uint32_t));

    w.end_chunk(start);
}

void write_mesh(AssetWriter& w, const std::string& name, const std::vector<float>& positions, const std::vector<uint32_t>& indices) {
    std::streampos meshStart = w.begin_chunk(ChunkType::MESH, 1);
    w.write_string(name);
    write_geom(w, positions, indices); // nested GEOM chunk
    w.end_chunk(meshStart);
}

// Texture payload layout (simple):
// uint32_t width, uint32_t height, uint32_t format, uint32_t dataSize, bytes[dataSize]

void write_texture(AssetWriter& w, const std::string& name, uint32_t wdt, uint32_t hgt, const std::vector<uint8_t>& data) {
    std::streampos texStart = w.begin_chunk(ChunkType::TEXR, 1);
    w.write_string(name);
    w.write_pod(wdt);
    w.write_pod(hgt);
    uint32_t fmt = 0; // 0 = raw RGBA8 for this example
    w.write_pod(fmt);
    uint32_t dataSize = static_cast<uint32_t>(data.size());
    w.write_pod(dataSize);
    if (dataSize) w.write_bytes(data.data(), dataSize);
    w.end_chunk(texStart);
}

// Example usage: write a file with one mesh and one texture
void example_write(const std::string& path) {
    AssetWriter w(path);
    // ROOT chunk to enclose everything (optional)
    std::streampos rootStart = w.begin_chunk(ChunkType::ROOT, 2);

    // a mesh
    std::vector<float> positions = { 0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f }; // triangle
    std::vector<uint32_t> indices = {0,1,2};
    write_mesh(w, "triangle", positions, indices);

    // a texture
    // std::vector<uint8_t> texdata = { 255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255 }; // 2x2 RGBA pixel data
    // write_texture(w, "debug_tex", 2, 2, texdata);



    w.end_chunk(rootStart);
}

// Reader helpers for GEOM and MESH
void read_geom(AssetReader& r, const ChunkHeader& hdr) {
    std::streampos payloadPos = r.tell();
    uint32_t vcount = 0;
    r.read_pod(vcount);
    std::vector<float> positions;
    if (vcount) {
        positions.resize(static_cast<size_t>(vcount) * 3);
        r.read_bytes(positions.data(), positions.size() * sizeof(float));
    }
    uint32_t icount = 0;
    r.read_pod(icount);
    std::vector<uint32_t> indices;
    if (icount) {
        indices.resize(icount);
        r.read_bytes(indices.data(), indices.size() * sizeof(uint32_t));
    }
    std::cout << "  GEOM: vertices=" << vcount << " indices=" << icount << "\n";
}

void read_mesh(AssetReader& r, const ChunkHeader& hdr) {
    std::streampos payloadPos = r.tell();
    std::string name = r.read_string();
    std::cout << "MESH name='" << name << "'\n";
    // After name, there may be nested chunks. We'll iterate until we've consumed the mesh payload.
    std::streampos meshPayloadStart = r.tell();
    std::streampos meshPayloadEnd = meshPayloadStart + static_cast<std::streamoff>(hdr.size - static_cast<uint32_t>(name.size() + sizeof(uint32_t)));
    // safer approach: read nested chunks until we've consumed hdr.size

    std::streampos endPos = meshPayloadStart + static_cast<std::streamoff>(hdr.size - static_cast<uint32_t>(name.size() + sizeof(uint32_t)));
    // Instead of complex math, we'll attempt to read nested chunk headers while there's data left in mesh payload.
    std::streampos payloadConsumed = r.tell();
    while (payloadConsumed < (payloadPos + static_cast<std::streamoff>(hdr.size))) {
        ChunkHeader sub;
        std::streampos subPayloadPos;
        if (!r.next_chunk(sub, subPayloadPos)) break;
        // Move into payload processing for sub
        if (sub.type == ChunkType::GEOM) {
            read_geom(r, sub);
        } else {
            std::cout << "  Unknown sub-chunk in MESH: " << sub.type << " (skipping)\n";
            r.skip_payload(sub);
        }
        payloadConsumed = r.tell();
    }
}

void read_texture(AssetReader& r, const ChunkHeader& hdr) {
    std::string name = r.read_string();
    uint32_t wdt = 0, hgt = 0, fmt = 0, dataSize = 0;
    r.read_pod(wdt);
    r.read_pod(hgt);
    r.read_pod(fmt);
    r.read_pod(dataSize);
    std::vector<uint8_t> data;
    if (dataSize) {
        data.resize(dataSize);
        r.read_bytes(data.data(), dataSize);
    }
    std::cout << "TEXR name='" << name << "' " << wdt << "x" << hgt << " data=" << dataSize << " bytes\n";
}

// Simple top-level reader that iterates chunks and dispatches
void example_read(const std::string& path) {
    AssetReader r(path);
    while (true) {
        ChunkHeader hdr;
        std::streampos payloadPos;
        if (!r.next_chunk(hdr, payloadPos)) break;
        // Dispatch on chunk type
        switch (hdr.type) {
            case ChunkType::ROOT: {
                std::cout << "ROOT chunk (size=" << hdr.size << ")\n";
                // Inside root there will be nested chunks; continue loop to process them (we're already positioned at payload)
                break;
            }
            case ChunkType::MESH: {
                read_mesh(r, hdr);
                break;
            }
            case ChunkType::GEOM: {
                read_geom(r, hdr);
                break;
            }
            case ChunkType::TEXR: {
                read_texture(r, hdr);
                break;
            }
            default: {
                std::cout << "Unknown chunk FourCC: " << hdr.type << " size=" << hdr.size << " (skipping)\n";
                r.skip_payload(hdr);
                break;
            }
        }
    }
}

int main() {
    const std::string path = "example.rwasset";
    try {
        example_write(path);
        std::cout << "Wrote asset file: " << path << "\n";
        example_read(path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}
