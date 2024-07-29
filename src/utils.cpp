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


bool v_utils::ipFormatChecked(const std::string &ip)
{
    /*
        @ function: ipFormatChecked
        @ description: check the ip format
        @ ip: std::string, the input ip
        @ return: bool
    */
   // 格式符合 xxx.xxx.xxx.xxx:xxxx，返回true
    std::regex ip_regex("((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}(:[0-9]{1,5})?");    // 正则表达式, 匹配IP地址格式, 包括IPv4和IPv6, 以及端口号, 如: 127.0.0.1:8080
    return std::regex_match(ip, ip_regex);
}


bool v_utils::engineTypeChecked(const std::string &enginePath)
{
    /*
        @ function: engineTypeChecked
        @ description: check the suffix of engine path, .engine or .trt
        @ enginePath: std::string, the input engine path
        @ return: bool
    */
    // 检查engine文件后缀是否为.engine或.trt
    if(_access(enginePath.c_str(), 0) != -1)    // 检查文件是否存在
    {
        std::string suffix = enginePath.substr(enginePath.find_last_of(".") + 1);
        if(suffix == "engine" || suffix == "trt")
        {
            struct _stat buffer;
            if (_stat(enginePath.c_str(), &buffer) != 0)    // 检查文件是否可读
            {
                return false;
            }
            return ((buffer.st_mode & _S_IFMT) == _S_IFREG);    // 检查文件是否为普通文件
        }
        else
        {
            printf("%s:%d %s not exist\n", __FILE__, __LINE__, enginePath.c_str());
            return false;
        }
    }
    else
    {
        return false;
    }
}