#include "yolowindow.h"
#include "./ui_yolowindow.h"
#include <QDebug>

cv::Size YOLO_SIZE = cv::Size{640, 640};

CMyFileDialog::CMyFileDialog(QWidget *parent) : QFileDialog(parent) {}

void CMyFileDialog::slot_myAccetp()
{
    QDialog::accept();
}


VIDEOTHREAD::VIDEOTHREAD(QObject *parent) : QThread(parent)
{
    connect(this, &VIDEOTHREAD::taskDone, this, &VIDEOTHREAD::deleteLater);
}


VIDEOTHREAD::~VIDEOTHREAD()
{
    this->quit(); // quit thread
    this->wait(); // wait for thread to finish
}


void VIDEOTHREAD::continueInfer()
{
    isPaused = false;
    pauseCondition.wakeOne(); // 唤醒等待的线程
}


void VIDEOTHREAD::pauseInfer()
{
    isPaused = true;
}


void VIDEOTHREAD::stopInfer()
{
    __stopRequested = 1;
}


void VIDEOTHREAD ::backRun()
{
    int currentFrame = cap.get(cv::CAP_PROP_POS_FRAMES);
    std::cout << currentFrame << std::endl;
    // 检查是否已经在视频的开始
    if (currentFrame <= 1)
    {
        return;
    }
    if (__stopRequested == 0)
    {
        cap.set(cv::CAP_PROP_POS_FRAMES, currentFrame - 15);
    }
}


void VIDEOTHREAD::run()
{
    cap.open(v_path);
    if (!cap.isOpened())
    {
        QMessageBox::warning(v_board, tr("Warning"), tr("can not open the video!"));
        return;
    }

    while (cap.read(v_image))
    {
        if (__stopRequested == 1)
        {
            return;
        }
        if (isPaused)
        {
            QMutexLocker locker(&mutex);
            pauseCondition.wait(&mutex); // 等待直到继续处理
        }

        v_objs.clear();
        v_yolov8->copy_from_Mat(v_image, YOLO_SIZE);
        auto start = std::chrono::system_clock::now();
        std::cout << v_yolov8->modelState << std::endl;
        v_yolov8->infer();
        auto end = std::chrono::system_clock::now();
        v_yolov8->postprocess(v_objs, YOLO_SCORE_THRES, YOLO_IOU_THRES, YOLO_TOPK, YOLO_SEG_CHANNELS, YOLO_SEG_H, YOLO_SEG_W);
        v_yolov8->draw_objects(v_image, v_res, v_objs, CLASS_NAMES, COLORS, MASK_COLORS);
        auto tc = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.;
        v_tc = tc;
        emit videoDone(v_image, v_res, v_objs, v_tc, true);
        QThread::msleep(3);
    }
    emit taskDone();
}


CAMERAThread::CAMERAThread(QObject *parent, YOLO *t_yolo, std::vector<Object> *t_objs) : QThread(parent), stopped(false)
{
    connect(this, &CAMERAThread::taskDone, this, &CAMERAThread::deleteLater);
}


CAMERAThread::~CAMERAThread()
{
    this->quit(); // quit thread
    this->wait(); // wait for thread to finish
}


void CAMERAThread::run()
{
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        QMessageBox::warning(c_board, tr("warning"), tr("Can not open camera"));
        return;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    for (;;)
    {
        mutex.lock();
        if (stopped)
        {
            condition.wait(&mutex);
        }
        mutex.unlock();

        cap >> *c_image;
        if ((*c_image).empty())
        {
            continue;
        }
        c_objs->clear();
        c_yolov8->copy_from_Mat(*c_image, YOLO_SIZE);
        auto start = std::chrono::system_clock::now();
        std::cout << c_yolov8->modelState << std::endl;
        c_yolov8->infer();
        auto end = std::chrono::system_clock::now();
        c_yolov8->postprocess(*c_objs, YOLO_SCORE_THRES, YOLO_IOU_THRES, YOLO_TOPK, YOLO_SEG_CHANNELS, YOLO_SEG_H, YOLO_SEG_W);
        c_yolov8->draw_objects(*c_image, *c_res, *c_objs, CLASS_NAMES, COLORS, MASK_COLORS);
        *c_tc = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.;
        emit cameraDone();
        QThread::msleep(3);
    }
    taskDone();
}


