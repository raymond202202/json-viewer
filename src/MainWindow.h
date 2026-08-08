#pragma once

#include <QJsonValue>
#include <QMainWindow>
#include <QStringList>

class QCheckBox;
class QElapsedTimer;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSplitter;
class QTimer;
class QTreeView;
class JsonTreeDelegate;
class JsonTreeModel;

// 主窗口：工具栏 + 左编辑面板 + 右树形面板 + 状态栏 + 边缘折叠按钮
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onInputChanged();
    void parseAndRender();
    void formatJson();
    void compressJson();
    void openFileDialog();
    void clearAll();
    void expandAll();
    void collapseAll();
    void toggleTheme();
    void toggleLeftPanel();
    void toggleRightPanel();
    void onTreeClicked(const QModelIndex &index);
    void onTreeContextMenu(const QPoint &pos);
    void toggleSearch();
    void closeSearch();
    void doSearch();
    void searchPrev();
    void searchNext();
    void toggleSearchKeysOnly();
    void copyValue();
    void copyKeyValue();
    void copyBlock();
    void copyPath();

private:
    void buildUi();
    void applyTheme(const QString &theme);
    void loadJsonText(const QString &text, const QString &fileName, bool largeFile);
    void parseDirect(const QString &text, const QString &fileName);
    void updateStats();
    void updateStatus(const QString &cls, const QString &icon, const QString &text);
    void highlightInTextEdit(const QString &path, const QString &keyName);
    void showToast(const QString &msg);
    void loadFile(const QString &path);
    void setSearchBarVisible(bool visible);
    void positionEdgeButtons();
    void setEmptyState(bool empty);
    void collectSearchHits(const QJsonValue &v, const QString &path, const QString &term,
                           bool keysOnly, QList<QPair<QString, QString>> &out) const;
    static QString valueTextOf(const QJsonValue &v);
    static QJsonDocument docFromValue(const QJsonValue &v);
    static int findMatchingBracketEnd(const QString &text, int openPos);
    QModelIndex currentIndex() const;

    bool m_leftCollapsed = false;
    bool m_rightCollapsed = false;

    // UI
    QSplitter *m_splitter = nullptr;
    QWidget *m_leftPanel = nullptr;
    QWidget *m_rightPanel = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QTreeView *m_tree = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusText = nullptr;
    QLabel *m_fileNameTag = nullptr;
    QLabel *m_stats = nullptr;
    QPushButton *m_themeBtn = nullptr;
    QPushButton *m_keyOnlyBtn = nullptr;
    QPushButton *m_leftEdgeBtn = nullptr;
    QPushButton *m_rightEdgeBtn = nullptr;
    QWidget *m_searchBar = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QLabel *m_searchInfo = nullptr;
    QWidget *m_dropOverlay = nullptr;
    QWidget *m_loadingOverlay = nullptr;
    QLabel *m_toast = nullptr;
    QTimer *m_toastTimer = nullptr;

    JsonTreeModel *m_model = nullptr;
    JsonTreeDelegate *m_delegate = nullptr;

    QJsonValue m_jsonData;
    bool m_hasJson = false;
    QString m_currentFileName;
    QStringList m_searchResults;   // 匹配路径
    int m_searchIndex = -1;
    QString m_searchTerm;
    bool m_searchKeysOnly = false;
    QString m_activePath;
    QString m_theme = "light";
    QTimer *m_debounceTimer = nullptr;
    int m_dragCounter = 0;

    static constexpr int LargeFileThreshold = 50000;
    static constexpr int MaxSearchResults = 200;
};
