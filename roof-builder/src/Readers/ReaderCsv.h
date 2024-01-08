#pragma once

class ReaderCsv : public Reader<> {
public:
    ReaderCsv();
    void Read(const std::string& filePath) override;
    void Get() override;
    void Flush() override;
    ~ReaderCsv();
};

