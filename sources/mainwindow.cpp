#include "headers/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _change_made = false;
    _img_set = false;
    _stop_work = false;

    _contrast_value = 1.5;
    _brightness_value = 0;
    _rot_value = 0;
    _binary_thresh = 0;
    _zoom = 0;

    _serial = new QSerialPort(this);
    connect(_serial, &QSerialPort::readyRead, this, &MainWindow::on_serial_data);


    _process_timer = new QTimer(this);
    connect(_process_timer, SIGNAL(timeout()), this, SLOT(process()));
    _process_timer->start(1000/REFRESH_HZ);

    _resize_timer = new QTimer(this);
    connect(_resize_timer, SIGNAL(timeout()), this, SLOT(on_resize()));
    _resize_timer->start(1000/REFRESH_HZ);

    connect(ui->actionabout, SIGNAL(triggered()), this, SLOT(about()));

//    _worker = new std::thread(&MainWindow::working_thread, this);

    set_empty_img();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_bu_imgSet_clicked()
{
//    ui->txt_output->document()->setPlainText("hello");
    QStringList fileLocs =  QFileDialog::getOpenFileNames(this, tr("Open"), " " ,tr("Images (*.png *.jpg *.bmp *.jpeg *.tis)"));

    if(!fileLocs.isEmpty()){
        for(int loc = 0;  loc < fileLocs.size(); loc++){
            QString fileLoc = fileLocs.at(loc);
            bool add = true;
            for(auto it = img_list.begin(); it != img_list.end(); ++it ){
                if (it->toStdString() == fileLoc.toStdString()){
                    add = false;
                }
            }
            if(add){img_list.push_back(fileLoc);}
        }
        set_list_view();
    }
    else{
        std::cout << "Gotta give me something" << std::endl;
        return;
    }
}

void MainWindow::set_list_view()
{
    ui->lst_setImgs->clear();

    for(uint i = 0; i < img_list.size(); i++){
        QString data = img_list.at(i);

        if(ui->chk_nameOnly->isChecked()){
            QString testChar('/');

            int pos = data.lastIndexOf(testChar);
            data = data.right(data.length() - pos - 1);
        }

        ui->lst_setImgs->addItem(data);
    }
}

void MainWindow::draw_img()
{
    try{
        cv::Mat temp_img = use_img.clone();

        if(ui->chk_keep_size->isChecked()){
            int width = ui->img_raw->width() - 5;
            int height = ui->img_raw->height() - 5;

            temp_img = resizeKeepAspectRatio(temp_img, cv::Size(width,height), false);
        }

//        temp_img = perform_zoom(temp_img);

        img = QPixmap::fromImage(QImage((unsigned char*) temp_img.data, temp_img.cols, temp_img.rows, temp_img.step1(), QImage::Format_RGB888).rgbSwapped());
        scene = new QGraphicsScene(this);
        scene->addPixmap(img);
        scene->setSceneRect(img.rect());

        ui->img_raw->setScene(scene);
    }
    catch (const std::exception& e) {
         std::cout << e.what();
    }
}

void MainWindow::set_empty_img()
{
    cv::Mat image(320, 240, CV_8UC3, cv::Scalar(0,0,0));
    use_img = image.clone();

    cv::Point2f pt;
    pt.x = 50;
    pt.y = 320/2;

    cv::putText(use_img, "No Image Selected",pt, 1, 1, cv::Scalar(255,255,255));

    draw_img();
}

void MainWindow::working_thread()
{
    while(!_stop_work){
//        std::cout << "hi" << std::endl;
//        process();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000/REFRESH_HZ));
    }

}

void MainWindow::change_brightness_contrast(cv::Mat &img)
{
    cv::Mat temp_img = cv::Mat::zeros( img.size(), img.type() );

//    int value = 0;

//    if (_brightness_value < 50){value = 50 - _brightness_value;}
//    if (_brightness_value > 50){value = _brightness_value - 50;}

    for( int y = 0; y < img.rows; y++ ) {
        for( int x = 0; x < img.cols; x++ ) {
            for( int c = 0; c < 3; c++ ) {
                temp_img.at<cv::Vec3b>(y,x)[c] =
                  cv::saturate_cast<uchar>( _contrast_value*( img.at<cv::Vec3b>(y,x)[c] ) + _brightness_value );
            }
        }
    }

    if(ui->chk_binary->isChecked()){
        cv::cvtColor(temp_img, temp_img, CV_BGR2GRAY);
        cv::threshold(temp_img, temp_img, _binary_thresh, 255, cv::THRESH_BINARY);
        _binary_img = temp_img.clone();
        cv::cvtColor(temp_img, temp_img, CV_GRAY2BGR);
    }

    cv::Point2f src_center(temp_img.cols/2.0F, temp_img.rows/2.0F);
    cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, _rot_value, 1.0);
    cv::Mat dst;
    cv::warpAffine(temp_img, dst, rot_mat, temp_img.size());


    img = dst.clone();
}

