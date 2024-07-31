# ThreadRollDetection-QtYOLO-TRT
A Qt-based thread roll detection and segmentation project integrating YOLO and TensorRT (TRT). This project can handle video, image, local camera, and remote camera inputs to achieve efficient detection and segmentation of thread roll images.

![alt text](assets/demo.png#pic_center)


## Requirement
1. Qt 6.8
2. OpenCV 4.10.0
3. TensorRT 8.5.17
4. CUDA 10.2


## Build Kits
1. Microsoft Visual Studio 2022
2. CMake ≥ 2.8
3. Ninja

## Build
```bash
cd ThreadRollDetection-QtYOLO-TRT
cmake -B build -G Ninja
cd build
Ninja -j4
```

## Usage
1. Open `THREAD_YOLO_RT_VSCODE.exe`, and select detection pattern, e.g. `图像`;
2. Select a file or a file directory to be detected;
3. Select a yolov8 engine of tensorrt, e.g. `best_sim.engine`，and click the `加载模型` to load the model;
4. Click `开始检测` to start the detection task.
