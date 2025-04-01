#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QString>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QDateTime>

class VideoItem {
public:
    VideoItem(const QString &filePath);
    ~VideoItem() = default;

    // 获取视频信息
    QString filePath() const { return m_filePath; }
    QString fileName() const { return m_fileName; }
    QString folderPath() const { return m_folderPath; }
    QPixmap posterImage() const { return m_posterImage; }
    QPixmap fanartImage() const { return m_fanartImage; }
    qint64 fileSize() const { return m_fileSize; }
    QDateTime creationTime() const { return m_creationTime; }
    QDateTime modifiedTime() const { return m_modifiedTime; }
    
    // 播放视频
    bool play() const;

private:
    // 从文件夹加载海报和背景图
    void loadImages();
    
    // 创建默认图片
    void createDefaultPoster();
    void createDefaultFanart();
    
    QString m_filePath;    // 视频完整路径
    QString m_fileName;    // 视频文件名
    QString m_folderPath;  // 视频所在文件夹
    QPixmap m_posterImage; // 海报图片 (poster.jpg)
    QPixmap m_fanartImage; // 背景图片 (fanart.jpg)
    qint64 m_fileSize;     // 文件大小
    QDateTime m_creationTime; // 文件创建时间
    QDateTime m_modifiedTime; // 文件修改时间
};

#endif // VIDEOITEM_H 