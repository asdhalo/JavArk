#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QString>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QDateTime>

class VideoItem {
public:
    VideoItem(const QString &filePath, bool loadImagesNow = true);
    ~VideoItem() = default;

    // 获取视频信息
    QString filePath() const { return m_filePath; }
    QString fileName() const { return m_fileName; }
    QString folderPath() const { return m_folderPath; }
    qint64 fileSize() const { return m_fileSize; }
    QDateTime creationTime() const { return m_creationTime; }
    QDateTime modifiedTime() const { return m_modifiedTime; }

    // 获取图片
    const QPixmap& posterImage() const;
    const QPixmap& fanartImage() const;

    // 检查是否有封面图
    bool hasPoster() const;

    // 检查并使用提取的封面图
    bool checkExtractedPoster(const QString &pictureDir);

    // 播放视频
    bool play() const;

    // 标记和检查是否需要生成封面
    void setNeedsPosterGeneration(bool needs);
    bool needsPosterGeneration() const;

private:
    // 从文件夹加载海报和背景图
    void loadImages();

    // 加载提取的封面图
    bool loadExtractedPoster(const QString &posterPath);

    // 创建默认图片
    void createDefaultPoster();
    void createDefaultFanart();

    QString m_filePath;    // 视频完整路径
    QString m_fileName;    // 视频文件名
    QString m_folderPath;  // 视频所在文件夹
    qint64 m_fileSize;     // 文件大小
    QDateTime m_creationTime; // 文件创建时间
    QDateTime m_modifiedTime; // 文件修改时间

    mutable QPixmap m_posterImage;
    mutable QPixmap m_fanartImage;
    mutable bool m_imagesLoaded;
    bool m_needsPosterGeneration; // 新增：标记是否需要生成封面
};

#endif // VIDEOITEM_H