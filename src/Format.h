#ifndef FORMAT_H
#define FORMAT_H
#include <memory>
#include <vector>

#include "AssetReader.h"
#include "AssetWriter.h"

void write_geom(rw::AssetWriter& w, const std::vector<float>& positions /* x,y,z x,y,z ... */, const std::vector<uint32_t>& indices);

void write_mesh(rw::AssetWriter& w, const std::string& name, const std::vector<float>& positions, const std::vector<uint32_t>& indices);

void write_texture(rw::AssetWriter& w, const std::string& name, uint32_t wdt, uint32_t hgt, const std::vector<uint8_t>& data);

void example_write(const std::string& path);

void read_geom(rw::AssetReader& r, const rw::chunk::ChunkHeader& hdr);

void read_mesh(rw::AssetReader& r, const rw::chunk::ChunkHeader& hdr);

struct Texture {
    std::string name;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    std::vector<uint8_t> data; // raw pixel data
};

std::unique_ptr<Texture> read_texture(rw::AssetReader& r, const rw::chunk::ChunkHeader& hdr);

void example_read(const std::string& path);

#endif //FORMAT_H