void MainWindow::change_rotation(cv::Mat &img)
{
    cv::Point2f src_center(img.cols/2.0F, img.rows/2.0F);
    cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, _rot_value, 1.0);
    cv::Mat dst;
    cv::warpAffine(img, dst, rot_mat, img.size());

    img = dst.clone();
}

//void MainWindow::change_contrast(cv::Mat &img)
//{

//}

void MainWindow::process()
{
    std::string base = "/dev/ttyACM";


    for(int i = 0; i < 10; i++){
        std::string full = base;
        full.append(std::to_string(i));
        QDir dir(full.c_str());

        if (dir.exists(full.c_str())){
            bool add = true;
            for (int x = 0; x < ui->com_ports->count(); x++){
                std::string qs = ui->com_ports->itemText(x).toUtf8().constData();

                if(qs.compare(full.c_str()) == 0){
                    add = false;
                }
                else{
                    ui->txt_output->document()->setPlainText(full.c_str());
                }
            }
            if (add){ui->com_ports->addItem(full.c_str());}
        }
    }

    for (int x = 0; x < ui->com_ports->count(); x++){
        std::string qs = ui->com_ports->itemText(x).toUtf8().constData();
        QDir dir(qs.c_str());

        if(!dir.exists(qs.c_str())){ui->com_ports->removeItem(x);}

    }


    if (!_img_set){return;}

    cv::Mat img = current_img.clone();
    bool change_made = false;

    if (_change_made){
        change_brightness_contrast(img);
        _change_made = false;
        change_made = true;
    }

    if(change_made){

        if(ui->chk_16by16->isChecked()){
            if(ui->chk_16by16_force->isChecked()){img = resizeKeepAspectRatio(img, cv::Size(16,16), true);}
            else{cv::resize(img, img, cv::Size(16,16),0,0, CV_INTER_CUBIC);}
        }

        use_img = img.clone();
        draw_img();
    }
}

cv::Mat MainWindow::resizeKeepAspectRatio(const cv::Mat &input, const cv::Size &newSize, bool force_size)
{
    double resize_ratio_x = double(newSize.width)/double(input.cols);
    double resize_ratio_y = double(newSize.height)/double(input.rows);

    double resize_ratio = resize_ratio_x;
    if(resize_ratio_y < resize_ratio_x){resize_ratio = resize_ratio_y;}

    cv::Mat newMat;

    cv::resize(input, newMat, cv::Size(input.cols * resize_ratio, input.rows * resize_ratio), 0, 0, CV_INTER_CUBIC);

    if(force_size){

        cv::Mat temp(newSize, input.type(), cv::Scalar(255,255,255));

        int x = (temp.cols - newMat.cols)/2;
        int y = (temp.rows - newMat.rows)/2;

        newMat.copyTo(temp(cv::Rect(x,y,newMat.cols, newMat.rows)));

        newMat = temp.clone();

    }
    return newMat;


}

cv::Mat MainWindow::perform_zoom(cv::Mat &input)
{
    //Broken
    if(_zoom == 0){return input;}

    cv::Mat temp_img(input.rows * _zoom, input.cols * _zoom, input.type());

    for(int inY = 0; inY < input.rows; inY++){
        for(int inX = 0; inX < input.cols; inX++){

            auto pixel = input.at<cv::Vec3b>(inY,inX);

            for(int y = inY*_zoom; y < inY*_zoom + _zoom; y++){
                for(int x = inX*_zoom; x < inX*_zoom + _zoom; x++){
                    temp_img.at<cv::Vec3b>(y,x) = pixel;
                }
            }
        }
    }

    return temp_img;
}

uchar MainWindow::truncate(int value)
{
    if (value < 0){
        return 0;
    }

    if (value > 254){
        return 255;
    }

    return uchar(value);
}

void MainWindow::on_chk_nameOnly_stateChanged()
{
    set_list_view();
}


void MainWindow::on_lst_setImgs_currentRowChanged(int currentRow)
{
    if (currentRow < 0){
        return;
    }

    std::string loc_as_string = img_list.at(currentRow).toUtf8().constData();

    current_img = cv::imread(loc_as_string.c_str(), CV_LOAD_IMAGE_COLOR);
    use_img = cv::imread(loc_as_string.c_str(), CV_LOAD_IMAGE_COLOR);
    _img_set = true;
    _change_made = true;

    _current_pos = currentRow;

//    draw_img();
}

