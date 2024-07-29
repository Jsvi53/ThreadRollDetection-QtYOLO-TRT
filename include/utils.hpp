#pragma once

#include <opencv2/opencv.hpp>
#include <QImage>
#include <regex>
#include <unistd.h>




namespace v_utils
{
    void QImageFromCvMat(const cv::Mat& cvMat, QImage& qImage);
    bool ipFormatChecked(const std::string &ip);
    bool engineTypeChecked(const std::string &enginePath);
}