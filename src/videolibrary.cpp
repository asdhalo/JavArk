#include "videolibrary.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrent/QtConcurrentMap>
#include <algorithm>
#include <QFile>
#include <QDebug>

// 支持的视频扩展名
static const QStringList VIDEO_EXTENSIONS = {
    "*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", 
    "*.webm", "*.m4v", "*.mpg", "*.mpeg", "*.ts", "*.3gp", "*.rm"
};

VideoLibrary::VideoLibrary(QObject *parent)
    : QObject(parent),
      m_watcher(new QFutureWatcher<QVector<std::shared_ptr<VideoItem>>>(this)),
      m_pendingScanCount(0)
{
    // 连接信号和槽
    connect(m_watcher, &QFutureWatcher<QVector<std::shared_ptr<VideoItem>>>::resultReadyAt,
            this, [this](int index) {
                auto results = m_watcher->resultAt(index);
                for (const auto &video : results) {
                    m_videos.append(video);
                    emit videoAdded(video);
                }
                emit scanProgress(index + 1, m_pendingScanCount);
            });
    
    connect(m_watcher, &QFutureWatcher<QVector<std::shared_ptr<VideoItem>>>::finished,
            this, &VideoLibrary::scanDirectoryFinished);
}

VideoLibrary::~VideoLibrary()
{
    // 取消正在进行的扫描
    if (m_watcher->isRunning()) {
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }
}

void VideoLibrary::addDirectory(const QString &path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.exists() && fileInfo.isDir()) {
        m_directories.insert(QDir(path).absolutePath());
    }
}

void VideoLibrary::removeDirectory(const QString &path, bool removeFiles)
{
    QString absPath = QDir(path).absolutePath();
    if (m_directories.contains(absPath)) {
        m_directories.remove(absPath);
        
        // 找出需要删除的视频
        QVector<std::shared_ptr<VideoItem>> videosToRemove;
        for (const auto& video : m_videos) {
            if (video->folderPath().startsWith(absPath)) {
                videosToRemove.append(video);
            }
        }
        
        // 如果需要物理删除文件
        if (removeFiles) {
            for (const auto& video : videosToRemove) {
                removeVideoFiles(video);
            }
        }
        
        // 从列表中移除视频
        auto it = std::remove_if(m_videos.begin(), m_videos.end(),
                         [&absPath](const std::shared_ptr<VideoItem>& video) {
                             return video->folderPath().startsWith(absPath);
                         });
        m_videos.erase(it, m_videos.end());
    }
}

QStringList VideoLibrary::directories() const
{
    return QStringList(m_directories.begin(), m_directories.end());
}

const QVector<std::shared_ptr<VideoItem>>& VideoLibrary::videos() const
{
    return m_videos;
}

void VideoLibrary::scanLibrary()
{
    // 如果已经有扫描在进行中，取消它
    if (m_watcher->isRunning()) {
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }

    emit scanStarted();
    
    // 清空视频列表
    m_videos.clear();
    
    // 获取目录列表
    QStringList dirs = directories();
    m_pendingScanCount = dirs.size();
    
    if (dirs.isEmpty()) {
        emit scanFinished();
        return;
    }
    
    // 创建扫描任务
    QFuture<QVector<std::shared_ptr<VideoItem>>> future = QtConcurrent::mapped(dirs, 
        [this](const QString &dir) {
            return this->findVideosInDirectory(dir);
        });
    
    // 监视结果
    m_watcher->setFuture(future);
}

void VideoLibrary::scanDirectoryFinished()
{
    emit scanFinished();
}

QVector<std::shared_ptr<VideoItem>> VideoLibrary::findVideosInDirectory(const QString &path)
{
    QVector<std::shared_ptr<VideoItem>> results;
    QDir dir(path);
    
    // 获取视频文件
    QStringList videoFiles;
    for (const QString &ext : VIDEO_EXTENSIONS) {
        dir.setNameFilters(QStringList(ext));
        dir.setFilter(QDir::Files);
        videoFiles.append(dir.entryList());
    }
    
    // 添加到结果
    for (const QString &file : videoFiles) {
        QString filePath = dir.filePath(file);
        if (isVideoFile(filePath)) {
            results.append(std::make_shared<VideoItem>(filePath));
        }
    }
    
    // 递归扫描子目录
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setNameFilters(QStringList());
    QStringList subdirs = dir.entryList();
    for (const QString &subdir : subdirs) {
        QString subdirPath = dir.filePath(subdir);
        auto subdirResults = findVideosInDirectory(subdirPath);
        results.append(subdirResults);
    }
    
    return results;
}

