#include "file_utils.h"
#include <experimental/filesystem>
#include "logger/logger.h"

namespace fs = std::experimental::filesystem;

namespace foundation
{

bool FileUtils::GetFileList(const std::string& dir, std::vector<std::string>& files, std::vector<std::string>& dirs)
{
    if (!fs::is_directory(dir))
    {
        LOG_ERROR << "not a dir str: " << dir;
        return false;
    }

    files.clear();
    dirs.clear();
    try
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            std::string name     = entry.path().filename().u8string();
            std::string fullPath = entry.path().u8string();

            // 检查是否是文件夹
            if (fs::is_directory(entry.status()))
            {
                // 文件夹列表
                dirs.push_back(fullPath);
            }
            else
            {
                // 文件列表
                files.push_back(fullPath);
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "get dir list failed! " << e.what();
    }
    return false;
}

} // namespace foundation