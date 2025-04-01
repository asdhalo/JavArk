#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <Windows.h>
#include <QPainter>
#include <QPen>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>

// 设置自定义消息处理函数，记录到日志文件
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 打开或创建日志文件
    QString logFilePath = QCoreApplication::applicationDirPath() + "/logs";
    QDir logDir(logFilePath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    QString fileName = logFilePath + "/javark-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    
    // 生成日志内容
    QTextStream out(&file);
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr;
    
    switch (type) {
        case QtDebugMsg:
            levelStr = "DEBUG";
            break;
        case QtInfoMsg:
            levelStr = "INFO";
            break;
        case QtWarningMsg:
            levelStr = "WARNING";
            break;
        case QtCriticalMsg:
            levelStr = "CRITICAL";
            break;
        case QtFatalMsg:
            levelStr = "FATAL";
            break;
    }
    
    out << QString("[%1] [%2] %3\n").arg(timeStr, levelStr, msg);
    file.close();
}

// 创建默认图标
void createDefaultIcons()
{
    try {
        // 获取应用程序所在目录
        QString appDir = QCoreApplication::applicationDirPath();
        QDir iconDir(appDir + "/resources/icons");
        if (!iconDir.exists()) {
            bool created = iconDir.mkpath(".");
            if (!created) {
                qDebug() << "无法创建图标目录:" << iconDir.path();
                return;
            }
        }
        
        // 创建占位图标文件(如果不存在)
        QStringList iconFiles = {
            "add.png", "refresh.png", "delete.png", "play.png", 
            "default_poster.png", "default_fanart.png"
        };
        
        for (const QString &file : iconFiles) {
            QString path = iconDir.filePath(file);
            if (!QFile::exists(path)) {
                // 创建一个简单的图标，实际项目应该使用真实图标
                QPixmap pixmap(64, 64);
                pixmap.fill(Qt::transparent);
                
                QPainter painter(&pixmap);
                painter.setPen(QPen(Qt::black, 2));
                painter.setBrush(Qt::white);
                painter.drawRect(4, 4, 56, 56);
                
                if (file == "add.png") {
                    painter.drawText(24, 38, "+");
                } else if (file == "refresh.png") {
                    painter.drawText(24, 38, "R");
                } else if (file == "delete.png") {
                    painter.drawText(24, 38, "X");
                } else if (file == "play.png") {
                    painter.drawText(24, 38, "▶");
                }
                
                bool success = pixmap.save(path);
                if (!success) {
                    qDebug() << "无法保存图标文件:" << path;
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "创建默认图标时出错:" << e.what();
    } catch (...) {
        qDebug() << "创建默认图标时出现未知错误";
    }
}

// 应用高DPI设置
void setHighDPISettings()
{
    // 启用高DPI支持
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
}

// 添加高优先级处理
void setHighPriority()
{
    // 对于Windows系统，设置线程优先级为高
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

int main(int argc, char *argv[])
{
    // 安装消息处理函数
    qInstallMessageHandler(messageHandler);
    
    // 设置应用程序信息
    QCoreApplication::setOrganizationName("JavArk");
    QCoreApplication::setApplicationName("JavArk");
    QCoreApplication::setApplicationVersion("1.0.0");
    
    // 高DPI设置
    setHighDPISettings();
    
    QApplication app(argc, argv);
    
    try {
        // 创建默认图标
        createDefaultIcons();
        
        // 设置高优先级
        setHighPriority();
        
        // 创建并显示主窗口
        MainWindow mainWindow;
        mainWindow.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        qDebug() << "应用程序启动时发生异常:" << e.what();
        QMessageBox::critical(nullptr, "错误", QString("应用程序启动失败:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        qDebug() << "应用程序启动时发生未知异常";
        QMessageBox::critical(nullptr, "错误", "应用程序启动失败，发生未知错误");
        return 1;
    }
} 