void CAMERAThread::stop()
{
    QMutexLocker locker(&mutex);
    stopped = true;
}


void CAMERAThread::restart()
{
    QMutexLocker locker(&mutex);
    stopped = false;
    condition.wakeAll();
}


SOCKETCAMERATHREAD::SOCKETCAMERATHREAD(YOLOWINDOW *parent) : y_parent(parent), stopped(false)
{
    connect(this, &SOCKETCAMERATHREAD::taskDone, this, &SOCKETCAMERATHREAD::deleteLater);
}


SOCKETCAMERATHREAD::~SOCKETCAMERATHREAD()
{
    this->quit(); // quit thread
    this->wait(); // wait for thread to finish
}


void SOCKETCAMERATHREAD::run()
{
    const char *message = "Begin to transmit images!";
    y_parent->client.writeDatagram(message, strlen(message), y_parent->__ip,  y_parent->__port);
    while (true)
    {   decodedimg.clear();
        y_parent->image = cv::Mat();
        mutex.lock();
        if (stopped)
        {   if(semaphore != 1)
            {
                semaphore = 1;
            }
            condition.wait(&mutex);
        }
        mutex.unlock();

        if(semaphore == 1)
        {
            y_parent->client.writeDatagram(message, strlen(message), y_parent->__ip,  y_parent->__port);
        }

        while (y_parent->client.hasPendingDatagrams())
        {
            QNetworkDatagram datagram = y_parent->client.receiveDatagram();
            QByteArray data = datagram.data();
            if (data.size() == 0 || std::string(data, data.size()).find("END_OF_IMAGE") != std::string::npos)
            {
                break;
            }
            decodedimg.insert(decodedimg.end(), data.begin(), data.end());
        }

        if (!decodedimg.empty())
        {
            y_parent->image = cv::imdecode(decodedimg, cv::IMREAD_COLOR);
            if (y_parent->image.empty())
            {
                std::cerr << "Error: Image decoding failed." << std::endl;
                continue;
            }
            qDebug() << "decodedimg size:" << decodedimg.size();
        }
        else
        {
            std::cerr << "Error: decodedimg is empty." << std::endl;
            continue;
        }

        // Check if the image data is continuous
        if (!y_parent->image.isContinuous())
        {
            std::cerr << "Warning: Image data is not continuous." << std::endl;
        }

        y_parent->yolov8->copy_from_Mat(y_parent->image, YOLO_SIZE);
        auto start = std::chrono::system_clock::now();
        y_parent->yolov8->infer();
        auto end = std::chrono::system_clock::now();
        y_parent->yolov8->postprocess(y_parent->objs, YOLO_SCORE_THRES, YOLO_IOU_THRES, YOLO_TOPK, YOLO_SEG_CHANNELS, YOLO_SEG_H, YOLO_SEG_W);
        y_parent->yolov8->draw_objects(y_parent->image, y_parent->res, y_parent->objs, CLASS_NAMES, COLORS, MASK_COLORS);
        y_parent->__tc = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.;

        emit socketCameraDone();
        QThread::msleep(50);
    }
    taskDone();
}


void SOCKETCAMERATHREAD::stop()
{
    QMutexLocker locker(&mutex);
    stopped = true;
}


void SOCKETCAMERATHREAD::restart()
{
    QMutexLocker locker(&mutex);
    stopped = false;
    condition.wakeAll();
}


