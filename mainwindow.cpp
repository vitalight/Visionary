#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filter.h"

#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    images[imageIndex].originalImage = NULL;
    setWindowFlags(windowFlags()
                   &~Qt::WindowMaximizeButtonHint); // 禁止最大化按钮
    setFixedSize(this->width(),this->height());     // 禁止拖动窗口大小


    qlabel = new QLabel(ui->whitebg);
    qlabel->resize(ui->whitebg->geometry().width(), ui->whitebg->geometry().height());
    qlabel->setAlignment(Qt::AlignCenter);
    qlabel->setMouseTracking(true);                 // 鼠标跟踪

    ui->histogramArea->setVisible(false);           // 默认隐藏直方图
#ifndef __RELEASE__
    on_actionOpen_triggered();
    on_actionOtsu_triggered();
    on_actionDistanceTransform_triggered();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

QImage *MainWindow::getCurrentImage()
{
    return images[imageIndex].currentImage;
}

QImage *MainWindow::getAnotherImage()
{
    return images[1-imageIndex].currentImage;
}

void MainWindow::setCurrentImage(QImage *image)
{
    images[imageIndex].currentImage = image;
}

void MainWindow::setAnotherImage(QImage *image)
{
    images[1-imageIndex].currentImage = image;
}

void MainWindow::showResponseTime()
{
    QString s = QString("响应时间：%1s").arg(F_responseTime());
    ui->responseTime->setText(s);
}

void MainWindow::showImage_without_history(QImage *image)
{
    if (!image) {
        qlabel->clear();
        return;
    }
    setCurrentImage(image);
    images[imageIndex].thumbnail = autoscale();
    updateHistogram();

    qlabel->setPixmap(QPixmap::fromImage(images[imageIndex].thumbnail));
}

void MainWindow::showImage(QImage *image)
{
//    if (image->width() != getCurrentImage()->width() ||
//            image->height() != getCurrentImage()->height()) {
//        qlabel->resize(image->width(), image->height());
//    }
    if (getCurrentImage() == image)
    {
        return;
    }

    // log in images[imageIndex].historyImages for future undo and redo operation
    if (images[imageIndex].historyImages.size() >= 1) {
        ui->actionUndo->setEnabled(true);
    }

    if ((int)images[imageIndex].historyImages.size() > images[imageIndex].historyIndex + 2) {
        images[imageIndex].historyImages[++images[imageIndex].historyIndex] = image;
        images[imageIndex].historyImages.erase(images[imageIndex].historyImages.begin()+images[imageIndex].historyIndex+1, images[imageIndex].historyImages.end());
        ui->actionRedo->setEnabled(false);
    } else {
        images[imageIndex].historyImages.push_back(image);
        if (images[imageIndex].historyImages.size() > HISTORY_MAX)
        {
            QImage *back = images[imageIndex].historyImages[0];
            images[imageIndex].historyImages.erase(images[imageIndex].historyImages.begin());

            // function in [filter.h] is allowed to return input image pointer,
            // so (back != historyImage.front()) is required
            if (back != images[imageIndex].originalImage && back != images[imageIndex].historyImages[0])
                free(back);
        } else {
            images[imageIndex].historyIndex++;
        }
    }

    setCurrentImage(image);
    images[imageIndex].thumbnail = autoscale();
    qlabel->setPixmap(QPixmap::fromImage(images[imageIndex].thumbnail));
    updateHistogram();
    ui->actionRecover->setEnabled(true);
    ui->actionRedo->setEnabled(false);
}

QImage MainWindow::autoscale()
{
    if (!getCurrentImage())
    {
        qDebug()<<"[error] autoscale: null getCurrentImage()";
        exit(-1);
    }
    QImage newImage= getCurrentImage()->scaled(ui->whitebg->geometry().height(), ui->whitebg->geometry().height(),
                                          Qt::KeepAspectRatio, Qt::SmoothTransformation);

    images[imageIndex].showWidth = newImage.width();
    images[imageIndex].showHeight = newImage.height();
    return newImage;
}

void MainWindow::showTip(QString str)
{
    ui->tip->setText(str);
}

/************************************************
 * Input slider
 ************************************************/
void MainWindow::on_slider1_sliderReleased()
{
    //showImage(F_adjustHSB(images[imageIndex].originalImage, ui->slider1->value(), ui->slider2->value(), ui->slider3->value()));
}

