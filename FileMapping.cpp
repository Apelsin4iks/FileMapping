#include <iostream>
#include <vector>
#include <windows.h>

const size_t FILE_SIZE = std::nextafter(2L, 1.0) * 1024 * 1024 * 1024;
const size_t RECORD_SIZE = 20;

struct Record {
    uint8_t index;
    char data[19];
};

void create_file(const char* filename) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error creating file: " << GetLastError() << std::endl;
        return;
    }

    LARGE_INTEGER size;
    size.QuadPart = FILE_SIZE;
    if (!SetFilePointerEx(hFile, size, nullptr, FILE_BEGIN) || !SetEndOfFile(hFile)) {
        std::cerr << "Error setting file size: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return;
    }

    CloseHandle(hFile);
}

void write_records_to_file(const char* filename, const std::vector<Record>& records) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return;
    }

    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (hMap == nullptr) {
        std::cerr << "Error creating file mapping: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return;
    }

    void* map = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (map == nullptr) {
        std::cerr << "Error mapping view of file: " << GetLastError() << std::endl;
        CloseHandle(hMap);
        CloseHandle(hFile);
        return;
    }

    for (const auto& record : records) {
        size_t pos = record.index * RECORD_SIZE;
        if (pos + RECORD_SIZE > FILE_SIZE) {
            std::cerr << "Record index " << static_cast<int>(record.index) << " exceeds file size. Skipping." << std::endl;
            continue;
        }
        std::memcpy(static_cast<char*>(map) + pos, &record, RECORD_SIZE);
    }

    UnmapViewOfFile(map);
    CloseHandle(hMap);
    CloseHandle(hFile);
}

void print_records(const char* filename, size_t count) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return;
    }

    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMap == nullptr) {
        std::cerr << "Error creating file mapping: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return;
    }

    void* map = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (map == nullptr) {
        std::cerr << "Error mapping view of file: " << GetLastError() << std::endl;
        CloseHandle(hMap);
        CloseHandle(hFile);
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        size_t pos = i * RECORD_SIZE;
        Record record;
        std::memcpy(&record, static_cast<char*>(map) + pos, RECORD_SIZE);
        if (record.index == 0 && i != 0) {
            std::cout << "Index: " << static_cast<int>(record.index) << "(" << i << "), Data: NULL" << std::endl;
        } else {
            std::cout << "Index: " << static_cast<int>(record.index) << ", Data: " << std::string(record.data, 19) << std::endl;
        }
    }

    UnmapViewOfFile(map);
    CloseHandle(hMap);
    CloseHandle(hFile);
}

int main() {
    const char* filename = "large_file.dat";
    create_file(filename);

    std::vector<Record> records = {
        {0, "1 record data"},
        {2, "3 record data"},
        {5, "6 record data"},
        {7, "8 record data"},
        {10, "11 record data"},
        {50, "51 record data"},
    };

    write_records_to_file(filename, records);

    print_records(filename, records.back().index+1);

    return 0;
}