YOLOWINDOW::YOLOWINDOW(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::YOLOWINDOW)
{
    ui->setupUi(this);
    // 创建一个按钮组
    QButtonGroup *buttonGroup = new QButtonGroup(this);
    // 将所有单选按钮添加到按钮组中
    buttonGroup->addButton(ui->radioButton);
    buttonGroup->addButton(ui->radioButton_2);
    buttonGroup->addButton(ui->radioButton_3);
    buttonGroup->addButton(ui->radioButton_4);
    // 设置按钮组为互斥
    buttonGroup->setExclusive(true);

    scene = new QGraphicsScene(this);
    camera_mode_scene = new QGraphicsScene(this);
    ui->graphicsView_2->setScene(camera_mode_scene);
    ui->graphicsView->setScene(scene);
    checkCameraAvailability();

    __videoThread = new VIDEOTHREAD();
    __cameraThread = new CAMERAThread();
    __cameraThread->c_objs = &objs;
    __cameraThread->c_tc = &__tc;

    __socketCameraThread = new SOCKETCAMERATHREAD(this);

    // initiate switch button
    switchButton = new SwitchButton(ui->camera);
    switchButton->move(10, 40);
    switchButton->resize(50, 21);

    // initiate label_raw
    label_raw = new QLabel(ui->Visualization->widget(0));
    label_raw->move(0, 0);
    label_raw->resize(640, 480);
    label_res = new QLabel(ui->Visualization->widget(1));
    label_res->move(0, 0);
    label_res->resize(640, 480);

    __timer = new QTimer(this);
    // connect timeout signal to timerTimeout signal
    connect(__timer, &QTimer::timeout, this, [=]()
            {
                emit timerTimeout(true); // emit timerTimeout signal
            });
    // connect timer timeout signal to pressBtnSlot
    connect(this, &YOLOWINDOW::timerTimeout, this, &YOLOWINDOW::pressBtnSlot);

    // video
    connect(__videoThread, &VIDEOTHREAD::videoDone, this, [=](cv::Mat imageReceived, cv::Mat resReceived, std::vector<Object> objsReceived, double tcReceived, bool finishedReceived)
            {
                // update image, res, objs, __tc
                image = imageReceived;
                res = resReceived;
                objs = objsReceived;
                __tc = tcReceived;
                Mat2QPixmap();
                c_displayImage();
                graphicDisplay();
            });

    // local camera
    connect(__cameraThread, &CAMERAThread::cameraDone, this, [=]()
    {
        Mat2QPixmap();
        c_displayImage();
        graphicDisplay();
    });
    // remote camera
    connect(__socketCameraThread, &SOCKETCAMERATHREAD::socketCameraDone, this, [=]()
    {
        Mat2QPixmap();
        c_displayImage();
        graphicDisplay();
    });
}


YOLOWINDOW::~YOLOWINDOW()
{
    delete ui;
    delete switchButton;
    delete yolov8;
    delete __timer;
    delete label_raw;
    delete label_res;
    delete __videoThread;
    delete __cameraThread;
    client.close();
    delete __socketCameraThread;
}


void YOLOWINDOW::on_toolButton_clicked()
{
    CMyFileDialog *dialog = new CMyFileDialog(this);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);

    // 支持多选
    // QListView *listView = dialog->findChild<QListView*>("listView");
    // if (listView)
    //     listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // QTreeView *treeView = dialog->findChild<QTreeView*>();

    // if (treeView)
    //     treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QDialogButtonBox *button = dialog->findChild<QDialogButtonBox *>("buttonBox");
    disconnect(button, SIGNAL(accepted()), dialog, SLOT(accept()));     // 使链接失效
    connect(button, SIGNAL(accepted()), dialog, SLOT(slot_myAccetp())); // 改成自己的槽
    if (dialog->exec() == QDialog::Accepted)
    {
        ui->lineEdit->setText(dialog->selectedFiles().join(";"));
    }
}


void YOLOWINDOW::on_toolButton_2_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Engine File"), "", tr("Files (*.engine *.trt)"));
    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        // display name of the file with dir
        ui->lineEdit_2->setText(fileName);
    }
}


void YOLOWINDOW::on_pushButton_clicked()
{
    __yoloEnableFlag = false;
    QString modelFilePath = ui->lineEdit_2->text(); // 获取lineEdit_2的文本
    QFileInfo checkFile(modelFilePath);
    // 检查文件路径的扩展名是否为.engine或.trt
    if (checkFile.exists() && checkFile.isFile() &&
        (checkFile.suffix() == "engine" || checkFile.suffix() == "trt"))
    {
        yolov8 = new YOLO(modelFilePath.toStdString()); // 实例化YOLO对象
        yolov8->make_pipe(true);
        // 在terminal， a QTextBrowser中显示模型加载进度
        ui->terminal->append("Loading engine file...");
        if (yolov8->modelState)
        {
            ui->terminal->append("Engine file loaded successfully!");
            __yoloEnableFlag = true;
        }
    }
    else
    {
        // warning box
        QMessageBox::warning(this, tr("Warning"), tr("Invalid engine file path!"));
    }
}


