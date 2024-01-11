#pragma once

#include <string>

template <typename T>
class Reader {
public:
    virtual void Read() = 0;
    virtual void Read(const std::string& filePath) = 0;
    virtual T* Get() = 0;
    virtual void Flush() = 0;
    virtual ~Reader() {}
};