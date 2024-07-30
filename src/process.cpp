#include "process.h"


void test(void)
{
    cv::Mat src;
    src = cv::imread("assets/0.png");
    cv::imshow("rgb", src);
    cv::waitKey(0);
    cv::destroyAllWindows();

}

std::unique_ptr<float> prc::PROCESS::preprocess(const std::string& filename) const
{
	cv::Mat ori_img, lettered_img;
	ori_img = __read(filename);
	int w = ori_img.cols;
	int h = ori_img.rows;
	int max = std::max(w, h);
	int min = std::min(w, h);
	int margin = static_cast<int>(std::round((max - min) / 2));
	// make border to 640, 640
	cv::copyMakeBorder(ori_img, lettered_img, margin, margin, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

	// 转换为float类型并归一化
	lettered_img.convertTo(lettered_img, CV_32FC3, 1.0 / 255.0);

	// 计算归一化后图像的尺寸
	int let_h = lettered_img.rows;
	int let_w = lettered_img.cols;

	// 分配内存并复制数据
	auto _lettered_img = std::unique_ptr<float>{new float[let_h * let_w * 3] };

	// 仿照提供的内存复制方式进行数据复制
	for (int c = 0; c < 3; ++c) // 假设3个通道
	{
		for (int i = 0; i < let_h; ++i)
		{
			for (int j = 0; j < let_w; ++j)
			{
				// 将lettered_img中的像素值复制到_lettered_img数组中
				_lettered_img.get()[(c * let_h + i) * let_w + j] = lettered_img.at<cv::Vec3f>(i, j)[c];
			}
		}
	}
	return _lettered_img;
}


std::unique_ptr<float> prc::PROCESS::preprocess2(const std::string& filename) const
{
	cv::Mat iImg = __read(filename);
	cv::Mat oImg;
	float resizeScales;
	std::vector<int> iImgSize = { 640, 640 };
	if (iImg.channels() == 3)
    {
        oImg = iImg.clone();
        cv::cvtColor(oImg, oImg, cv::COLOR_BGR2RGB);
    }
    else
    {
        cv::cvtColor(iImg, oImg, cv::COLOR_GRAY2RGB);
    }

	if (iImg.cols >= iImg.rows)
	{
		resizeScales = iImg.cols / (float)iImgSize.at(0);
		cv::resize(oImg, oImg, cv::Size(iImgSize.at(0), int(iImg.rows / resizeScales)));
	}
	else
	{
		resizeScales = iImg.rows / (float)iImgSize.at(0);
		cv::resize(oImg, oImg, cv::Size(int(iImg.cols / resizeScales), iImgSize.at(1)));
	}
	cv::Mat tempImg = cv::Mat::zeros(iImgSize.at(0), iImgSize.at(1), CV_8UC3);
	oImg.copyTo(tempImg(cv::Rect(0, 80, oImg.cols, oImg.rows)));
	oImg = tempImg;
	oImg.convertTo(oImg, CV_32FC3, 1.0 / 255.0f);
	auto _oImg = std::unique_ptr<float>{ new float[iImgSize.at(0) * iImgSize.at(1) * 3] };
	for (int c = 0; c < 3; ++c)
	{
		for (int i = 0; i < iImgSize.at(0); ++i)
		{
			for (int j = 0; j < iImgSize.at(1); ++j)
			{
				_oImg.get()[(c * iImgSize.at(0) + i) * iImgSize.at(1) + j] = oImg.at<cv::Vec3f>(i, j)[c];
			}
		}
	}
	return _oImg;
}




// void YOLOv8_seg::letterbox(const cv::Mat& image, cv::Mat& out, cv::Size& size)
std::unique_ptr<float> prc::PROCESS::preprocess3(const std::string& filename) const
{
	cv::Size size = { 640, 640 };
	cv::Mat image = __read(filename);
	cv::Mat out;
    const float inp_h  = size.height;
    const float inp_w  = size.width;
    float       height = image.rows;
    float       width  = image.cols;

    float r    = std::min(inp_h / height, inp_w / width);
    int   padw = std::round(width * r);
    int   padh = std::round(height * r);

    cv::Mat tmp;
    if ((int)width != padw || (int)height != padh) {
        cv::resize(image, tmp, cv::Size(padw, padh));
    }
    else {
        tmp = image.clone();
    }

    float dw = inp_w - padw;
    float dh = inp_h - padh;

    dw /= 2.0f;
    dh /= 2.0f;
    int top    = int(std::round(dh - 0.1f));
    int bottom = int(std::round(dh + 0.1f));
    int left   = int(std::round(dw - 0.1f));
    int right  = int(std::round(dw + 0.1f));

    cv::copyMakeBorder(tmp, tmp, top, bottom, left, right, cv::BORDER_CONSTANT, {114, 114, 114});

    cv::dnn::blobFromImage(tmp, out, 1 / 255.f, cv::Size(), cv::Scalar(0, 0, 0), true, false, CV_32F);

	std::unique_ptr<float> ptr(new float[out.total() * out.elemSize() / sizeof(float)]);

	std::memcpy(ptr.get(), reinterpret_cast<float*>(out.data), out.total() * out.elemSize());

	return ptr;
}



cv::Mat prc::PROCESS::__read(const std::string& name) const
{
	cv::Mat read_img = cv::imread(name, cv::IMREAD_COLOR);
	if (read_img.empty()) {
		std::cerr << "Error: Image not loaded." << std::endl;
		std::exit(EXIT_FAILURE); // 立即终止程序，不返回到调用者
	}
	return read_img;
}




