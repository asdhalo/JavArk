#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <memory>
#include "videolibrary.h"

class VideoWidget;

// 定义排序方式枚举
enum class SortOrder {
    CreationTimeAsc,    // 创建时间升序
    CreationTimeDesc,   // 创建时间降序
    ModifiedTimeAsc,    // 修改时间升序
    ModifiedTimeDesc,   // 修改时间降序
    NameAsc,            // 文件名升序
    NameDesc            // 文件名降序
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAddDirectory();
    void onScanLibrary();
    void onVideoAdded(std::shared_ptr<VideoItem> video);
    void onScanStarted();
    void onScanProgress(int current, int total);
    void onScanFinished();
    void onRemoveDirectory();
    void onToggleCoverMode();
    void onIncreaseThumbnailSize();
    void onDecreaseThumbnailSize();
    void onToggleSortOrder();    // 新增：切换排序方式
    void onSearchTextChanged(const QString &text); // 新增：处理搜索文本变化

private:
    void createUI();
    void createActions();
    void createMenus();
    void saveSettings();
    void loadSettings();
    void updateDirectoryList();
    void clearVideoWidgets();
    void adjustGridColumns();
    void reLayoutVideos();
    void reLayoutFilteredVideos(); // 新增：重新布局过滤后的视频
    void sortVideos();           // 新增：排序视频
    void updateSortButtonText(); // 新增：更新排序按钮文本
    void filterVideos();         // 新增：根据搜索条件过滤视频

    // 视频库
    VideoLibrary *m_library;

    // 配置文件路径
    QString m_configFile;

    // UI组件
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    
    // 顶部工具栏
    QHBoxLayout *m_toolbarLayout;
    QPushButton *m_addDirButton;
    QPushButton *m_removeDirButton;
    QPushButton *m_scanButton;
    QPushButton *m_toggleCoverButton;
    QPushButton *m_increaseButton;
    QPushButton *m_decreaseButton;
    QPushButton *m_sortButton;   // 新增：排序按钮
    QLineEdit *m_searchEdit;     // 新增：搜索框
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    // 底部状态栏
    QHBoxLayout *m_bottomLayout;
    
    // 视频显示区域
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_gridLayout;
    
    // 目录列表
    QMenu *m_dirMenu;
    QList<QAction*> m_dirActions;
    
    // 视频小部件
    QList<VideoWidget*> m_videoWidgets;
    QList<VideoWidget*> m_filteredVideoWidgets; // 新增：过滤后的视频列表
    
    // 显示设置
    int m_gridColumns;
    int m_thumbnailSize;
    bool m_useFanartMode;
    SortOrder m_sortOrder;       // 新增：当前排序方式
    QString m_searchText;        // 新增：当前搜索文本
};

// 视频小部件，显示单个视频项目
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(std::shared_ptr<VideoItem> video, int thumbnailSize, QWidget *parent = nullptr);
    
    std::shared_ptr<VideoItem> video() const { return m_video; }
    void setThumbnailSize(int size);
    
    // 新增：设置封面模式
    void setUseFanartMode(bool useFanart) { m_useFanartMode = useFanart; update(); }
    bool useFanartMode() const { return m_useFanartMode; }
    
    // 新增：选中状态管理
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; update(); }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override; // 新增：双击事件
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    std::shared_ptr<VideoItem> m_video;
    QLabel *m_titleLabel;
    int m_thumbnailSize;
    bool m_hover;
    bool m_useFanartMode; // 是否使用背景图作为封面
    bool m_selected;      // 新增：是否被选中
};

#endif // MAINWINDOW_H 