#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>
#include <type_traits>

constexpr uint32_t MAKE_FOURCC(char a, char b, char c, char d) {
    return (uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24);
}

namespace rw {
    namespace chunk {
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
        } // namespace ChunkType

#pragma pack(push,1)
        struct ChunkHeader {
            uint32_t type;             // FourCC
            uint32_t size;             // size of stored payload (compressed if applicable)
            uint32_t version;          // chunk-specific version
            uint32_t uncompressedSize; // size of original payload (== size if not compressed)
        };
#pragma pack(pop)

        static_assert(sizeof(ChunkHeader) == 16, "ChunkHeader must be 16 bytes");

    } // namespace chunk
} // namespace rw
#endif //CHUNK_H
