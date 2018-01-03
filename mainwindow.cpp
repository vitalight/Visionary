#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filter.h"

#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

#define DEFAULT_FILENAME "F:/MyCodes/Visionary/images/standered.png"
#define DEFAULT_FUNCTION on_actionadjustGradation_triggered()

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    images[imageIndex].originalImage = NULL;
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint); // 禁止最大化按钮
    setFixedSize(this->width(),this->height());     // 禁止拖动窗口大小


    qlabel = new QLabel(ui->whitebg);
    qlabel->resize(ui->whitebg->geometry().width(), ui->whitebg->geometry().height());
    qlabel->setAlignment(Qt::AlignCenter);
    qlabel->setMouseTracking(true);                 // 鼠标跟踪

    ui->responseTime->setVisible(false);
    ui->histogramArea->setVisible(false);           // 默认隐藏直方图
    ui->gridLayout->setHorizontalSpacing(1);

    // 初始化InputKernel
    for (int i = 0; i < 25; i++)
    {
        inputKernel.push_back(0.0);
    }
#ifndef __RELEASE__
    on_actionOpen_triggered();
    DEFAULT_FUNCTION;
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

void MainWindow::showThumbnail(QImage *image)
{
    if (!image) {
        qlabel->clear();
        return;
    }
    images[imageIndex].thumbnail = autoscale(image);
    qlabel->setPixmap(QPixmap::fromImage(images[imageIndex].thumbnail));
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

    if ((int)images[imageIndex].historyImages.size() > images[imageIndex].historyIndex + 1) {
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

QImage MainWindow::autoscale(QImage *image)
{
    if (!getCurrentImage())
    {
        qDebug()<<"[error] autoscale: null getCurrentImage()";
        exit(-1);
    }

    QImage newImage;
    if (showScale) {
        if (image) {
            newImage = image->scaled(ui->whitebg->geometry().height(), ui->whitebg->geometry().height(),
                                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            newImage = getCurrentImage()->scaled(ui->whitebg->geometry().height(), ui->whitebg->geometry().height(),
                                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        if (image) {
            newImage = QImage(*image);
        } else {
            newImage = QImage(*getCurrentImage());
        }
    }
    images[imageIndex].showWidth = newImage.width();
    images[imageIndex].showHeight = newImage.height();
    return newImage;
}

void MainWindow::showTip(QString str)
{
    ui->tip->setText(str);
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
            .arg(x*images[imageIndex].currentImage->width()/images[imageIndex].showWidth)
            .arg(y*images[imageIndex].currentImage->height()/images[imageIndex].showHeight);
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
    QString fileName = DEFAULT_FILENAME;
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
    ui->menuFilter->setEnabled(true);
    ui->actionRecover->setEnabled(false);
    showTip("已打开文件");
}

void MainWindow::on_switchButton_clicked()
{
    ui_recover();
    ui_clear();

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
    QImage *newImage = new QImage(*getAnotherImage());
    showImage(newImage);
    imageIndex = 1-imageIndex;
    showTip("已复制到另一图层");
}

void MainWindow::on_actionRecover_triggered()
{
    if (images[imageIndex].originalImage!=NULL) {
        showImage(images[imageIndex].originalImage);
        ui->actionRecover->setEnabled(false);
    }
    showTip("已还原文件");
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
    showTip("上次操作：恢复");
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
    ui_clear();
    showImage(F_decolor(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：去色");
}

void MainWindow::on_actionBlur_triggered()
{
    ui_clear();
    showImage(F_blur_gaussian(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：高斯模糊");
}

void MainWindow::on_actionBlurMean_triggered()
{
    ui_clear();
    showImage(F_blur_mean(getCurrentImage(), 5));
    showResponseTime();
    showTip("上次操作：中值模糊");
}

void MainWindow::on_actionBlurMedian_triggered()
{
    ui_clear();
    showImage(F_blur_median(getCurrentImage(), 3));
    showResponseTime();
    showTip("上次操作：均值模糊");
}

void MainWindow::on_actionSharpen_triggered()
{
    ui_clear();
    showImage(F_sharpen(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：锐化");
}



void MainWindow::on_actionEqualizeHistogram_triggered()
{
    ui_clear();
    showImage(F_equalizeHistogram(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：直方图均衡化");
}

void MainWindow::on_actionDetectEdgeSobel_triggered()
{
    ui_clear();
    showImage(F_detectEdge(getCurrentImage(), F_SOBEL));
    showResponseTime();
    showTip("上次操作：Sobel边缘检测");
}

void MainWindow::on_actionDetectEdgeLaplacian_triggered()
{
    ui_clear();
    showImage(F_detectEdge(getCurrentImage(), F_LAPLACIAN));
    showResponseTime();
    showTip("上次操作：拉普拉斯边缘检测");
}

void MainWindow::on_actionDetectEdgeCanny_triggered()
{
    ui_clear();
    showImage(F_detectEdge(getCurrentImage(), F_CANNY));
    showResponseTime();
    showTip("上次操作：Canny边缘检测");
}

void MainWindow::on_actionOtsu_triggered()
{
    ui_clear();
    showImage(F_binarization_Otsu(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：大津算法二值化");
}

void MainWindow::on_actionAdd_triggered()
{
    ui_clear();
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
    ui_clear();
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
    ui_clear();
    if (!getAnotherImage()) {
        QMessageBox::about(this, "非法操作",
                           "另一图层暂无图像。");
        return;
    }
    showImage(F_times(getCurrentImage(), getAnotherImage()));
    showResponseTime();
    showTip("上次操作：乘操作");
}

void MainWindow::updateHistogram()
{
    if (showHistogram)
    {
        ui->histogramArea->histogram = F_getHistogram(getCurrentImage());
        update();
    }
}

void MainWindow::updateHistogram_thumbnail()
{
    if (showHistogram)
    {
        ui->histogramArea->histogram = F_getHistogram(&(images[imageIndex].thumbnail));
        update();
    }
}

void MainWindow::on_actionShowHistogram_toggled(bool arg1)
{
    showHistogram = arg1;
    ui->histogramArea->setVisible(arg1);
    updateHistogram();
}

void MainWindow::on_actionAutoscale_toggled(bool arg1)
{
    showScale = arg1;
    showImage_without_history(getCurrentImage());
}

void MainWindow::on_actionSkeletonize_triggered()
{
    ui_clear();
    showImage(F_skeletonize(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：骨架");
}

void MainWindow::on_actionContrastStretch_triggered()
{
    ui_clear();
    showImage(F_contrastStretch(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：对比度拉伸");
}

void MainWindow::on_actionSkeletonReconstruct_triggered()
{
    ui_clear();
    showImage(F_skeletonReconstruct(getCurrentImage()));
    showResponseTime();
    showTip("上次操作：骨架重构");
}

void MainWindow::on_actionReconstruct_triggered()
{
    ui_clear();
    showImage(F_reconstruct(getCurrentImage(), getAnotherImage()));
    showResponseTime();
    showTip("上次操作：形态学重构");
}
/************************************************
 * UI slot
 ************************************************/
QDialogButtonBox *MainWindow::createButtonBox()
{
    QDialogButtonBox *dialogButtonBox = new QDialogButtonBox;
    dialogButtonBox->setStyleSheet("top:30px");
    addMyWidget(dialogButtonBox);
    dialogButtonBox->addButton("确定", QDialogButtonBox::AcceptRole);
    dialogButtonBox->addButton("取消", QDialogButtonBox::RejectRole);
    connect(dialogButtonBox, SIGNAL(rejected()), this, SLOT(ui_recover()));
    connect(dialogButtonBox, SIGNAL(rejected()), this, SLOT(ui_clear()));

    return dialogButtonBox;
}

void MainWindow::addMyWidget(QWidget *widget)
{
    widgetList.push_back(widget);
//    ui->gridLayout->addWidget(widget, widgetList.size()-1, 0, 2, -1);
    ui->gridLayout->addWidget(widget, ui->gridLayout->rowCount(), 0, 2, -1);
}

void MainWindow::addMyWidget(QWidget *widget, int row, int column, int rowSpan, int columnSpan)
{
    widgetList.push_back(widget);
    ui->gridLayout->addWidget(widget, row, column, rowSpan, columnSpan);
}

void MainWindow::ui_recover()
{
    showImage_without_history(getCurrentImage());
}

void MainWindow::ui_clear()
{
    for (QWidget *cur:widgetList)
    {
        ui->gridLayout->removeWidget(cur);
        delete cur;
    }
    spinBoxes.clear();
    widgetList.clear();
    ui->gridLayout->update();
    ui->histogramArea->clearLine();

    for (int i = 0; i<25; i++)
    {
        inputKernel[i] = 0;
    }
}

void MainWindow::ui_change_val1(int val)
{
    ui_val1 = val;
}

void MainWindow::ui_change_val2(int val)
{
    ui_val2 = val;
}

void MainWindow::ui_change_val3(int val)
{
    ui_val3 = val;
}

void MainWindow::ui_change_val4(int val)
{
    ui_val4 = val;
}

void MainWindow::ui_change_kernel(int val)
{
    inputKernel[val] = ui_val2;
}

void MainWindow::ui_change_spinBox_number(int val)
{
    ui_val1 = 5 - val*2;
    if (val==1) {
        for (QWidget *widget:spinBoxes)
        {
            widget->setVisible(false);
        }
    } else {
        for (QWidget *widget:spinBoxes)
        {
            widget->setVisible(true);
        }
    }
}

void MainWindow::ui_input_kernel()
{
    ui_val1 = 5;
    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("Kernel大小5", 0);
    comboBox->addItem("Kernel大小3", 1);
    addMyWidget(comboBox);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_change_spinBox_number(int)));

    QSignalMapper *signalMapper = new QSignalMapper(this);
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            QSpinBox *widget = new QSpinBox;
            widget->setMaximum(20);
            widget->setMinimum(-20);
            if (i > 2 || j > 2)
                spinBoxes.push_back(widget);
            addMyWidget(widget, i+2, j, 1, 1);
            connect(widget, SIGNAL(valueChanged(int)), this , SLOT(ui_change_val2(int)));
            connect(widget, SIGNAL(valueChanged(int)), signalMapper, SLOT(map()));
            signalMapper->setMapping(widget, i*5+j);
        }
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(ui_change_kernel(int)));
}

QSpinBox *MainWindow::ui_mySpinBox(int minimum, int maximum, int value)
{
    QSpinBox *spinBox = new QSpinBox;
    spinBox->setMinimum(minimum);
    spinBox->setMaximum(maximum);
    spinBox->setValue(value);
    addMyWidget(spinBox);
    return spinBox;
}

U_Kernel_i MainWindow::constructKernel_i()
{
    U_Kernel_i kernel;
    for (int i = 0; i < ui_val1; i++)
    {
        std::vector<int> line;
        for (int j = 0; j < ui_val1; j++)
        {
            line.push_back(inputKernel[i*5+j]);
        }
        kernel.push_back(line);
    }
    return kernel;
}

U_Kernel_d MainWindow::constructKernel_d()
{
    U_Kernel_d kernel;
    for (int i = 0; i < ui_val1; i++)
    {
        std::vector<double> line;
        for (int j = 0; j < ui_val1; j++)
        {
            line.push_back(inputKernel[i*5+j]);
        }
        kernel.push_back(line);
    }
    return kernel;
}

QSlider *MainWindow::ui_mySlider(int minimum, int maximum, int singleStep, QString name)
{
    QSpinBox *spinBox1 = new QSpinBox;
    spinBox1->setMinimum(minimum);
    spinBox1->setMaximum(maximum);
    //addMyWidget(spinBox1);

    if (name != "") {
        QLabel *label = new QLabel;
        label->setStyleSheet("color:white");
        label->setText(name);
        addMyWidget(label, ui->gridLayout->rowCount(), 0, 1, 1);
        addMyWidget(spinBox1, ui->gridLayout->rowCount()-1, 1, 1, 3);
    }

    QSlider *slider1 = new QSlider(Qt::Horizontal);
    slider1->setMinimum(minimum);
    slider1->setMaximum(maximum);
    slider1->setSingleStep(singleStep);
    addMyWidget(slider1);

    connect(slider1, SIGNAL(valueChanged(int)), spinBox1, SLOT(setValue(int)));
    connect(spinBox1, SIGNAL(valueChanged(int)), slider1, SLOT(setValue(int)));

    return slider1;
}

/************************************************
 * Signal slot function with custom input
 ************************************************/
void MainWindow::slot_channelSeperation_preview()
{
    switch (ui_val1)
    {
    case 0:
        showThumbnail(F_seperation(getCurrentImage(), F_R));
        break;
    case 1:
        showThumbnail(F_seperation(getCurrentImage(), F_G));
        break;
    case 2:
        showThumbnail(F_seperation(getCurrentImage(), F_B));
        break;
    default:
        break;
    }
}

void MainWindow::slot_channelSeperation()
{
    switch (ui_val1)
    {
    case 0:
        showImage(F_seperation(getCurrentImage(), F_R));
        break;
    case 1:
        showImage(F_seperation(getCurrentImage(), F_G));
        break;
    case 2:
        showImage(F_seperation(getCurrentImage(), F_B));
        break;
    default:
        break;
    }
    showResponseTime();
    showTip("上次操作：通道分离");
}

void MainWindow::on_actionChannelSeperation_triggered()
{
    ui_clear();
    ui_val1 = 0;
    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("红", 0);
    comboBox->addItem("绿", 1);
    comboBox->addItem("蓝", 2);
    addMyWidget(comboBox);

    QDialogButtonBox *dialogButtonBox = createButtonBox();

    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_channelSeperation_preview()));
    connect(dialogButtonBox, SIGNAL(accepted()), this, SLOT(slot_channelSeperation()));
    connect(dialogButtonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));

    slot_channelSeperation_preview();
    showTip("正在进行：通道分离");
}

void MainWindow::slot_adjustHSB()
{
    showImage(F_adjustHSB(getCurrentImage(), ui_val1, ui_val2, ui_val3));
    showResponseTime();
    showTip("上次操作：调整HSB");
}

void MainWindow::slot_adjustHSB_preview()
{
    showThumbnail(F_adjustHSB(getCurrentImage(), ui_val1, ui_val2, ui_val3));
}

void MainWindow::on_actionadjustHSB_triggered()
{
    ui_clear();
    ui_val1 = 0;
    ui_val2 = 0;
    ui_val3 = 0;
    QSlider *slider1 = ui_mySlider(-180, 180, 5),
            *slider2 = ui_mySlider(-100, 100, 5),
            *slider3 = ui_mySlider(-100, 100, 5);

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_adjustHSB_preview()));

    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_adjustHSB_preview()));

    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val3(int)));
    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(slot_adjustHSB_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_adjustHSB()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：调整HSB");
}

void MainWindow::slot_convolution()
{
    U_Kernel_d kernel = constructKernel_d();

    showImage(F_convolution(getCurrentImage(), kernel, U_getKernelSum(kernel)));
    showResponseTime();
    showTip("上次操作：自定义滤波");
}

void MainWindow::on_actionCustom_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_convolution()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：自定义滤波");
}

void MainWindow::slot_doubleTreshold_preview()
{
    int val1 = std::min(ui_val1, ui_val2),
        val2 = std::max(ui_val1, ui_val2);

    showThumbnail(F_binarization_double(getCurrentImage(), val1, val2));
}

void MainWindow::slot_doubleTreshold()
{
    int val1 = std::min(ui_val1, ui_val2),
        val2 = std::max(ui_val1, ui_val2);
    showImage(F_binarization_double(getCurrentImage(), val1, val2));
    showResponseTime();
    showTip("上次操作：双阈值二值化");
}

void MainWindow::on_actionDoubleThreshold_triggered()
{
    ui_clear();

    ui_val1 = 0;
    ui_val2 = 0;
    QSlider *slider1 = ui_mySlider(0, 255, 1);
    QSlider *slider2 = ui_mySlider(0, 255, 1);

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_doubleTreshold_preview()));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_doubleTreshold_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_doubleTreshold()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：双阈值二值化");
}

void MainWindow::slot_cut_preview()
{
    showThumbnail(F_cut(getCurrentImage(), ui_val1, ui_val2, ui_val3, ui_val4));
}

void MainWindow::slot_cut()
{
    showImage(F_cut(getCurrentImage(), ui_val1, ui_val2, ui_val3, ui_val4));
    showResponseTime();
    showTip("上次操作：裁剪");
}

void MainWindow::on_actionCut_triggered()
{
    ui_clear();
    ui_val1 = 1;
    ui_val2 = 1;
    ui_val3 = getCurrentImage()->width();
    ui_val4 = getCurrentImage()->height();

    QSpinBox *spinBox1 = ui_mySpinBox(1, getCurrentImage()->width(), 1),
             *spinBox2 = ui_mySpinBox(1, getCurrentImage()->height(), 1),
             *spinBox3 = ui_mySpinBox(1, getCurrentImage()->width(), getCurrentImage()->width()),
             *spinBox4 = ui_mySpinBox(1, getCurrentImage()->height(), getCurrentImage()->height());

    connect(spinBox1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(spinBox1, SIGNAL(valueChanged(int)), this, SLOT(slot_cut_preview()));

    connect(spinBox2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(spinBox2, SIGNAL(valueChanged(int)), this, SLOT(slot_cut_preview()));

    connect(spinBox3, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val3(int)));
    connect(spinBox3, SIGNAL(valueChanged(int)), this, SLOT(slot_cut_preview()));

    connect(spinBox4, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val4(int)));
    connect(spinBox4, SIGNAL(valueChanged(int)), this, SLOT(slot_cut_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_cut()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));

    showTip("正在进行：裁剪");
}

void MainWindow::slot_resize_preview()
{
    if (ui_val3) {
        showThumbnail(F_resize(getCurrentImage(), ui_val1, ui_val2, F_LINEAR));
    } else {
        showThumbnail(F_resize(getCurrentImage(), ui_val1, ui_val2, F_NEAREST));
    }
}

void MainWindow::slot_resize()
{
    if (ui_val3) {
        showImage(F_resize(getCurrentImage(), ui_val1, ui_val2, F_LINEAR));
    } else {
        showImage(F_resize(getCurrentImage(), ui_val1, ui_val2, F_NEAREST));
    }
    showResponseTime();
    showTip("上次操作：缩放");
}

void MainWindow::on_actionResize_triggered()
{
    ui_clear();

    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("最近邻", 0);
    comboBox->addItem("双线性插值", 1);
    addMyWidget(comboBox);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_change_val3(int)));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_resize_preview()));

    ui_val1 = getCurrentImage()->width();
    ui_val2 = getCurrentImage()->height();

    QSlider *slider1 = ui_mySlider(1, 2000, 1),
            *slider2 = ui_mySlider(1, 2000, 1);
    slider1->setValue(getCurrentImage()->width());
    slider2->setValue(getCurrentImage()->height());

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_resize_preview()));

    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_resize_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_resize()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));

    showTip("正在进行：缩放");
}

