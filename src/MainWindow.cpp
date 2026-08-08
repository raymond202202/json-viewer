#include "MainWindow.h"

#include "JsonTreeDelegate.h"
#include "JsonTreeModel.h"

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QTextCursor>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_debounceTimer(new QTimer(this))
    , m_toastTimer(new QTimer(this))
{
    setWindowTitle("JSON 阅读器");
    resize(1200, 800);
    setMinimumSize(800, 500);

    buildUi();

    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(400);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::parseAndRender);

    m_toastTimer->setSingleShot(true);
    m_toastTimer->setInterval(1600);
    connect(m_toastTimer, &QTimer::timeout, this, [this] { m_toast->hide(); });

    // 主题持久化
    QSettings s;
    applyTheme(s.value("theme", "light").toString());

    // 快捷键
    new QShortcut(QKeySequence("Ctrl+O"), this, SLOT(openFileDialog()));
    new QShortcut(QKeySequence("Ctrl+F"), this, SLOT(toggleSearch()));
    new QShortcut(QKeySequence("Ctrl+B"), this, SLOT(toggleLeftPanel()));
    new QShortcut(QKeySequence("Ctrl+Enter"), this, SLOT(formatJson()));
    new QShortcut(QKeySequence("Ctrl+Shift+C"), this, SLOT(compressJson()));
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(closeSearch()));

    // 启动示例数据（对齐原版 v1.1.0）
    const QJsonObject features{
        {"fileImport", "支持打开 .json / .har / .txt / .jsonc 文件（Ctrl+O）"},
        {"dragDrop", "拖拽文件到窗口直接打开"},
        {"search", "搜索键名或值，自动高亮定位（Ctrl+F）"},
        {"harAnalysis", "HAR 文件识别与请求统计"},
        {"format", "一键格式化（Ctrl+Enter）"},
        {"compress", "一键压缩（Ctrl+Shift+C）"},
        {"theme", "浅色 / 深色主题切换"},
    };
    const QJsonObject shortcuts{
        {"openFile", "Ctrl + O"},
        {"search", "Ctrl + F"},
        {"format", "Ctrl + Enter"},
        {"compress", "Ctrl + Shift + C"},
        {"expandAll", "全部展开"},
        {"collapseAll", "全部收拢"},
    };
    const QJsonObject defaultData{
        {"app", "JSON 阅读器"},
        {"version", QStringLiteral(APP_VERSION)},
        {"description", "树形结构查看 JSON / HAR 文件的桌面工具（Qt 版）"},
        {"features", features},
        {"shortcuts", shortcuts},
    };
    m_input->setPlainText(QString::fromUtf8(QJsonDocument(defaultData).toJson(QJsonDocument::Indented)));
    parseAndRender();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName("centralWidget");
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── 工具栏 ──
    auto *toolbar = new QWidget(central);
    toolbar->setObjectName("toolbar");
    auto *tb = new QHBoxLayout(toolbar);
    tb->setContentsMargins(12, 8, 12, 8);
    tb->setSpacing(8);

    auto *title = new QLabel("{ } JSON 阅读器", toolbar);
    title->setObjectName("appTitle");
    tb->addWidget(title);
    tb->addStretch();

    auto makeBtn = [&](const QString &text, const char *slot) {
        auto *b = new QPushButton(text, toolbar);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, SIGNAL(clicked()), this, slot);
        tb->addWidget(b);
        return b;
    };
    auto makeDivider = [&] {
        auto *d = new QFrame(toolbar);
        d->setObjectName("divider");
        d->setFixedWidth(1);
        d->setFixedHeight(20);
        tb->addWidget(d);
    };

    makeBtn("美观", SLOT(formatJson()));
    makeBtn("压缩", SLOT(compressJson()));
    makeBtn("📂 打开文件", SLOT(openFileDialog()));
    makeBtn("清空", SLOT(clearAll()));
    makeDivider();
    makeBtn("全部展开", SLOT(expandAll()));
    makeBtn("全部收拢", SLOT(collapseAll()));
    makeDivider();
    m_themeBtn = new QPushButton("☀️", toolbar);
    m_themeBtn->setObjectName("themeBtn");
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setToolTip("切换主题");
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    tb->addWidget(m_themeBtn);

    rootLayout->addWidget(toolbar);

    // ── 中央分割区 ──
    m_splitter = new QSplitter(Qt::Horizontal, central);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);

    // 左面板：输入
    m_leftPanel = new QWidget(m_splitter);
    m_leftPanel->setObjectName("leftPanel");
    auto *leftLay = new QVBoxLayout(m_leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);

    auto *leftHeader = new QWidget(m_leftPanel);
    leftHeader->setObjectName("panelHeader");
    auto *lh = new QHBoxLayout(leftHeader);
    lh->setContentsMargins(12, 6, 12, 6);
    auto *leftLabel = new QLabel("📝 输入", leftHeader);
    leftLabel->setObjectName("panelLabel");
    lh->addWidget(leftLabel);
    leftLay->addWidget(leftHeader);

    auto *inputWrap = new QWidget(m_leftPanel);
    auto *iw = new QVBoxLayout(inputWrap);
    iw->setContentsMargins(0, 0, 0, 0);
    iw->setSpacing(0);
    m_input = new QPlainTextEdit(inputWrap);
    m_input->setObjectName("inputEdit");
    m_input->setPlaceholderText("在此粘贴 JSON 内容，或拖拽文件到窗口...");
    connect(m_input, &QPlainTextEdit::textChanged, this, &MainWindow::onInputChanged);
    iw->addWidget(m_input);
    m_loadingOverlay = new QWidget(inputWrap);
    m_loadingOverlay->setObjectName("loadingOverlay");
    auto *lo = new QVBoxLayout(m_loadingOverlay);
    lo->setAlignment(Qt::AlignCenter);
    auto *spinner = new QLabel("⟳", m_loadingOverlay);
    spinner->setObjectName("spinner");
    spinner->setAlignment(Qt::AlignCenter);
    auto *loadingText = new QLabel("导入中…", m_loadingOverlay);
    loadingText->setObjectName("loadingText");
    loadingText->setAlignment(Qt::AlignCenter);
    lo->addWidget(spinner);
    lo->addWidget(loadingText);
    m_loadingOverlay->hide();
    leftLay->addWidget(inputWrap);

    auto *leftStatus = new QWidget(m_leftPanel);
    leftStatus->setObjectName("leftStatusBar");
    auto *ls = new QHBoxLayout(leftStatus);
    ls->setContentsMargins(12, 4, 12, 4);
    m_statusIcon = new QLabel(leftStatus);
    m_statusText = new QLabel("等待输入...", leftStatus);
    m_statusText->setObjectName("statusText");
    m_fileNameTag = new QLabel(leftStatus);
    m_fileNameTag->setObjectName("fileNameTag");
    m_fileNameTag->hide();
    ls->addWidget(m_statusIcon);
    ls->addWidget(m_statusText);
    ls->addStretch();
    ls->addWidget(m_fileNameTag);
    leftLay->addWidget(leftStatus);

    // 右面板：树
    m_rightPanel = new QWidget(m_splitter);
    m_rightPanel->setObjectName("rightPanel");
    auto *rightLay = new QVBoxLayout(m_rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    auto *rightHeader = new QWidget(m_rightPanel);
    rightHeader->setObjectName("panelHeader");
    auto *rh = new QHBoxLayout(rightHeader);
    rh->setContentsMargins(12, 6, 12, 6);
    auto *rightLabel = new QLabel("📋 解析结果", rightHeader);
    rightLabel->setObjectName("panelLabel");
    m_stats = new QLabel(rightHeader);
    m_stats->setObjectName("stats");
    rh->addWidget(rightLabel);
    rh->addStretch();
    rh->addWidget(m_stats);
    rightLay->addWidget(rightHeader);

    // 搜索栏（默认隐藏）
    m_searchBar = new QWidget(m_rightPanel);
    m_searchBar->setObjectName("searchBar");
    auto *sb = new QHBoxLayout(m_searchBar);
    sb->setContentsMargins(12, 5, 12, 5);
    sb->setSpacing(6);
    m_searchInput = new QLineEdit(m_searchBar);
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText("搜索键或值...");
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::doSearch);
    m_keyOnlyBtn = new QPushButton("键", m_searchBar);
    m_keyOnlyBtn->setObjectName("navBtn");
    m_keyOnlyBtn->setCheckable(true);
    m_keyOnlyBtn->setToolTip("只匹配键名");
    connect(m_keyOnlyBtn, &QPushButton::toggled, this, &MainWindow::toggleSearchKeysOnly);
    m_searchInfo = new QLabel(m_searchBar);
    m_searchInfo->setObjectName("searchInfo");
    m_searchInfo->setMinimumWidth(52);
    m_searchInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *prevBtn = new QPushButton("▲", m_searchBar);
    prevBtn->setObjectName("navBtn");
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::searchPrev);
    auto *nextBtn = new QPushButton("▼", m_searchBar);
    nextBtn->setObjectName("navBtn");
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::searchNext);
    auto *closeBtn = new QPushButton("✕", m_searchBar);
    closeBtn->setObjectName("navBtn");
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::closeSearch);
    sb->addWidget(m_searchInput);
    sb->addWidget(m_keyOnlyBtn);
    sb->addWidget(m_searchInfo);
    sb->addWidget(prevBtn);
    sb->addWidget(nextBtn);
    sb->addWidget(closeBtn);
    m_searchBar->hide();
    rightLay->addWidget(m_searchBar);

    // 树 + 空状态
    auto *treeStack = new QStackedWidget(m_rightPanel);
    m_tree = new QTreeView(treeStack);
    m_tree->setObjectName("treeView");
    m_tree->setHeaderHidden(true);
    m_tree->setAnimated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setExpandsOnDoubleClick(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_model = new JsonTreeModel(this);
    m_delegate = new JsonTreeDelegate(m_tree);
    m_tree->setModel(m_model);
    m_tree->setItemDelegate(m_delegate);
    connect(m_tree, &QTreeView::clicked, this, &MainWindow::onTreeClicked);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &MainWindow::onTreeContextMenu);
    auto *emptyLabel = new QLabel("← 在左侧粘贴 JSON，自动解析展示", treeStack);
    emptyLabel->setObjectName("emptyState");
    emptyLabel->setAlignment(Qt::AlignCenter);
    treeStack->addWidget(m_tree);
    treeStack->addWidget(emptyLabel);
    treeStack->setCurrentWidget(emptyLabel);
    rightLay->addWidget(treeStack);

    m_splitter->addWidget(m_leftPanel);
    m_splitter->addWidget(m_rightPanel);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({600, 600});
    // 关键：splitter 吃掉全部多余空间，否则 toolbar 会被 QVBoxLayout 平均拉伸（宽额头 bug）
    rootLayout->addWidget(m_splitter, 1);

    // ── 边缘折叠按钮 ──
    m_leftEdgeBtn = new QPushButton("◀", central);
    m_leftEdgeBtn->setObjectName("edgeBtn");
    m_leftEdgeBtn->setCursor(Qt::PointingHandCursor);
    m_leftEdgeBtn->setToolTip("收起左侧");
    connect(m_leftEdgeBtn, &QPushButton::clicked, this, &MainWindow::toggleLeftPanel);
    m_rightEdgeBtn = new QPushButton("▶", central);
    m_rightEdgeBtn->setObjectName("edgeBtn");
    m_rightEdgeBtn->setCursor(Qt::PointingHandCursor);
    m_rightEdgeBtn->setToolTip("收起右侧");
    connect(m_rightEdgeBtn, &QPushButton::clicked, this, &MainWindow::toggleRightPanel);

    // ── 拖放遮罩 ──
    m_dropOverlay = new QWidget(central);
    m_dropOverlay->setObjectName("dropOverlay");
    auto *doLay = new QVBoxLayout(m_dropOverlay);
    doLay->setAlignment(Qt::AlignCenter);
    auto *dropIcon = new QLabel("📂", m_dropOverlay);
    dropIcon->setObjectName("dropIcon");
    dropIcon->setAlignment(Qt::AlignCenter);
    auto *dropTitle = new QLabel("释放文件以导入", m_dropOverlay);
    dropTitle->setObjectName("dropTitle");
    dropTitle->setAlignment(Qt::AlignCenter);
    auto *dropSub = new QLabel("支持 .json / .har / .txt / .jsonc", m_dropOverlay);
    dropSub->setObjectName("dropSub");
    dropSub->setAlignment(Qt::AlignCenter);
    doLay->addWidget(dropIcon);
    doLay->addWidget(dropTitle);
    doLay->addWidget(dropSub);
    m_dropOverlay->hide();

    // ── 复制 toast ──
    m_toast = new QLabel(central);
    m_toast->setObjectName("toast");
    m_toast->setAlignment(Qt::AlignCenter);
    m_toast->hide();

    setCentralWidget(central);
    setAcceptDrops(true);
}