void MainWindow::on_slide_contrast_valueChanged(int value)
{

    _contrast_value = ((3.0/99.0)*float(value));
    _change_made = true;

}

void MainWindow::on_slide_brightness_sliderMoved(int position)
{
    _brightness_value = position;
    _change_made = true;

}

void MainWindow::on_slide_rot_sliderMoved(int position)
{
    _rot_value = -((180.0/50.0)*position - 180);
    _change_made = true;
}

void MainWindow::on_bu_delImgs_clicked()
{
    if(!_img_set){return;}

    if(img_list.empty()){return;}

    img_list.erase(img_list.begin() + _current_pos);

    set_empty_img(); _img_set = false;

    set_list_view();

}

void MainWindow::on_serial_data()
{
    const QByteArray data = _serial->readAll();

//    ui->txt_output->document()->setPlainText(data);

    std::string byte_data = "recieved: ";

    for(int i = 0; i < data.size(); i++){
        byte_data.append(std::to_string(data.at(i)));
    }

    ui->txt_output->document()->setPlainText(byte_data.c_str());
}

void MainWindow::on_bu_connect_clicked()
{
    if(ui->com_ports->currentText().isEmpty()){return;}

    _serial->setPortName(ui->com_ports->currentText());
    _serial->setBaudRate(QSerialPort::Baud115200);
    _serial->setDataBits(QSerialPort::Data8);
    _serial->setParity(QSerialPort::NoParity);
    _serial->setStopBits(QSerialPort::OneStop);
    _serial->setFlowControl(QSerialPort::NoFlowControl);

    if (_serial->open(QIODevice::ReadWrite)) {
        QMessageBox::information(this, tr("Connection"), tr("Serial Port has successfully connected"));
    } else {
      QMessageBox::critical(this, tr("Error"), _serial->errorString());
    }

}

void MainWindow::on_bu_disconnect_clicked()
{
    if (_serial->isOpen()){
        _serial->close();
        QMessageBox::information(this, tr("Disconnected"), tr("Serial Port Has now been disconnected"));
    }
}

void MainWindow::on_resize()
{
    if(ui->chk_keep_size->isChecked()){
//        _change_made = true;
        draw_img();
    }
}

void MainWindow::on_slide_binary_thresh_sliderMoved(int position)
{
    _binary_thresh = (255/99) * position;
    _change_made = true;
}

void MainWindow::on_bu_upload_clicked()
{
    if(_binary_img.empty()){QMessageBox::critical(this, tr("Error"), tr("No Binary Image Set")); return;}
    if(!_serial->isOpen()){QMessageBox::critical(this, tr("Error"), tr("Not Connected Yet")); return;}

    cv::Mat img;
    if(ui->chk_16by16_force->isChecked()){img = resizeKeepAspectRatio(_binary_img, cv::Size(16,16), true);}
    else{cv::resize(_binary_img, img, cv::Size(16,16));}

    QByteArray payload;

    uchar on = 1;
    uchar off = 0;
    uchar end_row = 2;
    uchar start_transmission = 3;
    uchar end_transmission = 4;
    uchar signal = 5;

    payload.push_back(start_transmission);

    for(int y = 0; y < img.rows; y++){
        for(int x = 0; x < img.cols; x++){
            uchar pixel = img.at<uchar>(y,x);

            if(pixel == 0){payload.push_back(on);}
                else{payload.push_back(off);}

        }
        payload.push_back(end_row);
    }
    payload.push_back(end_transmission);

    QByteArray signalArr;

    signalArr.push_back(signal);

    _serial->write(signalArr);

    _serial->write(payload);
}

void MainWindow::on_chk_16by16_stateChanged()
{
    _change_made = true;
}

void MainWindow::on_chk_binary_stateChanged()
{
    _change_made = true;
}

void MainWindow::on_chk_16by16_force_clicked()
{
    _change_made = true;
}

void MainWindow::about()
{
    QMessageBox::information(this, tr("About"), tr("Made for some reason"));
}

void MainWindow::on_chk_keep_size_stateChanged()
{
    draw_img();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    return;
    if(event->delta() > 0){
        _zoom++;
        if (_zoom > ZOOM_MAX){_zoom = ZOOM_MAX;}
    }
    else{
        _zoom--;
        if (_zoom < -ZOOM_MAX){_zoom = -ZOOM_MAX;}
    }

    draw_img();
}
