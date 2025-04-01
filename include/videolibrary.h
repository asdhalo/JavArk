#ifndef VIDEOLIBRARY_H
#define VIDEOLIBRARY_H

#include <QObject>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QFuture>
#include <QFutureWatcher>
#include <memory>
#include "videoitem.h"

class VideoLibrary : public QObject
{
    Q_OBJECT

public:
    explicit VideoLibrary(QObject *parent = nullptr);
    ~VideoLibrary();

    // 管理视频库路径
    void addDirectory(const QString &path);
    void removeDirectory(const QString &path, bool removeFiles = false);
    QStringList directories() const;
    
    // 获取视频列表
    const QVector<std::shared_ptr<VideoItem>>& videos() const;
    
    // 扫描视频库
    void scanLibrary();
    
    // 保存和加载库配置
    void saveLibraryConfig(const QString &filePath);
    void loadLibraryConfig(const QString &filePath);

signals:
    void scanStarted();
    void scanProgress(int current, int total);
    void scanFinished();
    void videoAdded(std::shared_ptr<VideoItem> video);

private slots:
    void scanDirectoryFinished();

private:
    // 多线程扫描单个目录
    void scanDirectory(const QString &path);
    
    // 扫描指定目录下的视频文件
    QVector<std::shared_ptr<VideoItem>> findVideosInDirectory(const QString &path);
    
    // 检查是否是视频文件
    bool isVideoFile(const QString &filePath) const;
    
    // 删除视频文件
    bool removeVideoFiles(const std::shared_ptr<VideoItem>& video);
    
    // 删除目录下的封面图片
    void removeDirectoryCovers(const QString &dirPath);

    QSet<QString> m_directories;  // 视频库目录集合
    QVector<std::shared_ptr<VideoItem>> m_videos;  // 视频项目列表
    
    // 多线程支持
    QFutureWatcher<QVector<std::shared_ptr<VideoItem>>> *m_watcher;
    int m_pendingScanCount;
};

#endif // VIDEOLIBRARY_H 