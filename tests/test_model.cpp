// 模型自测：验证懒加载、路径索引、搜索、统计
#include "JsonTreeModel.h"
#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QElapsedTimer>
#include <QFile>
#include <QTimer>

static int failures = 0;
#define CHECK(cond, msg) \
    do { \
        if (cond) { qInfo().noquote() << "  ✅" << msg; } \
        else { qInfo().noquote() << "  ❌ FAIL:" << msg; failures++; } \
    } while (0)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile f("/tmp/jv-test.json");
    f.open(QIODevice::ReadOnly);
    const QByteArray raw = f.readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    CHECK(err.error == QJsonParseError::NoError, "解析测试 JSON");

    JsonTreeModel model;
    const QJsonValue root = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    model.setJson(root);

    qInfo() << "═══ 模型结构 ═══";
    CHECK(model.rowCount(QModelIndex()) == 9, QString("根节点 9 个子项（实际 %1）").arg(model.rowCount(QModelIndex())));
    CHECK(model.totalNodes() == 26, QString("总节点 26（实际 %1）").arg(model.totalNodes()));
    CHECK(model.keyCount() == 19, QString("键数 19（实际 %1）").arg(model.keyCount()));

    qInfo() << "═══ 懒加载 ═══";
    // 根节点 children 已加载（setJson 时 loadChildren）
    CHECK(model.canFetchMore(QModelIndex()) == false, "根节点已加载");
    // 找 "嵌套" 子节点
    QModelIndex nestedIdx;
    for (int r = 0; r < model.rowCount(QModelIndex()); ++r) {
        const QModelIndex idx = model.index(r, 0);
        if (idx.data(JsonTreeModel::KeyRole).toString() == "嵌套") { nestedIdx = idx; break; }
    }
    CHECK(nestedIdx.isValid(), "找到「嵌套」节点");
    CHECK(model.hasChildren(nestedIdx), "「嵌套」有子节点");
    CHECK(model.canFetchMore(nestedIdx), "「嵌套」未加载（懒加载生效）");
    CHECK(model.rowCount(nestedIdx) == 0, "未加载前 rowCount=0");
    model.fetchMore(nestedIdx);
    CHECK(model.rowCount(nestedIdx) == 1, "fetchMore 后「嵌套」1 个子项（层级1）");

    qInfo() << "═══ indexForPath ═══";
    const QModelIndex deep = model.indexForPath("嵌套.层级1.层级2.值");
    CHECK(deep.isValid(), "indexForPath 定位深层节点");
    if (deep.isValid())
        CHECK(deep.data(JsonTreeModel::ValueTextRole).toString() == "\"深层字符串\"",
              QString("深层值 = %1").arg(deep.data(JsonTreeModel::ValueTextRole).toString()));

    qInfo() << "═══ 数组索引 ═══";
    const QModelIndex arrItem = model.indexForPath("数组对象.1");
    CHECK(arrItem.isValid(), "数组对象[1] 可定位");
    if (arrItem.isValid())
        CHECK(arrItem.data(JsonTreeModel::KeyNameRole).toString() == "1", "数组索引键名 = '1'");

    qInfo() << "═══ 搜索遍历（模拟 MainWindow 逻辑） ═══";
    // 简易命中检查：遍历 QJsonValue
    auto valueText = [](const QJsonValue &v) -> QString {
        switch (v.type()) {
        case QJsonValue::String: {
            const QByteArray a = QJsonDocument(QJsonArray{QJsonValue(v.toString())}).toJson(QJsonDocument::Compact);
            return QString::fromUtf8(a.mid(1, a.size() - 2));
        }
        case QJsonValue::Double: return QString::number(v.toDouble(), 'g', 17);
        case QJsonValue::Bool: return v.toBool() ? "true" : "false";
        case QJsonValue::Null: return "null";
        default: return QString();
        }
    };
    int hits = 0;
    QString hitPath;
    std::function<void(const QJsonValue &, const QString &)> walk = [&](const QJsonValue &v, const QString &path) {
        if (v.isObject()) {
            const QJsonObject o = v.toObject();
            for (auto it = o.begin(); it != o.end(); ++it) {
                const QString k = it.key();
                const QJsonValue cv = it.value();
                const QString cp = path.isEmpty() ? k : path + "." + k;
                if (k.contains("深层", Qt::CaseInsensitive) || valueText(cv).contains("深层", Qt::CaseInsensitive)) {
                    hits++; hitPath = cp;
                }
                if (cv.isObject() || cv.isArray()) walk(cv, cp);
            }
        } else if (v.isArray()) {
            const QJsonArray a = v.toArray();
            for (int i = 0; i < a.size(); ++i) {
                const QJsonValue cv = a.at(i);
                const QString cp = path.isEmpty() ? QString::number(i) : path + "." + QString::number(i);
                if (valueText(cv).contains("深层", Qt::CaseInsensitive)) { hits++; hitPath = cp; }
                if (cv.isObject() || cv.isArray()) walk(cv, cp);
            }
        }
    };
    walk(root, QString());
    CHECK(hits == 1, QString("搜索「深层」命中 1 处（实际 %1，路径 %2）").arg(hits).arg(hitPath));

    qInfo() << "═══ HAR 检测 ═══";
    QFile hf("/tmp/jv-test.har");
    hf.open(QIODevice::ReadOnly);
    const QJsonDocument hdoc = QJsonDocument::fromJson(hf.readAll(), &err);
    JsonTreeModel hmodel;
    const QJsonValue hroot = hdoc.object();
    hmodel.setJson(hroot);
    CHECK(hmodel.isHAR(), "HAR 识别成功");
    CHECK(hmodel.harRequestCount() == 4, QString("HAR 请求数 4（实际 %1）").arg(hmodel.harRequestCount()));
    CHECK(hmodel.harDomainCount() == 1, QString("HAR 域名 1（实际 %1）").arg(hmodel.harDomainCount()));

    qInfo() << "═══ 大文件（1.4MB / 5万键） ═══";
    QElapsedTimer timer;
    timer.start();
    QFile lf("/tmp/jv-large.json");
    lf.open(QIODevice::ReadOnly);
    const QJsonDocument ldoc = QJsonDocument::fromJson(lf.readAll(), &err);
    const qint64 parseMs = timer.elapsed();
    CHECK(err.error == QJsonParseError::NoError, QString("大文件解析成功（%1 ms）").arg(parseMs));
    timer.restart();
    JsonTreeModel lmodel;
    lmodel.setJson(ldoc.isArray() ? QJsonValue(ldoc.array()) : QJsonValue(ldoc.object()));
    CHECK(lmodel.rowCount(QModelIndex()) == 1001, QString("根节点 5 万键按 MaxChildren=1000 截断为 1001 行（实际 %1）").arg(lmodel.rowCount(QModelIndex())));
    CHECK(lmodel.totalNodes() == 50001, QString("总节点 50001（实际 %1）").arg(lmodel.totalNodes()));

    qInfo() << "═══ 结果 ═══";
    if (failures == 0) qInfo() << "全部通过 ✅";
    else qInfo() << QString("%1 项失败 ❌").arg(failures);
    return failures == 0 ? 0 : 1;
}