// ── 主题 ──
void MainWindow::applyTheme(const QString &theme)
{
    m_theme = theme;
    QFile qssFile(theme == "dark" ? ":/styles/dark.qss" : ":/styles/light.qss");
    if (qssFile.open(QIODevice::ReadOnly))
        qApp->setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    m_themeBtn->setText(theme == "dark" ? "🌙" : "☀️");
    m_delegate->setThemeColors(theme == "dark");
    m_tree->viewport()->update();
    QSettings s;
    s.setValue("theme", theme);
}

void MainWindow::toggleTheme()
{
    applyTheme(m_theme == "dark" ? "light" : "dark");
}

// ── 解析 ──
void MainWindow::onInputChanged()
{
    m_debounceTimer->start();
}

void MainWindow::parseAndRender()
{
    const QString raw = m_input->toPlainText().trimmed();
    if (raw.isEmpty()) {
        m_hasJson = false;
        m_jsonData = QJsonValue();
        m_model->clearData();
        m_activePath.clear();
        m_delegate->setActivePath(QString());
        setEmptyState(true);
        updateStatus("", "", "等待输入...");
        m_stats->clear();
        return;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || doc.isNull()) {
        m_hasJson = false;
        m_jsonData = QJsonValue();
        m_model->clearData();
        m_activePath.clear();
        m_delegate->setActivePath(QString());
        setEmptyState(true);
        updateStatus("error", "❌", "JSON 格式错误");
        m_stats->clear();
        return;
    }
    m_jsonData = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    m_hasJson = true;
    m_activePath.clear();
    m_delegate->setActivePath(QString());
    m_model->setJson(m_jsonData);
    setEmptyState(false);
    updateStatus("ok", "✅", "JSON 解析成功");
    updateStats();
    // 大文件自动全折叠（>500 节点），否则全展开（对齐原版）
    if (m_model->totalNodes() > 500) m_tree->collapseAll();
    else m_tree->expandAll();
    m_tree->scrollToTop();
}