void YOLOWINDOW::on_pushButton_start_clicked()
{
    windowAllStop();
    windowModeChecked();
    inputConfig();

    // video
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone)
    {   setAllButtonsEnabled(true);
        __videoThread->__stopRequested = 0;
        __videoThread->continueInfer();
        ui->terminal->append("\n-------------------------Vedio start inference! -------------------------\n");
        // 将forwardInfer(path); 放到子线程中，避免主线程阻塞
        __videoThread->v_path = path;
        __videoThread->v_yolov8 = yolov8;
        __videoThread->start();
        __windowState = WINDOW_STATE::VIDEORUNNING;
    }

    // image mode
    if(__windowMode == WINDOW_MODE::IMAGEMODE && configDone)
    {   setAllButtonsEnabled(true);
        ui->terminal->append("\n-------------------------Image start inference! -------------------------\n");
        QStringList buttonNames = {"pushButton", "pushButton_start", "pushButton_back", "pushButton_continue", "pushButton_save"};
        setSpecificButtonsEnabled(buttonNames, true);

        // cv::imshow("tetst", cv::imread(imagePathList[__traverseIndex]));
        // cv::waitKey(0);
        __traverseIndex = 0;
        forwardInfer(__traverseIndex);
        Mat2QPixmap();
        //cv::imshow("tset", image);
        __windowState = WINDOW_STATE::IMAGERUNNING;
        displayImage();
        graphicDisplay();
    }

    // local camera
    if (__windowMode == WINDOW_MODE::LOCALCAMERAMODE && configDone)
    {
        setAllButtonsEnabled(true);
        if(__cameraThreadState == CAMERATHREAD_FLAGS::RESTARTED)
        {
            __cameraThread->restart();
            __cameraThreadState = CAMERATHREAD_FLAGS::RUNNING;
            ui->terminal->append("\n------------------------- Camera restart inference! -------------------------\n");
            QStringList buttonNames = {"pushButton", "pushButton_start", "pushButton_stop"};
            setSpecificButtonsEnabled(buttonNames, true);
            return;
        }
        // update camera parameters
        __cameraThread->c_yolov8 = yolov8;
        __cameraThread->c_image = &image;
        __cameraThread->c_res = &res;
        __cameraThread->start();

        ui->terminal->append("\n------------------------- Camera start inference! -------------------------\n");
        QStringList buttonNames = {"pushButton", "pushButton_start",  "pushButton_stop"};
        setSpecificButtonsEnabled(buttonNames, true);
        __cameraThreadState = CAMERATHREAD_FLAGS::RUNNING;
        __windowState = WINDOW_STATE::LOCALCAMERARUNNING;
    }

    // remote camera
    if(__windowMode == WINDOW_MODE::REMOTECAMERAMODE && configDone)
    {   setAllButtonsEnabled(true);
        QStringList buttonNames = {"pushButton", "pushButton_start", "pushButton_stop"};
        setSpecificButtonsEnabled(buttonNames, true);
        if(__socketCameraThreadState == CAMERATHREAD_FLAGS::RESTARTED)
        {
            __socketCameraThread->restart();
            __socketCameraThreadState = CAMERATHREAD_FLAGS::RUNNING;
            ui->terminal->append("\n------------------------- Camera restart inference! -------------------------\n");
            QStringList buttonNames = {"pushButton", "pushButton_start", "pushButton_stop"};
            setSpecificButtonsEnabled(buttonNames, true);
            return;
        }
        client.connectToHost(__ip, __port);

        if (!client.waitForConnected(5000))
        { // 等待5秒钟以尝试连接
            QMessageBox::warning(nullptr, "Connection Failed", "Could not connect to host: " + client.errorString());
            return;
        }
        ui->terminal->append("\n------------------------- Camera start inference! -------------------------\n");
        __socketCameraThread->start();
        __socketCameraThreadState = CAMERATHREAD_FLAGS::RUNNING;
        __windowState = WINDOW_STATE::REMOTECAMERARUNNING;
    }
}


