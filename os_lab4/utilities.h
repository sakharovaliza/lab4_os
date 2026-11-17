#pragma once
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>

inline char* toCharStr(const std::string& str) {
    char* cmdLine = new char[str.length() + 1];
    std::strcpy(cmdLine, str.c_str());
    return cmdLine;
}

inline void writefileAt(std::fstream& fout, const char* buff, size_t size, std::streamoff offset) {
    if (size < 20) throw std::invalid_argument("Buffer size must be at least 20 bytes");
    fout.seekp(offset);
    fout.write(buff, 20);
    fout.flush();
}

inline int readfileAt(std::fstream& fin, char* buff, size_t size, std::streamoff offset) {
    if (size < 20) throw std::invalid_argument("Buffer size must be at least 20 bytes");
    fin.seekg(offset);
    fin.read(buff, 20);
    fin.clear();
    return (int)fin.gcount();
}