void MainWindow::slot_spin_preview()
{
    if (ui_val2) {
        showThumbnail(F_spin(getCurrentImage(), ui_val1, F_LINEAR));
    } else {
        showThumbnail(F_spin(getCurrentImage(), ui_val1, F_NEAREST));
    }
}

void MainWindow::slot_spin()
{
    if (ui_val2) {
        showImage(F_spin(getCurrentImage(), ui_val1, F_LINEAR));
    } else {
        showImage(F_spin(getCurrentImage(), ui_val1, F_NEAREST));
    }
    showResponseTime();
    showTip("上次操作：旋转");
}

void MainWindow::on_actionSpinNearest_triggered()
{
    ui_clear();

    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("最近邻", 0);
    comboBox->addItem("双线性插值", 1);
    addMyWidget(comboBox);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_spin_preview()));

    ui_val1 = 0;
    ui_val2 = 0;

    QSlider *slider1 = ui_mySlider(0, 360, 5);

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_spin_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_spin()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：旋转");
}

void MainWindow::slot_dilation()
{
    U_Kernel_i kernel = constructKernel_i();

    showImage(F_dilation(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：膨胀");
}

void MainWindow::on_actionDilation_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_dilation()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：膨胀");
}

void MainWindow::slot_erosion()
{
    U_Kernel_i kernel = constructKernel_i();

    showImage(F_erosion(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：腐蚀");
}

void MainWindow::on_actionErosion_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_erosion()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：腐蚀");
}

void MainWindow::slot_morphologicalOpen()
{
    U_Kernel_i kernel = constructKernel_i();

    showImage(F_open(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：开操作");
}

void MainWindow::on_actionMorphologicalOpen_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_morphologicalOpen()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：开操作");
}

void MainWindow::slot_morphologicalClose()
{
    std::vector<std::vector<int>> kernel = constructKernel_i();
    showImage(F_close(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：闭操作");
}

void MainWindow::on_actionMorphologicalClose_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_morphologicalClose()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：闭操作");
}

void MainWindow::slot_thining()
{
    U_Kernel_i kernel = constructKernel_i();
    showImage(F_thinning(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：细化");
}

void MainWindow::on_actionThining_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_thining()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：细化");
}

void MainWindow::slot_thickening()
{
    U_Kernel_i kernel = constructKernel_i();
    showImage(F_thickening(getCurrentImage(), kernel));
    showResponseTime();
    showTip("上次操作：粗化");
}

void MainWindow::on_actionThickening_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_thickening()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：粗化");
}

void MainWindow::slot_distance()
{
    U_Kernel_i kernel = constructKernel_i();
    showImage(F_contrastStretch(F_distance(getCurrentImage())));
    showResponseTime();
    showTip("上次操作：距离变换");
}

void MainWindow::on_actionDistanceTransform_triggered()
{
    ui_clear();
    ui_input_kernel();

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_distance()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    showTip("正在进行：距离变换");
}

void MainWindow::slot_contrastLinear_preview()
{
    showThumbnail(F_contrast_linear(getCurrentImage(), ui_val1/10.0, ui_val2));
    updateHistogram_thumbnail();
    ui->histogramArea->painterLine(ui_val1/10.0, ui_val2);
}

void MainWindow::slot_contrastLinear()
{
    showImage(F_contrast_linear(getCurrentImage(), ui_val1/10.0, ui_val2));
    showTip("上次操作：线性对比度调节");
    ui->histogramArea->clearLine();
    showResponseTime();
}

void MainWindow::on_actionContrastLinear_triggered()
{
    ui_clear();

    ui_val1 = 10;
    ui_val2 = 0;
    QSlider *slider1 = ui_mySlider(0, 30, 1),
            *slider2 = ui_mySlider(-255, 255, 1);

    slider1->setValue(10);
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastLinear_preview()));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastLinear_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_contrastLinear()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    slot_contrastLinear_preview();
    showTip("正在进行：线性对比度调节");
}

void MainWindow::slot_contrastSectionLinear_preview()
{
    showThumbnail(F_contrast_section(getCurrentImage(), ui_val1, ui_val2, ui_val3, ui_val4));
    updateHistogram_thumbnail();
    ui->histogramArea->painterSection(ui_val1, ui_val2, ui_val3, ui_val4);
}

void MainWindow::slot_contrastSectionLinear()
{
    showImage(F_contrast_section(getCurrentImage(), ui_val1, ui_val2, ui_val3, ui_val4));
    showTip("上次操作：分段线性调节");
    showResponseTime();
    ui->histogramArea->clearLine();
}

void MainWindow::on_actionContrastSectionLinear_triggered()
{
    ui_clear();
    ui_val1 = 0;
    ui_val2 = 0;
    ui_val3 = 255;
    ui_val4 = 255;
    QSlider *slider1 = ui_mySlider(0, 255, 1),
            *slider2 = ui_mySlider(0, 255, 1),
            *slider3 = ui_mySlider(0, 255, 1),
            *slider4 = ui_mySlider(0, 255, 1);

    slider1->setValue(0);
    slider2->setValue(0);
    slider3->setValue(255);
    slider4->setValue(255);

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastSectionLinear_preview()));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastSectionLinear_preview()));
    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val3(int)));
    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastSectionLinear_preview()));
    connect(slider4, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val4(int)));
    connect(slider4, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastSectionLinear_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_contrastSectionLinear()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));
    slot_contrastSectionLinear_preview();
    showTip("正在进行：分段线性调节");
}

