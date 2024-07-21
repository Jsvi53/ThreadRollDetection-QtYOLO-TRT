#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>
#include <memory>
#include <opencv2/opencv.hpp>
#include "process.h"
#include "yolov8-seg.hpp"
#include "infer.h"


#include "logging.h"
#include "NvInfer.h"
#include "cuda_runtime_api.h"
#include "NvInferPlugin.h"
#include "util.h"

#include "yolowindow.h"
#include "qswitchbutton.h"
#include <QGuiApplication>
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QQmlApplicationEngine>



extern const std::vector<std::string> CLASS_NAMES;
extern const std::vector<std::vector<unsigned int>> COLORS;
extern const std::vector<std::vector<unsigned int>> MASK_COLORS;