void MainWindow::parseDirect(const QString &text, const QString &fileName)
{
    Q_UNUSED(fileName);
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || doc.isNull()) {
        m_hasJson = false;
        m_jsonData = QJsonValue();
        m_model->clearData();
        m_activePath.clear();
        setEmptyState(true);
        updateStatus("error", "❌", "JSON 格式错误");
        m_stats->clear();
        return;
    }
    m_jsonData = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    m_hasJson = true;
    m_activePath.clear();
    m_delegate->setActivePath(QString());
    m_model->setJson(m_jsonData);
    setEmptyState(false);
    updateStatus("ok", "✅", "大文件加载完成");
    updateStats();
    if (m_model->totalNodes() > 500) m_tree->collapseAll();
    else m_tree->expandAll();
    m_tree->scrollToTop();
}

void MainWindow::updateStats()
{
    if (!m_hasJson) {
        m_stats->clear();
        return;
    }
    if (m_model->isHAR()) {
        QStringList parts;
        parts << QString("🐙 HAR · %1 个请求").arg(m_model->harRequestCount());
        if (m_model->harDomainCount())
            parts << QString("%1 个域名").arg(m_model->harDomainCount());
        if (!m_model->harMethodSummary().isEmpty())
            parts << m_model->harMethodSummary();
        m_stats->setText(parts.join(" · "));
    } else {
        m_stats->setText(QString("%1 个键 · %2 个值 · %3 个节点")
                             .arg(m_model->keyCount())
                             .arg(m_model->leafCount())
                             .arg(m_model->totalNodes()));
    }
}

