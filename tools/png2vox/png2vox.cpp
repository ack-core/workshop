// tool to convert square png into voxel tile and adjust color to template's palette
// use: png2vox <src path>.png <template path>.vox <out path>.png

#include <cstddef>
#include <cstdint>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <vector>
#include <filesystem>

#include <windows.h>

#include "upng.h"
#include "upng.cpp"

char g_buffer[4096];

void logError(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf_s(g_buffer, fmt, args);
    va_end(args);

    OutputDebugStringA(g_buffer);
    OutputDebugStringA("\n");

    printf("%s\n", g_buffer);
    DebugBreak();
}

bool loadFile(const char *filePath, std::unique_ptr<unsigned char[]> &data, std::size_t &size) {
    std::fstream fileStream(filePath, std::ios::binary | std::ios::in | std::ios::ate);

    if (fileStream.is_open() && fileStream.good()) {
        std::size_t fileSize = std::size_t(fileStream.tellg());
        data = std::make_unique<unsigned char[]>(fileSize);
        fileStream.seekg(0);
        fileStream.read((char *)data.get(), fileSize);
        size = fileSize;

        return true;
    }

    return false;
}

bool writeFile(const char *filePath, const std::unique_ptr<unsigned char[]> &data, std::size_t size) {
    std::fstream fileStream(filePath, std::ios::binary | std::ios::out);

    if (fileStream.is_open()) {
        fileStream.write((char *)data.get(), size);
        return fileStream.good();
    }

    return false;
}

struct Voxel {
    Voxel(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t i) {
        positionX = x;
        positionY = y;
        positionZ = z;
        colorIndex = i + 1;
    }

    std::uint8_t positionZ, positionX, positionY, colorIndex;
};

struct VoxInfo {
    std::size_t sizeBlockOffset;
    std::unique_ptr<std::uint8_t[]> voxHeadData;
    std::size_t voxHeadSize;
    std::unique_ptr<std::uint8_t[]> voxFootData;
    std::size_t voxFootSize;
};

bool loadVoxInfo(const char *fullPath, std::uint32_t (&palette)[256], VoxInfo &output) {
    const std::int32_t version = 150;

    std::unique_ptr<std::uint8_t[]> voxData;
    std::size_t voxSize = 0;

    if (loadFile(fullPath, voxData, voxSize)) {
        std::uint8_t *data = voxData.get();

        if (memcmp(data, "VOX ", 4) == 0 && *(std::int32_t *)(data + 4) == version) {
            // skip bytes of main chunk to start of the first child ('PACK')
            data += 20;
            std::int32_t modelCount = 1;

            if (memcmp(data, "PACK", 4) != 0) {
                if (memcmp(data, "SIZE", 4) == 0) {
                    data += 12;
                    output.sizeBlockOffset = data - voxData.get();

                    std::int8_t sizeZ = *(std::int8_t *)(data + 0);
                    std::int8_t sizeX = *(std::int8_t *)(data + 4);
                    std::int8_t sizeY = *(std::int8_t *)(data + 8);

                    data += 12;

                    output.voxHeadSize = data - voxData.get();
                    output.voxHeadData = std::make_unique<std::uint8_t[]>(output.voxHeadSize);
                    memcpy(output.voxHeadData.get(), voxData.get(), output.voxHeadSize);

                    if (memcmp(data, "XYZI", 4) == 0) {
                        std::size_t voxelCount = *(std::uint32_t *)(data + 12);

                        data += 16;
                        data += voxelCount * 4;

                        output.voxFootSize = voxSize - (data - voxData.get());
                        output.voxFootData = std::make_unique<std::uint8_t[]>(output.voxFootSize);
                        memcpy(output.voxFootData.get(), data, output.voxFootSize);

                        if (memcmp(data, "RGBA", 4) == 0) {
                            memcpy(palette, data + 12, 256 * sizeof(std::uint32_t));
                            return true;
                        }
                        else {
                            logError("[png2vox] RGBA chunk is not found in '%s'", fullPath);
                        }
                    }
                    else {
                        logError("[png2vox] XYZI chunk is not found in '%s'", fullPath);
                    }
                }
                else {
                    logError("[png2vox] SIZE chunk is not found in '%s'", fullPath);
                }
            }
            else {
                logError("[png2vox] template '%s' has more than one model", fullPath);
            }
        }
        else {
            logError("[png2vox] Incorrect vox-header in '%s'", fullPath);
        }
    }
    else {
        logError("[png2vox] Unable to find file '%s'", fullPath);
    }

    return false;
}

