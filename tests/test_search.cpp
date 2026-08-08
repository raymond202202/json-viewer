// 搜索集成测试：输入 JSON → 触发搜索 → 验证结果计数/导航/清空
#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTimer>

static int failures = 0;
#define CHECK(cond, msg) \
    do { \
        if (cond) qInfo().noquote() << "  ✅" << msg; \
        else { qInfo().noquote() << "  ❌ FAIL:" << msg; failures++; } \
    } while (0)

static QLabel *findLabel(QWidget &w, const QString &name)
{
    return w.findChild<QLabel *>(name);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1200, 800);
    w.show();

    QTimer::singleShot(400, [&] {
        // 输入测试 JSON
        auto *input = w.findChild<QPlainTextEdit *>("inputEdit");
        const QString testJson = R"({
  "name": "json-viewer",
  "version": 2.0,
  "active": true,
  "tags": ["json", "测试", "qt"],
  "nested": { "deep": "深层字符串", "count": 42 },
  "items": [ {"id": 1}, {"id": 2} ]
})";
        input->setPlainText(testJson);
        // 等 debounce（400ms）+ 解析
        QTimer::singleShot(700, [&] {
            auto *searchBar = w.findChild<QWidget *>("searchBar");
            auto *searchInput = w.findChild<QLineEdit *>("searchInput");
            auto *searchInfo = findLabel(w, "searchInfo");

            qInfo() << "═══ 打开搜索栏 (Ctrl+F) ═══";
            QMetaObject::invokeMethod(&w, "toggleSearch", Qt::DirectConnection);
            QApplication::processEvents();
            CHECK(searchBar->isVisible(), "搜索栏显示");

            qInfo() << "═══ 搜索 'deep'（键+值）═══";
            searchInput->setText("deep");
            QApplication::processEvents();
            const QString info1 = searchInfo->text();
            qInfo() << "  searchInfo:" << info1;
            CHECK(!info1.isEmpty() && !info1.startsWith("无结果"), "搜索有结果");
            CHECK(info1.contains("1 /"), "显示 1/N 计数");

            qInfo() << "═══ 搜索 'zzz'（无匹配）═══";
            searchInput->setText("zzz_nomatch");
            QApplication::processEvents();
            CHECK(searchInfo->text() == "无结果", "无匹配显示'无结果'");

            qInfo() << "═══ 搜索中文 '深层' ═══";
            searchInput->setText("深层");
            QApplication::processEvents();
            const QString info3 = searchInfo->text();
            qInfo() << "  searchInfo:" << info3;
            CHECK(info3.contains("1 /"), "中文搜索命中");

            qInfo() << "═══ 关闭搜索 (Esc) ═══";
            QMetaObject::invokeMethod(&w, "closeSearch", Qt::DirectConnection);
            QApplication::processEvents();
            CHECK(!searchBar->isVisible(), "搜索栏隐藏");
            CHECK(searchInput->text().isEmpty(), "搜索词清空");

            qInfo() << "═══ 结果 ═══";
            if (failures == 0) qInfo() << "全部通过 ✅";
            else qInfo() << QString("%1 项失败 ❌").arg(failures);
            app.exit(failures == 0 ? 0 : 1);
        });
    });
    return app.exec();
}