void MainWindow::updateStatus(const QString &cls, const QString &icon, const QString &text)
{
    m_statusIcon->setText(icon);
    m_statusText->setText(text);
    if (cls == "ok") m_statusText->setProperty("state", "ok");
    else if (cls == "error") m_statusText->setProperty("state", "error");
    else m_statusText->setProperty("state", "");
    m_statusText->style()->unpolish(m_statusText);
    m_statusText->style()->polish(m_statusText);
}

void MainWindow::setEmptyState(bool empty)
{
    if (auto *stack = qobject_cast<QStackedWidget *>(m_tree->parentWidget()))
        stack->setCurrentWidget(empty ? stack->widget(1) : stack->widget(0));
}

// ── 工具栏 ──
void MainWindow::formatJson()
{
    if (!m_hasJson) { parseAndRender(); return; }
    m_input->setPlainText(QString::fromUtf8(docFromValue(m_jsonData).toJson(QJsonDocument::Indented)));
    m_activePath.clear();
    m_delegate->setActivePath(QString());
    if (m_model->totalNodes() > 500) m_tree->collapseAll();
    else m_tree->expandAll();
    updateStats();
}

void MainWindow::compressJson()
{
    if (!m_hasJson) { parseAndRender(); return; }
    m_input->setPlainText(QString::fromUtf8(docFromValue(m_jsonData).toJson(QJsonDocument::Compact)));
    m_activePath.clear();
    m_delegate->setActivePath(QString());
    if (m_model->totalNodes() > 500) m_tree->collapseAll();
    else m_tree->expandAll();
    updateStats();
}

void MainWindow::openFileDialog()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "打开文件", QString(),
        "JSON 文件 (*.json *.har *.jsonc);;文本文件 (*.txt);;JavaScript (*.js);;所有文件 (*)");
    if (!path.isEmpty()) loadFile(path);
}

