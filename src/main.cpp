#include <QApplication>
#include <QDataStream>
#include <QFileInfo>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "MainWindow.h"

namespace {
const QString kServerName = "json-viewer-qt-single-instance";

QString fileFromArgs(const QStringList &args)
{
    for (int i = 1; i < args.size(); ++i) {
        const QString a = args.at(i);
        if (a.startsWith('-')) continue;
        const QFileInfo fi(a);
        if (fi.exists() && fi.isFile()) {
            const QString ext = fi.suffix().toLower();
            if (ext == "json" || ext == "har" || ext == "txt" || ext == "js" || ext == "jsonc")
                return fi.absoluteFilePath();
        }
    }
    return QString();
}
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("json-viewer-qt");
    app.setOrganizationName("raymond202202");
    app.setWindowIcon(QIcon(":/icon.png"));

    const QString pendingFile = fileFromArgs(app.arguments());

    // 单实例：已有实例则转发文件路径并退出
    QLocalSocket probe;
    probe.connectToServer(kServerName);
    if (probe.waitForConnected(200)) {
        QDataStream out(&probe);
        out << pendingFile;
        probe.waitForBytesWritten(500);
        return 0;
    }

    QLocalServer::removeServer(kServerName);
    QLocalServer server;
    if (!server.listen(kServerName)) {
        // 降级：监听失败时仅提示，不阻止启动（应用无数据冲突，双实例无害）
        qWarning() << "单实例监听失败，未启用单实例保护（socket 可能被占用）";
    }

    MainWindow window;
    window.show();

    if (!pendingFile.isEmpty())
        QTimer::singleShot(0, &window, [&window, pendingFile] { window.openFile(pendingFile); });

    QObject::connect(&server, &QLocalServer::newConnection, &window, [&window, &server] {
        QLocalSocket *sock = server.nextPendingConnection();
        if (!sock) return;
        sock->waitForReadyRead(2000);
        QString path;
        QDataStream in(sock);
        in >> path;
        if (!path.isEmpty()) {
            window.showNormal();
            window.raise();
            window.activateWindow();
            window.openFile(path);
        }
        sock->deleteLater();
    });

    return app.exec();
}