void MainWindow::on_slider2_sliderReleased()
{
    //showImage(F_adjustHSB(images[imageIndex].originalImage, ui->slider1->value(), ui->slider2->value(), ui->slider3->value()));
}

void MainWindow::on_slider3_sliderReleased()
{
    //showImage(F_adjustHSB(images[imageIndex].originalImage, ui->slider1->value(), ui->slider2->value(), ui->slider3->value()));
}

void MainWindow::on_slider1_sliderMoved(int position)
{
    position++;
    //showImage(F_adjustHSB(images[imageIndex].originalImage, ui->slider1->value(), ui->slider2->value(), ui->slider3->value()));
}

void MainWindow::on_slider2_sliderMoved(int position)
{
    position++;
}

void MainWindow::on_slider3_sliderMoved(int position)
{
    position++;
}
/************************************************
 * Signal slot function for UI operation
 ************************************************/
void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!getCurrentImage())
        return;
    QPoint p = event->pos();
    static int x_minus = centralWidget()->geometry().x() +
                         ui->whitebg->geometry().x() + ui->whitebg->geometry().width()/2,
               y_minus = centralWidget()->geometry().y() +
                         ui->whitebg->geometry().y() + ui->whitebg->geometry().height()/2;
    int x = qBound(0, p.x() - x_minus + images[imageIndex].showWidth/2, images[imageIndex].showWidth-1),
        y = qBound(0, p.y() - y_minus + images[imageIndex].showHeight/2, images[imageIndex].showHeight-1);

    // 显示鼠标坐标
    QString s = QString("鼠标位置：[%1, %2]")
            .arg(x+1)
            .arg(y+1);
    ui->mouseLocation->setText(s);

    // 显示对应颜色
    QRgb *bits = (QRgb*)images[imageIndex].thumbnail.constBits();
    int r = qRed(bits[y*images[imageIndex].showWidth+x]),
        g = qGreen(bits[y*images[imageIndex].showWidth+x]),
        b = qBlue(bits[y*images[imageIndex].showWidth+x]);
    s = QString("background-color:rgb(%1,%2,%3)").arg(r).arg(g).arg(b);
    ui->colorBar->setStyleSheet(s);
    s = QString("RGB:(%1,%2,%3)").arg(r).arg(g).arg(b);
    ui->colorText->setText(s);
}

void MainWindow::on_actionOpen_triggered()
{
    // open file
#ifdef __RELEASE__
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("打开图片"),
                                                    "F:/MyCodes/Visionary/images",
                                                    "Images (*.png *.bmp *.jpg *.jpeg *.gif)");
#else
    QString fileName = "F:/MyCodes/Visionary/images/distance-test.png";
#endif
    if (fileName == "" || fileName == NULL) {
        return;
    }
    QImage *image = new QImage;
    if(!image->load(fileName)) {
        QMessageBox::information(this, tr("打开图像失败"), tr("打开图像失败"));
        return;
    }

    images[imageIndex].originalImage = image;

    // log
    showImage(image);
    //currentFile = fileName;
    //qlabel->resize(860, 680);
    //qDebug()<<"Resize: "<<ui->whitebg->geometry().width()<<", "<<ui->whitebg->geometry().height();
    ui->menuFilter->setEnabled(true);
    ui->actionRecover->setEnabled(false);
    showTip("已打开文件");
}

void MainWindow::on_switchButton_clicked()
{
    imageIndex = 1-imageIndex;
    showImage_without_history(getCurrentImage());
    ui->menuFilter->setEnabled(getCurrentImage() != NULL);
    ui->actionRecover->setEnabled(getCurrentImage() != images[imageIndex].originalImage);
    ui->actionUndo->setEnabled(images[imageIndex].historyIndex > 0);
    ui->actionRedo->setEnabled(images[imageIndex].historyIndex < (int)images[imageIndex].historyImages.size()-1);
    showTip(QString("已切换到图层%1").arg(imageIndex+1));
}

void MainWindow::on_actionSwitch_triggered()
{
    on_switchButton_clicked();
}

void MainWindow::on_actionCopyToAnother_triggered()
{
    imageIndex = 1-imageIndex;
    showImage(getAnotherImage());
    imageIndex = 1-imageIndex;
    showTip("已复制到另一图层");
}

