/*
Joshua Kenneth Pinto 08-07-2022

Program to convert PPM file to PNG
http://paulbourke.net/dataformats/ppm/
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <lodepng.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Too few arguments.\n";
        std::cout << "USAGE: ppm2png <input file> <output file>\n";
        return -1;
    }

    std::string fname = argv[1];
    std::string foutname = argv[2];

    std::ifstream f(fname, std::ios::in | std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open input file: " << fname << "\n";
        return -1;
    }

    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<char> imagebuffer(size);
    if (!f.read(imagebuffer.data(), size)) {
        std::cerr << "Error: Failed to read file contents.\n";
        return -1;
    }
    f.close();

    // Parse PPM Header safely using a string stream
    // Format expected: P3 (or P6), width, height, max_val
    std::string header_buffer(imagebuffer.begin(), imagebuffer.end());
    std::stringstream header_stream(header_buffer);

    std::string magic_number;
    int width = 0, height = 0, max_val = 0;

    header_stream >> magic_number;
    if (magic_number != "P3") {
        std::cerr << "Error: Only plain PPM (P3) format is supported by this parser.\n";
        return -1;
    }

    header_stream >> width >> height >> max_val;
    if (width <= 0 || height <= 0 || max_val <= 0) {
        std::cerr << "Error: Invalid PPM header dimensions.\n";
        return -1;
    }

    // Find where the pixel data actually starts in the buffer after the header
    // We skip past magic number, width, height, and max_val whitespace tokens.
    std::string token;
    int tokens_to_skip = 3; // P3, width, height, max_val (4 items total, parsed 1 magic + 3 values)
    size_t data_start_pos = 0;
    int parsed_count = 0;

    // Scan through the raw buffer to find the end of the 4th header token
    for (size_t i = 0; i < imagebuffer.size(); ++i) {
        if (imagebuffer[i] == ' ' || imagebuffer[i] == '\t' || imagebuffer[i] == '\n' || imagebuffer[i] == '\r') {
            parsed_count++;
            if (parsed_count == 4) {
                // Skip any trailing consecutive whitespace right after max_val
                while (i < imagebuffer.size() && (imagebuffer[i] == ' ' || imagebuffer[i] == '\t' || imagebuffer[i] == '\n' || imagebuffer[i] == '\r')) {
                    i++;
                }
                data_start_pos = i;
                break;
            }
        }
    }

    if (data_start_pos == 0 || data_start_pos >= (size_t)size) {
        std::cerr << "Error: Malformed PPM data offset.\n";
        return -1;
    }

    // Parse pixel data
    std::vector<uint8_t> tocodePNG;
    tocodePNG.reserve(width * height * 4);

    // Using a stringstream over the remaining raw data to safely parse ASCII values for P3
    std::stringstream data_stream(std::string(imagebuffer.begin() + data_start_pos, imagebuffer.end()));
    int r, g, b;
    int pixel_count = 0;

    while (data_stream >> r >> g >> b) {
        tocodePNG.push_back(static_cast<uint8_t>(r));
        tocodePNG.push_back(static_cast<uint8_t>(g));
        tocodePNG.push_back(static_cast<uint8_t>(b));
        tocodePNG.push_back(255); // Alpha channel
        pixel_count++;
    }

    if (pixel_count != width * height) {
        std::cout << "Warning: Expected " << (width * height) << " pixels, but parsed " << pixel_count << " pixels.\n";
    }

    // Encode to PNG using LodePNG
    std::vector<uint8_t> encodedPNG;
    unsigned error = lodepng::encode(encodedPNG, tocodePNG, width, height);
    if (error) {
        std::cerr << "Error: LodePNG encoding failed: " << lodepng_error_text(error) << "\n";
        return -1;
    }

    error = lodepng::save_file(encodedPNG, foutname);
    if (error) {
        std::cerr << "Error: Failed to save output PNG file.\n";
        return -1;
    }

    std::cout << "Successfully converted " << fname << " to " << foutname << " (" << width << "x" << height << ")\n";
    return 0;
}