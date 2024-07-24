#ifndef YOLOWINDOW_H
#define YOLOWINDOW_H
#include "qswitchbutton.h"
#include "infer.h"
#include "utils.hpp"

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QFileDialog>
#include <QListView>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QMessageBox>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <thread>
#include <QUDPSocket>
#include <QIODevice>
#include <QStringList>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  YOLO PARAMETERS                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define YOLO_TOPK                                               100                 // 100                                  //
#define YOLO_SEG_H                                              160                 //                                      //
#define YOLO_SEG_W                                              160                 //                                      //
#define YOLO_SEG_CHANNELS                                       32                  //                                      //
#define YOLO_SCORE_THRES                                        0.25f               //                                      //
#define YOLO_IOU_THRES                                          0.65f               //                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern cv::Size YOLO_SIZE;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  YOLO PARAMETERS                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BUFFER_SIZE                                             300000                 // 100                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::vector<std::string> CLASS_NAMES = {
    "thread",         "bicycle"};

const std::vector<std::vector<unsigned int>> COLORS = {
    {0, 114, 189},   {217, 83, 25}};

const std::vector<std::vector<unsigned int>> MASK_COLORS = {
    {255, 56, 56},  {255, 157, 151}};


QT_BEGIN_NAMESPACE
namespace Ui {
class YOLOWINDOW;
}
QT_END_NAMESPACE

class SOCKETCAMERATHREAD;



// CMyFileDialog
class CMyFileDialog : public QFileDialog
{
    Q_OBJECT

public:
    explicit CMyFileDialog(QWidget *parent = nullptr);

public slots:
    void                                                            slot_myAccetp();

signals:

};

// video thread
class VIDEOTHREAD : public QThread
{
    Q_OBJECT

public:
    explicit VIDEOTHREAD(QObject *parent = nullptr);
    ~VIDEOTHREAD();

    void                                                            run() override;

    YOLO *                                                          v_yolov8;
    cv::Mat                                                         v_res;
    cv::Mat                                                         v_image;
    std::vector<Object>                                             v_objs;

    double                                                          v_tc = 0;
    std::string                                                     v_path;

    QWidget *                                                       v_board;
    QAtomicInt                                                     __stopRequested = 0;

signals:
    void                                                            videoDone(cv::Mat               u_image,
                                                                              cv::Mat               u_res,
                                                                              std::vector<Object>   u_objs,
                                                                              double                u_tc,
                                                                              bool                  u_finished
                                                                              );
    void                                                            taskDone();


public slots:
    void                                                            continueInfer();
    void                                                            pauseInfer();
    void                                                            stopInfer();
    void                                                            backRun();


private:
    bool                                                            isPaused;
    QMutex                                                          mutex;
    QWaitCondition                                                  pauseCondition;
    cv::VideoCapture                                                cap;
};


class CAMERAThread : public QThread
{
    Q_OBJECT

public:
    explicit CAMERAThread(QObject *parent = nullptr, YOLO *t_yolo = nullptr, std::vector<Object> *t_objs = nullptr);
    ~CAMERAThread();

    YOLO *                                                          c_yolov8;
    std::vector<Object> *                                           c_objs;
    cv::Mat *                                                       c_res;
    cv::Mat *                                                       c_image;
    double *                                                        c_tc;

protected:
    void                                                            run() override;

signals:
    void                                                            cameraDone();
    void                                                            taskDone();

public slots:



private:
    QWidget *                                                        c_board;
};


class YOLOWINDOW : public QMainWindow
{
    Q_OBJECT

public:
    YOLOWINDOW(QWidget *parent = nullptr);
    ~YOLOWINDOW();

    QButtonGroup *                                                  buttonGroup{nullptr};
    YOLO *                                                          yolov8;
    std::vector<std::string>                                        imagePathList;
    bool                                                            isVideo{false};

    cv::VideoCapture *                                              cap;
    cv::Mat                                                         res;
    cv::Mat                                                         image;
    std::vector<Object>                                             objs;

    double                                                          __tc = 0;
    QImage                                                          QimgRes;
    QImage                                                          QimgRaw;
    QPixmap                                                         pixmapRes;
    QPixmap                                                         pixmapRaw;

    QImage                                                          Mat2QImage(cv::Mat const& src);
    QUdpSocket                                                      client{nullptr};

    QStringList                                                     __addressParser;
    QHostAddress                                                    __Address;

    QGraphicsScene *                                                scene;


signals:
    void                                                            timerTimeout(bool direction);

private slots:
    void                                                            on_toolButton_clicked();
    void                                                            on_toolButton_2_clicked();
    void                                                            on_pushButton_clicked();
    void                                                            on_pushButton_start_clicked();
    void                                                            on_pushButton_back_clicked();
    void                                                            on_pushButton_back_pressed();
    void                                                            on_pushButton_back_released();
    void                                                            on_pushButton_continue_clicked();
    void                                                            on_pushButton_continue_pressed();
    void                                                            on_pushButton_continue_released();
    void                                                            on_pushButton_pause_clicked();
    void                                                            on_pushButton_stop_clicked();
    void                                                            on_pushButton_save_clicked();
    void                                                            c_forwardInfer(cv::Mat cameraImage);
    void                                                            forwardInfer(int itemIndex);
    void                                                            forwardInfer(cv::Mat cameraImage);
    void                                                            pressBtnSlot(bool direction);
    void                                                            displayImage();
    void                                                            c_displayImage();

private:
    Ui::YOLOWINDOW *                                                ui;
    SwitchButton *                                                  switchButton;
    QLabel *                                                        label_raw;
    QLabel *                                                        label_res;


    QString                                                         __engineFile;
    QString                                                         __inputFile;
    bool                                                            __yoloEnableFlag{false};
    bool                                                            __yoloDataFlag{false};
    int                                                             __traverseIndex = 0;
    QTimer *                                                        __timer;

    QImage                                                          gpcImage;
    void                                                            setAllButtonsEnabled(bool enabled);
    void                                                            setSpecificButtonsEnabled(const QStringList& buttonNames, bool enabled);
    void                                                            graphicDisplay();
    void                                                            Mat2QPixmap();


    VIDEOTHREAD *                                                   __videoThread;
    CAMERAThread *                                                  __cameraThread;

    SOCKETCAMERATHREAD *                                            __socketCameraThread;

};


class SOCKETCAMERATHREAD : public QThread
{
    Q_OBJECT

public:
    explicit SOCKETCAMERATHREAD(YOLOWINDOW *parent = nullptr);
    ~SOCKETCAMERATHREAD();

    QUdpSocket                                                      client;
    QHostAddress                                                    serverAddress;

protected:
    void                                                            run() override;

    signals:
    void                                                            socketCameraDone();
    void                                                            taskDone();

    public slots:

private:
    void                                                            parserImage();
    std::vector<uchar>                                              decodedimg;
    char                                                            recv_buf[BUFFER_SIZE];  // recieve cache buffer
    YOLOWINDOW *                                                    y_parent;
};

#endif // YOLOWINDOW_H