void writeVox(const char *fullPath, VoxInfo &voxTemplate, const std::vector<Voxel> &voxels) {
    std::size_t outputSize = voxTemplate.voxHeadSize + 16 + voxels.size() * sizeof(Voxel) + voxTemplate.voxFootSize;
    std::unique_ptr<std::uint8_t[]> outputData = std::make_unique<std::uint8_t[]>(outputSize);

    std::uint8_t maxX = 0, maxY = 0, maxZ = 0;

    for (auto &item : voxels) {
        if (item.positionX >= maxX) {
            maxX = item.positionX + 1;
        }
        if (item.positionY >= maxY) {
            maxY = item.positionY + 1;
        }
        if (item.positionZ >= maxZ) {
            maxZ = item.positionZ + 1;
        }
    }

    *(std::uint32_t *)(voxTemplate.voxHeadData.get() + 16) = std::uint32_t(outputSize - 20); // MAIN chunk children size
    *(std::uint32_t *)(voxTemplate.voxHeadData.get() + voxTemplate.sizeBlockOffset + 0) = maxZ;
    *(std::uint32_t *)(voxTemplate.voxHeadData.get() + voxTemplate.sizeBlockOffset + 4) = maxX;
    *(std::uint32_t *)(voxTemplate.voxHeadData.get() + voxTemplate.sizeBlockOffset + 8) = maxY;

    char xyziHeader[16] = "XYZI";
    *(std::uint32_t *)(xyziHeader + 4) = std::uint32_t(voxels.size() * sizeof(Voxel)) + sizeof(std::uint32_t);
    *(std::uint32_t *)(xyziHeader + 8) = 0;
    *(std::uint32_t *)(xyziHeader + 12) = std::uint32_t(voxels.size());

    memcpy(outputData.get(), voxTemplate.voxHeadData.get(), voxTemplate.voxHeadSize);
    memcpy(outputData.get() + voxTemplate.voxHeadSize, xyziHeader, 16);
    memcpy(outputData.get() + voxTemplate.voxHeadSize + 16, &voxels[0], voxels.size() * sizeof(Voxel));
    memcpy(outputData.get() + voxTemplate.voxHeadSize + 16 + voxels.size() * sizeof(Voxel), voxTemplate.voxFootData.get(), voxTemplate.voxFootSize);

    if (!writeFile(fullPath, outputData, outputSize)) {
        logError("[png2vox] Unable to write file '%s'", fullPath);
    }
}

std::uint8_t getNearestPalleteIndex(std::uint32_t color, std::uint32_t (&palette)[256]) {
    std::uint8_t colorR = std::uint8_t(color & 0xff);
    std::uint8_t colorG = std::uint8_t((color >> 8) & 0xff);
    std::uint8_t colorB = std::uint8_t((color >> 16) & 0xff);

    std::uint32_t minDelta = 0xffffffff;
    std::uint8_t minIndex = 0xff;

    for (std::uint8_t i = 0; i < 255; i++) {
        std::uint8_t r = std::uint8_t(palette[i] & 0xff);
        std::uint8_t g = std::uint8_t((palette[i] >> 8) & 0xff);
        std::uint8_t b = std::uint8_t((palette[i] >> 16) & 0xff);

        std::uint32_t delta = 0;

        delta += colorR > r ? colorR - r : r - colorR;
        delta += colorG > g ? colorG - g : g - colorG;
        delta += colorB > b ? colorB - b : b - colorB;

        if (delta < minDelta) {
            minDelta = delta;
            minIndex = i;
        }
    }

    return minIndex;
}

bool convert(VoxInfo &info, std::uint32_t(&palette)[256], const char *imagePath, const char *outputPath) {
    std::unique_ptr<std::uint8_t[]> imageData;
    std::size_t imageSize;

    if (loadFile(imagePath, imageData, imageSize)) {
        upng_t *upng = upng_new_from_bytes(imageData.get(), unsigned long(imageSize));

        if (upng != nullptr) {
            if (*reinterpret_cast<const unsigned *>(imageData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                if (upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == upng_get_height(upng) && upng_get_width(upng) < 256) {
                    const std::uint32_t *pngmem = reinterpret_cast<const std::uint32_t *>(upng_get_buffer(upng));
                    const std::uint8_t pngsize = upng_get_width(upng);

                    std::vector<Voxel> voxels;

                    for (std::uint8_t i = 0; i < pngsize; i++) {
                        for (std::uint8_t c = 0; c < pngsize; c++) {
                            voxels.emplace_back(Voxel{ c, 0, i, getNearestPalleteIndex(pngmem[i * pngsize + c], palette) });
                        }
                    }

                    writeVox(outputPath, info, voxels);
                    return true;
                }
                else {
                    logError("[png2vox] '%s' is not square <256 RGBA png file", imagePath);
                }
            }
            else {
                logError("[png2vox] '%s' is not a valid png file", imagePath);
            }

            upng_free(upng);
        }
    }
    else {
        logError("[png2vox] '%s' not found", imagePath);
    }

    return false;
}

int main(int argc, const char *argv[]) {
    if (argc == 3) {
        std::uint32_t palette[256];
        VoxInfo info;

        const char *templatePath = argv[1];

        if (loadVoxInfo(templatePath, palette, info)) {
            std::string fullPath = argv[2];

            for (auto &entry : std::filesystem::directory_iterator(fullPath)) {
                if (entry.is_regular_file()) {
                    const int EXTLEN = 3;
                    std::string path = entry.path().generic_u8string();

                    if (path.rfind("png") + EXTLEN == path.size() ) {
                        std::string output = std::string( path.begin(), path.end() - EXTLEN) + "vox";
                        convert(info, palette, path.data(), output.data());
                    }
                }
            }

            return 0;
        }
        else {
            logError("[png2vox] '%s' has invalid format", templatePath);
        }
    }
    else {
        printf("Tool to convert square png's into voxel tile and adjust color to template's palette\nUsage: png2vox <template path>.vox <directory path>\n");
    }

    return 1;
}

