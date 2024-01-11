#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <locale>
#include <codecvt>
#include "Reader.h"

class ReaderCsv : public Reader<std::vector<std::string>> {
public:
    ReaderCsv();
    void Read() override;
    void Read(const std::string& filePath) override;
    std::vector<std::string>* Get() override;
    std::vector<std::string> Ottieni();
    void Flush() override;
    ~ReaderCsv();

private:
    std::vector<std::string> m_Lines;
};

