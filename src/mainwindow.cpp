#include "mainwindow.h"
#include <QMouseEvent>
#include <QPainter>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QStyleOption>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QTabWidget>

// 默认配置
const int DEFAULT_GRID_COLUMNS = 5;
const int DEFAULT_THUMBNAIL_SIZE = 240;
const QString CONFIG_FILENAME = "javark.ini";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_library(new VideoLibrary(this)),
      m_gridColumns(DEFAULT_GRID_COLUMNS),
      m_thumbnailSize(DEFAULT_THUMBNAIL_SIZE),
      m_useFanartMode(false), // 默认使用海报模式
      m_sortOrder(SortOrder::NameAsc), // 默认按文件名排序
      m_searchText("") // 初始化搜索文本为空
{
    // 设置配置文件路径 - 使用应用程序目录下的配置文件（便携版）
    m_configFile = QCoreApplication::applicationDirPath() + "/" + CONFIG_FILENAME;

    // 确保日志目录存在
    QDir logDir(QCoreApplication::applicationDirPath() + "/logs");
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    // 设置暗色主题
    QString darkStyleSheet = R"(
        QMainWindow, QWidget {
            background-color: #2D2D30;
            color: #E1E1E1;
        }
        QPushButton {
            background-color: #505060;
            color: #FFFFFF;
            border: 1px solid #777777;
            border-radius: 4px;
            padding: 5px;
            font-weight: bold;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: #606070;
            border: 1px solid #999999;
        }
        QPushButton:pressed {
            background-color: #0078D7;
            border: 1px solid #0095FF;
        }
        QLineEdit {
            background-color: #3A3A42;
            color: #FFFFFF;
            border: 1px solid #666666;
            border-radius: 4px;
            padding: 5px;
            selection-background-color: #0078D7;
            min-height: 28px;
        }
        QLineEdit:focus {
            border: 1px solid #0078D7;
            background-color: #454550;
        }
        QScrollArea, QScrollBar {
            background-color: #252526;
        }
        QLabel {
            color: #E1E1E1;
        }
        QMenu {
            background-color: #2D2D30;
            color: #E1E1E1;
            border: 1px solid #3F3F46;
        }
        QMenu::item:selected {
            background-color: #3F3F46;
        }
    )";
    this->setStyleSheet(darkStyleSheet);

    // 创建UI
    createUI();
    createActions();
    createMenus();

    // 连接视频库信号
    connect(m_library, &VideoLibrary::videoAdded, this, &MainWindow::onVideoAdded);
    connect(m_library, &VideoLibrary::scanStarted, this, &MainWindow::onScanStarted);
    connect(m_library, &VideoLibrary::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_library, &VideoLibrary::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_library, &VideoLibrary::videoPosterReady, this, &MainWindow::onVideoPosterReady);

    // 连接工具栏按钮
    connect(m_addDirButton, &QPushButton::clicked, this, &MainWindow::onAddDirectory);
    connect(m_removeDirButton, &QPushButton::clicked, this, &MainWindow::onRemoveDirectory);
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanLibrary);
    connect(m_sortButton, &QPushButton::clicked, this, &MainWindow::onToggleSortOrder);
    connect(m_toggleCoverButton, &QPushButton::clicked, this, &MainWindow::onToggleCoverMode);
    connect(m_increaseButton, &QPushButton::clicked, this, &MainWindow::onIncreaseThumbnailSize);
    connect(m_decreaseButton, &QPushButton::clicked, this, &MainWindow::onDecreaseThumbnailSize);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    // 加载设置
    loadSettings();

    // 设置窗口大小和标题
    setWindowTitle(tr("JavArk"));
    resize(1024, 768);

    // 将窗口居中显示
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), screen->availableGeometry()));
    }
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::createUI()
{
    // 创建中央窗口部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建主布局
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);

    // 创建工具栏布局
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setContentsMargins(0, 0, 0, 0);
    m_toolbarLayout->setSpacing(5);

    // 创建底部状态栏布局
    m_bottomLayout = new QHBoxLayout();
    m_bottomLayout->setContentsMargins(0, 0, 0, 0);
    m_bottomLayout->setSpacing(5);

    // 创建工具栏按钮和状态显示
    m_addDirButton = new QPushButton(tr("添加目录"), this);
    m_addDirButton->setIcon(QIcon(":/icons/add.png"));
    m_addDirButton->setFixedHeight(32);
    m_addDirButton->setStyleSheet("background-color: #2C5F9B; color: white;");

    m_removeDirButton = new QPushButton(tr("移除目录"), this);
    m_removeDirButton->setIcon(QIcon(":/icons/delete.png"));
    m_removeDirButton->setFixedHeight(32);

    m_scanButton = new QPushButton(tr("扫描媒体库"), this);
    m_scanButton->setIcon(QIcon(":/icons/refresh.png"));
    m_scanButton->setFixedHeight(32);
    m_scanButton->setStyleSheet("background-color: #2C5F9B; color: white;");

    // 创建排序按钮
    m_sortButton = new QPushButton(tr("排序：按文件名"), this);
    m_sortButton->setIcon(QIcon(":/icons/sort.png"));
    m_sortButton->setFixedHeight(32);
    m_sortButton->setStyleSheet("background-color: #505060; color: white; font-weight: bold;");

    // 创建切换封面模式按钮
    m_toggleCoverButton = new QPushButton(tr("切换封面"), this);
    m_toggleCoverButton->setIcon(QIcon(":/icons/switch.png"));
    m_toggleCoverButton->setFixedHeight(32);
    m_toggleCoverButton->setStyleSheet("background-color: #505060; color: white; font-weight: bold;");

    // 创建缩略图尺寸调整按钮
    m_increaseButton = new QPushButton("+", this);
    m_increaseButton->setFixedSize(32, 32);
    m_increaseButton->setStyleSheet("font-weight: bold; font-size: 14px;");

    m_decreaseButton = new QPushButton("-", this);
    m_decreaseButton->setFixedSize(32, 32);
    m_decreaseButton->setStyleSheet("font-weight: bold; font-size: 14px;");

    // 创建搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索视频..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(32);
    m_searchEdit->setMinimumWidth(200);
    m_searchEdit->setStyleSheet("background-color: #3A3A42; color: white; border: 1px solid #666666; border-radius: 15px; padding-left: 10px; padding-right: 10px;");
    // 添加搜索图标
    QAction* searchIcon = new QAction(this);
    searchIcon->setIcon(QIcon(":/icons/search.png"));
    m_searchEdit->addAction(searchIcon, QLineEdit::LeadingPosition);

    // 创建状态标签和进度条
    m_statusLabel = new QLabel(tr("就绪"), this);
    m_statusLabel->setFrameShape(QFrame::NoFrame);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setTextVisible(true);
    m_progressBar->setMinimumWidth(300); // 可以适当加宽
    m_progressBar->setStyleSheet("QProgressBar { border: 1px solid grey; border-radius: 5px; text-align: center; background-color: #3A3A42; } QProgressBar::chunk { background-color: #0078D7; width: 10px; margin: 0.5px; }"); // 添加一些样式

    // 添加控件到工具栏布局
    m_toolbarLayout->addWidget(m_addDirButton);
    m_toolbarLayout->addWidget(m_removeDirButton);
    m_toolbarLayout->addWidget(m_scanButton);
    m_toolbarLayout->addWidget(m_sortButton);
    m_toolbarLayout->addWidget(m_toggleCoverButton);
    m_toolbarLayout->addWidget(m_decreaseButton);
    m_toolbarLayout->addWidget(m_increaseButton);
    m_toolbarLayout->addStretch(1);
    m_toolbarLayout->addWidget(m_searchEdit);

    // 添加控件到底部状态栏布局
    m_bottomLayout->addWidget(m_statusLabel, 1);

    // 创建 QTabWidget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(false); // 根据需要设置是否可关闭
    m_tabWidget->setStyleSheet("QTabBar::tab { padding: 8px; background-color: #3A3A42; border-top-left-radius: 4px; border-top-right-radius: 4px; font-size: 11pt; font-weight: bold; } QTabBar::tab:selected { background-color: #2D2D30; border: 1px solid #3F3F46; border-bottom: none; } QTabWidget::pane { border: 1px solid #3F3F46; border-top: none; background-color: #2D2D30; }");

    // 添加到主布局 (替换旧的 m_scrollArea)
    m_mainLayout->addLayout(m_toolbarLayout);

    // ---- 新增：创建并添加居中的进度条布局 ----
    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->setContentsMargins(0, 5, 0, 5); // 添加垂直边距
    progressLayout->addStretch(1);
    progressLayout->addWidget(m_progressBar);
    progressLayout->addStretch(1);
    m_mainLayout->addLayout(progressLayout);
    // ---- 结束新增 ----

    m_mainLayout->addWidget(m_tabWidget); // 添加 TabWidget
    m_mainLayout->addLayout(m_bottomLayout);
}