void MainWindow::slot_contrastNonlinear_preview()
{
    if (ui_val2) {
        showThumbnail(F_contrast_exponential(getCurrentImage(), ui_val1/10.0));
        ui->histogramArea->painterExp(ui_val1/10.0);
    } else {
        showThumbnail(F_contrast_logarithm(getCurrentImage(), ui_val1/10.0));
        ui->histogramArea->painterLog(ui_val1/10.0);
    }
    updateHistogram_thumbnail();
}

void MainWindow::slot_contrastNonlinear()
{
    if (ui_val2) {
        showImage(F_contrast_exponential(getCurrentImage(), ui_val1/10.0));
    } else {
        showImage(F_contrast_logarithm(getCurrentImage(), ui_val1/10.0));
    }
    ui->histogramArea->clearLine();
    showTip("上次操作：非线性调节");
    showResponseTime();
}

void MainWindow::on_actionContrastNonlinear_triggered()
{
    ui_clear();
    ui_val1 = 0;
    ui_val2 = 0;

    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("对数变换", 0);
    comboBox->addItem("指数变换", 1);
    addMyWidget(comboBox);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_contrastNonlinear_preview()));

    QSlider *slider1 = ui_mySlider(0, 50, 1);
    slider1->setValue(0);
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_contrastNonlinear_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_contrastNonlinear()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));

    slot_contrastNonlinear_preview();
    showTip("正在进行：分段线性调节");
}

