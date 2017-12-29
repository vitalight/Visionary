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
    originalImage = NULL;
    setWindowFlags(windowFlags()
                   &~Qt::WindowMaximizeButtonHint); // 禁止最大化按钮
    setFixedSize(this->width(),this->height());     // 禁止拖动窗口大小


    qlabel = new QLabel(ui->whitebg);
    qlabel->resize(ui->whitebg->geometry().width(), ui->whitebg->geometry().height());
    qlabel->setAlignment(Qt::AlignCenter);
    qlabel->setMouseTracking(true);                 // 鼠标跟踪
#ifndef __RELEASE__
    on_actionOpen_triggered();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint p = event->pos();
    static int x_minus = centralWidget()->geometry().x() + ui->whitebg->geometry().x() + ui->whitebg->geometry().width()/2,
               y_minus = centralWidget()->geometry().y() + ui->whitebg->geometry().y() + ui->whitebg->geometry().height()/2;
    int x = qBound(0, p.x() - x_minus + showWidth/2, showWidth-1),
        y = qBound(0, p.y() - y_minus + showHeight/2, showHeight-1);

    // 显示鼠标坐标
    QString s = QString("Mouse Location: [%1, %2]")
            .arg(x+1)
            .arg(y+1);
    ui->mouseLocation->setText(s);

    // 显示对应颜色
    QRgb *bits = (QRgb*)thumbnail.constBits();
    int r = qRed(bits[y*showWidth+x]),
        g = qGreen(bits[y*showWidth+x]),
        b = qBlue(bits[y*showWidth+x]);
    s = QString("background-color:rgb(%1,%2,%3)").arg(r).arg(g).arg(b);
    ui->colorBar->setStyleSheet(s);
    s = QString("RGB:(%1,%2,%3)").arg(r).arg(g).arg(b);
    ui->colorText->setText(s);

}

void MainWindow::showResponseTime()
{
    QString s = QString("Response Time：%1s").arg(F_responseTime());
    ui->responseTime->setText(s);
}

void MainWindow::showImage_without_history(QImage *image)
{
    currentImage = image;
    thumbnail = autoscale();
    qlabel->setPixmap(QPixmap::fromImage(thumbnail));
}

void MainWindow::showImage(QImage *image)
{
//    if (image->width() != currentImage->width() ||
//            image->height() != currentImage->height()) {
//        qlabel->resize(image->width(), image->height());
//    }
    if (currentImage == image)
    {
        return;
    }

    // log in historyImages for future undo and redo operation
    if (historyImages.size() >= 1) {
        ui->actionUndo->setEnabled(true);
    }

    if ((int)historyImages.size() > historyIndex + 2) {
        historyImages[++historyIndex] = image;
        historyImages.erase(historyImages.begin()+historyIndex+1, historyImages.end());
        ui->actionRedo->setEnabled(false);
    } else {
        historyImages.push_back(image);
        if (historyImages.size() > HISTORY_MAX)
        {
            QImage *back = historyImages[0];
            historyImages.erase(historyImages.begin());

            // function in [filter.h] is allowed to return input image pointer,
            // so (back != historyImage.front()) is required
            if (back != originalImage && back != historyImages[0])
                free(back);
            qDebug()<<"[log] showImage:historyImage.size(): "<<historyImages.size();
        } else {
            historyIndex++;
        }
    }

    currentImage = image;
    thumbnail = autoscale();
    qlabel->setPixmap(QPixmap::fromImage(thumbnail));
    ui->actionRecover->setEnabled(true);
}

QImage MainWindow::autoscale()
{
    if (!currentImage)
    {
        qDebug()<<"[error] autoscale: null currentImage";
        exit(-1);
    }
    QImage newImage= currentImage->scaled(ui->whitebg->geometry().height(), ui->whitebg->geometry().height(),
                                          Qt::KeepAspectRatio, Qt::SmoothTransformation);

    showWidth = newImage.width();
    showHeight = newImage.height();
    return newImage;
}

/************************************************
 * Signal slot function for UI to call
 ************************************************/
void MainWindow::on_actionOpen_triggered()
{
    // open file
#ifdef __RELEASE__
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("打开图片"),
                                                    "F:/MyCodes/Visionary/images",
                                                    "Images (*.png *.bmp *.jpg *.jpeg)");
#else
    QString fileName = "F:/MyCodes/Visionary/images/standered.png";