void MainWindow::createActions()
{
    // 创建移除目录的菜单动作
    m_dirMenu = new QMenu(this);
}

void MainWindow::createMenus()
{
    // 更新目录列表
    updateDirectoryList();
}

void MainWindow::updateDirectoryList()
{
    // 清除旧的菜单项和动作
    m_dirMenu->clear();
    // 释放旧的QAction对象内存
    qDeleteAll(m_dirActions);
    m_dirActions.clear();

    QStringList dirs = m_library->directories();
    if (dirs.isEmpty()) {
        QAction *emptyAction = new QAction(tr("没有媒体库目录"), this);
        emptyAction->setEnabled(false);
        m_dirMenu->addAction(emptyAction);
        m_dirActions.append(emptyAction); // 即使禁用也添加到列表，以便后续清理
        m_removeDirButton->setEnabled(false); // 禁用移除按钮
    } else {
        for (const QString &dir : dirs) {
            QAction *action = new QAction(dir, this);
            // 使用 lambda 连接信号和槽，捕获目录路径
            connect(action, &QAction::triggered, this, [this, dir]() {
                // 弹出确认对话框
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, tr("移除目录"),
                                              tr("确定要从库中移除目录 '%1' 吗？\\n(这不会删除硬盘上的实际文件)").arg(dir),
                                              QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    m_library->removeDirectory(dir); // 从库中移除目录
                    updateDirectoryList();           // 更新目录菜单显示

                    // 从 TabWidget 中移除对应的标签页
                    if (m_tabContents.contains(dir)) {
                        QWidget* tabContent = m_tabContents.value(dir);
                        int index = m_tabWidget->indexOf(tabContent);
                        if (index != -1) {
                            m_tabWidget->removeTab(index);
                        }
                        // 清理相关数据结构
                        delete m_tabGridLayouts.value(dir); // 删除布局会删除其中的 QScrollArea
                        m_tabGridLayouts.remove(dir);
                        qDeleteAll(m_tabVideoWidgets.value(dir));
                        m_tabVideoWidgets.remove(dir);
                        delete tabContent; // 删除主内容 QWidget
                        m_tabContents.remove(dir);
                    }
                }
            });
            m_dirMenu->addAction(action);
            m_dirActions.append(action); // 添加到列表，以便后续清理
        }
        m_removeDirButton->setEnabled(true); // 启用移除按钮
    }
}

