#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QLabel>
#include <vector>

//#define __RELEASE__
//#define PIC_HEIGHT (860.0)
//#define PIC_WIDTH  (680.0)
#define HISTORY_MAX (8)

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
    void on_actionOpen_triggered();

    void on_actionDecolor_triggered();

    void on_actionRecover_triggered();

    void on_actionExit_triggered();

    void on_actionSave_as_triggered();

    void on_actionAbout_triggered();

    void on_actionBinarization_triggered();

    void on_actionBlur_triggered();

    void on_actionSharpen_triggered();

    void on_actionDilation_triggered();

    void on_actionErosion_triggered();

    void on_actionEqualizeHistogram_triggered();

    void on_actionResize_triggered();

    void on_actionResizeLinear_triggered();

    void on_actionChannelSeperation_triggered();

    void on_actionSpinNearest_triggered();

    void on_actionSpinLinear_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionDetectEdgeSobel_triggered();

    void on_actionDetectEdgeLaplacian_triggered();

    void on_actionDetectEdgeCanny_triggered();

private:
    Ui::MainWindow *ui;
    QString currentFile;
    QImage *originalImage,
           *currentImage;
    QImage thumbnail;
    QLabel *qlabel;
    std::vector<QImage *> historyImages;
    int historyIndex = -1;
    int showWidth = 0, showHeight = 0;
    //enum {V_NORMAL, V_GREY, V_BINARY} imageType;

    void showResponseTime();
    void showImage_without_history(QImage *image);
    void showImage(QImage *image);
    QImage autoscale();

protected:
    void mouseMoveEvent(QMouseEvent* event);

};

#endif // MAINWINDOW_H