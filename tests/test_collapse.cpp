// 折叠功能测试：验证面板彻底收起（visible=false），对齐老版 display:none 语义
#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QWidget>

static int failures = 0;
#define CHECK(cond, msg) \
    do { \
        if (cond) qInfo().noquote() << "  ✅" << msg; \
        else { qInfo().noquote() << "  ❌ FAIL:" << msg; failures++; } \
    } while (0)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1200, 800);
    w.show();

    QTimer::singleShot(400, [&] {
        auto *splitter = w.findChild<QSplitter *>();
        QWidget *leftPanel = splitter->widget(0);
        QWidget *rightPanel = splitter->widget(1);
        // 两个边缘按钮 objectName 均为 "edgeBtn"，按 x 坐标区分左右
        const auto btns = w.findChildren<QPushButton *>("edgeBtn");
        QPushButton *leftBtn = nullptr, *rightBtn = nullptr;
        for (auto *b : btns) {
            if (b->geometry().x() < w.width() / 2) leftBtn = b;
            else rightBtn = b;
        }
        CHECK(leftBtn != nullptr && rightBtn != nullptr, "找到左右边缘按钮");

        qInfo() << "═══ 初始状态 ═══";
        CHECK(leftPanel->isVisible() && rightPanel->isVisible(), "两面板初始可见");
        qInfo() << "  splitter sizes:" << splitter->sizes();

        qInfo() << "═══ 折叠左侧（Ctrl+B）═══";
        // 模拟 Ctrl+B 快捷键：QShortcut 触发。直接调用内部槽不可行，用 QMetaObject::invokeMethod
        QMetaObject::invokeMethod(&w, "toggleLeftPanel", Qt::DirectConnection);
        QApplication::processEvents();
        CHECK(!leftPanel->isVisible(), "左面板彻底收起 (visible=false)");
        CHECK(rightPanel->isVisible(), "右面板仍可见");
        CHECK(!rightBtn->isEnabled(), "右折叠按钮被禁用（互斥）");
        const QList<int> s1 = splitter->sizes();
        CHECK(s1[0] == 0, QString("左面板宽度为 0（实际 %1）").arg(s1[0]));

        qInfo() << "═══ 展开左侧 ═══";
        QMetaObject::invokeMethod(&w, "toggleLeftPanel", Qt::DirectConnection);
        QApplication::processEvents();
        CHECK(leftPanel->isVisible(), "左面板恢复可见");
        CHECK(rightBtn->isEnabled(), "右折叠按钮恢复可用");

        qInfo() << "═══ 折叠右侧 ═══";
        QMetaObject::invokeMethod(&w, "toggleRightPanel", Qt::DirectConnection);
        QApplication::processEvents();
        CHECK(!rightPanel->isVisible(), "右面板彻底收起 (visible=false)");
        CHECK(leftPanel->isVisible(), "左面板仍可见");
        CHECK(!leftBtn->isEnabled(), "左折叠按钮被禁用（互斥）");
        const QList<int> s2 = splitter->sizes();
        CHECK(s2[1] == 0, QString("右面板宽度为 0（实际 %1）").arg(s2[1]));

        qInfo() << "═══ 展开右侧 ═══";
        QMetaObject::invokeMethod(&w, "toggleRightPanel", Qt::DirectConnection);
        QApplication::processEvents();
        CHECK(rightPanel->isVisible(), "右面板恢复可见");

        qInfo() << "═══ 结果 ═══";
        if (failures == 0) qInfo() << "全部通过 ✅";
        else qInfo() << QString("%1 项失败 ❌").arg(failures);
        app.exit(failures == 0 ? 0 : 1);
    });
    return app.exec();
}
