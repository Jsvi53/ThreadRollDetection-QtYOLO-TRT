#ifndef INFER_H
#define INFER_H

#include <fstream>
#include <opencv2/opencv.hpp>
#include "NvInferPlugin.h"
#include "util.h"
#include "common.hpp"

using namespace seg;

class YOLO
{
public:
    explicit YOLO(const std::string& engine_file_path);
    ~YOLO();
    void                                        copy_from_Mat(const cv::Mat& image);
    void                                        copy_from_Mat(const cv::Mat& image, cv::Size& size);
    void                                        make_pipe(bool warmup = true);
    void                                        infer(void);
    void                                        letterbox(const cv::Mat& image, cv::Mat& out, cv::Size& size);
    void                                        postprocess(std::vector<Object>& objs,
                                                            float                score_thres  = 0.25f,
                                                            float                iou_thres    = 0.65f,
                                                            int                  topk         = 100,
                                                            int                  seg_channels = 32,
                                                            int                  seg_h        = 160,
                                                            int                  seg_w        = 160);
    static void                                 draw_objects(const cv::Mat&                               image,
                                                            cv::Mat&                                      res,
                                                            const std::vector<Object>&                    objs,
                                                            const std::vector<std::string>&               CLASS_NAMES,
                                                            const std::vector<std::vector<unsigned int>>& COLORS,
                                                            const std::vector<std::vector<unsigned int>>& MASK_COLORS);

    int                                          num_bindings;
    int                                          num_inputs  = 0;
    int                                          num_outputs = 0;
    std::vector<seg::Binding>                    input_bindings;
    std::vector<seg::Binding>                    output_bindings;
    std::vector<void *>                          host_ptrs;
    std::vector<void *>                          device_ptrs;

    PreParam                                     pparam;

    bool                                         modelState{false};

private:

    util::UniquePtr<nvinfer1::IRuntime>          runtime = nullptr;
    util::UniquePtr<nvinfer1::ICudaEngine>       mEngine = nullptr;
    util::UniquePtr<nvinfer1::IExecutionContext> context = nullptr;
    cudaStream_t                                 stream  = nullptr;
    Logger                                       gLogger{nvinfer1::ILogger::Severity::kERROR};
};


#endif