void MainWindow::loadFile(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists()) return;
    if (m_rightCollapsed) toggleRightPanel();
    m_loadingOverlay->raise();
    m_loadingOverlay->resize(m_input->size());
    m_loadingOverlay->show();
    m_input->clear();
    m_stats->clear();
    m_currentFileName = fi.fileName();
    m_fileNameTag->setText("📄 " + fi.fileName());
    m_fileNameTag->show();
    updateStatus("ok", "⏳", "正在读取文件...");

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        updateStatus("error", "❌", "文件读取失败");
        m_loadingOverlay->hide();
        return;
    }
    QString text = QString::fromUtf8(f.readAll());
    // 处理 BOM
    if (text.startsWith(QChar(0xFEFF))) text = text.mid(1);

    const bool isLarge = fi.size() > LargeFileThreshold;
    if (isLarge) {
        // 取消 m_input->clear() 触发的待处理 debounce，避免覆盖大文件状态
        m_debounceTimer->stop();
        m_input->setPlaceholderText(QString("文件已加载（%1 MB），在右侧查看解析结果")
                                        .arg(fi.size() / 1024.0 / 1024.0, 0, 'f', 1));
        parseDirect(text, fi.fileName());
        updateStatus("ok", "🔒", "大文件加载完成 · 仅查看模式");
    } else {
        m_input->setPlaceholderText("在此粘贴 JSON 内容，或拖拽文件到窗口...");
        m_input->setPlainText(text);
        parseAndRender();
        updateStatus("ok", "✅", "JSON 解析成功");
    }
    m_loadingOverlay->hide();
}

void MainWindow::openFile(const QString &path)
{
    if (!path.isEmpty()) loadFile(path);
}

void MainWindow::clearAll()
{
    m_input->clear();
    m_hasJson = false;
    m_jsonData = QJsonValue();
    m_model->clearData();
    m_activePath.clear();
    m_delegate->setActivePath(QString());
    setEmptyState(true);
    updateStatus("", "", "等待输入...");
    m_stats->clear();
    m_currentFileName.clear();
    m_fileNameTag->hide();
    closeSearch();
}

void MainWindow::expandAll()
{
    if (!m_hasJson) return;
    m_tree->expandAll();
}

void MainWindow::collapseAll()
{
    if (!m_hasJson) return;
    m_tree->collapseAll();
}

// ── 面板折叠（互斥；对齐老版：左折叠 width:0 / 右折叠 display:none，均彻底收起）──
void MainWindow::toggleLeftPanel()
{
    if (m_rightCollapsed) return;
    m_leftCollapsed = !m_leftCollapsed;
    if (m_leftCollapsed) {
        m_leftPanel->setVisible(false);   // 彻底收起（QSplitter 自动把空间让给右侧）
        m_leftEdgeBtn->setText("▶");
        m_leftEdgeBtn->setToolTip("展开左侧");
        m_rightEdgeBtn->setEnabled(false);
    } else {
        m_leftPanel->setVisible(true);
        m_leftEdgeBtn->setText("◀");
        m_leftEdgeBtn->setToolTip("收起左侧");
        m_rightEdgeBtn->setEnabled(true);
        m_splitter->setSizes({1, 1});     // 恢复等宽
    }
    positionEdgeButtons();
}

void MainWindow::toggleRightPanel()
{
    if (m_leftCollapsed) return;
    m_rightCollapsed = !m_rightCollapsed;
    if (m_rightCollapsed) {
        m_rightPanel->setVisible(false);  // 彻底收起
        m_rightEdgeBtn->setText("◀");
        m_rightEdgeBtn->setToolTip("展开右侧");
        m_leftEdgeBtn->setEnabled(false);
    } else {
        m_rightPanel->setVisible(true);
        m_rightEdgeBtn->setText("▶");
        m_rightEdgeBtn->setToolTip("收起右侧");
        m_leftEdgeBtn->setEnabled(true);
        m_splitter->setSizes({1, 1});     // 恢复等宽
    }
    positionEdgeButtons();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionEdgeButtons();
    if (m_dropOverlay) m_dropOverlay->setGeometry(centralWidget()->rect());
    if (m_toast) {
        m_toast->adjustSize();
        m_toast->move((width() - m_toast->width()) / 2, height() - 60);
    }
    if (m_loadingOverlay && m_loadingOverlay->isVisible())
        m_loadingOverlay->resize(m_input->size());
}

