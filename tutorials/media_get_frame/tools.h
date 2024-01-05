#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

void ImageToBitmap(const std::string& src, const std::string& dest);

void GetFileList(const std::string& dir, std::vector<std::string>& fileList, const std::string& filter = "*.*");