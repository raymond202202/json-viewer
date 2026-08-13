// 高亮定位自测：验证按 path 逐级定位修复同名键 indexOf 只命中第一个的问题
#include "MainWindow.h"

#include <QApplication>
#include <QDebug>

static int failures = 0;
#define CHECK(cond, msg) \
    do { \
        const QString _m = msg; \
        if (cond) { fprintf(stderr, "  ✅ %s\n", _m.toUtf8().constData()); } \
        else { fprintf(stderr, "  ❌ FAIL: %s\n", _m.toUtf8().constData()); failures++; } \
    } while (0)
// 诊断输出实际值
#define SHOW(label, v) fprintf(stderr, "    [%s] => %s\n", label, QString(v).toUtf8().constData())

// 模拟 highlightInTextEdit 的逐级定位循环，返回 (keyStart, valStart)；失败返回 (-1,-1)
static QPair<int, int> locate(const QString &text, const QString &path)
{
    const QStringList segs = path.split('.', Qt::SkipEmptyParts);
    if (segs.isEmpty()) return {-1, -1};
    const int rootPos = MainWindow::skipJsonSpace(text, 0);
    if (rootPos >= text.length() || (text[rootPos] != '{' && text[rootPos] != '['))
        return {-1, -1};

    int scopeStart = rootPos;
    int scopeEnd = text.length();
    int finalKeyStart = -1, finalValStart = -1;

    for (int i = 0; i < segs.size(); ++i) {
        const QString &seg = segs[i];
        const bool isLast = (i == segs.size() - 1);
        bool isArrayIdx = true;
        for (const QChar &c : seg)
            if (!c.isDigit()) { isArrayIdx = false; break; }

        int keyStart = -1, valStart = -1;
        if (isArrayIdx)
            keyStart = MainWindow::findArrayElementInRange(text, scopeStart, scopeEnd, seg.toInt(), &valStart);
        else
            keyStart = MainWindow::findKeyInObjectRange(text, scopeStart, scopeEnd, seg, &valStart);
        if (keyStart < 0) return {-1, -1};

        if (isLast) { finalKeyStart = keyStart; finalValStart = valStart; break; }
        if (valStart >= text.length() || (text[valStart] != '{' && text[valStart] != '['))
            return {-1, -1};
        scopeStart = valStart;
        scopeEnd = MainWindow::findMatchingBracketEnd(text, valStart);
    }
    return {finalKeyStart, finalValStart};
}

// 提取值文本（keyStart 之后、valueEnd 之前的原文）
static QString valueTextAt(const QString &text, const QPair<int, int> &loc)
{
    if (loc.first < 0 || loc.second < 0) return QString();
    const QChar ch = text[loc.second];
    if (ch == '"') {
        int p = loc.second + 1;
        while (p < text.length()) {
            if (text[p] == '\\') { p += 2; continue; }
            if (text[p] == '"') return text.mid(loc.second, p - loc.second + 1);
            p++;
        }
        return QString();
    }
    if (ch == '{' || ch == '[') {
        const int end = MainWindow::findMatchingBracketEnd(text, loc.second);
        return text.mid(loc.second, end - loc.second);
    }
    int p = loc.second;
    while (p < text.length() && text[p] != ',' && text[p] != '}' && text[p] != ']') p++;
    return text.mid(loc.second, p - loc.second).trimmed(); // 裸值去尾部空白
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    fprintf(stderr, "═══ 场景1：同名 id 键（评估报告 Bug D 复现场景） ═══\n");
    const QString t1 = R"({
    "id": "Nl1Y660",
    "data": {
        "items": [
            {
                "created_by": {
                    "id": "hyremanchxMT7LwUqv3grxSwtFKetwwAa"
                }
            }
        ]
    }
})";
    {
        const auto loc = locate(t1, "data.items.0.created_by.id");
        CHECK(loc.first >= 0, "data.items.0.created_by.id 定位成功");
        const QString v = valueTextAt(t1, loc);
        CHECK(v == "\"hyremanchxMT7LwUqv3grxSwtFKetwwAa\"",
              QString("高亮到深层同名 id（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景2：根级 id 仍可定位 ═══\n");
    {
        const auto loc = locate(t1, "id");
        const QString v = valueTextAt(t1, loc);
        CHECK(v == "\"Nl1Y660\"", QString("根级 id 定位正确（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景3：值文本含同名 key 文本不误判 ═══\n");
    const QString t2 = R"({
    "a": "data.id",
    "data": {
        "id": 42
    }
})";
    {
        const auto loc = locate(t2, "data.id");
        const QString v = valueTextAt(t2, loc);
        SHOW("场景3 loc", QString("%1,%2").arg(loc.first).arg(loc.second));
        SHOW("场景3 val", v);
        CHECK(v == "42", QString("跳过同名值文本，定位到真正的 data.id（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景4：嵌套数组多元素定位 ═══\n");
    const QString t3 = R"({
    "list": [
        {"name": "first"},
        {"name": "second"},
        {"name": "third"}
    ]
})";
    {
        const auto loc = locate(t3, "list.2.name");
        const QString v = valueTextAt(t3, loc);
        CHECK(v == "\"third\"", QString("list[2].name 定位正确（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景5：深层整块容器定位 ═══\n");
    {
        const auto loc = locate(t3, "list.1");
        const QString v = valueTextAt(t3, loc);
        SHOW("场景5 loc", QString("%1,%2").arg(loc.first).arg(loc.second));
        SHOW("场景5 val", v);
        CHECK(v == "{\"name\": \"second\"}", QString("list[1] 整块定位（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景6：字符串转义值 ═══\n");
    const QString t4 = R"({
    "escaped": "say \"hi\" ok",
    "other": 1
})";
    {
        const auto loc = locate(t4, "escaped");
        const QString v = valueTextAt(t4, loc);
        CHECK(v == "\"say \\\"hi\\\" ok\"", QString("转义字符串完整高亮（实际 %1）").arg(v));
    }

    fprintf(stderr, "═══ 场景7：布尔/数字/null 值定位 ═══\n");
    const QString t5 = R"({
    "flag": true,
    "count": -12.5e2,
    "empty": null
})";
    {
        CHECK(valueTextAt(t5, locate(t5, "flag")) == "true", "布尔 true");
        CHECK(valueTextAt(t5, locate(t5, "count")) == "-12.5e2", "科学计数法数字");
    {
        const auto loc = locate(t5, "empty");
        const QString v = valueTextAt(t5, loc);
        SHOW("场景7-null loc", QString("%1,%2").arg(loc.first).arg(loc.second));
        SHOW("场景7-null val", v);
        CHECK(v == "null", "null");
    }
    }

    fprintf(stderr, "═══ 场景8：定位失败不崩溃（不存在键/截断区） ═══\n");
    {
        const auto loc = locate(t1, "no.such.key");
        CHECK(loc.first < 0, "不存在的路径返回失败");
    }

    fprintf(stderr, "═══ 结果 ═══\n");
    if (failures == 0) fprintf(stderr, "全部通过 ✅\n");
    else fprintf(stderr, "%d 项失败 ❌\n", failures);
    return failures == 0 ? 0 : 1;
}