bool VideoLibrary::isVideoFile(const QString &filePath) const
{
    // 使用扩展名检查是否是视频文件
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    static const QSet<QString> videoExtensions = {
        "mp4", "mkv", "avi", "mov", "wmv", "flv", 
        "webm", "m4v", "mpg", "mpeg", "ts", "3gp", "rm"
    };
    
    return videoExtensions.contains(suffix);
}

void VideoLibrary::saveLibraryConfig(const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    settings.clear();
    
    // 保存视频库目录
    settings.beginWriteArray("Directories");
    QStringList dirs = directories();
    for (int i = 0; i < dirs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("Path", dirs.at(i));
    }
    settings.endArray();
    
    settings.sync();
}

void VideoLibrary::loadLibraryConfig(const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    
    // 加载目录
    m_directories.clear();
    int dirCount = settings.beginReadArray("Directories");
    for (int i = 0; i < dirCount; ++i) {
        settings.setArrayIndex(i);
        QString path = settings.value("Path").toString();
        if (!path.isEmpty()) {
            addDirectory(path);
        }
    }
    settings.endArray();
}

bool VideoLibrary::removeVideoFiles(const std::shared_ptr<VideoItem>& video)
{
    bool success = false;
    
    try {
        // 删除视频文件
        QFile videoFile(video->filePath());
        if (videoFile.exists()) {
            success = videoFile.remove();
            if (!success) {
                qDebug() << "无法删除视频文件:" << video->filePath() << " - " << videoFile.errorString();
            }
        }
        
        // 查找并删除同名的海报和背景图片
        QDir folder(video->folderPath());
        QString baseName = QFileInfo(video->filePath()).baseName();
        
        // 检查并删除海报图片
        QString posterPath = folder.filePath(baseName + "-poster.jpg");
        if (QFileInfo::exists(posterPath)) {
            QFile posterFile(posterPath);
            if (!posterFile.remove()) {
                qDebug() << "无法删除海报图片:" << posterPath << " - " << posterFile.errorString();
            }
        }
        
        // 检查并删除背景图片
        QString fanartPath = folder.filePath(baseName + "-fanart.jpg");
        if (QFileInfo::exists(fanartPath)) {
            QFile fanartFile(fanartPath);
            if (!fanartFile.remove()) {
                qDebug() << "无法删除背景图片:" << fanartPath << " - " << fanartFile.errorString();
            }
        }
    }
    catch (const std::exception& e) {
        qDebug() << "删除视频文件时出错:" << e.what();
        success = false;
    }
    
    return success;
}

void VideoLibrary::removeDirectoryCovers(const QString &dirPath)
{
    try {
        QDir dir(dirPath);
        
        qDebug() << "开始删除目录下的封面图片:" << dirPath;
        
        // 递归遍历目录
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            removeDirectoryCovers(dir.filePath(subdir));
        }
        
        // 删除当前目录下的封面图片
        QStringList filters;
        filters << "poster.jpg" << "fanart.jpg"; // 通用封面
        
        // 添加可能存在的特定视频封面格式
        filters << "*-poster.jpg" << "*-fanart.jpg" << "*.png" << "*.jpeg";
        
        // 设置过滤器
        dir.setNameFilters(filters);
        dir.setFilter(QDir::Files);
        QStringList coverFiles = dir.entryList();
        
        qDebug() << "在目录" << dirPath << "中找到" << coverFiles.size() << "个封面图片";
        
        // 删除每个封面文件
        for (const QString &file : coverFiles) {
            QString filePath = dir.filePath(file);
            QFile coverFile(filePath);
            
            // 确保文件不是只读的
            QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner);
            
            if (coverFile.remove()) {
                qDebug() << "成功删除封面图片:" << filePath;
            } else {
                qDebug() << "无法删除封面图片:" << filePath << " - " << coverFile.errorString();
                
                // 尝试使用系统API删除文件
                if (QFile::exists(filePath)) {
                    bool result = QFile::remove(filePath);
                    qDebug() << "使用系统API删除文件:" << filePath << (result ? "成功" : "失败");
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "删除封面图片时出错:" << e.what();
    } catch (...) {
        qDebug() << "删除封面图片时发生未知错误";
    }
} 