void MainWindow::positionEdgeButtons()
{
    if (!m_splitter) return;
    const int handleX = m_splitter->handleWidth() > 0 ? 0 : 0;
    const int btnW = 20, btnH = 56;
    const int cy = height() / 2 - btnH / 2;
    // 左按钮：splitter 左边缘（即 0）
    m_leftEdgeBtn->setGeometry(handleX, cy, btnW, btnH);
    // 右按钮：窗口右边缘
    m_rightEdgeBtn->setGeometry(width() - btnW, cy, btnW, btnH);
    m_leftEdgeBtn->raise();
    m_rightEdgeBtn->raise();
}

// ── 树点击 → 左栏联动定位 ──
void MainWindow::onTreeClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    const QString path = index.data(JsonTreeModel::PathRole).toString();
    const QString key = index.data(JsonTreeModel::KeyNameRole).toString();
    if (m_activePath == path) {
        m_activePath.clear();
        m_delegate->setActivePath(QString());
        m_tree->viewport()->update();
        QTextCursor cur = m_input->textCursor();
        cur.setPosition(0);
        m_input->setTextCursor(cur);
        return;
    }
    m_activePath = path;
    m_delegate->setActivePath(path);
    m_tree->viewport()->update();
    highlightInTextEdit(path, key);
}

void MainWindow::highlightInTextEdit(const QString &path, const QString &keyName)
{
    const QString text = m_input->toPlainText();
    if (path.isEmpty() || path == "root") {
        m_input->setFocus();
        QTextCursor cur = m_input->textCursor();
        cur.setPosition(0);
        cur.setPosition(text.length(), QTextCursor::KeepAnchor);
        m_input->setTextCursor(cur);
        return;
    }
    const QString searchKey = "\"" + keyName + "\"";
    int keyIdx = text.indexOf(searchKey);
    if (keyIdx < 0) return;

    int pos = keyIdx + searchKey.length();
    while (pos < text.length() && text[pos].isSpace()) pos++;
    if (pos >= text.length() || text[pos] != ':') return;
    pos++;
    while (pos < text.length() && text[pos].isSpace()) pos++;
    if (pos >= text.length()) return;

    const QChar ch = text[pos];
    int endPos = pos;
    if (ch == '"') {
        endPos = pos + 1;
        while (endPos < text.length()) {
            if (text[endPos] == '\\') { endPos += 2; continue; }
            if (text[endPos] == '"') { endPos++; break; }
            endPos++;
        }
    } else if (ch == '{' || ch == '[') {
        endPos = findMatchingBracketEnd(text, pos);
    } else if (ch == 't') {
        endPos = pos + 4;
    } else if (ch == 'f') {
        endPos = pos + 5;
    } else if (ch == 'n') {
        endPos = pos + 4;
    } else if (ch == '-' || ch.isDigit()) {
        endPos = pos;
        while (endPos < text.length() &&
               (text[endPos].isDigit() || text[endPos] == '-' || text[endPos] == '+' ||
                text[endPos] == '.' || text[endPos] == 'e' || text[endPos] == 'E'))
            endPos++;
    } else {
        return;
    }

    m_input->setFocus();
    QTextCursor cur = m_input->textCursor();
    cur.setPosition(keyIdx);
    cur.setPosition(endPos, QTextCursor::KeepAnchor);
    m_input->setTextCursor(cur);

    QList<QTextEdit::ExtraSelection> sel;
    QTextEdit::ExtraSelection es;
    es.cursor = cur;
    es.format.setBackground(m_theme == "dark" ? QColor(137, 180, 250, 60) : QColor(255, 200, 80, 90));
    sel << es;
    m_input->setExtraSelections(sel);

    m_input->ensureCursorVisible();
}

int MainWindow::findMatchingBracketEnd(const QString &text, int openPos)
{
    const QChar open = text[openPos];
    const QChar close = open == '{' ? '}' : ']';
    int depth = 1;
    int i = openPos + 1;
    while (i < text.length() && depth > 0) {
        const QChar c = text[i];
        if (c == '"') {
            i++;
            while (i < text.length()) {
                if (text[i] == '\\') { i += 2; continue; }
                if (text[i] == '"') break;
                i++;
            }
        } else if (c == open) {
            depth++;
        } else if (c == close) {
            depth--;
        }
        i++;
    }
    return i;
}

// ── 右键菜单 ──
QModelIndex MainWindow::currentIndex() const
{
    return m_tree->currentIndex();
}

