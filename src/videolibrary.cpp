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
#include <QFuture>

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
    // 这个构造函数不再需要连接 m_watcher 的信号
    // 现在我们为每个目录的扫描创建单独的 watcher 并在 lambda 中处理结果
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

        // 找出需要删除的视频 - 现在需要从哈希表中删除
        if (m_videosByDirectory.contains(absPath)) {
            QVector<std::shared_ptr<VideoItem>>& videosInDir = m_videosByDirectory[absPath];
            // 如果需要物理删除文件
            if (removeFiles) {
                for (const auto& video : videosInDir) {
                    removeVideoFiles(video);
                }
            }
            // 从哈希表中移除该目录及其视频
            m_videosByDirectory.remove(absPath);
        }

        // 从待生成封面列表中移除相关视频
        auto it = std::remove_if(m_videosNeedingPoster.begin(), m_videosNeedingPoster.end(),
                         [&absPath](const std::shared_ptr<VideoItem>& video) {
                             return video->folderPath().startsWith(absPath);
                         });
        m_videosNeedingPoster.erase(it, m_videosNeedingPoster.end());

    }
}

QStringList VideoLibrary::directories() const
{
    return QStringList(m_directories.begin(), m_directories.end());
}

const QHash<QString, QVector<std::shared_ptr<VideoItem>>>& VideoLibrary::videosByDirectory() const
{
    return m_videosByDirectory;
}

void VideoLibrary::scanLibrary()
{
    // 如果已经有扫描在进行中，取消它
    if (m_watcher->isRunning()) {
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }

    emit scanStarted();

    // 清空视频哈希表
    m_videosByDirectory.clear();

    // 获取目录列表
    QStringList dirs = directories();
    m_pendingScanCount = dirs.size();

    if (dirs.isEmpty()) {
        emit scanFinished();
        return;
    }

    // 创建扫描任务，使用共享计数器来跟踪完成情况
    std::shared_ptr<int> completedCount = std::make_shared<int>(0);

    for (const QString &dir : dirs) {
        // 直接在 lambda 中捕获目录路径
        auto futureWatcher = new QFutureWatcher<QVector<std::shared_ptr<VideoItem>>>(this);
        QFuture<QVector<std::shared_ptr<VideoItem>>> future = QtConcurrent::run(
            [this, dir]() {
                return this->findVideosInDirectory(dir);
            }
        );

        // 连接 finished 信号
        connect(futureWatcher, &QFutureWatcher<QVector<std::shared_ptr<VideoItem>>>::finished, this,
            [this, dir, completedCount, futureWatcher]() {
                // 使用 lambda 捕获的 dir 参数，而不是从 map 中查找
                QVector<std::shared_ptr<VideoItem>> results = futureWatcher->result();

                if (!m_videosByDirectory.contains(dir)) {
                    m_videosByDirectory[dir] = QVector<std::shared_ptr<VideoItem>>();
                }

                for (const auto &video : results) {
                    m_videosByDirectory[dir].append(video);
                    emit videoAdded(dir, video);
                    if (video->needsPosterGeneration()) {
                        m_videosNeedingPoster.append(video);
                    }
                }

                // 增加完成计数
                (*completedCount)++;
                emit scanProgress(*completedCount, m_pendingScanCount);

                // 所有目录都扫描完成时，发送完成信号并开始生成封面
                if (*completedCount >= m_pendingScanCount) {
                    emit scanFinished();
                    startPosterGeneration();
                }

                // 清理 watcher
                futureWatcher->deleteLater();
            }
        );

        // 开始监视结果
        futureWatcher->setFuture(future);
    }
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

    // 加载视频库目录
    m_directories.clear();
    int size = settings.beginReadArray("Directories");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString dirPath = settings.value("Path").toString();
        if (!dirPath.isEmpty() && QDir(dirPath).exists()) {
            m_directories.insert(dirPath);
        } else {
             qWarning() << "配置文件中保存的目录无效或不存在:" << dirPath;
        }
    }
    settings.endArray();

    // 加载后可以触发一次扫描
    // scanLibrary(); // 或者由调用者决定何时扫描
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
    QString pictureDir = QDir(dirPath).filePath("picture");
    if (!QDir(pictureDir).exists()) {
        return;
    }

    QDirIterator it(pictureDir, QStringList() << "*.jpg", QDir::Files);
    while (it.hasNext()) {
        QString coverPath = it.next();
        if (!QFile::remove(coverPath)) {
            qWarning() << "无法删除封面文件:" << coverPath;
        }
    }
}

// 将视频添加到 m_videosByDirectory 而不是 m_videos
void VideoLibrary::startPosterGeneration()
{
    if (m_videosNeedingPoster.isEmpty()) {
        return;
    }

    qInfo() << "开始异步生成" << m_videosNeedingPoster.size() << "个视频的封面...";

    // 使用 QtConcurrent::map 进行异步处理
    QFuture<void> future = QtConcurrent::map(m_videosNeedingPoster, [this](std::shared_ptr<VideoItem> video) {
        QString pictureDir = ensurePictureDirectory(video->folderPath());
        // 使用与VideoItem::checkExtractedPoster相同的文件名格式
        QString baseName = QFileInfo(video->fileName()).completeBaseName();
        QString posterPath = QDir(pictureDir).filePath(baseName + ".jpg");

        if (!QFileInfo::exists(posterPath)) { // 再次检查，以防万一
             if (extractFrameFromVideo(video->filePath(), posterPath)) {
                // 使用 VideoItem 提供的公有方法加载生成的封面图片
                bool loaded = video->checkExtractedPoster(pictureDir);
                qDebug() << "封面生成完成，加载状态:" << loaded << "路径:" << posterPath;

                 // 在主线程中处理UI更新或信号发射
                 QMetaObject::invokeMethod(this, "processGeneratedPoster", Qt::QueuedConnection,
                                         Q_ARG(std::shared_ptr<VideoItem>, video));
             } else {
                 qWarning() << "无法为视频生成封面:" << video->filePath();
             }
        }
    });

    // 可以选择性地添加一个 QFutureWatcher 来监视封面生成的完成
    // 但目前只在完成后处理单个视频

    // 清空待处理列表
    m_videosNeedingPoster.clear();
}

void VideoLibrary::processGeneratedPoster(std::shared_ptr<VideoItem> video)
{
    if (!video) {
        qWarning() << "处理生成的封面时收到空视频对象";
        return;
    }

    // 即使没有检测到海报，也发送信号更新UI
    // 因为封面可能已经生成但没有被正确检测到
    qDebug() << "封面已生成并准备就绪:" << video->fileName();

    // 强制重新加载图片
    QString pictureDir = ensurePictureDirectory(video->folderPath());
    video->checkExtractedPoster(pictureDir);

    // 发送信号更新UI
    emit videoPosterReady(video);
}

// 添加获取所有视频列表的实现
QVector<std::shared_ptr<VideoItem>> VideoLibrary::allVideosFlattened() const
{
    QVector<std::shared_ptr<VideoItem>> allVideos;
    for (const auto& videoList : m_videosByDirectory.values()) {
        allVideos.append(videoList);
    }
    return allVideos;
}