void MainWindow::onAddDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择媒体目录"),
                                                   QDir::homePath(),
                                                   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_library->addDirectory(dir);
        updateDirectoryList();

        // 自动开始扫描
        onScanLibrary();
    }
}

void MainWindow::onRemoveDirectory()
{
    m_dirMenu->exec(QCursor::pos());
}

void MainWindow::onScanLibrary()
{
    m_library->scanLibrary();
}

void MainWindow::onVideoAdded(const QString& directory, std::shared_ptr<VideoItem> video)
{
    QWidget* tabContentWidget = nullptr;
    QGridLayout* gridLayout = nullptr;
    QList<VideoWidget*>* videoWidgets = nullptr;

    // 检查此目录的标签页是否已存在
    if (!m_tabContents.contains(directory)) {
        // 创建新的标签页内容
        tabContentWidget = new QWidget(m_tabWidget);
        QVBoxLayout* tabLayout = new QVBoxLayout(tabContentWidget); // 使用 QVBoxLayout 包裹 ScrollArea
        tabLayout->setContentsMargins(0, 0, 0, 0);

        QScrollArea* scrollArea = new QScrollArea(tabContentWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setStyleSheet("background-color: #2D2D30;"); // 确保滚动区域背景色一致

        QWidget* scrollContent = new QWidget(scrollArea);
        gridLayout = new QGridLayout(scrollContent);
        gridLayout->setContentsMargins(10, 10, 10, 10); // 增加边距
        gridLayout->setSpacing(10);
        scrollArea->setWidget(scrollContent);

        tabLayout->addWidget(scrollArea); // 将 ScrollArea 添加到标签页布局

        // 存储新创建的元素
        m_tabContents.insert(directory, tabContentWidget);
        m_tabGridLayouts.insert(directory, gridLayout);
        m_tabVideoWidgets.insert(directory, QList<VideoWidget*>());

        // 添加新的标签页到 TabWidget
        // 使用 QDir 获取目录名作为标签文本
        QString tabLabel = QDir(directory).dirName();
        if (tabLabel.isEmpty()) tabLabel = directory; // 如果是根目录，显示完整路径
        m_tabWidget->addTab(tabContentWidget, tabLabel);
        m_tabWidget->setTabToolTip(m_tabWidget->indexOf(tabContentWidget), directory); // 设置完整路径为 ToolTip
    } else {
        // 获取现有的元素
        tabContentWidget = m_tabContents.value(directory);
        gridLayout = m_tabGridLayouts.value(directory);
    }

    videoWidgets = &m_tabVideoWidgets[directory];

    // 计算网格位置
    int currentGridColumns = calculateColumnsForTab(tabContentWidget);
    int row = videoWidgets->size() / currentGridColumns;
    int col = videoWidgets->size() % currentGridColumns;

    // 创建视频小部件 (传递当前封面模式)
    VideoWidget *widget = new VideoWidget(video, m_thumbnailSize, m_useFanartMode, tabContentWidget);
    videoWidgets->append(widget);

    // 添加到对应的网格布局
    gridLayout->addWidget(widget, row, col);
}

void MainWindow::onScanStarted()
{
    // 清除所有标签页的内容
    clearVideoWidgets();

    // 显示进度条
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("扫描中..."));

    // 禁用扫描按钮
    m_scanButton->setEnabled(false);
}

