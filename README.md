# [ThreadRollDetection-QtYOLO-TRT](https://github.com/Jsvi53/ThreadRollDetection-QtYOLO-TRT.git)
A Qt-based thread roll detection and segmentation project integrating YOLO and TensorRT (TRT). This project can handle video, image, local camera, and remote camera inputs to achieve efficient detection and segmentation of thread roll images.


### 输出
```python
if isinstance(self.model, SegmentationModel):
    dynamic["output0"] = {0: "batch", 2: "anchors"}  # shape(1, 116, 8400)
    dynamic["output1"] = {0: "batch", 2: "mask_height", 3: "mask_width"}  # shape(1,32,160,160)
elif isinstance(self.model, DetectionModel):
    dynamic["output0"] = {0: "batch", 2: "anchors"}  # shape(1, 84, 8400)

    https://github.com/Jsvi53/ThreadRollDetection-QtYOLO-TRT.git
```