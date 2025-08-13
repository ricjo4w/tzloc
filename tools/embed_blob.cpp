#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: embed_blob <input_bin> <output_cpp>\n";
        return 2;
    }
    const std::string in  = argv[1];
    const std::string out = argv[2];

    std::ifstream ifs(in, std::ios::binary);
    if (!ifs) { std::cerr << "ERROR: open " << in << "\n"; return 1; }
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());

    std::ofstream os(out, std::ios::trunc);
    if (!os) { std::cerr << "ERROR: open " << out << " for write\n"; return 1; }

    os << "// AUTO-GENERATED. Do not edit.\n"
          "#include <cstddef>\n"
          "namespace tzloc_blob_ns {\n"
          "alignas(16) const unsigned char tzloc_blob[] = {";
    os << std::hex << std::setfill('0');
    for (size_t i=0;i<data.size();++i) {
        if (i % 12 == 0) os << "\n  ";
        os << "0x" << std::setw(2) << (unsigned)(data[i]);
        if (i + 1 != data.size()) os << ", ";
    }
    os << "\n};\n";
    os << "const std::size_t tzloc_blob_size = " << std::dec << data.size() << ";\n";
    os << "} // namespace tzloc_blob_ns\n";
    return 0;
}
