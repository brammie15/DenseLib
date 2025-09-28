#include "Format.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "AssetReader.h"
#include "AssetWriter.h"
#include "Chunk.h"

using namespace rw;

// GEOM payload layout (simple):
// uint32_t vertexCount
// float[3]*vertexCount (positions)
// uint32_t indexCount
// uint32_t[indexCount] (indices)

void write_geom(AssetWriter& w, const std::vector<float>& positions /* x,y,z x,y,z ... */, const std::vector<uint32_t>& indices) {
    std::streampos start = w.begin_chunk(chunk::ChunkType::GEOM, 1);
    uint32_t vcount = static_cast<uint32_t>(positions.size() / 3);
    w.write_pod(vcount);
    if (!positions.empty()) w.write_bytes(positions.data(), positions.size() * sizeof(float));
    uint32_t icount = static_cast<uint32_t>(indices.size());
    w.write_pod(icount);
    if (!indices.empty()) w.write_bytes(indices.data(), indices.size() * sizeof(uint32_t));
    w.end_chunk(start);
}

void write_mesh(AssetWriter& w, const std::string& name, const std::vector<float>& positions, const std::vector<uint32_t>& indices) {
    std::streampos meshStart = w.begin_chunk(chunk::ChunkType::MESH, 1);
    w.write_string(name);
    write_geom(w, positions, indices); // nested GEOM chunk
    w.end_chunk(meshStart);
}

// Texture payload layout (simple):
// uint32_t width, uint32_t height, uint32_t format, uint32_t dataSize, bytes[dataSize]

void write_texture(AssetWriter& w, const std::string& name, uint32_t wdt, uint32_t hgt, const std::vector<uint8_t>& data) {
    std::streampos texStart = w.begin_chunk(chunk::ChunkType::TEXR, 1);
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
    std::streampos rootStart = w.begin_chunk(chunk::ChunkType::ROOT, 2);

    // a mesh
    std::vector<float> positions = { 0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f }; // triangle
    std::vector<uint32_t> indices = {0,1,2};
    write_mesh(w, "triangle", positions, indices);

    // a texture (commented out originally, leaving example commented)
    std::vector<uint8_t> texdata = { 255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255 }; // 2x2 RGBA
    write_texture(w, "debug_tex", 2, 2, texdata);

    w.end_chunk(rootStart);
}

// Reader helpers for GEOM and MESH
void read_geom(AssetReader& r, const chunk::ChunkHeader& hdr) {
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

void read_mesh(AssetReader& r, const chunk::ChunkHeader& hdr) {
    std::streampos payloadStart = r.tell();
    std::string name = r.read_string();
    std::cout << "MESH name='" << name << "'\n";

    std::streampos payloadEnd = payloadStart + static_cast<std::streamoff>(hdr.size);
    while (r.tell() < payloadEnd) {
        chunk::ChunkHeader sub;
        std::streampos subPayloadPos;
        if (!r.next_chunk(sub, subPayloadPos)) break;
        if (sub.type == chunk::ChunkType::GEOM) {
            read_geom(r, sub);
        } else {
            std::cout << "  Unknown sub-chunk in MESH: " << sub.type << " (skipping)\n";
            r.skip_payload(sub);
        }
    }
}

std::unique_ptr<Texture> read_texture(AssetReader& r, const chunk::ChunkHeader& hdr) {
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

    auto tex = std::make_unique<Texture>();
    tex->name = name;
    tex->width = wdt;
    tex->height = hgt;
    tex->format = fmt;
    tex->data = std::move(data);
    return tex;
}

// Simple top-level reader that iterates chunks and dispatches
void example_read(const std::string& path) {
    AssetReader r(path);
    while (true) {
        chunk::ChunkHeader hdr;
        std::streampos payloadPos;
        if (!r.next_chunk(hdr, payloadPos)) break;
        switch (hdr.type) {
            case chunk::ChunkType::ROOT: {
                std::cout << "ROOT chunk (size=" << hdr.size << ")\n";
                break;
            }
            case chunk::ChunkType::MESH: {
                read_mesh(r, hdr);
                break;
            }
            case chunk::ChunkType::GEOM: {
                read_geom(r, hdr);
                break;
            }
            case chunk::ChunkType::TEXR: {
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