void MainWindow::onTreeContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_tree->indexAt(pos);
    if (!idx.isValid()) return;
    m_tree->setCurrentIndex(idx);
    const int type = idx.data(JsonTreeModel::TypeRole).toInt();
    QMenu menu(this);
    menu.addAction("复制值", this, &MainWindow::copyValue);
    menu.addAction("复制键值对", this, &MainWindow::copyKeyValue);
    if (type == JsonTreeModel::Object || type == JsonTreeModel::Array)
        menu.addAction("复制整块", this, &MainWindow::copyBlock);
    menu.addAction("复制路径", this, &MainWindow::copyPath);
    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void MainWindow::copyValue()
{
    const QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;
    const int type = idx.data(JsonTreeModel::TypeRole).toInt();
    if (type == JsonTreeModel::Object || type == JsonTreeModel::Array) {
        copyBlock();
        return;
    }
    const QString val = idx.data(JsonTreeModel::ValueTextRole).toString();
    QGuiApplication::clipboard()->setText(val);
    showToast("已复制 ✓");
}

void MainWindow::copyKeyValue()
{
    const QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;
    const QString key = idx.data(JsonTreeModel::KeyRole).toString();
    const int type = idx.data(JsonTreeModel::TypeRole).toInt();
    QString out;
    if (!key.isEmpty() && type != JsonTreeModel::Truncated)
        out = "\"" + key + "\": ";
    if (type == JsonTreeModel::Object || type == JsonTreeModel::Array) {
        const QJsonValue v = idx.data(JsonTreeModel::RawRole).value<QJsonValue>();
        out += QString::fromUtf8(docFromValue(v).toJson(QJsonDocument::Compact));
    } else {
        out += idx.data(JsonTreeModel::ValueTextRole).toString();
    }
    QGuiApplication::clipboard()->setText(out);
    showToast("已复制 ✓");
}

void MainWindow::copyBlock()
{
    const QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;
    const QJsonValue v = idx.data(JsonTreeModel::RawRole).value<QJsonValue>();
    QGuiApplication::clipboard()->setText(
        QString::fromUtf8(docFromValue(v).toJson(QJsonDocument::Indented)));
    showToast("已复制 ✓");
}

void MainWindow::copyPath()
{
    const QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;
    QGuiApplication::clipboard()->setText(idx.data(JsonTreeModel::PathRole).toString());
    showToast("已复制 ✓");
}

void MainWindow::showToast(const QString &msg)
{
    m_toast->setText(msg);
    m_toast->adjustSize();
    m_toast->move((width() - m_toast->width()) / 2, height() - 60);
    m_toast->raise();
    m_toast->show();
    m_toastTimer->start();
}

// ── 搜索 ──
void MainWindow::toggleSearch()
{
    if (m_searchBar->isHidden()) {
        if (m_rightCollapsed) toggleRightPanel();
        setSearchBarVisible(true);
        m_searchInput->setFocus();
    } else {
        closeSearch();
    }
}

void MainWindow::closeSearch()
{
    if (m_searchBar->isHidden()) return;
    setSearchBarVisible(false);
    m_searchInput->clear();
    m_searchTerm.clear();
    m_searchResults.clear();
    m_searchIndex = -1;
    m_searchInfo->clear();
    m_delegate->setMatchedPath(QString());
    m_delegate->setSearchActive(false);
    m_tree->viewport()->update();
}

void MainWindow::setSearchBarVisible(bool visible)
{
    m_searchBar->setVisible(visible);
}

void MainWindow::doSearch()
{
    m_searchTerm = m_searchInput->text().trimmed();
    m_searchResults.clear();
    m_searchIndex = -1;
    m_delegate->setMatchedPath(QString());
    m_delegate->setSearchActive(false);

    if (m_searchTerm.isEmpty() || !m_hasJson) {
        m_searchInfo->clear();
        m_tree->viewport()->update();
        return;
    }
    if (m_rightCollapsed) toggleRightPanel();

    // 收集匹配路径（键/值，大小写不敏感；仅键名开关）
    QList<QPair<QString, QString>> hits; // path, display
    collectSearchHits(m_jsonData, QString(), m_searchTerm, m_searchKeysOnly, hits);
    for (const auto &h : hits) {
        if (m_searchResults.size() >= MaxSearchResults) break;
        // 截断区（>1000 子项被隐藏）无法通过模型定位，跳过 —— 对齐老版"只搜已渲染内容"
        if (!m_model->indexForPath(h.first).isValid()) continue;
        m_searchResults << h.first;
    }

    if (m_searchResults.isEmpty()) {
        m_searchInfo->setText("无结果");
        m_tree->viewport()->update();
        return;
    }

    // 智能展开：逐级展开所有匹配路径的祖先
    for (const QString &path : m_searchResults) {
        const QModelIndex idx = m_model->indexForPath(path);
        if (!idx.isValid()) continue;
        QModelIndex cur = idx;
        QList<QModelIndex> chain;
        while (cur.isValid()) {
            chain.prepend(cur);
            cur = cur.parent();
        }
        for (const QModelIndex &ci : chain) m_tree->expand(ci);
    }

    m_searchIndex = 0;
    m_delegate->setSearchActive(true);
    m_delegate->setMatchedPath(m_searchResults.value(0));
    m_tree->scrollTo(m_model->indexForPath(m_searchResults.value(0)),
                     QAbstractItemView::PositionAtCenter);
    m_tree->viewport()->update();
    m_searchInfo->setText(QString("%1 / %2%3")
                              .arg(m_searchIndex + 1)
                              .arg(m_searchResults.size())
                              .arg(m_searchResults.size() >= MaxSearchResults ? " (已截断)" : ""));
}

