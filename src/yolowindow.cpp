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

CAMERAThread::CAMERAThread(QObject *parent, YOLO *t_yolo, std::vector<Object> *t_objs) : QThread(parent)
{
    connect(this, &CAMERAThread::cameraDone, this, &CAMERAThread::deleteLater);
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

    }
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

    __videoThread = new VIDEOTHREAD();
    __cameraThread = new CAMERAThread();
    __cameraThread->c_objs = &objs;
    __cameraThread->c_tc = &__tc;

    __engineFile = ui->lineEdit_2->text();
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

    connect(__videoThread, &VIDEOTHREAD::videoDone, this, [=](cv::Mat imageReceived, cv::Mat resReceived, std::vector<Object> objsReceived, double tcReceived, bool finishedReceived)
            { displayImage(imageReceived, resReceived, objsReceived, tcReceived, finishedReceived); });

    connect(__cameraThread, &CAMERAThread::cameraDone, this, [=](){
        c_displayImage();
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
    __yoloDataFlag = false;
    imagePathList.clear();
    __inputFile = ui->lineEdit->text(); // 获取lineEdit_2的文本
    std::string path = __inputFile.toStdString();
    if (IsFile(path))
    {
        std::string suffix = path.substr(path.find_last_of('.') + 1);
        if (suffix == "jpg" || suffix == "jpeg" || suffix == "png")
        {
            imagePathList.push_back(path);
            __yoloDataFlag = true;
        }
        else if (suffix == "mp4" || suffix == "avi" || suffix == "m4v" || suffix == "mpeg" || suffix == "mov" || suffix == "mkv")
        {
            isVideo = true;
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

    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()) && __yoloEnableFlag)
    {
        __videoThread->__stopRequested = 0;
        __videoThread->continueInfer();
        ui->terminal->append("\n-------------------------Vedio start inference! -------------------------\n");
        // 将forwardInfer(path); 放到子线程中，避免主线程阻塞
        __videoThread->v_path = path;
        __videoThread->v_yolov8 = yolov8;
        __videoThread->start();
    }

    if(__yoloDataFlag && (ui->radioButton->isChecked()) && __yoloEnableFlag)
    {
        ui->terminal->append("\n-------------------------Image start inference! -------------------------\n");
        __traverseIndex = 0;
        forwardInfer(__traverseIndex);
        displayImage();
    }

    if (ui->radioButton_3->isChecked() && __yoloEnableFlag)
    {
        __cameraThread->c_yolov8 = yolov8;
        __cameraThread->c_image = &image;
        __cameraThread->c_res = &res;
        ui->terminal->append("\n------------------------- Camera start inference! -------------------------\n");
        // disenable btn lsit
        setSpecificButtonsEnabled({"pushButton_back"}, false);
        __cameraThread->start();
        // initialize camera
    }
}

void YOLOWINDOW::on_pushButton_back_clicked()
{
    if (__videoThread->__stopRequested == 1)
    {
        ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
        return;
    }

    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
        __videoThread->backRun();
        ui->terminal->append("\n------------------------- Backward 15 frames! -------------------------\n");
    }
    else
    {
        if (__traverseIndex > 0)
        {
            forwardInfer(--__traverseIndex);
            displayImage();
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
    connect(this, &YOLOWINDOW::timerTimeout, this, &YOLOWINDOW::pressBtnSlot);
    __timer->start(150); // 每隔100ms触发一次
}

void YOLOWINDOW::on_pushButton_back_released()
{
    __timer->stop();
}

void YOLOWINDOW::on_pushButton_continue_clicked()
{
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
        if (__videoThread->__stopRequested == 1)
        {
            ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
            return;
        }

        __videoThread->continueInfer();
        ui->terminal->append("\n------------------------- Continue inference! -------------------------\n");
    }
    else
    {
        if (__traverseIndex < imagePathList.size() - 1)
        {
            forwardInfer(++__traverseIndex);
            displayImage();
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("This is the last image!"));
        }
    }
}

// 在源文件 yolowindow.cpp 中实现这两个槽函数
void YOLOWINDOW::on_pushButton_continue_pressed()
{
    if (isVideo && __yoloDataFlag)
    {
        if (__videoThread->__stopRequested == 1)
        {
            ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
            return;
        }
    }
    else
    {
        connect(this, &YOLOWINDOW::timerTimeout, this, &YOLOWINDOW::pressBtnSlot);
        __timer->start(150); // 每隔100ms触发一次
    }
}

