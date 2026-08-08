// 布局诊断：输出各 widget 实际 geometry
#include "MainWindow.h"
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QTreeView>
#include <QLabel>
#include <QPushButton>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1200, 800);
    w.show();

    QTimer::singleShot(500, [&] {
        qInfo() << "═══ 窗口布局诊断 (1200x800) ═══";
        qInfo() << "窗口大小:" << w.size();
        const auto *central = w.centralWidget();
        qInfo() << "central:" << central->geometry();

        // 遍历 central 的直接子 widget
        const QList<QWidget *> children = central->findChildren<QWidget *>();
        for (auto *cw : children) {
            if (cw->parentWidget() == central) {
                qInfo().noquote() << QString("  central子: %1 geo=%2x%3+%4+%5 visible=%6")
                    .arg(cw->objectName().isEmpty() ? cw->metaObject()->className() : cw->objectName())
                    .arg(cw->width()).arg(cw->height())
                    .arg(cw->x()).arg(cw->y()).arg(cw->isVisible());
            }
        }
        // 找 splitter 和主要区域
        auto *spl = w.findChild<QSplitter *>();
        if (spl) {
            qInfo() << "splitter:" << spl->geometry() << "sizes:" << spl->sizes();
            for (int i = 0; i < spl->count(); ++i) {
                auto *panel = spl->widget(i);
                qInfo().noquote() << QString("  面板%1: %2 geo=%3x%4+%5+%6")
                    .arg(i).arg(panel->objectName())
                    .arg(panel->width()).arg(panel->height()).arg(panel->x()).arg(panel->y());
                const auto kids = panel->findChildren<QWidget *>();
                for (auto *k : kids) {
                    if (k->parentWidget() == panel)
                        qInfo().noquote() << QString("    └ %1: %2x%3+%4+%5 visible=%6")
                            .arg(k->objectName().isEmpty() ? k->metaObject()->className() : k->objectName())
                            .arg(k->width()).arg(k->height()).arg(k->x()).arg(k->y()).arg(k->isVisible());
                }
            }
        }
        app.quit();
    });
    return app.exec();
}
