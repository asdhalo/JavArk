#include "videolibrary.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrent/QtConcurrentMap>
#include <algorithm>
#include <QFile>
#include <QDebug>
#include <QDirIterator>
#include <QProcess>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QTemporaryDir>

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
                    // 如果视频需要生成封面，添加到待处理列表
                    if (video->needsPosterGeneration()) {
                        m_videosNeedingPoster.append(video);
                    }
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
    // 扫描完成后开始生成封面
    startPosterGeneration();
}

QVector<std::shared_ptr<VideoItem>> VideoLibrary::findVideosInDirectory(const QString &path)
{
    QVector<std::shared_ptr<VideoItem>> results;
    QDir dir(path);

    // 确保picture文件夹存在
    QString pictureDir = ensurePictureDirectory(path);

    // 使用QDirIterator进行递归扫描，更高效
    QDirIterator it(path, VIDEO_EXTENSIONS, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        if (isVideoFile(filePath)) {
            // 创建VideoItem时不立即加载图片
            auto video = std::make_shared<VideoItem>(filePath, false);

            // 检查视频是否有封面图
            if (!video->hasPoster()) {
                // 如果没有封面图，先检查是否有提取的封面图
                if (!video->checkExtractedPoster(pictureDir)) {
                    // 标记需要生成封面
                    video->setNeedsPosterGeneration(true);
                }
            }
            results.append(video);
        }
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

QString VideoLibrary::ensurePictureDirectory(const QString &rootDir)
{
    QDir dir(rootDir);
    QString pictureDirPath = dir.filePath("picture");

    // 确保目录存在
    if (!QDir(pictureDirPath).exists()) {
        dir.mkdir("picture");
        qDebug() << "创建封面图文件夹:" << pictureDirPath;
    }

    return pictureDirPath;
}

bool VideoLibrary::extractFrameFromVideo(const QString &videoPath, const QString &outputPath)
{
    // 检查视频文件是否存在
    if (!QFileInfo::exists(videoPath)) {
        qDebug() << "视频文件不存在:" << videoPath;
        return false;
    }

    // 检查输出路径是否可写
    QFileInfo outputInfo(outputPath);
    QDir outputDir = outputInfo.dir();
    if (!outputDir.exists()) {
        qDebug() << "输出目录不存在:" << outputDir.path();
        return false;
    }

    // 使用FFmpeg提取视频帧
    QProcess process;

    // 首先获取视频时长
    QStringList durationArgs;
    durationArgs << "-v" << "error"
                << "-show_entries" << "format=duration"
                << "-of" << "default=noprint_wrappers=1:nokey=1"
                << videoPath;

    process.start("ffprobe", durationArgs);
    if (!process.waitForFinished(5000)) {
        qDebug() << "FFprobe执行超时";
        return false;
    }

    QString durationStr = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    bool ok;
    double duration = durationStr.toDouble(&ok);

    if (!ok || duration <= 0) {
        qDebug() << "无法获取视频时长:" << durationStr;
        duration = 60.0; // 默认值
    }

    // 选择一个随机时间点（避开前10%和后10%的部分）
    double startTime = duration * 0.1;
    double endTime = duration * 0.9;

    // 处理非常短的视频
    if (endTime <= startTime) {
        startTime = 0;
        endTime = duration;
    }

    // 生成随机时间点
    double randomTime;
    if (endTime > startTime) {
        randomTime = startTime + (QRandomGenerator::global()->generateDouble() * (endTime - startTime));
    } else {
        randomTime = duration / 2.0;
    }

    // 使用FFmpeg提取帧
    QStringList args;
    args << "-y"  // 覆盖现有文件
         << "-ss" << QString::number(randomTime)
         << "-i" << videoPath
         << "-vframes" << "1"
         << "-q:v" << "2"
         << outputPath;

    process.start("ffmpeg", args);
    if (!process.waitForFinished(10000)) { // 10秒超时
        qDebug() << "FFmpeg执行超时";
        return false;
    }

    // 检查输出文件是否存在
    if (QFileInfo::exists(outputPath)) {
        qDebug() << "成功提取视频帧:" << outputPath;
        return true;
    } else {
        qDebug() << "提取视频帧失败:" << process.readAllStandardError();
        return false;
    }
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

// 新增方法：开始异步生成封面
void VideoLibrary::startPosterGeneration()
{
    if (m_videosNeedingPoster.isEmpty()) {
        return;
    }

    qDebug() << "开始异步生成封面图，共" << m_videosNeedingPoster.size() << "个视频";

    // 使用QtConcurrent::run在后台线程执行封面生成
    for (const auto& video : m_videosNeedingPoster) {
        QtConcurrent::run([this, video]() {
            QString pictureDir = ensurePictureDirectory(video->folderPath());
            QString baseName = QFileInfo(video->fileName()).completeBaseName();
            QString outputPath = QDir(pictureDir).filePath(baseName + ".jpg");

            // 提取视频帧作为封面图
            if (extractFrameFromVideo(video->filePath(), outputPath)) {
                // 封面生成成功，通知主线程
                QMetaObject::invokeMethod(this, "processGeneratedPoster", Qt::QueuedConnection,
                                          Q_ARG(std::shared_ptr<VideoItem>, video));
            }
        });
    }

    // 清空待处理列表
    m_videosNeedingPoster.clear();
}

// 新增槽：处理生成的封面
void VideoLibrary::processGeneratedPoster(std::shared_ptr<VideoItem> video)
{
    if (video) {
        qDebug() << "封面生成完成:" << video->fileName();
        // 尝试加载新生成的封面
        QString pictureDir = ensurePictureDirectory(video->folderPath());
        video->checkExtractedPoster(pictureDir); // 这会加载图片并设置m_imagesLoaded
        // 发射信号通知界面更新
        emit videoPosterReady(video);
    }
}