#endif
    if (fileName == "" || fileName == NULL) {
        return;
    }
    QImage *image = new QImage;
    if(!image->load(fileName)) {
        QMessageBox::information(this, tr("打开图像失败"), tr("打开图像失败"));
        return;
    }

    originalImage = image;

    // log
    showImage(image);
    //currentFile = fileName;
    //qlabel->resize(860, 680);
    //qDebug()<<"Resize: "<<ui->whitebg->geometry().width()<<", "<<ui->whitebg->geometry().height();
    ui->menuFilter->setEnabled(true);
    ui->actionRecover->setEnabled(true);
}

void MainWindow::on_actionRecover_triggered()
{
    if (originalImage!=NULL) {
        showImage(originalImage);
        ui->actionRecover->setEnabled(false);
    }
}

void MainWindow::on_actionUndo_triggered()
{
    if (historyImages.size()==0) {
        qDebug() << "[Error] Undo without history";
        return;
    }
    showImage_without_history(historyImages[--historyIndex]);
    if (!historyIndex) {
        ui->actionUndo->setEnabled(false);
    }
    ui->actionRedo->setEnabled(true);
}

void MainWindow::on_actionRedo_triggered()
{
    showImage_without_history(historyImages[++historyIndex]);
    if (historyIndex == (int)historyImages.size() - 1) {
        ui->actionRedo->setEnabled(false);
    }
    ui->actionUndo->setEnabled(true);
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
    currentImage->save(fileName);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于Visionary",
                       "Visionary是一个强大的图像处理软件。");
}

void MainWindow::on_actionDecolor_triggered()
{
    showImage(F_decolor(currentImage));
    showResponseTime();
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
//    showImage(F_binarization(currentImage, threshold));
//    showResponseTime();
//}

void MainWindow::on_actionBlur_triggered()
{
    showImage(F_blur_gaussian(currentImage));
    showResponseTime();
}

void MainWindow::on_actionSharpen_triggered()
{
    showImage(F_sharpen(currentImage));
    showResponseTime();
}

void MainWindow::on_actionDilation_triggered()
{
    showImage(F_dilation(currentImage));
    showResponseTime();
}

void MainWindow::on_actionErosion_triggered()
{
    showImage(F_erosion(currentImage));
    showResponseTime();
}

void MainWindow::on_actionEqualizeHistogram_triggered()
{
    showImage(F_equalizeHistogram(currentImage));
    showResponseTime();
}

void MainWindow::on_actionResize_triggered()
{
    showImage(F_resize(currentImage, F_NEAREST));
    showResponseTime();
}

void MainWindow::on_actionResizeLinear_triggered()
{
    showImage(F_resize(currentImage, F_LINEAR));
    showResponseTime();
}

void MainWindow::on_actionChannelSeperation_triggered()
{
    showImage(F_seperation(currentImage, F_R));
    showResponseTime();
}

void MainWindow::on_actionSpinNearest_triggered()
{
    showImage(F_spin(currentImage, 45, F_NEAREST));
    showResponseTime();
}

void MainWindow::on_actionSpinLinear_triggered()
{
    showImage(F_spin(currentImage, 45, F_LINEAR));
    showResponseTime();
}

void MainWindow::on_actionDetectEdgeSobel_triggered()
{
    showImage(F_detectEdge(currentImage, F_SOBEL));
    showResponseTime();
}

void MainWindow::on_actionDetectEdgeLaplacian_triggered()
{
    showImage(F_detectEdge(currentImage, F_LAPLACIAN));
    showResponseTime();
}

void MainWindow::on_actionDetectEdgeCanny_triggered()
{
    showImage(F_detectEdge(currentImage, F_CANNY));
    showResponseTime();
}

void MainWindow::on_actionOtsu_triggered()
{
    showImage(F_binarization_Otsu(currentImage));
    showResponseTime();
}

void MainWindow::on_actionDoubleThreshold_triggered()
{
    showImage(F_binarization_double(currentImage, 100, 200));
    showResponseTime();
}

void MainWindow::on_actionBlurMean_triggered()
{
    showImage(F_blur_mean(currentImage, 5));
    showResponseTime();
}

void MainWindow::on_actionBlurMedian_triggered()
{
    showImage(F_blur_median(currentImage, 3));
    showResponseTime();
}

void MainWindow::on_actionMorphologicalOpen_triggered()
{
    showImage(F_open(currentImage));
    showResponseTime();
}

void MainWindow::on_actionMorphologicalClose_triggered()
{
    showImage(F_close(currentImage));
    showResponseTime();
}