void MainWindow::onScanProgress(int current, int total)
{
    if (total > 0) {
        int percent = (current * 100) / total;
        m_progressBar->setValue(percent);
        m_progressBar->setFormat(tr("扫描中... %1/%2").arg(current).arg(total));
        m_statusLabel->setText(tr("扫描媒体库..."));
    }
}

void MainWindow::onScanFinished()
{
    // 隐藏进度条
    m_progressBar->setVisible(false);

    // 扫描完成后，可能需要重新排序和布局所有标签页的内容
    refreshVideoDisplay(); // 使用新的刷新逻辑

    // 启用扫描按钮
    m_scanButton->setEnabled(true);
}

void MainWindow::clearVideoWidgets()
{
    // 清除所有标签页的内容
    for (const QString& dir : m_tabContents.keys()) {
        QWidget* tabContentWidget = m_tabContents.value(dir);
        clearVideoWidgetsInTab(tabContentWidget);
    }
}

// 清空特定标签页的视频小部件
void MainWindow::clearVideoWidgetsInTab(QWidget* tabContentWidget)
{
    QString dir = m_tabContents.key(tabContentWidget);
    if (dir.isEmpty()) return;

    if (m_tabGridLayouts.contains(dir) && m_tabVideoWidgets.contains(dir)) {
        QGridLayout* gridLayout = m_tabGridLayouts.value(dir);
        QList<VideoWidget*>& videoWidgets = m_tabVideoWidgets[dir];

        for (VideoWidget *widget : qAsConst(videoWidgets)) {
            gridLayout->removeWidget(widget);
            widget->deleteLater();
        }
        videoWidgets.clear();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWindow::saveSettings()
{
    try {
        // 确保目录存在
        QFileInfo fileInfo(m_configFile);
        QDir().mkpath(fileInfo.path());

        // 保存库目录
        m_library->saveLibraryConfig(m_configFile);

        // 保存窗口设置
        QSettings settings(m_configFile, QSettings::IniFormat);
        settings.beginGroup("MainWindow");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("state", saveState());
        settings.setValue("gridColumns", m_gridColumns);
        settings.setValue("thumbnailSize", m_thumbnailSize);
        settings.setValue("useFanartMode", m_useFanartMode);
        settings.setValue("sortOrder", static_cast<int>(m_sortOrder));
        settings.endGroup();

        if (settings.status() != QSettings::NoError) {
            qDebug() << "保存设置时出错:" << settings.status();
        }

        settings.sync();
    } catch (const std::exception& e) {
        qDebug() << "保存设置时发生异常:" << e.what();
    } catch (...) {
        qDebug() << "保存设置时发生未知异常";
    }
}

void MainWindow::loadSettings()
{
    // 加载库目录
    m_library->loadLibraryConfig(m_configFile);
    updateDirectoryList();

    // 加载窗口设置
    QSettings settings(m_configFile, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    m_gridColumns = settings.value("gridColumns", DEFAULT_GRID_COLUMNS).toInt();
    m_thumbnailSize = settings.value("thumbnailSize", DEFAULT_THUMBNAIL_SIZE).toInt();
    m_useFanartMode = settings.value("useFanartMode", false).toBool();
    m_sortOrder = static_cast<SortOrder>(settings.value("sortOrder", static_cast<int>(SortOrder::NameAsc)).toInt());
    settings.endGroup();

    // 根据当前封面模式设置按钮文本
    if (m_useFanartMode) {
        m_toggleCoverButton->setText(tr("使用海报"));
    } else {
        m_toggleCoverButton->setText(tr("使用背景"));
    }

    // 更新排序按钮文本
    updateSortButtonText();

    // 加载后调整网格列数
    adjustGridColumns();

    // 如果加载了目录，则自动开始扫描以加载视频和封面
    // 使用 QTimer::singleShot 将扫描操作推迟到事件循环空闲时执行，
    // 避免阻塞 MainWindow 的构造和显示过程。
    if (!m_library->directories().isEmpty()) {
         QTimer::singleShot(0, this, &MainWindow::onScanLibrary);
    }
}

void MainWindow::onToggleCoverMode()
{
    // 切换封面模式
    m_useFanartMode = !m_useFanartMode;

    // 更新按钮文本
    if (m_useFanartMode) {
        m_toggleCoverButton->setText(tr("使用海报"));
    } else {
        m_toggleCoverButton->setText(tr("使用背景"));
    }

    // 更新所有当前存在的视频小部件的封面模式
    for (const QString& dir : m_tabVideoWidgets.keys()) {
        for (VideoWidget *widget : m_tabVideoWidgets.value(dir)) {
            widget->setUseFanartMode(m_useFanartMode);
        }
    }

    // 保存设置
    QSettings settings(m_configFile, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("useFanartMode", m_useFanartMode);
    settings.endGroup();
}

void MainWindow::onIncreaseThumbnailSize()
{
    m_thumbnailSize += 10;

    // 更新所有当前存在的视频小部件的尺寸
    for (const QString& dir : m_tabVideoWidgets.keys()) {
        for (VideoWidget *widget : m_tabVideoWidgets.value(dir)) {
            widget->setThumbnailSize(m_thumbnailSize);
        }
    }

    // 调整当前标签页的网格列数
    adjustGridColumns();

    // 保存设置
    QSettings settings(m_configFile, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("thumbnailSize", m_thumbnailSize);
    settings.endGroup();
}

void MainWindow::onDecreaseThumbnailSize()
{
    if (m_thumbnailSize > 10) {
        m_thumbnailSize -= 10;

        // 更新所有当前存在的视频小部件的尺寸
        for (const QString& dir : m_tabVideoWidgets.keys()) {
            for (VideoWidget *widget : m_tabVideoWidgets.value(dir)) {
                widget->setThumbnailSize(m_thumbnailSize);
            }
        }

        // 调整当前标签页的网格列数
        adjustGridColumns();

        // 保存设置
        QSettings settings(m_configFile, QSettings::IniFormat);
        settings.beginGroup("MainWindow");
        settings.setValue("thumbnailSize", m_thumbnailSize);
        settings.endGroup();
    }
}

void MainWindow::adjustGridColumns()
{
    QWidget* currentTabWidget = m_tabWidget->currentWidget();
    if (!currentTabWidget) return;

    QScrollArea* scrollArea = currentTabWidget->findChild<QScrollArea*>();
    if (!scrollArea) return;

    QString dir = m_tabContents.key(currentTabWidget);
    if (dir.isEmpty() || !m_tabGridLayouts.contains(dir)) return;

    QGridLayout* gridLayout = m_tabGridLayouts.value(dir);

    // 获取滚动区域的宽度
    int availableWidth = scrollArea->viewport()->width();

    // 每个缩略图需要的宽度（包括间距）
    int itemWidth = m_thumbnailSize + gridLayout->spacing();

    // 计算可容纳的列数
    int newColumns = std::max(1, availableWidth / itemWidth);

    // 如果列数发生变化，重新布局当前标签页
    int currentGridColumns = calculateColumnsForTab(currentTabWidget);
    if (newColumns != currentGridColumns) {
        // 保存列数设置
        QSettings settings(m_configFile, QSettings::IniFormat);
        settings.beginGroup("MainWindow");
        settings.setValue("gridColumns", newColumns);
        settings.endGroup();

        // 重新布局当前标签页的视频小部件
        reLayoutVideosInTab(currentTabWidget);
    }
}

// 计算给定标签页当前的列数
int MainWindow::calculateColumnsForTab(QWidget* tabContentWidget)
{
    if (!tabContentWidget) return DEFAULT_GRID_COLUMNS;

    QScrollArea* scrollArea = tabContentWidget->findChild<QScrollArea*>();
    QString dir = m_tabContents.key(tabContentWidget);
    if (!scrollArea || dir.isEmpty() || !m_tabGridLayouts.contains(dir)) return DEFAULT_GRID_COLUMNS;

    QGridLayout* gridLayout = m_tabGridLayouts.value(dir);
    int availableWidth = scrollArea->viewport()->width();
    int itemWidth = m_thumbnailSize + gridLayout->spacing();
    return std::max(1, availableWidth / itemWidth);
}

void MainWindow::reLayoutVideosInTab(QWidget* tabContentWidget)
{
    if (!tabContentWidget) return;

    QString dir = m_tabContents.key(tabContentWidget);
    if (dir.isEmpty() || !m_tabGridLayouts.contains(dir) || !m_tabVideoWidgets.contains(dir)) return;

    QGridLayout* gridLayout = m_tabGridLayouts.value(dir);
    QList<VideoWidget*>& videoWidgets = m_tabVideoWidgets[dir];

    // 如果没有视频，直接返回
    if (videoWidgets.isEmpty()) {
        return;
    }

    // 暂时从布局中移除所有小部件
    for (VideoWidget *widget : videoWidgets) {
        gridLayout->removeWidget(widget);
        widget->hide(); // 隐藏以避免闪烁
    }

    // 根据新的列数重新布局
    int currentGridColumns = calculateColumnsForTab(tabContentWidget);
    for (int i = 0; i < videoWidgets.size(); ++i) {
        int row = i / currentGridColumns;
        int col = i % currentGridColumns;
        gridLayout->addWidget(videoWidgets[i], row, col);
        videoWidgets[i]->show();
    }
    gridLayout->update(); // 确保布局更新
}

void MainWindow::reLayoutVideos()
{
    // 重新布局当前标签页
    QWidget* currentTabWidget = m_tabWidget->currentWidget();
    if (currentTabWidget) {
        reLayoutVideosInTab(currentTabWidget);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 窗口大小变化时调整网格列数
    adjustGridColumns();

    sortVideos();

    // 保存设置
    QSettings settings(m_configFile, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.endGroup();
}

void MainWindow::onToggleSortOrder()
{
    // 切换排序方式
    switch (m_sortOrder) {
        case SortOrder::NameAsc:
            m_sortOrder = SortOrder::NameDesc;
            break;
        case SortOrder::NameDesc:
            m_sortOrder = SortOrder::CreationTimeAsc;
            break;
        case SortOrder::CreationTimeAsc:
            m_sortOrder = SortOrder::CreationTimeDesc;
            break;
        case SortOrder::CreationTimeDesc:
            m_sortOrder = SortOrder::ModifiedTimeAsc;
            break;
        case SortOrder::ModifiedTimeAsc:
            m_sortOrder = SortOrder::ModifiedTimeDesc;
            break;
        case SortOrder::ModifiedTimeDesc:
            m_sortOrder = SortOrder::NameAsc;
            break;
    }

    // 更新排序按钮文本
    updateSortButtonText();

    // 执行排序
    sortVideos();

    // 保存设置
    QSettings settings(m_configFile, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("sortOrder", static_cast<int>(m_sortOrder));
    settings.endGroup();
}

void MainWindow::updateSortButtonText()
{
    // 根据当前排序方式设置按钮文本
    switch (m_sortOrder) {
        case SortOrder::NameAsc:
            m_sortButton->setText(tr("排序：文件名 ↑"));
            break;
        case SortOrder::NameDesc:
            m_sortButton->setText(tr("排序：文件名 ↓"));
            break;
        case SortOrder::CreationTimeAsc:
            m_sortButton->setText(tr("排序：创建时间 ↑"));
            break;
        case SortOrder::CreationTimeDesc:
            m_sortButton->setText(tr("排序：创建时间 ↓"));
            break;
        case SortOrder::ModifiedTimeAsc:
            m_sortButton->setText(tr("排序：修改时间 ↑"));
            break;
        case SortOrder::ModifiedTimeDesc:
            m_sortButton->setText(tr("排序：修改时间 ↓"));
            break;
    }
}

void MainWindow::sortVideos()
{
    // 对当前活动的标签页执行排序
    QWidget* currentTabWidget = m_tabWidget->currentWidget();
    if (!currentTabWidget) return;

    QString dir = m_tabContents.key(currentTabWidget);
    if (dir.isEmpty() || !m_tabVideoWidgets.contains(dir)) return;

    QList<VideoWidget*>& videoWidgets = m_tabVideoWidgets[dir];

    // 如果没有视频，直接返回
    if (videoWidgets.isEmpty()) {
        return;
    }

    // 根据当前排序方式对视频小部件进行排序
    std::sort(videoWidgets.begin(), videoWidgets.end(),
        [this](const VideoWidget* a, const VideoWidget* b) -> bool {
            switch (m_sortOrder) {
                case SortOrder::NameAsc:
                    return a->video()->fileName().toLower() < b->video()->fileName().toLower();
                case SortOrder::NameDesc:
                    return a->video()->fileName().toLower() > b->video()->fileName().toLower();
                case SortOrder::CreationTimeAsc:
                    return a->video()->creationTime() < b->video()->creationTime();
                case SortOrder::CreationTimeDesc:
                    return a->video()->creationTime() > b->video()->creationTime();
                case SortOrder::ModifiedTimeAsc:
                    return a->video()->modifiedTime() < b->video()->modifiedTime();
                case SortOrder::ModifiedTimeDesc:
                    return a->video()->modifiedTime() > b->video()->modifiedTime();
                default:
                    return a->video()->fileName().toLower() < b->video()->fileName().toLower();
            }
        }
    );

    // 重新布局当前标签页
    reLayoutVideosInTab(currentTabWidget);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    // 保存搜索文本
    m_searchText = text.trimmed();

    // 根据搜索条件过滤视频
    filterVideos();
}

void MainWindow::filterVideos()
{
    // 这个函数的逻辑需要重写以适应标签页结构
    // 简单的实现是刷新整个显示，让 refreshVideoDisplay 处理过滤
    refreshVideoDisplay();
}

void MainWindow::reLayoutFilteredVideos()
{
    // 此函数不再需要，过滤后的布局由 refreshVideoDisplay 或 reLayoutVideosInTab 处理
    // 将在 refreshVideoDisplay 中重新实现
}

void MainWindow::refreshVideoDisplay()
{
    // 获取当前活动的标签页索引，以便后续恢复
    int currentTabIndex = m_tabWidget->currentIndex();

    // 移除所有现有标签页（但不删除内容）
    while(m_tabWidget->count() > 0) {
        m_tabWidget->removeTab(0);
    }

    // 清空所有标签页的视频小部件列表，但不删除布局等
    for (auto it = m_tabVideoWidgets.begin(); it != m_tabVideoWidgets.end(); ++it) {
        qDeleteAll(it.value());
        it.value().clear();
    }

    // 获取按目录分组的数据
    const auto& videosByDir = m_library->videosByDirectory();
    int totalVideoCount = 0;
    int filteredVideoCount = 0;

    // 遍历库中的目录
    QStringList sortedDirs = videosByDir.keys();
    std::sort(sortedDirs.begin(), sortedDirs.end()); // 按目录名排序标签页

    for (const QString& dir : sortedDirs) {
        const auto& videosInDir = videosByDir.value(dir);
        totalVideoCount += videosInDir.size();

        QWidget* tabContentWidget = nullptr;
        QGridLayout* gridLayout = nullptr;
        QList<VideoWidget*>* videoWidgetsPtr = nullptr;

        // 检查或创建标签页及其内容
        if (!m_tabContents.contains(dir)) {
            // 创建新的标签页内容
            tabContentWidget = new QWidget(m_tabWidget);
            QVBoxLayout* tabLayout = new QVBoxLayout(tabContentWidget);
            tabLayout->setContentsMargins(0, 0, 0, 0);

            QScrollArea* scrollArea = new QScrollArea(tabContentWidget);
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollArea->setStyleSheet("background-color: #2D2D30;");

            QWidget* scrollContent = new QWidget(scrollArea);
            gridLayout = new QGridLayout(scrollContent);
            gridLayout->setContentsMargins(10, 10, 10, 10);
            gridLayout->setSpacing(10);
            scrollArea->setWidget(scrollContent);

            tabLayout->addWidget(scrollArea);

            m_tabContents.insert(dir, tabContentWidget);
            m_tabGridLayouts.insert(dir, gridLayout);
            m_tabVideoWidgets.insert(dir, QList<VideoWidget*>());
        } else {
            tabContentWidget = m_tabContents.value(dir);
            gridLayout = m_tabGridLayouts.value(dir);
        }

        videoWidgetsPtr = &m_tabVideoWidgets[dir];

        // 填充视频小部件 (应用过滤)
        QList<VideoWidget*> currentTabFilteredWidgets;
        for (const auto& video : videosInDir) {
            // 检查是否匹配搜索条件
            if (m_searchText.isEmpty() || video->fileName().contains(m_searchText, Qt::CaseInsensitive)) {
                VideoWidget *widget = new VideoWidget(video, m_thumbnailSize, m_useFanartMode, tabContentWidget);
                videoWidgetsPtr->append(widget); // 添加到总列表
                currentTabFilteredWidgets.append(widget); // 添加到当前过滤后列表
                filteredVideoCount++;
            }
        }

        // 如果此标签页有内容，则添加到 TabWidget 并进行排序和布局
        if (!currentTabFilteredWidgets.isEmpty() || m_searchText.isEmpty()) {
            // 获取标签文本
            QString tabLabel = QDir(dir).dirName();
            if (tabLabel.isEmpty()) tabLabel = dir;

            // 添加标签页，在标签中显示视频数量
            m_tabWidget->addTab(tabContentWidget, QString("%1 (%2)").arg(tabLabel).arg(currentTabFilteredWidgets.size()));
            m_tabWidget->setTabToolTip(m_tabWidget->indexOf(tabContentWidget), dir);

            // 对当前标签页的视频进行排序
            std::sort(currentTabFilteredWidgets.begin(), currentTabFilteredWidgets.end(),
                [this](const VideoWidget* a, const VideoWidget* b) -> bool {
                    switch (m_sortOrder) {
                        case SortOrder::NameAsc: return a->video()->fileName().toLower() < b->video()->fileName().toLower();
                        case SortOrder::NameDesc: return a->video()->fileName().toLower() > b->video()->fileName().toLower();
                        case SortOrder::CreationTimeAsc: return a->video()->creationTime() < b->video()->creationTime();
                        case SortOrder::CreationTimeDesc: return a->video()->creationTime() > b->video()->creationTime();
                        case SortOrder::ModifiedTimeAsc: return a->video()->modifiedTime() < b->video()->modifiedTime();
                        case SortOrder::ModifiedTimeDesc: return a->video()->modifiedTime() > b->video()->modifiedTime();
                        default: return a->video()->fileName().toLower() < b->video()->fileName().toLower();
                    }
                });

            // 布局当前标签页的视频
            int cols = calculateColumnsForTab(tabContentWidget);
            for (int i = 0; i < currentTabFilteredWidgets.size(); ++i) {
                int row = i / cols;
                int col = i % cols;
                gridLayout->addWidget(currentTabFilteredWidgets[i], row, col);
                currentTabFilteredWidgets[i]->show();
            }
        }
    }

    // 更新状态栏
    if (m_searchText.isEmpty()) {
        m_statusLabel->setText(tr("就绪 - %1 个视频").arg(totalVideoCount));
    } else {
        m_statusLabel->setText(tr("已过滤 - %1/%2 个视频").arg(filteredVideoCount).arg(totalVideoCount));
    }

    // 恢复之前活动的标签页（如果它仍然存在）
    if (currentTabIndex != -1 && currentTabIndex < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(currentTabIndex);
    } else if (m_tabWidget->count() > 0) {
        m_tabWidget->setCurrentIndex(0);
    }

    // 调整当前标签页的网格
    adjustGridColumns();
}

void MainWindow::onVideoPosterReady(std::shared_ptr<VideoItem> video)
{
    // 遍历所有标签页查找对应的 VideoWidget
    for (const QString& dir : m_tabVideoWidgets.keys()) {
        for (VideoWidget *widget : m_tabVideoWidgets.value(dir)) {
            if (widget->video() == video) {
                widget->updateThumbnail();
                return;
            }
        }
    }
}

// 添加设置封面模式的实现
void VideoWidget::setUseFanartMode(bool useFanart)
{
    m_useFanartMode = useFanart;
    update(); // 触发重绘以显示正确的封面
}

// ------ VideoWidget 实现 ------

VideoWidget::VideoWidget(std::shared_ptr<VideoItem> video, int thumbnailSize, bool useFanart, QWidget *parent)
    : QWidget(parent),
      m_video(video),
      m_thumbnailSize(thumbnailSize),
      m_hover(false),
      m_useFanartMode(useFanart), // 使用传入的值初始化
      m_selected(false)
{
    // 设置大小策略和最小尺寸
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // 调整大小以包含边框和标签
    setFixedSize(m_thumbnailSize + 10, m_thumbnailSize + 30);

    // 创建标题标签
    m_titleLabel = new QLabel(this);
    m_titleLabel->setText(m_video->fileName());
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);
    // 调整标题位置和大小
    m_titleLabel->setGeometry(5, m_thumbnailSize + 5, m_thumbnailSize, 25);
    m_titleLabel->setStyleSheet("color: #E1E1E1; background-color: transparent;");

    // 启用鼠标追踪以便接收鼠标移入/移出事件
    setMouseTracking(true);

    // 设置工具提示
    setToolTip(m_video->fileName());
}

void VideoWidget::setThumbnailSize(int size)
{
    m_thumbnailSize = size;
    setFixedSize(m_thumbnailSize + 10, m_thumbnailSize + 30); // 考虑边框和标签的高度
    update(); // 重新绘制以应用新的尺寸
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 改为设置选中状态而不是播放视频
        setSelected(true);

        // 取消选择其他视频小部件（遍历父窗口的所有VideoWidget）
        if (parent()) {
            QList<VideoWidget*> siblings = parent()->findChildren<VideoWidget*>();
            for (VideoWidget* widget : siblings) {
                if (widget != this && widget->isSelected()) {
                    widget->setSelected(false);
                }
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 双击时播放视频
        m_video->play();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制背景
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 绘制缩略图背景 - 在深色主题中添加浅灰色背景
    painter.fillRect(0, 0, m_thumbnailSize, m_thumbnailSize, QColor(50, 50, 55));

    // 根据当前模式选择要显示的图片
    QPixmap image;
    if (m_useFanartMode) {
        // 使用背景图
        image = m_video->fanartImage().scaled(m_thumbnailSize, m_thumbnailSize,
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
    } else {
        // 使用海报图
        image = m_video->posterImage().scaled(m_thumbnailSize, m_thumbnailSize,
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
    }

    int x = (m_thumbnailSize - image.width()) / 2;
    int y = (m_thumbnailSize - image.height()) / 2;

    // 绘制边框
    if (m_hover) {
        // 在深色主题中使用亮蓝色边框
        painter.setPen(QPen(QColor(0, 120, 215), 2));
        painter.drawRect(x-1, y-1, image.width()+2, image.height()+2);

        // 绘制播放图标
        QPixmap playIcon(":/icons/play.png");
        int iconSize = 48;
        QPixmap scaledIcon = playIcon.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int iconX = (width() - iconSize) / 2;
        int iconY = (m_thumbnailSize - iconSize) / 2;
        painter.drawPixmap(iconX, iconY, scaledIcon);
    }
    // 选中状态时绘制高亮边框
    else if (m_selected) {
        // 使用高亮橙色边框表示选中状态
        painter.setPen(QPen(QColor(255, 165, 0), 3));
        painter.drawRect(x-2, y-2, image.width()+4, image.height()+4);
    }

    painter.drawPixmap(x, y, image);
}

void VideoWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_hover = true;
    update();
}

void VideoWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hover = false;
    update();
}

// 新增：更新缩略图方法实现
void VideoWidget::updateThumbnail()
{
    // 强制重新加载图片
    if (m_video) {
        // 将图片加载状态重置，强制重新加载
        m_video->posterImage(); // 这会触发重新加载
        m_video->fanartImage(); // 这会触发重新加载
        qDebug() << "更新视频缩略图:" << m_video->fileName();
    }

    // 触发重绘
    update();
}