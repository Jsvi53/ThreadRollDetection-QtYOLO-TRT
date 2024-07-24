#pragma once

#include <opencv2/opencv.hpp>
#include <QImage>

namespace v_utils
{
    void QImageFromCvMat(const cv::Mat& cvMat, QImage& qImage);
}