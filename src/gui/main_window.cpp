#include "main_window.h"
#include "ui_main_window.h"
#include "../service/database.h"
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    database = PicDatabase(QString("main_thread"));

}
MainWindow::~MainWindow() {
    delete ui;
}
void MainWindow::initInterface() {
    ui->selectedTagScrollArea->hide();
}
void MainWindow::fillComboBox() {
    ui->searchComboBox->addItem("选择搜索字段");
    ui->searchComboBox->addItem("图片ID");
    ui->searchComboBox->addItem("pixiv作品ID");
    ui->searchComboBox->addItem("pixiv作者ID");
    ui->searchComboBox->addItem("pixiv作者名");
    ui->searchComboBox->addItem("pixiv标题");
    ui->searchComboBox->addItem("pixiv描述");
    ui->searchComboBox->addItem("推特ID");
    ui->searchComboBox->addItem("推特作者ID");
    ui->searchComboBox->addItem("推特作者名");
    ui->searchComboBox->addItem("推特作者别名");
    ui->searchComboBox->addItem("推特描述");
    ui->searchComboBox->setCurrentIndex(0);

    ui->sortComboBox->addItem("无");
    ui->sortComboBox->addItem("图片ID");
    ui->sortComboBox->addItem("日期");
    ui->sortComboBox->addItem("大小");
    ui->sortComboBox->setCurrentIndex(0);

    ui->orderComboBox->addItem("递增");
    ui->orderComboBox->addItem("递减");
    ui->orderComboBox->setCurrentIndex(0);
}
void MainWindow::connectSignalSlots() {
    connect(ui->jpgCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowJPG);
    connect(ui->pngCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowPNG);
    connect(ui->gifCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowGIF);
    connect(ui->webpCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowWEBP);
    connect(ui->sortComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortBy);
    connect(ui->orderComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortOrder);
    connect(ui->enableRatioCheckBox, &QCheckBox::toggled, this, &MainWindow::updateEnableRatioSort);
    connect(ui->ratioSlider, &QSlider::valueChanged, this, &MainWindow::updateRatioSlider);
    connect(ui->widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(ui->heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(ui->searchComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSearchField);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchText);
    connect(ui->resolutionHeightEdit, &QLineEdit::textChanged, this, &MainWindow::updateWidthHeightLimit);
    connect(ui->resolutionWidthEdit, &QLineEdit::textChanged, this, &MainWindow::updateWidthHeightLimit);
}
void MainWindow::updateShowJPG(bool checked) {
    showJPG = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowPNG(bool checked) {
    showPNG = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowGIF(bool checked) {
    showGIF = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowWEBP(bool checked) {
    showWEBP = checked;
    refreshPicDisplay();
}
void MainWindow::updateSortBy(int index) {
    sortBy = static_cast<SortBy>(index);
    sortPics();
    refreshPicDisplay();
}
void MainWindow::updateSortOrder(int index) {
    sortOrder = static_cast<SortOrder>(index);
    sortPics();
    refreshPicDisplay();
}
void MainWindow::updateEnableRatioSort(bool checked) {
    ratioSortEnabled = checked;
    if (ratioSortEnabled) {
        ui->sortComboBox->setDisabled(true);
        ui->orderComboBox->setDisabled(true);
    } else {
        ui->sortComboBox->setEnabled(true);
        ui->orderComboBox->setEnabled(true);
    }
    sortPics();
    refreshPicDisplay();
}
int ratioToSliderValue(double ratio) {
    if (ratio <= 0) {
        return 40;
    } else if (ratio >= 3) {
        return -40;
    }
    if (ratio < 1.0) {
        return static_cast<int>(((1/(ratio)) - 1) * 20);
    } else {
        return static_cast<int>((1 - ratio) * 20);
    }
}
void MainWindow::updateRatioSlider(int value) {
    if (ratioSpinBoxEditing) return;
    ratioSliderEditing = true;
    ratioSliderValue = value;
    if (value >= 0) {
        double temp = 1.0 + (value / 20.0);
        ratio = 1.0 / temp;
        widthRatioSpinBoxValue = 1.0;
        heightRatioSpinBoxValue = temp;
    } else {
        double temp = 1.0 - (value / 20.0);
        ratio = temp;
        widthRatioSpinBoxValue = temp;
        heightRatioSpinBoxValue = 1.0;
    }
    ui->widthRatioSpinBox->setValue(widthRatioSpinBoxValue);
    ui->heightRatioSpinBox->setValue(heightRatioSpinBoxValue);
    ratioSliderEditing = false;
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
    }
}
void MainWindow::updateRatioSpinBox(double value) {
    if (ratioSliderEditing) return;
    ratioSpinBoxEditing = true;
    widthRatioSpinBoxValue = ui->widthRatioSpinBox->value();
    heightRatioSpinBoxValue = ui->heightRatioSpinBox->value();
    if (widthRatioSpinBoxValue == 0) {
        ratio = 0;
    } else if (heightRatioSpinBoxValue == 0) {
        ratio = std::numeric_limits<double>::infinity();
    } else {
        ratio = widthRatioSpinBoxValue / heightRatioSpinBoxValue;
    }
    ui->ratioSlider->setValue(ratioToSliderValue(ratio));
    ratioSpinBoxEditing = false;
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
    }
}
void MainWindow::updateSearchField(int index) {
    searchField = static_cast<SearchField>(index);
}
void MainWindow::updateSearchText(const QString& text) {
    searchText = text.toStdString();
    searchTextChanged = true;
}
inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}
std::pair<uint, uint> parseRange(const std::string& text) {
    std::string s = trim(text);
    if (s.empty()) return {0, UINT_MAX};

    if (s[0] == '<') {
        uint val = std::stoul(s.substr(1));
        return {0, val};
    }
    if (s[0] == '>') {
        uint val = std::stoul(s.substr(1));
        return {val, UINT_MAX};
    }
    size_t dash = s.find('-');
    if (dash != std::string::npos) {
        uint a = std::stoul(s.substr(0, dash));
        uint b = std::stoul(s.substr(dash + 1));
        if (a > b) std::swap(a, b);
        return {a, b};
    }
    uint val = std::stoul(s);
    return {val, val};
}
void MainWindow::updateWidthHeightLimit() {
    auto heightRange = parseRange(ui->resolutionHeightEdit->text().toStdString());
    minHeight = heightRange.first;
    maxHeight = heightRange.second;
    auto widthRange = parseRange(ui->resolutionWidthEdit->text().toStdString());
    minWidth = widthRange.first;
    maxWidth = widthRange.second;
    refreshPicDisplay();
}
void MainWindow::handleWindowSizeChange() {
    int width = ui->picBrowseWidget->width();
    widgetsPerRow = width / 250;
    if (widgetsPerRow < 1) widgetsPerRow = 1;
    rearrangePicFrames();
}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    handleWindowSizeChange();
}