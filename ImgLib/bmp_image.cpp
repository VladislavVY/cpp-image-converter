#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {
    
PACKED_STRUCT_BEGIN BitmapFileHeader {
    char sign1 = 'B'; 
    char sign2 = 'M';
    uint32_t sum_size;
    uint32_t reserved_space = 0;
    uint32_t size_of_both_headers = 54;
}
PACKED_STRUCT_END    

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40;
    uint32_t inf_width;
    uint32_t inf_height;
    uint16_t layers = 1;
    uint16_t bit_per_pixel = 24;
    uint32_t compression_type = 0;
    uint32_t bytes_in_data;
    int32_t horizontal_pixel_per_meter = 11811;
    int32_t vertical_pixel_per_meter = 11811;
    int32_t use_colors = 0;
    int32_t colors = 0x1000000;
}
PACKED_STRUCT_END    

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    const int bytes_per_pixel = 3;
    const int alignment = 4;
    const int rounding = 3;
    return alignment * ((w * bytes_per_pixel + rounding) / alignment);
}

bool SaveBMP(const Path& file, const Image& image) {
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    
    int width = image.GetWidth();
    int height = image.GetHeight();
    int stride = GetBMPStride(width);

    file_header.sum_size = stride * height + file_header.size_of_both_headers;
    info_header.inf_width = width;
    info_header.inf_height = height;
    info_header.bytes_in_data = stride * height;

    ofstream out(file, ios::binary);
    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));    
    vector<char> buff(stride);  
    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(reinterpret_cast<const char*>(buff.data()), stride);
    }
    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs) {
        return Image();
    }

    char signature[2];
    ifs.read(signature, 2);
    if (signature[0] != 'B' || signature[1] != 'M') {
        return Image();
    }

    int width, height;
    ifs.seekg(18, ios::beg);
    if (!ifs.read(reinterpret_cast<char*>(&width), sizeof(width))) {
        return Image();
    }
    if (!ifs.read(reinterpret_cast<char*>(&height), sizeof(height))) {
        return Image();
    }
    ifs.seekg(28, ios::cur);

    int stride = GetBMPStride(width);
    auto color = Color::Black();
    const int bytes_per_pixel = 3;
    Image result(stride / bytes_per_pixel, height, color);
    std::vector<char> buff(width * bytes_per_pixel);

    for (int y = result.GetHeight() - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        if (!ifs.read(buff.data(), stride)) {
            return Image();
        }

        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buff[x * bytes_per_pixel + 0]);
            line[x].g = static_cast<byte>(buff[x * bytes_per_pixel + 1]);
            line[x].r = static_cast<byte>(buff[x * bytes_per_pixel + 2]);
        }
    }
    return result;
}

}  // namespace img_lib
