#include <cstdio>
#include <memory>
#include <stdexcept>

class File {
private:
    std::FILE* file = nullptr;

public:
    explicit File(const char name[], const char mode[]) : file(std::fopen(name, mode)){
        if (!file) {
            throw std::runtime_error("Failed to open file");
        }
    }
    ~File() {
        if (file) {
            std::fclose(file);
        }
    }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& other) noexcept : file(other.file) {
        other.file = nullptr;
    }
    File& operator=(File&& other) noexcept {
        if (this != &other) {
            if (file) {
                std::fclose(file);
            }
            file = other.file;
            other.file = nullptr;
        }
        return *this;
    }
};

// Rule of ZERO

class Files{
private:
    struct deleter { void operator()(std::FILE* file) const noexcept { std::fclose(file); } };
    std::unique_ptr<std::FILE, deleter> file;
    
public:
    explicit Files(const char name[], const char mode[]) : file(std::fopen(name, mode)) {
        if (!file) {
            throw std::runtime_error("Failed to open file");
        }
    }
};