void YOLOWINDOW::on_pushButton_continue_released()
{
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
    }
    else
    {
        __timer->stop();
    }
}

void YOLOWINDOW::on_pushButton_pause_clicked()
{
    if (__videoThread->__stopRequested == 1)
    {
        ui->terminal->append("\n------------------------- Inference running has been stoped -------------------------\n");
        return;
    }
    __videoThread->pauseInfer();
    ui->terminal->append("\n------------------------- Pause inference! -------------------------\n");
}

void YOLOWINDOW::on_pushButton_stop_clicked()
{
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
        __videoThread->stopInfer();
        label_raw->clear();
        label_res->clear();
        ui->terminal->append("\n------------------------- Stop inference! -------------------------\n");
    }
}

void YOLOWINDOW::forwardInfer(int itemIndex)
{
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
        // pass
    }
    else
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

void YOLOWINDOW::c_forwardInfer(cv::Mat cameraImage)
{
    if (ui->radioButton_3->isChecked() && __yoloEnableFlag)
    {
        objs.clear();
        __tc = 0;
        image = cameraImage;
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

void YOLOWINDOW::forwardInfer(cv::Mat cameraImage)
{
    if (ui->radioButton_3->isChecked() && __yoloEnableFlag)
    {
        objs.clear();
        __tc = 0;
        image = cameraImage;
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
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
    {
        // pass
    }
    else
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
                displayImage();
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
                displayImage();
            }
        }
    }
}

void YOLOWINDOW::on_pushButton_save_clicked()
{
    if (isVideo && __yoloDataFlag && (ui->radioButton_2->isChecked()))
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
    else
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
    if (res.empty() && image.empty())
    {
        return;
    }
    else
    {
        cv::cvtColor(res, res, cv::COLOR_BGR2RGB);
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // interfere inferring ?
        QimgRes = QImage((const unsigned char *)(res.data), res.cols, res.rows, res.step, QImage::Format_RGB888);
        QimgRaw = QImage((const unsigned char *)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
        QPixmap pixmapRes = QPixmap::fromImage(QimgRes);
        QPixmap pixmapRaw = QPixmap::fromImage(QimgRaw);
        label_res->setPixmap(pixmapRes);
        label_raw->setPixmap(pixmapRaw);

        ui->terminal->append(QString("%1    cost %2 ms").arg(QString::fromStdString(imagePathList[__traverseIndex])).arg(__tc, 2, 'f', 4));
    }
}


void YOLOWINDOW::c_displayImage()
{
    if (res.empty() && image.empty())
    {
        return;
    }
    else
    {
        cv::cvtColor(res, res, cv::COLOR_BGR2RGB);
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // interfere inferring ?
        QimgRes = QImage((const unsigned char *)(res.data), res.cols, res.rows, res.step, QImage::Format_RGB888);
        QimgRaw = QImage((const unsigned char *)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
        QPixmap pixmapRes = QPixmap::fromImage(QimgRes);
        QPixmap pixmapRaw = QPixmap::fromImage(QimgRaw);
        label_res->setPixmap(pixmapRes);
        label_raw->setPixmap(pixmapRaw);

        ui->terminal->append(QString("cost %1 ms").arg(__tc, 2, 'f', 4));
    }
}

void YOLOWINDOW::displayImage(cv::Mat imageReceived, cv::Mat resReceived, std::vector<Object> objsReceived, double tcReceived, bool finishedReceived)
{
    // update image, res, objs, tc
    image = imageReceived;
    res = resReceived;
    objs = objsReceived;
    __tc = tcReceived;

    if (res.empty() && image.empty())
    {
        return;
    }
    else
    {
        label_raw->clear();
        label_res->clear();
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        cv::cvtColor(res, res, cv::COLOR_BGR2RGB); // interfere inferring ?
        QimgRes = QImage((const unsigned char *)(res.data), res.cols, res.rows, res.step, QImage::Format_RGB888);
        QimgRaw = QImage((const unsigned char *)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
        QPixmap pixmapRes = QPixmap::fromImage(QimgRes);
        QPixmap pixmapRaw = QPixmap::fromImage(QimgRaw);
        label_res->setPixmap(pixmapRes);
        label_raw->setPixmap(pixmapRaw);
        ui->terminal->append(QString("cost %1 ms").arg(__tc, 2, 'f', 4));
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
