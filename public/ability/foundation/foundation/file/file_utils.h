#pragma once
#include <string>
#include <vector>

namespace foundation
{

struct FileInfo
{
    std::string fileName;
    std::string filePath;
    uint64_t    lastWriteTime; //milliseconds
    bool        isDirectory;
};

class FileUtils
{
public:
    static bool GetFileList(const std::string& dir, std::vector<std::string>& files, const bool recursive, const std::string& suffix = "all");
    static bool GetDirList(const std::string& dir, std::vector<std::string>& dirs, bool recursive);
    static bool GetDirInfoList(const std::string& dir, std::vector<FileInfo>& dirs, bool recursive);

#ifdef _WIN32
    static void GetFileList2(const std::string& dir, std::vector<std::string>& fileList, const std::string& filter = "*.*");
#endif
};
}