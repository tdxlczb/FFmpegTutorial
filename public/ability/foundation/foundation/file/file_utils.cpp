#include "file_utils.h"
#include <experimental/filesystem>
#include <regex>
#include "logger/logger.h"

namespace fs = std::experimental::filesystem;

namespace foundation
{

bool FileUtils::GetFileList(const std::string& dir, std::vector<std::string>& files, const bool recursive, const std::string& suffix)
{
    try
    {
        if (!fs::is_directory(dir))
        {
            LOG_ERROR << "not dir path: " << dir;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dir))
        {
            //std::string name      = entry.path().filename().u8string();
            std::string fullPath  = entry.path().u8string();
            std::string extension = entry.path().extension().u8string();

            // 检查是否是文件夹
            if (fs::is_directory(entry.status()))
            {
                //递归
                if (recursive)
                {
                    GetFileList(fullPath, files, recursive, suffix);
                }
            }
            else
            {
                if (suffix == "all" || extension == suffix)
                {
                    // 文件列表
                    files.push_back(fullPath);
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "get file list failed! " << e.what();
    }
    return false;
}

bool FileUtils::GetDirList(const std::string& dir, std::vector<std::string>& dirs, bool recursive)
{
    try
    {
        if (!fs::is_directory(dir))
        {
            LOG_ERROR << "not dir path: " << dir;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dir))
        {
            //std::string name     = entry.path().filename().u8string();
            std::string fullPath = entry.path().u8string();

            if (fs::is_directory(entry.status()))
            {
                dirs.push_back(fullPath);
                //递归
                if (recursive)
                {
                    GetDirList(fullPath, dirs, recursive);
                }
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

bool FileUtils::GetDirInfoList(const std::string& dir, std::vector<FileInfo>& dirs, bool recursive)
{
    try
    {
        if (!fs::is_directory(dir))
        {
            LOG_ERROR << "not dir path: " << dir;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dir))
        {
            std::string name     = entry.path().filename().u8string();
            std::string fullPath = entry.path().u8string();

            if (fs::is_directory(entry.status()))
            {
                FileInfo info;
                info.fileName      = name;
                info.filePath      = fullPath;
                auto lastWriteTime = fs::last_write_time(entry.path());
                info.lastWriteTime = std::chrono::duration_cast<std::chrono::milliseconds>(lastWriteTime.time_since_epoch()).count();
                info.isDirectory   = true;
                dirs.push_back(info);
                //递归
                if (recursive)
                {
                    GetDirInfoList(fullPath, dirs, recursive);
                }
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

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <shellapi.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

void FileUtils::GetFileList2(const std::string& dir, std::vector<std::string>& fileList, const std::string& filter)
{
    //文件句柄
    intptr_t           hFile = 0;
    //文件信息
    struct _finddata_t fileinfo;
    std::string        path = dir + "\\" + filter;
    //string p = "F:\\TY_code\\sqliteR8\\TySrvDevelop\\bin\\TY-CM-2116A2-V1\\snap\\*.jpg";
    if ((hFile = _findfirst(path.c_str(), &fileinfo)) != -1)
    {
        do
        {
            //如果是目录,迭代之
            //如果不是,加入列表
            std::string filePath = dir + "\\" + fileinfo.name;
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    GetFileList2(filePath, fileList, filter);
            }
            else
            {
                fileList.push_back(filePath);
            }
        }
        while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

#endif

} // namespace foundation