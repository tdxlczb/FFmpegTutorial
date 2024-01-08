#pragma once
#include <string>
#include <vector>

namespace foundation
{

class FileUtils
{
public:
    static bool GetFileList(const std::string& dir, std::vector<std::string>& files, std::vector<std::string>& dirs);
};

}