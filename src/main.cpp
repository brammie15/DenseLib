#include <iostream>
#include "format.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <zlib.h>

std::vector<uint8_t> compress_data(const std::vector<uint8_t>& input) {
    uLongf compressedSize = compressBound(input.size());
    std::vector<uint8_t> compressed(compressedSize);

    int res = compress(compressed.data(), &compressedSize, input.data(), input.size());
    if (res != Z_OK) {
        throw std::runtime_error("Compression failed");
    }

    compressed.resize(compressedSize); // resize to actual compressed size
    return compressed;
}

// Decompress data using zlib
std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed, size_t originalSize) {
    std::vector<uint8_t> decompressed(originalSize);
    uLongf destLen = originalSize;

    int res = uncompress(decompressed.data(), &destLen, compressed.data(), compressed.size());
    if (res != Z_OK) {
        throw std::runtime_error("Decompression failed");
    }

    return decompressed;
}

int main() {
    const std::string path = "example.bram";
    const std::string compressedPath = "example.bm";
    const std::string texturePath = "resources/aPoes.jpeg";

    // Load the image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cerr << "Failed to load texture image: " << texturePath << "\n";
        return 1;
    }
    rw::AssetWriter w(compressedPath);
    std::streampos rootStart = w.begin_chunk(rw::chunk::ChunkType::ROOT, 2);
    auto compressedPixels = compress_data(std::vector<uint8_t>(pixels, pixels + texWidth * texHeight * 4));
    write_texture(w, "aPoes", texWidth, texHeight, compressedPixels);
    // write_texture(w, "aPoes", static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
    //               std::vector<uint8_t>(pixels, pixels + texWidth * texHeight * 4));
    w.end_chunk(rootStart);


    rw::AssetWriter w2(path);
    std::streampos rootStart2 = w2.begin_chunk(rw::chunk::ChunkType::ROOT, 2);
    write_texture(w2, "aPoes", static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), std::vector<uint8_t>(pixels, pixels + texWidth * texHeight * 4));

    w2.end_chunk(rootStart2);

    stbi_image_free(pixels);

    rw::AssetReader r(compressedPath);
    while (true) {
        rw::chunk::ChunkHeader hdr;
        std::streampos payloadPos;
        if (!r.next_chunk(hdr, payloadPos)) break;
        switch (hdr.type) {
            case rw::chunk::ChunkType::ROOT: {
                std::cout << "ROOT chunk (size=" << hdr.size << ")\n";
                break;
            }
            case rw::chunk::ChunkType::MESH: {
                read_mesh(r, hdr);
                break;
            }
            case rw::chunk::ChunkType::GEOM: {
                read_geom(r, hdr);
                break;
            }
            case rw::chunk::ChunkType::TEXR: {
                auto textureInfo = read_texture(r, hdr);
                if (textureInfo) {
                    // Save the texture as a PNG file
                    auto decompressedPixels = decompress_data(textureInfo->data, textureInfo->width * textureInfo->height * 4);

                    std::string outPngPath = textureInfo->name + ".jpg";
                    if (stbi_write_png(outPngPath.c_str(), textureInfo->width, textureInfo->height, 4,
                                       decompressedPixels.data(), textureInfo->width * 4)) {
                        std::cout << "Wrote texture PNG: " << outPngPath << "\n";
                    } else {
                        std::cerr << "Failed to write texture PNG: " << outPngPath << "\n";
                    }
                }
                break;
            }
            default: {
                std::cout << "Unknown chunk FourCC: " << hdr.type << " size=" << hdr.size << " (skipping)\n";
                r.skip_payload(hdr);
                break;
            }
        }
    }

    //
    // try {
    //     // example_write(path);
    //     std::cout << "Wrote asset file: " << path << "\n";
    //     example_read(path);
    // } catch (const std::exception& e) {
    //     std::cerr << "Error: " << e.what() << "\n";
    // }
    return 0;
}