void YOLOWINDOW::on_pushButton_back_clicked()
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING)
    {
        if (__videoThread->__stopRequested == 1)
        {
            ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
            return;
        }
        __videoThread->backRun();
        ui->terminal->append("\n------------------------- Backward 15 frames! -------------------------\n");
    }

    // Image mode
    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        if (__traverseIndex > 0)
        {
            forwardInfer(--__traverseIndex);
            Mat2QPixmap();
            displayImage();
            graphicDisplay();
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("This is the first image!"));
        }
    }
}

// 在源文件 yolowindow.cpp 中实现这两个槽函数
void YOLOWINDOW::on_pushButton_back_pressed()
{
    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        connect(this, &YOLOWINDOW::timerTimeout, this, &YOLOWINDOW::pressBtnSlot);
        __timer->start(150); // 每隔100ms触发一次
    }
}


void YOLOWINDOW::on_pushButton_back_released()
{
    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        __timer->stop();
    }
}


void YOLOWINDOW::on_pushButton_continue_clicked()
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEOPAUSED)
    {
        if (__videoThread->__stopRequested == 1)
        {
            ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
            return;
        }

        __videoThread->continueInfer();
        ui->terminal->append("\n------------------------- Continue inference! -------------------------\n");
        return;
    }

    // image mode
    if(__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        if (__traverseIndex < imagePathList.size() - 1)
        {
            forwardInfer(++__traverseIndex);
            Mat2QPixmap();
            displayImage();
            graphicDisplay();
            return;
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("This is the last image!"));
            return;
        }
    }
}

// 在源文件 yolowindow.cpp 中实现这两个槽函数
void YOLOWINDOW::on_pushButton_continue_pressed()
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING)
    {
        if (__videoThread->__stopRequested == 1)
        {
            ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
            return;
        }
    }

    // image mode
    if(__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        connect(this, &YOLOWINDOW::timerTimeout, this, &YOLOWINDOW::pressBtnSlot);
        __timer->start(150); // 每隔100ms触发一次
        return;
    }
}


void YOLOWINDOW::on_pushButton_continue_released()
{
    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        __timer->stop();
    }
}


void YOLOWINDOW::on_pushButton_pause_clicked()
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING && __videoThread->__stopRequested == 1)
    {
        ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
        return;
    }

    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING)
    {
        __videoThread->pauseInfer();
        ui->terminal->append("\n------------------------- Pause inference! -------------------------\n");
        __windowState = WINDOW_STATE::VIDEOPAUSED;
        return;
    }
}


void YOLOWINDOW::on_pushButton_stop_clicked()
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING)
    {
        __videoThread->stopInfer();
        label_raw->clear();
        label_res->clear();
        ui->terminal->append("\n------------------------- Stop inference! -------------------------\n");
        return;
    }

    // local camera mode
    if(__windowMode == WINDOW_MODE::LOCALCAMERAMODE && configDone && __windowState == WINDOW_STATE::LOCALCAMERARUNNING)
    {
        __cameraThread->stop();
        __cameraThreadState = CAMERATHREAD_FLAGS::RESTARTED;
        label_raw->clear();
        label_res->clear();
        ui->label->clear();
        ui->label_2->clear();
        ui->terminal->append("\n------------------------- Stop inference! -------------------------\n");
        return;
    }

    // remote camera mode
   if(__windowMode == WINDOW_MODE::REMOTECAMERAMODE && configDone && __windowState == WINDOW_STATE::REMOTECAMERARUNNING)
   {
       __socketCameraThread->stop();
       __socketCameraThreadState = CAMERATHREAD_FLAGS::RESTARTED;
       label_raw->clear();
       label_res->clear();
       ui->label->clear();
       ui->label_2->clear();
       ui->terminal->append("\n------------------------- Stop inference! -------------------------\n");
       return;
   }
}