void MainWindow::on_actionRecover_triggered()
{
    if (images[imageIndex].originalImage!=NULL) {
        showImage(images[imageIndex].originalImage);
        ui->actionRecover->setEnabled(false);
    }
    showTip("已恢复原文件");
}

void MainWindow::on_actionUndo_triggered()
{
    if (images[imageIndex].historyImages.size()==0) {
        qDebug() << "[Error] Undo without history";
        return;
    }
    showImage_without_history(images[imageIndex].historyImages[--images[imageIndex].historyIndex]);
    if (!images[imageIndex].historyIndex) {
        ui->actionUndo->setEnabled(false);
    }
    ui->actionRedo->setEnabled(true);
    showTip("上次操作：撤销");
}

void MainWindow::on_actionRedo_triggered()
{
    showImage_without_history(images[imageIndex].historyImages[++images[imageIndex].historyIndex]);
    if (images[imageIndex].historyIndex == (int)images[imageIndex].historyImages.size() - 1) {
        ui->actionRedo->setEnabled(false);
    }
    ui->actionUndo->setEnabled(true);
    showTip("上次操作：重做");
}

void MainWindow::on_actionExit_triggered()
{
    QApplication* app;
    app->exit(0);
}

void MainWindow::on_actionSave_as_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("保存为"),
                                                    "C:\\Users\\Administrator\\Desktop",
                                                    "Images (*.png *.bmp *.jpg *.jpeg)");
    getCurrentImage()->save(fileName);
    showTip("已另存文件");
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于Visionary",
                       "Visionary是一个强大的图像处理软件。");
}

/************************************************
 * Signal slot function for image processing
 ************************************************/
