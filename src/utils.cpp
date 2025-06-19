#include "utils.h"
#include <chrono>
#include <ctime>

std::string hex_encode(const unsigned char* data, size_t length) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(data[i]);
    }

    return ss.str();
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}