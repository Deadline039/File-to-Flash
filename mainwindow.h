/**
 * @file    mainwindow.h
 * @author  Deadline039
 * @brief   
 * @version 0.1
 * @date    2025-09-10
 */


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QFile>
#include <QByteArray>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE
#define MAX_FILE_INFO_NUMBER 62

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

public slots:
    void onChangeOutputFileButtonClicked();

    void onAddFilesButtonClicked();

    void onRemoveFilesButtonClicked();

    void onMoveUpButtonClicked();

    void onMoveDownButtonClicked();

    void onStartOutputButtonClicked();

    void onOutputFormatChanged(int index);

    void onAddressAlignmentChanged(int index);

    void onStartAddrTextChanged(const QString &text);

    void onAddFileIndexStateChanged(Qt::CheckState state);

    void onFlashSizeTextChanged();

    void onFlashSizeUnitChanged(int index);

private:
    Ui::MainWindow *ui;

    QString outputFile;

    typedef struct {
        QString path;
        QString name;
        quint32 address;
        qint64 size;
    } fileList_t;

    quint32 flashStartAddress;
    quint32 fileStartAddress;
    quint32 flashSize;
    quint32 alignment;
    qint64 totalSize;

    QList<fileList_t *> fileList;
    QByteArray fileBuffer;

    struct {
        uint32_t count;

        struct {
            uint32_t address;
            uint32_t size;
            char fileName[58];
        } __attribute__((packed)) fileInfo[MAX_FILE_INFO_NUMBER];
    } __attribute__((packed)) fileIndex;


    QString bytes2Text(const quint8 *data, qsizetype len);

    void refreshFileList();

    void refreshTable();

    void outputHexFile(QFile *file);

    void outputBinFile(QFile *file);

    void outputS19File(QFile *file);
};


#endif //MAINWINDOW_H
