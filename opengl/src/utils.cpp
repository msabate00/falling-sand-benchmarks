#include "utils.h"
#include <fstream>
#include <sstream>

std::string readTextFile(const char* path) {
    std::ifstream f(path, std::ios::in);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}