void MainWindow::on_actionDecolor_triggered()
{
    showImage(F_decolor(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：去色");
}

//void MainWindow::on_actionBinarization_triggered()
//{
//    bool ok = false;
//    int threshold = QInputDialog::getInt(this,tr("Visionary"),
//                                         tr("请输入阈值"),
//                                         0, 0, 255,
//                                         1, &ok);
//    if (!ok)
//        return;
//    showImage(F_binarization(getCurrentImage(), threshold));
//    showResponseTime();
//}

void MainWindow::on_actionBlur_triggered()
{
    showImage(F_blur_gaussian(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：高斯模糊");
}

void MainWindow::on_actionBlurMean_triggered()
{
    showImage(F_blur_mean(getCurrentImage(), 5));
    showResponseTime();
    showTip("上次操作：中值模糊");
}

void MainWindow::on_actionBlurMedian_triggered()
{
    showImage(F_blur_median(getCurrentImage(), 3));
    showResponseTime();
    showTip("上次操作：均值模糊");
}

void MainWindow::on_actionSharpen_triggered()
{
    showImage(F_sharpen(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：锐化");
}

void MainWindow::on_actionDilation_triggered()
{
    std::vector<std::vector<int>> kernel = U_getFlatKernel_i(5);
    showImage(F_dilation(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：膨胀");
}

void MainWindow::on_actionErosion_triggered()
{
    std::vector<std::vector<int>> kernel = U_getFlatKernel_i(5);
    showImage(F_erosion(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：腐蚀");
}

void MainWindow::on_actionMorphologicalOpen_triggered()
{
    std::vector<std::vector<int>> kernel = U_getFlatKernel_i(5);
    showImage(F_open(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：开操作");
}

void MainWindow::on_actionMorphologicalClose_triggered()
{
    std::vector<std::vector<int>> kernel = U_getFlatKernel_i(5);
    showImage(F_close(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：闭操作");
}

void MainWindow::on_actionEqualizeHistogram_triggered()
{
    showImage(F_equalizeHistogram(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：直方图均衡化");
}

void MainWindow::on_actionResize_triggered()
{
    showImage(F_resize(getCurrentImage(), F_NEAREST));
    showResponseTime();
    showTip("上次操作：最近邻缩放");
}

void MainWindow::on_actionResizeLinear_triggered()
{
    showImage(F_resize(getCurrentImage(), F_LINEAR));
    showResponseTime();
    showTip("上次操作：双线性插值缩放");
}

void MainWindow::on_actionChannelSeperation_triggered()
{
    showImage(F_seperation(getCurrentImage(), F_R));
    showResponseTime();
    showTip("上次操作：通道分离");
}

void MainWindow::on_actionSpinNearest_triggered()
{
    showImage(F_spin(getCurrentImage(), 45, F_NEAREST));
    showResponseTime();
    showTip("上次操作：最近邻旋转");
}

void MainWindow::on_actionSpinLinear_triggered()
{
    showImage(F_spin(getCurrentImage(), 45, F_LINEAR));
    showResponseTime();
    showTip("上次操作：双线性插值旋转");
}

void MainWindow::on_actionDetectEdgeSobel_triggered()
{
    showImage(F_detectEdge(getCurrentImage(), F_SOBEL));
    showResponseTime();
    showTip("上次操作：Sobel边缘检测");
}

void MainWindow::on_actionDetectEdgeLaplacian_triggered()
{
    showImage(F_detectEdge(getCurrentImage(), F_LAPLACIAN));
    showResponseTime();
    showTip("上次操作：拉普拉斯边缘检测");
}

void MainWindow::on_actionDetectEdgeCanny_triggered()
{
    showImage(F_detectEdge(getCurrentImage(), F_CANNY));
    showResponseTime();
    showTip("上次操作：Canny边缘检测");
}

void MainWindow::on_actionOtsu_triggered()
{
    showImage(F_binarization_Otsu(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：大津算法二值化");
}

void MainWindow::on_actionDoubleThreshold_triggered()
{
    showImage(F_binarization_double(getCurrentImage(), 100, 200));
    showResponseTime();
    showTip("上次操作：双阈值二值化");
}



void MainWindow::on_actionAdd_triggered()
{
    if (!getAnotherImage()) {
        QMessageBox::about(this, "非法操作",
                           "另一图层暂无图像。");
        return;
    }
    showImage(F_add(getCurrentImage(), getAnotherImage()));
    showResponseTime();
    showTip("上次操作：加操作");
}

void MainWindow::on_actionMinus_triggered()
{
    if (!getAnotherImage()) {
        QMessageBox::about(this, "非法操作",
                           "另一图层暂无图像。");
        return;
    }
    showImage(F_minus(getCurrentImage(), getAnotherImage()));
    showResponseTime();
    showTip("上次操作：减操作");
}

void MainWindow::on_actionTimes_triggered()
{
    if (!getAnotherImage()) {
        QMessageBox::about(this, "非法操作",
                           "另一图层暂无图像。");
        return;
    }
    showImage(F_times(getCurrentImage(), getAnotherImage()));
    showResponseTime();
    showTip("上次操作：乘操作");
}

void MainWindow::on_actionCut_triggered()
{
    showImage(F_cut(getCurrentImage(), 100, 100, 300, 400));
    showResponseTime();
    showTip("上次操作：裁剪");
}

void MainWindow::updateHistogram()
{
    if (showHistogram)
    {
        ui->histogramArea->histogram = F_getHistogram(getCurrentImage());
        update();
    }
}

void MainWindow::on_actionShowHistogram_toggled(bool arg1)
{
    showHistogram = arg1;
    ui->histogramArea->setVisible(arg1);
    updateHistogram();
}

void MainWindow::on_actionadjustHSI_triggered()
{
    showImage(F_adjustHSB(getCurrentImage(), 60, -50, 50));
    showResponseTime();
    showTip("上次操作：调整HSI");
}


void MainWindow::on_actionThining_triggered()
{
    F_Kernel_i kernel = {{0, 0, 0},
                         {2, 1, 2},
                         {1, 1, 1}};
    showImage(F_thinning(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：细化");
}

void MainWindow::on_actionThickening_triggered()
{
    F_Kernel_i kernel = {{1, 1, 2},
                         {1, 0, 2},
                         {1, 2, 0}};
    showImage(F_thickening(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：粗化");
}

void MainWindow::on_actionDistanceTransform_triggered()
{
    showImage(F_improve(F_distance(getCurrentImage())));
    showResponseTime();
    showTip("上次操作：距离变换");
}

void MainWindow::on_actionSkeletonize_triggered()
{
    showImage(F_improve(F_skeletonize(getCurrentImage())));
    showResponseTime();
    showTip("上次操作：骨架");
}
