#include "utils.hpp"


void v_utils::QImageFromCvMat(const cv::Mat& cvMat, QImage& qImage)
{
    /*
        @ function: QImageFromCvMat
        @ description: convert cv::Mat to QImage
        @ cvMat: cv::Mat, the input cv::Mat
        @ qImage: QImage, the output QImage
        @ return: void
    */

    int depth = cvMat.depth();
    int channels = cvMat.channels();
    if (depth == CV_8U && channels == 3)
    {
        // 8-bit无符号整型，3个通道（BGR）
        qImage = QImage(cvMat.data, cvMat.cols, cvMat.rows, cvMat.step, QImage::Format_RGB888);
    }
    else if (depth == CV_8U && channels == 4)
    {
        // 8-bit无符号整型，4个通道（BGRA）
        qImage = QImage(cvMat.data, cvMat.cols, cvMat.rows, cvMat.step, QImage::Format_ARGB32).copy();
    }
}