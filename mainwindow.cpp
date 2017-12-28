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
    qlabel = new QLabel(ui->whitebg);
    originalImage = NULL;
    setWindowFlags(windowFlags()
                   &~Qt::WindowMaximizeButtonHint); // 禁止最大化按钮
    setFixedSize(this->width(),this->height());     // 禁止拖动窗口大小
                      // 鼠标跟踪

//    centralWidget()->setMouseTracking(true);
//    setMouseTracking(true);
    qlabel->setMouseTracking(true);
#ifndef __RELEASE__
    on_actionOpen_triggered();
    on_actionDetectEdgeCanny_triggered();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint p = event->pos();
    QString s = QString("Mouse Location: [%1, %2]").arg(p.x()).arg(p.y());
    //qDebug()<<"mouseMoveEvent triggered.";
    ui->mouseLocation->setText(s);
    ui->colorBar->setStyleSheet("background-color:red");
}

void MainWindow::showResponseTime()
{
    QString s = QString("Response Time：%1s").arg(F_responseTime());
    ui->responseTime->setText(s);
}

void MainWindow::showImage_without_history(QImage *image)
{
    currentImage = image;
    qlabel->setPixmap(QPixmap::fromImage(autoscale()));
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
    qlabel->setPixmap(QPixmap::fromImage(autoscale()));
    ui->actionRecover->setEnabled(true);
}

QImage MainWindow::autoscale()
{
    if (!currentImage)
    {
        qDebug()<<"[error] autoscale: null currentImage";
        exit(-1);
    }
    QImage newImage= currentImage->scaled(PIC_HEIGHT, PIC_WIDTH,Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    }
    originalImage = image;
    //currentImage = image;

    // scale to appropriate size
    //QImage scaledImage = autoscale();

    // log
    showImage(image);
    //currentFile = fileName;
    //qlabel->setPixmap(QPixmap::fromImage(scaledImage));
    qlabel->resize(PIC_HEIGHT, PIC_WIDTH);
    qlabel->setAlignment(Qt::AlignCenter);
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

void MainWindow::on_actionBinarization_triggered()
{
    bool ok = false;
    int threshold = QInputDialog::getInt(this,tr("Visionary"),
                                         tr("请输入阈值"),
                                         0, 0, 255,
                                         1, &ok);
    if (!ok)
        return;
    showImage(F_binarization(currentImage, threshold));
    showResponseTime();
}

void MainWindow::on_actionBlur_triggered()
{
    showImage(F_blur(currentImage));
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
