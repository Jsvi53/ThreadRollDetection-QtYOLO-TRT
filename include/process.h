#ifndef PROCESS_H
#define PROCESS_H
#include <iostream>
#include <opencv2/opencv.hpp>

void test(void);

namespace prc
{
	class PROCESS
	{
	public:
		std::string orgName;
		std::string _outPath;
		const std::vector<float> mean{ 0.485f, 0.456f, 0.406f };
		const std::vector<float> stddev{ 0.229f, 0.224f, 0.225f };

		PROCESS(const std::string& outpath) :_outPath(outpath) {};
		std::unique_ptr<float> preprocess(const std::string& filename) const;
		std::unique_ptr<float> preprocess2(const std::string& filename) const;
		std::unique_ptr<float> preprocess3(const std::string& filename) const;
		std::unique_ptr<float> postprocess() const;

	protected:
		cv::Mat org;
		cv::Mat inp;
		cv::Mat out;

	private:
		cv::Mat __read(const std::string& name) const;
	};	// class PROCESS

}; // namespace prc


#endif

