#ifndef VIDEOLIBRARY_H
#define VIDEOLIBRARY_H

#include <QObject>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QFuture>
#include <QFutureWatcher>
#include <QProcess>
#include <QHash>
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
    const QHash<QString, QVector<std::shared_ptr<VideoItem>>>& videosByDirectory() const;
    QVector<std::shared_ptr<VideoItem>> allVideosFlattened() const;

    // 扫描视频库
    void scanLibrary();

    // 保存和加载库配置
    void saveLibraryConfig(const QString &filePath);
    void loadLibraryConfig(const QString &filePath);

signals:
    void scanStarted();
    void scanProgress(int current, int total);
    void scanFinished();
    void videoAdded(const QString& directory, std::shared_ptr<VideoItem> video);
    void videoPosterReady(std::shared_ptr<VideoItem> video);

private slots:
    void processGeneratedPoster(std::shared_ptr<VideoItem> video);

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

    // 从视频中提取随机帧作为封面图
    bool extractFrameFromVideo(const QString &videoPath, const QString &outputPath);

    // 确保picture文件夹存在
    QString ensurePictureDirectory(const QString &rootDir);

    // 异步生成封面
    void startPosterGeneration();

    QSet<QString> m_directories;  // 视频库目录集合
    QHash<QString, QVector<std::shared_ptr<VideoItem>>> m_videosByDirectory;
    QVector<std::shared_ptr<VideoItem>> m_videosNeedingPoster;

    // 多线程支持
    QFutureWatcher<QVector<std::shared_ptr<VideoItem>>> *m_watcher;
    int m_pendingScanCount;
};

#endif // VIDEOLIBRARY_H