void YOLOWINDOW::forwardInfer(int itemIndex)
{
    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone)
    {
        objs.clear();
        __tc = 0;
        image = cv::imread(imagePathList[itemIndex]);
        yolov8->copy_from_Mat(image, YOLO_SIZE);
        auto start = std::chrono::system_clock::now();
        yolov8->infer(); // forward inference
        auto end = std::chrono::system_clock::now();
        yolov8->postprocess(objs, YOLO_SCORE_THRES, YOLO_IOU_THRES, YOLO_TOPK, YOLO_SEG_CHANNELS, YOLO_SEG_H, YOLO_SEG_W);
        yolov8->draw_objects(image, res, objs, CLASS_NAMES, COLORS, MASK_COLORS);
        auto tc = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.;
        __tc = tc;
    }
}


void YOLOWINDOW::pressBtnSlot(bool direction)
{
    // video mode
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone && __windowState == WINDOW_STATE::VIDEORUNNING)
    {
        // pass
    }

    // image mode
    if(__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        // 向前infer
        if (direction == true)
        {
            if (__traverseIndex == imagePathList.size() - 1)
            {
                QMessageBox::warning(this, tr("Warning"), tr("This is the last image!"));
            }
            else
            {
                forwardInfer(++__traverseIndex);
                Mat2QPixmap();
                displayImage();
                graphicDisplay();
            }
        }
        else
        {
            if (__traverseIndex == 0)
            {
                QMessageBox::warning(this, tr("Warning"), tr("This is the first image!"));
            }
            else
            {
                forwardInfer(--__traverseIndex);
                Mat2QPixmap();
                displayImage();
                graphicDisplay();
            }
        }
    }
}


void YOLOWINDOW::on_pushButton_save_clicked()
{
    if (__windowMode == WINDOW_MODE::VIDEOMODE && configDone)
    {
        if(__windowState == WINDOW_STATE::VIDEORUNNING)
        {
            __videoThread->pauseInfer();
            __windowState = WINDOW_STATE::VIDEOPAUSED;
        }
        QString savePath = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.jpg)"));
        if (savePath.isEmpty())
        {
            return;
        }
        else
        {
            cv::imwrite(savePath.toStdString(), res);
        }
    }

    if (__windowMode == WINDOW_MODE::IMAGEMODE && configDone && __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        QString savePath = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.jpg)"));
        if (savePath.isEmpty())
        {
            return;
        }
        else
        {
            cv::imwrite(savePath.toStdString(), res);
        }
    }
}


void YOLOWINDOW::displayImage()
{
    if (pixmapRes.isNull() || pixmapRaw.isNull())
    {
        qDebug() << "pixmapRes or pixmapRaw is null";
        return;
    }
    else
    {
        label_res->setPixmap(pixmapRes);
        label_raw->setPixmap(pixmapRaw);
        ui->terminal->append(QString("%1    cost %2 ms").arg(QString::fromStdString(imagePathList[__traverseIndex])).arg(__tc, 2, 'f', 4));
    }
}


void YOLOWINDOW::c_displayImage()
{
    if (pixmapRes.isNull() || pixmapRaw.isNull())
    {
        qDebug() << "pixmapRes or pixmapRaw is null";
        return;
    }
    else
    {
        label_res->setPixmap(pixmapRes);
        label_raw->setPixmap(pixmapRaw);
        ui->terminal->append(QString("cost %1 ms").arg(__tc, 2, 'f', 4));
    }
}


void YOLOWINDOW::graphicDisplay()
{
    if (pixmapRaw.isNull()) {
        qDebug() << "pixmapRaw is null";
        return;
    }

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmapRaw);     // 创建QGraphicsPixmapItem

    // image
    if( __windowState == WINDOW_STATE::IMAGERUNNING || __windowState == WINDOW_STATE::VIDEORUNNING )
    {
        ui->graphicsView->scene()->clear();
        ui->graphicsView->scene()->addItem(item);
        ui->graphicsView->fitInView(item, Qt::KeepAspectRatio); // 缩放图像以适应 graphicsView的大小
        ui->graphicsView->update();                             // 确保UI进行刷新
        ui->graphicsView->show();
    }

    // camera
    if( __windowState == WINDOW_STATE::LOCALCAMERARUNNING || __windowState == WINDOW_STATE::REMOTECAMERARUNNING)
    {
        ui->graphicsView_2->scene()->clear();
        ui->graphicsView_2->scene()->addItem(item);

        // 设置 QGraphicsView 的场景矩形为项目的边界矩形
        ui->graphicsView_2->setSceneRect(item->boundingRect());
        // 将图像缩放到适应 QGraphicsView 的大小，并保持纵横比
        ui->graphicsView_2->fitInView(item, Qt::KeepAspectRatio);
        // 禁用滚动条
        ui->graphicsView_2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->graphicsView_2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // 确保图像填满整个视图
        ui->graphicsView_2->setRenderHint(QPainter::SmoothPixmapTransform, true);
        // 更新和显示视图
        ui->graphicsView_2->update();
        ui->graphicsView_2->show();
    }
}


