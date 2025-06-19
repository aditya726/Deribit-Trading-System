#pragma once

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>

std::string hex_encode(const unsigned char* data, size_t length);
std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string get_current_timestamp();