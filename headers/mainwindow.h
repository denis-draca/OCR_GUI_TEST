#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <qfiledialog.h>
#include <QString>
#include <vector>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QTimer>
#include <QSerialPort>
#include <QDir>
#include <QMessageBox>
#include <thread>
#include <QSharedPointer>
#include <chrono>
//#include <QWidget>


#define REFRESH_HZ 30
#define ZOOM_MAX 20

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_bu_imgSet_clicked();
    void on_chk_nameOnly_stateChanged();
    void on_lst_setImgs_currentRowChanged(int currentRow);

    void on_slide_contrast_valueChanged(int value);

    void on_slide_brightness_sliderMoved(int position);

    void process();

    void on_slide_rot_sliderMoved(int position);

    void on_bu_delImgs_clicked();

    void on_serial_data();

    void on_bu_connect_clicked();

    void on_bu_disconnect_clicked();
    void on_resize();
    void on_slide_binary_thresh_sliderMoved(int position);
    void on_bu_upload_clicked();
    void on_chk_16by16_stateChanged();
    void on_chk_binary_stateChanged();
    void on_chk_16by16_force_clicked();
    void about();

    void on_chk_keep_size_stateChanged();

    void wheelEvent(QWheelEvent * event);

private:
    Ui::MainWindow *ui;

private: //declarations
    struct _rgb_{
        uchar r;
        uchar g;
        uchar b;
    };

    struct _hsv_{
        uchar h;
        uchar s;
        uchar v;
    };

private: //members
    std::vector<QString> img_list;
    std::vector<std::vector<_hsv_> > _og_hsv;

    cv::Mat current_img;
    cv::Mat use_img;
    cv::Mat _binary_img;

    QPixmap img;
    QGraphicsScene *scene;

    float _contrast_value;
    int _brightness_value;
    int _rot_value;
    int _current_pos;
    int _binary_thresh;
    int _zoom;

    bool _change_made;
    bool _img_set;
    bool _stop_work;

    QTimer *_process_timer;
    QTimer *_resize_timer;
    QSerialPort *_serial;

//    std::thread *_worker;


private: //methods
    void set_list_view();
    void draw_img();
    void set_empty_img();
    void working_thread();
    void change_brightness_contrast(cv::Mat &img);
    void change_rotation(cv::Mat &img);

    uchar truncate(int value);

    cv::Mat resizeKeepAspectRatio(const cv::Mat &input, const cv::Size &newSize, bool force_size);
    cv::Mat perform_zoom(cv::Mat &input);
};

#endif // MAINWINDOW_H
