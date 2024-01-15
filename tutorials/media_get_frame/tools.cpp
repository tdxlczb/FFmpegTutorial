#include "tools.h"
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

void SaveBitmapToFile(HBITMAP hBitmap, const char* filePath)
{
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    infoHeader.biSize          = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth         = bitmap.bmWidth;
    infoHeader.biHeight        = -bitmap.bmHeight;
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = bitmap.bmBitsPixel;
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed       = 0;
    infoHeader.biClrImportant  = 0;

    fileHeader.bfType      = 0x4D42;
    fileHeader.bfSize      = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bitmap.bmWidthBytes * bitmap.bmHeight;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    FILE* file = fopen(filePath, "wb");
    if (file == NULL)
    {
        return;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);
    fwrite(bitmap.bmBits, bitmap.bmWidthBytes * bitmap.bmHeight, 1, file);

    fclose(file);
}

void ImageToBitmap(const std::string& src, const std::string& dest)
{
    cv::Mat image = cv::imread(src);
    //cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);

    //cv::imwrite(dest, image);

    int width  = image.cols;
    int height = image.rows;

    // 创建一个新的 Bitmap 对象
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth         = width;
    bitmapInfo.bmiHeader.biHeight        = -height; // 负号表示图像是自上而下的
    bitmapInfo.bmiHeader.biPlanes        = 1;
    bitmapInfo.bmiHeader.biBitCount      = 24; // 每个像素使用 32 位表示
    bitmapInfo.bmiHeader.biCompression   = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage     = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biClrUsed       = 0;
    bitmapInfo.bmiHeader.biClrImportant  = 0;

    // 创建一个 Device Context (DC)
    HDC hdc = GetDC(NULL);

    // 创建一个 DIB Section，并将其与 DC 关联
    void*   bitmapPixels;
    HBITMAP hBitmap    = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapPixels, NULL, 0);
    HDC     memDC      = CreateCompatibleDC(hdc);
    HBITMAP hOldBitmap = (HBITMAP) SelectObject(memDC, hBitmap);

    // 将 OpenCV 的图像数据复制到 Bitmap 对象中
    memcpy(bitmapPixels, image.data, image.total() * image.elemSize());

    // 将 Bitmap 对象保存为文件
    SaveBitmapToFile(hBitmap, dest.c_str());

    // 释放资源
    SelectObject(memDC, hOldBitmap);
    DeleteDC(memDC);
    DeleteObject(hBitmap);
    ReleaseDC(NULL, hdc);
}