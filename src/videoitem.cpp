#include "videoitem.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <Windows.h>
#include <shellapi.h>

VideoItem::VideoItem(const QString &filePath, bool loadImagesNow)
    : m_filePath(filePath),
      m_fileSize(0),
      m_imagesLoaded(false)
{
    QFileInfo fileInfo(filePath);
    m_fileName = fileInfo.fileName();
    m_folderPath = fileInfo.path();
    m_fileSize = fileInfo.size();
    m_creationTime = fileInfo.birthTime();
    m_modifiedTime = fileInfo.lastModified();
    
    // 只在需要时加载图片
    if (loadImagesNow) {
        loadImages();
    }
}

void VideoItem::loadImages()
{
    // 如果图片已经加载过，直接返回
    if (m_imagesLoaded) {
        return;
    }

    QDir folder(m_folderPath);
    
    // 查找海报图片
    QString posterPath = folder.filePath("poster.jpg");
    if (QFileInfo::exists(posterPath)) {
        m_posterImage = QPixmap(posterPath);
        if (m_posterImage.isNull()) {
            qDebug() << "无法加载海报图片:" << posterPath;
            createDefaultPoster();
        }
    } else {
        // 使用默认海报
        m_posterImage = QPixmap(":/icons/default_poster.png");
        if (m_posterImage.isNull()) {
            qDebug() << "无法加载默认海报图片";
            createDefaultPoster();
        }
    }
    
    // 查找背景图片
    QString fanartPath = folder.filePath("fanart.jpg");
    if (QFileInfo::exists(fanartPath)) {
        m_fanartImage = QPixmap(fanartPath);
        if (m_fanartImage.isNull()) {
            qDebug() << "无法加载背景图片:" << fanartPath;
            createDefaultFanart();
        }
    } else {
        // 使用默认背景
        m_fanartImage = QPixmap(":/icons/default_fanart.png");
        if (m_fanartImage.isNull()) {
            qDebug() << "无法加载默认背景图片";
            createDefaultFanart();
        }
    }

    m_imagesLoaded = true;
}

void VideoItem::createDefaultPoster()
{
    // 创建一个简单的默认海报图片
    m_posterImage = QPixmap(120, 180);
    m_posterImage.fill(Qt::lightGray);
    
    QPainter painter(&m_posterImage);
    painter.setPen(Qt::black);
    painter.drawRect(0, 0, m_posterImage.width()-1, m_posterImage.height()-1);
    painter.drawText(m_posterImage.rect(), Qt::AlignCenter, m_fileName);
}

void VideoItem::createDefaultFanart()
{
    // 创建一个简单的默认背景图片
    m_fanartImage = QPixmap(320, 180);
    m_fanartImage.fill(Qt::darkGray);
    
    QPainter painter(&m_fanartImage);
    painter.setPen(Qt::white);
    painter.drawRect(0, 0, m_fanartImage.width()-1, m_fanartImage.height()-1);
}

bool VideoItem::play() const
{
    // 使用系统默认程序打开视频
    return QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
}

const QPixmap& VideoItem::posterImage() const
{
    if (!m_imagesLoaded) {
        const_cast<VideoItem*>(this)->loadImages();
    }
    return m_posterImage;
}

const QPixmap& VideoItem::fanartImage() const
{
    if (!m_imagesLoaded) {
        const_cast<VideoItem*>(this)->loadImages();
    }
    return m_fanartImage;
} 