void MainWindow::slot_colorGradiation_preview()
{
    showThumbnail(F_colorGradation(getCurrentImage(), ui_val1, ui_val2/100.0, ui_val3));
}

void MainWindow::slot_colorGradiation()
{
    showImage(F_colorGradation(getCurrentImage(), ui_val1, ui_val2/100.0, ui_val3));
    showResponseTime();
    showTip("上次操作：色阶调节");
}

void MainWindow::on_actionadjustGradation_triggered()
{
    ui_clear();
    ui_val1 = 0;
    ui_val2 = 100;
    ui_val3 = 255;

    QSlider *slider1 = ui_mySlider(0, 253, 1, "阴影"),
            *slider2 = ui_mySlider(1, 1000, 1, "中间调"),
            *slider3 = ui_mySlider(2, 255, 1, "高光");
    slider1->setValue(0);
    slider2->setValue(100);
    slider3->setValue(255);

    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val1(int)));
    connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(slot_colorGradiation_preview()));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val2(int)));
    connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(slot_colorGradiation_preview()));
    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(ui_change_val3(int)));
    connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(slot_colorGradiation_preview()));

    QDialogButtonBox *buttonBox = createButtonBox();
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slot_colorGradiation()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(ui_clear()));

    slot_contrastNonlinear_preview();
    showTip("正在进行：色阶调节");
}