void MainWindow::collectSearchHits(const QJsonValue &v, const QString &path,
                                   const QString &term, bool keysOnly,
                                   QList<QPair<QString, QString>> &out) const
{
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it) {
            const QString k = it.key();
            const QJsonValue cv = it.value();
            const QString cp = path.isEmpty() ? k : path + "." + k;
            if (k.contains(term, Qt::CaseInsensitive))
                out.append({cp, k});
            else if (!keysOnly && valueTextOf(cv).contains(term, Qt::CaseInsensitive))
                out.append({cp, valueTextOf(cv)});
            if (cv.isObject() || cv.isArray())
                collectSearchHits(cv, cp, term, keysOnly, out);
        }
    } else if (v.isArray()) {
        const QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i) {
            const QJsonValue cv = a.at(i);
            const QString cp = path.isEmpty() ? QString::number(i) : path + "." + QString::number(i);
            if (!keysOnly && valueTextOf(cv).contains(term, Qt::CaseInsensitive))
                out.append({cp, valueTextOf(cv)});
            if (cv.isObject() || cv.isArray())
                collectSearchHits(cv, cp, term, keysOnly, out);
        }
    }
}

QString MainWindow::valueTextOf(const QJsonValue &v)
{
    switch (v.type()) {
    case QJsonValue::String:
        return JsonTreeModel::escapeJsonString(v.toString());
    case QJsonValue::Double:
        return QString::number(v.toDouble(), 'g', 17);
    case QJsonValue::Bool:
        return v.toBool() ? "true" : "false";
    case QJsonValue::Null:
        return "null";
    default:
        return QString();
    }
}

QJsonDocument MainWindow::docFromValue(const QJsonValue &v)
{
    if (v.isObject()) return QJsonDocument(v.toObject());
    if (v.isArray()) return QJsonDocument(v.toArray());
    return QJsonDocument(QJsonObject{{"value", v}});
}

void MainWindow::searchNext()
{
    if (m_searchResults.isEmpty()) return;
    m_searchIndex = (m_searchIndex + 1) % m_searchResults.size();
    const QString path = m_searchResults.value(m_searchIndex);
    m_delegate->setMatchedPath(path);
    m_tree->scrollTo(m_model->indexForPath(path), QAbstractItemView::PositionAtCenter);
    m_tree->viewport()->update();
    m_searchInfo->setText(QString("%1 / %2").arg(m_searchIndex + 1).arg(m_searchResults.size()));
}

void MainWindow::searchPrev()
{
    if (m_searchResults.isEmpty()) return;
    m_searchIndex = (m_searchIndex - 1 + m_searchResults.size()) % m_searchResults.size();
    const QString path = m_searchResults.value(m_searchIndex);
    m_delegate->setMatchedPath(path);
    m_tree->scrollTo(m_model->indexForPath(path), QAbstractItemView::PositionAtCenter);
    m_tree->viewport()->update();
    m_searchInfo->setText(QString("%1 / %2").arg(m_searchIndex + 1).arg(m_searchResults.size()));
}

void MainWindow::toggleSearchKeysOnly()
{
    m_searchKeysOnly = m_keyOnlyBtn->isChecked();
    doSearch();
}

// ── 拖放 ──
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_dragCounter++;
        m_dropOverlay->setGeometry(centralWidget()->rect());
        m_dropOverlay->raise();
        m_dropOverlay->show();
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragCounter--;
    if (m_dragCounter <= 0) {
        m_dragCounter = 0;
        m_dropOverlay->hide();
    }
    event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    m_dragCounter = 0;
    m_dropOverlay->hide();
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString path = urls.first().toLocalFile();
    if (path.isEmpty()) return;
    const QFileInfo fi(path);
    const QString ext = fi.suffix().toLower();
    const QStringList allowed{"json", "har", "txt", "js", "jsonc"};
    if (allowed.contains(ext)) {
        loadFile(path);
    } else {
        updateStatus("error", "❌", "不支持的文件类型：." + ext);
    }
    event->acceptProposedAction();
}
