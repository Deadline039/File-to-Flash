/**
 * @file    mainwindow.cpp
 * @author  Deadline039
 * @brief   
 * @version 0.1
 * @date    2025-09-10
 */


// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved

#include <QFileDialog>
#include <QMessageBox>

#include "mainwindow.h"

#include "ui_MainWindow.h"

#define ADDRESS_ALIGNMENT(address) ((address) + ((address) % alignment))

MainWindow::MainWindow(QWidget *parent) : QWidget(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->fileTable->setColumnWidth(1, 200);
    ui->fileTable->setColumnWidth(3, 300);
    ui->addFileIndexCheckbox->setText(
        "Add File Index (Recommend, Need " + QString::number(sizeof(fileIndex)) + " B Space, Maximum store " +
        QString::number(
            MAX_FILE_INFO_NUMBER) + " files)");

    /* Get default start address, alignment and flash size */
    onAddressAlignmentChanged(ui->addressAlignComboBox->currentIndex());
    onStartAddrTextChanged(ui->startAddrTextBox->text());
    onFlashSizeTextChanged();

    totalSize = 0;

    connect(ui->changeOutputFileButton, SIGNAL(clicked()), this, SLOT(onChangeOutputFileButtonClicked()));
    connect(ui->addFilesButton, SIGNAL(clicked()), this, SLOT(onAddFilesButtonClicked()));
    connect(ui->removeFilesButton, SIGNAL(clicked()), this, SLOT(onRemoveFilesButtonClicked()));
    connect(ui->moveUpButton, SIGNAL(clicked()), this, SLOT(onMoveUpButtonClicked()));
    connect(ui->moveDownButton, SIGNAL(clicked()), this, SLOT(onMoveDownButtonClicked()));
    connect(ui->startOutputButton, SIGNAL(clicked()), this, SLOT(onStartOutputButtonClicked()));

    connect(ui->outputFormatComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onOutputFormatChanged(int)));
    connect(ui->addressAlignComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onAddressAlignmentChanged(int)));
    connect(ui->flashSizeUnitComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onFlashSizeUnitChanged(int)));

    connect(ui->startAddrTextBox, SIGNAL(textChanged(const QString&)), this,
            SLOT(onStartAddrTextChanged(const QString&)));
    connect(ui->flashSizeTextBox, SIGNAL(editingFinished()), this,
            SLOT(onFlashSizeTextChanged()));

    connect(ui->addFileIndexCheckbox, SIGNAL(checkStateChanged(Qt::CheckState)), this,
            SLOT(onAddFileIndexStateChanged(Qt::CheckState)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onChangeOutputFileButtonClicked() {
    QFileDialog fileDialog;
    QString suffix = "*." + ui->outputFormatComboBox->currentText();
    QString fileName = fileDialog.getSaveFileName(this, "Save file", "", suffix);
    if (!fileName.isEmpty()) {
        ui->outputFilePathTextBox->setText(fileName);
    }
}

void MainWindow::onAddFilesButtonClicked() {
    QFileDialog fileDialog;
    const QString filePath = fileDialog.getOpenFileName(this, "Open file", "", "*");
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Open file " + filePath + " error: " + file.errorString());
        return;
    }

    quint32 fileAddress;
    if (ui->fileTable->rowCount() <= 0) {
        fileAddress = ADDRESS_ALIGNMENT(fileStartAddress);
    } else {
        /* last item address + size */
        fileAddress = ADDRESS_ALIGNMENT(flashStartAddress + totalSize);
    }

    const qint64 fileSize = file.size();

    if ((fileAddress + fileSize) > (flashStartAddress + flashSize)) {
        QMessageBox::warning(this, "Error", "File exceed the flash. ");
        file.close();
        return;
    }

    const QFileInfo fileInfo(file);
    auto *newFileItem = new fileList_t;
    newFileItem->name = fileInfo.fileName();
    newFileItem->path = filePath;
    newFileItem->address = fileAddress;
    newFileItem->size = fileSize;
    fileList.append(newFileItem);
    file.close();

    qDebug() << "===========================\nFile Index: " << fileList.count() - 1;
    qDebug() << "File path: " << filePath;
    qDebug() << "File size: " << newFileItem->size;
    qDebug() << "File address: " << newFileItem->address;
    qDebug() << "Total size: " << totalSize;

    refreshFileList();
    refreshTable();
}

void MainWindow::onRemoveFilesButtonClicked() {
    if (ui->fileTable->currentRow() == -1) {
        return;
    }

    delete fileList.at(ui->fileTable->currentRow());
    fileList.remove(ui->fileTable->currentRow());
    refreshFileList();
    refreshTable();
}

void MainWindow::onMoveUpButtonClicked() {
    if (ui->fileTable->currentRow() < 1) {
        /* No select (-1) or select top (0) */
        return;
    }
    const int rowIndex = ui->fileTable->currentRow();
    fileList.swapItemsAt(rowIndex, rowIndex - 1);
    ui->fileTable->setCurrentCell(rowIndex - 1, 0);
    refreshFileList();
    refreshTable();
}

void MainWindow::onMoveDownButtonClicked() {
    if (ui->fileTable->currentRow() == -1 || ui->fileTable->rowCount() <= 1) {
        /* No select (-1) or row count less than 1 */
        return;
    }

    if (ui->fileTable->currentRow() == ui->fileTable->rowCount() - 1) {
        /* Last one, don't need move down. */
        return;
    }

    const int rowIndex = ui->fileTable->currentRow();
    fileList.swapItemsAt(rowIndex, rowIndex + 1);
    ui->fileTable->setCurrentCell(rowIndex + 1, 0);
    refreshFileList();
    refreshTable();
}

void MainWindow::onStartOutputButtonClicked() {
    if (ui->outputFilePathTextBox->text().isEmpty()) {
        return;
    }

    /* Set file suffix */
    onOutputFormatChanged(ui->outputFormatComboBox->currentIndex());

    QFile outputFile = QFile(ui->outputFilePathTextBox->text());
    if (outputFile.exists()) {
        const QMessageBox::StandardButton reply = QMessageBox::question(this, "File exist", "File exist, replace it?",
                                                                        QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    if (!outputFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Open file error: " + outputFile.errorString());
        return;
    }

    /* Refresh address data */
    onFlashSizeTextChanged();
    refreshFileList();

    const quint32 fileCount = fileList.count();

    fileBuffer.reserve(flashSize);
    fileBuffer.fill(0);

    if (ui->addFileIndexCheckbox->isChecked()) {
        if (fileCount > MAX_FILE_INFO_NUMBER) {
            QMessageBox::warning(this, "Error", "File number out of range.");
            return;
        }

        memset(&fileIndex, 0, sizeof(fileIndex));

        fileIndex.count = fileCount;

        /* Write file index */
        for (int i = 0; i < fileCount; i++) {
            fileIndex.fileInfo[i].address = fileList.at(i)->address;
            fileIndex.fileInfo[i].size = fileList.at(i)->size;
            qstrncpy(fileIndex.fileInfo[i].fileName, fileList.at(i)->name.toUtf8(),
                     sizeof(fileIndex.fileInfo[i].fileName));
        }
        memcpy(fileBuffer.data(), &fileIndex, sizeof(fileIndex));
    }

    QFile currentFile;
    /* Write files */
    qsizetype bufferStart = 0;
    for (int i = 0; i < fileCount; i++) {
        currentFile.setFileName(fileList.at(i)->path);
        if (!currentFile.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, "Error",
                                 "Open file `" + fileList.at(i)->path + "` error: " + outputFile.errorString());
            continue;
        }
        QByteArray data = currentFile.readAll();

        if (data.size() > flashSize - bufferStart) {
            QMessageBox::warning(this, "Error", "File exceed the flash! ");
            outputFile.close();
            currentFile.close();
            return;
        }

        bufferStart = fileList.at(i)->address - flashStartAddress;
        memcpy(fileBuffer.data() + bufferStart, data.constData(), data.size());
        currentFile.close();
    }

    switch (ui->outputFormatComboBox->currentIndex()) {
        case 0:
            outputHexFile(&outputFile);
            break;

        case 1:
            outputBinFile(&outputFile);
            break;

        case 2:
            outputS19File(&outputFile);
            break;

        default:
            break;
    }
    outputFile.close();
    QMessageBox::information(this, "Info", "Successful output file: " + outputFile.fileName());
}

void MainWindow::onOutputFormatChanged(int index) {
    if (ui->outputFilePathTextBox->text().isEmpty()) {
        return;
    }

    QString outputFilePath = ui->outputFilePathTextBox->text();

    outputFilePath.replace(outputFilePath.size() - 3, 3, ui->outputFormatComboBox->currentText());
    ui->outputFilePathTextBox->setText(outputFilePath);
}

void MainWindow::onAddressAlignmentChanged(int index) {
    alignment = ui->addressAlignComboBox->currentText().toInt();
    refreshFileList();
    refreshTable();
}

void MainWindow::onStartAddrTextChanged(const QString &text) {
    flashStartAddress = text.toInt(nullptr, 16);
    if (ui->addFileIndexCheckbox->isChecked()) {
        fileStartAddress = flashStartAddress + sizeof(fileIndex);
    } else {
        fileStartAddress = flashStartAddress;
    }
    refreshFileList();
    refreshTable();
}

void MainWindow::onAddFileIndexStateChanged(Qt::CheckState state) {
    if (state == Qt::Checked) {
        fileStartAddress = flashStartAddress + sizeof(fileIndex);
    } else {
        fileStartAddress = flashStartAddress;
    }
    refreshFileList();
    refreshTable();
}

void MainWindow::onFlashSizeTextChanged() {
    const QString currentText = ui->flashSizeTextBox->text();
    if (currentText.isEmpty()) {
        return;
    }

    const quint32 number = currentText.toInt();
    quint32 newSize;
    QString lastSizeText;
    switch (ui->flashSizeUnitComboBox->currentIndex()) {
        case 0:
            /* MB */
            newSize = number * 1024 * 1024;
            lastSizeText = QString::number(flashSize / 1024 / 1024);
            break;
        case 1:
            /* kB */
            newSize = number * 1024;
            lastSizeText = QString::number(flashSize / 1024);
            break;
        case 2:
            /* Byte */
            newSize = number;
            lastSizeText = QString::number(flashSize);
            break;

        default:
            return;
    }

    if (newSize < (totalSize + (flashStartAddress % alignment))) {
        qDebug() << "Small in Text, current Text: " << currentText << ", last text: " << lastSizeText;
        QMessageBox::warning(this, "Error", "Flash size is too small to store the files. ");
        ui->flashSizeTextBox->setText(lastSizeText);
        return;
    }

    flashSize = newSize;
}

void MainWindow::onFlashSizeUnitChanged(int index) {
    const quint32 number = ui->flashSizeTextBox->text().toInt();
    quint32 newSize;
    switch (index) {
        case 0:
            /* MB */
            newSize = number * 1024 * 1024;
            break;
        case 1:
            /* kB */
            newSize = number * 1024;
            break;
        case 2:
            /* Byte */
            newSize = number;
            break;

        default:
            return;
    }
    if (newSize > (totalSize + (flashStartAddress % alignment))) {
        flashSize = newSize;
        return;
    }
    QMessageBox::warning(this, "Error", "Flash size is too small to store the files. ");
    /* Set least size and Unit */
    quint32 leastNumber;
    int newIndex;
    if (totalSize < 1024) {
        /* Byte */
        leastNumber = totalSize;
        newSize = 1;
        newIndex = 2;
    } else if (totalSize < 1024 * 1024) {
        /* KByte */
        leastNumber = totalSize / 1024 + 1;
        newSize = 1024;
        newIndex = 1;
    } else {
        /* MByte */
        leastNumber = totalSize / 1024 / 1024 + 1;
        newSize = 1024 * 1024;
        newIndex = 0;
    }
    if (leastNumber > number) {
        newSize *= leastNumber;
    } else {
        newSize *= number;
    }
    flashSize = newSize;

    ui->flashSizeUnitComboBox->setCurrentIndex(newIndex);
    if (leastNumber > number) {
        ui->flashSizeTextBox->setText(QString::number(leastNumber));
    }
}

void MainWindow::refreshFileList() {
    quint32 fileAddress = ADDRESS_ALIGNMENT(fileStartAddress);
    for (int i = 0; i < fileList.count(); i++) {
        fileList.at(i)->address = fileAddress;
        fileAddress += fileList.at(i)->size;
        /* next file address */
        fileAddress = ADDRESS_ALIGNMENT(fileAddress);
    }
    totalSize = fileAddress - flashStartAddress;
}

void MainWindow::refreshTable() {
    QString tempString;

    /* Add item and refresh text */
    for (int index = 0; index < fileList.count(); index++) {
        const fileList_t *currentFile = fileList.at(index);

        if (index >= ui->fileTable->rowCount()) {
            ui->fileTable->insertRow(index);
            ui->fileTable->setItem(index, 0, new QTableWidgetItem());
            ui->fileTable->setItem(index, 1, new QTableWidgetItem());
            ui->fileTable->setItem(index, 2, new QTableWidgetItem());
            ui->fileTable->setItem(index, 3, new QTableWidgetItem());
        }

        tempString = QString::asprintf("%#x", currentFile->address);
        ui->fileTable->item(index, 0)->setText(tempString);
        ui->fileTable->item(index, 1)->setText(currentFile->name);

        const qint64 fileSize = currentFile->size;
        if (fileSize < 1024) {
            tempString = QString::asprintf("%lld B", fileSize);
        } else if (fileSize < 1024 * 1024) {
            tempString = QString::asprintf("%.2f KB", static_cast<float>(fileSize) / 1024.0);
        } else {
            tempString = QString::asprintf("%.2f MB", static_cast<float>(fileSize) / 1024.0 / 1024.0);
        }
        ui->fileTable->item(index, 2)->setText(tempString);
        ui->fileTable->item(index, 3)->setText(currentFile->path);
    }

    /* Delete unused table item */
    for (int rowIndex = static_cast<int>(fileList.count()); rowIndex < ui->fileTable->rowCount(); rowIndex++) {
        delete ui->fileTable->item(rowIndex, 0);
        delete ui->fileTable->item(rowIndex, 1);
        delete ui->fileTable->item(rowIndex, 2);
        delete ui->fileTable->item(rowIndex, 3);
        ui->fileTable->removeRow(rowIndex);
    }
}

static quint8 calcCheckSum(const quint8 *data, qsizetype len) {
    quint8 checksum = 0;
    while (len--) {
        checksum += *data++;
    }
    checksum = ~checksum + 1;
    return checksum;
}

QString MainWindow::bytes2Text(const quint8 *data, qsizetype len) {
    QString result;
    while (len--) {
        result += QString::asprintf("%02X", *data);
        data++;
    }
    return result;
}


void MainWindow::outputHexFile(QFile *file) {
    QTextStream outHex(file);

    typedef enum {
        HEX_CODE_DATA = 0x00,
        HEX_CODE_EOF,
        HEX_CODE_EX_SEG_ADDR,
        HEX_CODE_ST_SEG_ADDR,
        HEX_CODE_EX_LINE_ADDR,
        HEX_CODE_ST_LINE_ADDR
    } __attribute((packed)) HexDataType;

    struct {
        quint8 length;
        uint8_t address[2];
        HexDataType dataType;
        uint8_t data[17]; /* The last byte is checksum. */
    }__attribute((packed)) hexFileData = {};

    quint32 address = flashStartAddress;
    qint64 bytesRemain = totalSize;
    quint32 dataLength;
    quint32 addressHigh = 0;

    while (bytesRemain > 0) {
        if (addressHigh != (address >> 16 & 0xFFFF)) {
            addressHigh = address >> 16 & 0xFFFF;
            /* Write extended address first even through the address is less than 16 bit. */
            hexFileData.length = 2;
            hexFileData.address[0] = 0x00;
            hexFileData.address[1] = 0x00;
            hexFileData.dataType = HEX_CODE_EX_LINE_ADDR;
            hexFileData.data[0] = address >> 24 & 0xFF;
            hexFileData.data[1] = address >> 16 & 0xFF;
            /* length (1 byte) + address (2 byte) + type (1 byte) + Data (2 byte) */
            hexFileData.data[2] = calcCheckSum(reinterpret_cast<uint8_t *>(&hexFileData), 6);
            /* Add one byte checksum */
            outHex << ":" << bytes2Text(reinterpret_cast<uint8_t *>(&hexFileData), 7) << Qt::endl;
        }

        hexFileData.dataType = HEX_CODE_DATA;
        hexFileData.address[0] = address >> 8 & 0xFF;
        hexFileData.address[1] = address & 0xFF;
        if (bytesRemain >= 16) {
            dataLength = 16;
        } else {
            dataLength = bytesRemain;
        }

        hexFileData.length = dataLength;
        memcpy(hexFileData.data, fileBuffer.data() + address - flashStartAddress, dataLength);
        hexFileData.data[dataLength] = calcCheckSum(reinterpret_cast<uint8_t *>(&hexFileData), dataLength + 4);

        /* Add one byte checksum */
        outHex << ":" << bytes2Text(reinterpret_cast<uint8_t *>(&hexFileData), dataLength + 5) << Qt::endl;

        bytesRemain -= dataLength;
        address += dataLength;
    }
    hexFileData.length = 0;
    hexFileData.address[0] = 0x00;
    hexFileData.address[1] = 0x00;
    hexFileData.dataType = HEX_CODE_EOF;
    /* length (1 byte) + address (2 byte) + type (1 byte) + Data (0 byte) */
    hexFileData.data[0] = calcCheckSum(reinterpret_cast<uint8_t *>(&hexFileData), 4);
    outHex << ":" << bytes2Text(reinterpret_cast<uint8_t *>(&hexFileData), 5) << Qt::endl;
}

void MainWindow::outputBinFile(QFile *file) {
    file->write(fileBuffer, totalSize);
}

void MainWindow::outputS19File(QFile *file) {
    QTextStream outS19(file);

    struct {
        uint8_t count;
        uint8_t address[4];
        uint8_t data[17]; /* The last byte is checksum. */
    } __attribute((packed)) s19FileData = {};

    qint64 bytesRemain = totalSize;
    quint32 address = flashStartAddress;
    quint32 dataLength;

    while (bytesRemain > 0) {
        if (bytesRemain >= 16) {
            dataLength = 16;
        } else {
            dataLength = bytesRemain;
        }

        s19FileData.address[0] = address >> 24 & 0xFF;
        s19FileData.address[1] = address >> 16 & 0xFF;
        s19FileData.address[2] = address >> 8 & 0xFF;
        s19FileData.address[3] = address & 0xFF;

        /* Count = address(4 bytes) + data (dataLength) + checksum (1 byte) */
        s19FileData.count = dataLength + 5;
        memcpy(s19FileData.data, fileBuffer.data() + address - flashStartAddress, dataLength);
        /* calc length = count (1 byte) + data (dataLength bytes) + address (4 bytes)  */
        s19FileData.data[dataLength] = calcCheckSum(&s19FileData.count, dataLength + 5) - 1;

        /* Add one byte checksum and one byte count */
        outS19 << "S3" << bytes2Text(reinterpret_cast<uint8_t *>(&s19FileData), dataLength + 6) <<
                Qt::endl;

        bytesRemain -= dataLength;
        address += dataLength;
    }
    s19FileData.count = 5;
    s19FileData.address[0] = 0;
    s19FileData.address[1] = 0;
    s19FileData.address[2] = 0;
    s19FileData.address[3] = 0;
    s19FileData.data[0] = calcCheckSum(reinterpret_cast<uint8_t *>(&s19FileData), 5) - 1;

    outS19 << "S7" << bytes2Text(reinterpret_cast<uint8_t *>(&s19FileData), 6) << Qt::endl;
}