void YOLOWINDOW::Mat2QPixmap()
{
    if (res.empty() && image.empty())
    {
        qDebug() << "image or res data is empty!";
        return;
    }
    else
    {
        cv::cvtColor(res, res, cv::COLOR_BGR2RGB);
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // interfere inferring ?
        QimgRes = QImage((const unsigned char *)(res.data), res.cols, res.rows, res.step, QImage::Format_RGB888);
        QimgRaw = QImage((const unsigned char *)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
        pixmapRes = QPixmap::fromImage(QimgRes);
        pixmapRaw = QPixmap::fromImage(QimgRaw);
    }
}


void YOLOWINDOW::windowModeChecked()
{
    if (ui->radioButton->isChecked()){__windowMode = WINDOW_MODE::IMAGEMODE;}
    if (ui->radioButton_2->isChecked()){__windowMode = WINDOW_MODE::VIDEOMODE;}
    if (ui->radioButton_3->isChecked()){__windowMode = WINDOW_MODE::LOCALCAMERAMODE;}
    if (ui->radioButton_4->isChecked()){__windowMode = WINDOW_MODE::REMOTECAMERAMODE;}
}


void YOLOWINDOW::windowAllStop()
{
    if( __windowState == WINDOW_STATE::IMAGERUNNING)
    {
        __windowState = WINDOW_STATE::WINDOWSTOPPED;
    }
    if (__windowState == WINDOW_STATE::VIDEORUNNING)
    {
        __videoThread->stopInfer();
        label_raw->clear();
        label_res->clear();
        __windowState = WINDOW_STATE::WINDOWSTOPPED;
    }

    if (__windowState == WINDOW_STATE::LOCALCAMERARUNNING)
    {
        __cameraThread->stop();
        label_raw->clear();
        label_res->clear();
        ui->label->clear();
        ui->label_2->clear();
        __windowState = WINDOW_STATE::WINDOWSTOPPED;
    }

    if (__windowState == WINDOW_STATE::REMOTECAMERARUNNING)
    {
       __socketCameraThread->stop();
       label_raw->clear();
       label_res->clear();
       ui->label->clear();
       ui->label_2->clear();
       __windowState = WINDOW_STATE::WINDOWSTOPPED;
   }
}


void YOLOWINDOW::inputConfig()
{
    if(__windowState != WINDOW_STATE::WINDOWSTOPPED){ return; }
    // clear image data
    __yoloDataFlag = false;

    // clear ip
    path.clear();
    __ip.clear();
    __port = 0;
    // clear engine file
    __engineFile.clear();
    __engineFileFlag = false;
    __engineFile = ui->lineEdit_2->text().toStdString();
    // clear config
    configDone = false;

    if(__windowMode == WINDOW_MODE::IMAGEMODE)
    {
        imagePathList.clear();
        path = ui->lineEdit->text().toStdString();
        if(IsFile(path))
        {
            std::string suffix = path.substr(path.find_last_of('.') + 1);
            if (suffix == "jpg" || suffix == "jpeg" || suffix == "png")
            {
                imagePathList.push_back(path);
                __yoloDataFlag = true;
            }
            else
            {
                QMessageBox::warning(this, tr("Warning"), tr("Invalid image file path!"));
                return;
            }
        }
        else if (IsFolder(path))
        {
            cv::glob(path + "/*.jpg", imagePathList);
            if (imagePathList.size() == 0)
            {
                QMessageBox::warning(this, tr("Warning"), tr("No image files in the folder!"));
                return;
            }
            else
            {
                __yoloDataFlag = true;
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Invalid file path!"));
            return;
        }
    }

    if(__windowMode == WINDOW_MODE::VIDEOMODE)
    {
        path = ui->lineEdit->text().toStdString();
        if (IsFile(path))
        {
            std::string suffix = path.substr(path.find_last_of('.') + 1);
            if (suffix == "mp4" || suffix == "avi" || suffix == "m4v" || suffix == "mpeg" || suffix == "mov" || suffix == "mkv")
            {
                __yoloDataFlag = true;
            }
            else
            {
                QMessageBox::warning(this, tr("Warning"), tr("Invalid video file path!"));
                return;
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Invalid video file path!"));
            return;
        }
    }

    if(__windowMode == WINDOW_MODE::LOCALCAMERAMODE)
    {
        __yoloDataFlag = true;
    }

    if(__windowMode == WINDOW_MODE::REMOTECAMERAMODE)
    {
        const std::string ip = ui->lineEdit_3->text().toStdString();
        if(v_utils::ipFormatChecked(ip))
        {
            QStringList __addressParser = ui->lineEdit_3->text().split(":");
            __ip = QHostAddress(__addressParser.at(0));
            __port = __addressParser.at(1).toULongLong();
            __yoloDataFlag = true;
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Invalid IP address!"));
            return;
        }
    }

    if(v_utils::engineTypeChecked(__engineFile))
    {
        __engineFileFlag = true;
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("Invalid engine file path!"));
        return;
    }

    if(__yoloDataFlag && __engineFileFlag)
    {
        configDone = true;
    }
}

void YOLOWINDOW::checkCameraAvailability()
{
    QIcon cameraOnIcon(":/menu/USB_Camera_btn_online.svg");      // :表示资源文件路径，/menuResource是资源文件所在文件夹的相对路径
    QIcon cameraOffIcon(":/menu/USB_Camera_btn_offline.svg");

    if (QMediaDevices::videoInputs().count() > 0)
    {
        ui->actionUSB_Camera->setIcon(cameraOnIcon);
    }
    else
    {
        ui->actionUSB_Camera->setIcon(cameraOffIcon);
    }

}
/*********************************************** Extra Functions *******************************************************/
void YOLOWINDOW::setSpecificButtonsEnabled(const QStringList &buttonNames, bool enabled)
{ /*
   * 该函数用于设置指定的按钮是否有效
   * buttonNames: 指定的按钮名称
   * enabled: 是否有效
   * e.g.
   *   QStringList buttonNames;
   *   buttonNames << "pushButton" << "pushButton_2";
   *   setSpecificButtonsEnabled(buttonNames, true);
   */

    // 首先，将所有按钮设置为无效
    QList<QPushButton *> list = this->findChildren<QPushButton *>();
    for (int i = 0; i < list.size(); ++i)
    {
        list[i]->setEnabled(false);
    }

    // 然后，将指定的按钮设置为有效
    for (const QString &buttonName : buttonNames)
    {
        QPushButton *button = this->findChild<QPushButton *>(buttonName);
        if (button)
        {
            button->setEnabled(enabled);
        }
    }
}


void YOLOWINDOW::setAllButtonsEnabled(bool enabled)
{ /*
   * 该函数用于设置所有按钮是否有效
   * enabled: 是否有效
   */
    QList<QPushButton *> list = this->findChildren<QPushButton *>();
    for (int i = 0; i < list.size(); ++i)
    {
        list[i]->setEnabled(enabled);
    }
}


QImage YOLOWINDOW::Mat2QImage(cv::Mat const &src)
{                                           /*
                                             * 该函数用于将cv::Mat转换为QImage
                                             * src: cv::Mat
                                             * return: QImage
                                             */
    cv::Mat temp;                           // make the same cv::Mat
    cvtColor(src, temp, cv::COLOR_BGR2RGB); // cvtColor Makes a copt, that what i need
    QImage dest((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    dest.bits(); // enforce deep copy, see documentation
    return dest;
}
