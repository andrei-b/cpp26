#include <QApplication>
#include <QPainter>
#include <QPushButton>
#include <QAbstractButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>

#include <algorithm>
#include <string>
#include <vector>

#include "method_table.hpp"

struct UmlClassInfo
{
    QString className;
    QStringList methods;
    QRect rect;
};

static QString visibilitySymbol(const MethodEntry& e)
{
    switch (e.access) {
    case AccessSpecifier::Private:   return "-";
    case AccessSpecifier::Protected: return "#";
    case AccessSpecifier::Public:    return "+";
    default:                         return "~";
    }
}

static QString joinArgTypes(const std::vector<std::string>& args)
{
    QStringList out;
    for (const auto& a : args) {
        out << QString::fromStdString(a);
    }
    return out.join(", ");
}

static QString formatMethodLine(std::string_view name, const MethodEntry& entry)
{
    QStringList mods;
    if (entry.is_static)   mods << "static";
    if (entry.is_virtual)  mods << "virtual";
    if (entry.is_const)    mods << "const";
    if (entry.is_noexcept) mods << "noexcept";

    QString suffix;
    if (!mods.isEmpty()) {
        suffix = " {" + mods.join(", ") + "}";
    }

    return QString("%1 %2(%3)%4")
        .arg(visibilitySymbol(entry),
             QString::fromStdString(std::string(name)),
             joinArgTypes(entry.arg_types),
             suffix);
}

template<typename T>
static void appendReflectedMethods(QStringList& out)
{
    const auto table = get_method_table<T>();
    for (const auto& [name, entry] : table) {
        out << formatMethodLine(name, entry);
    }
}

static QStringList collectAllQPushButtonMethods()
{
    QStringList methods;

    appendReflectedMethods<QPushButton>(methods);
    appendReflectedMethods<QAbstractButton>(methods);
    appendReflectedMethods<QWidget>(methods);
    appendReflectedMethods<QObject>(methods);

    methods.removeDuplicates();
    std::sort(methods.begin(), methods.end());
    return methods;
}

static UmlClassInfo makeQPushButtonInfo()
{
    UmlClassInfo info;
    info.className = "QPushButton";
    info.methods = collectAllQPushButtonMethods();
    return info;
}

class UmlDiagramWidget : public QWidget
{
public:
    explicit UmlDiagramWidget(QWidget* parent = nullptr)
        : QWidget(parent), m_info(makeQPushButtonInfo())
    {
        layoutDiagram();
        setMinimumSize(m_totalSize);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), Qt::white);
        drawClassBox(p, m_info);
    }

private:
    void layoutDiagram()
    {
        QFont titleFont = font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);

        QFont bodyFont = font();

        QFontMetrics titleFm(titleFont);
        QFontMetrics bodyFm(bodyFont);

        constexpr int outerMargin = 24;
        constexpr int innerPad = 10;
        constexpr int minWidth = 700;
        constexpr int minMethodArea = 120;

        int width = titleFm.horizontalAdvance(m_info.className);
        for (const auto& line : m_info.methods) {
            width = std::max(width, bodyFm.horizontalAdvance(line));
        }

        width += 2 * innerPad + 20;
        width = std::max(width, minWidth);

        const int titleHeight = titleFm.height() + 2 * innerPad;
        const int methodsHeight = std::max(
            minMethodArea,
            static_cast<int>(m_info.methods.size()) * (bodyFm.height() + 4) + 2 * innerPad
        );

        const int height = titleHeight + methodsHeight;

        m_info.rect = QRect(outerMargin, outerMargin, width, height);
        m_totalSize = QSize(width + 2 * outerMargin, height + 2 * outerMargin);
    }

    void drawClassBox(QPainter& p, const UmlClassInfo& cls)
    {
        QFont titleFont = font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);

        QFont bodyFont = font();

        QFontMetrics titleFm(titleFont);
        QFontMetrics bodyFm(bodyFont);

        constexpr int innerPad = 10;

        p.setPen(QPen(Qt::black, 2));
        p.setBrush(Qt::white);
        p.drawRect(cls.rect);

        const int titleHeight = titleFm.height() + 2 * innerPad;
        QRect titleRect(cls.rect.left(), cls.rect.top(), cls.rect.width(), titleHeight);
        QRect methodsRect(cls.rect.left(),
                          cls.rect.top() + titleHeight,
                          cls.rect.width(),
                          cls.rect.height() - titleHeight);

        p.drawLine(titleRect.bottomLeft(), titleRect.bottomRight());

        p.setFont(titleFont);
        p.drawText(titleRect, Qt::AlignCenter, cls.className);

        p.setFont(bodyFont);

        int y = methodsRect.top() + innerPad + bodyFm.ascent();
        for (const auto& line : cls.methods) {
            p.drawText(methodsRect.left() + innerPad, y, line);
            y += bodyFm.height() + 4;
        }
    }

private:
    UmlClassInfo m_info;
    QSize m_totalSize;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto* diagram = new UmlDiagramWidget;

    auto* scroll = new QScrollArea;
    scroll->setWidget(diagram);
    scroll->setWidgetResizable(false);

    QWidget window;
    window.setWindowTitle("UML - QPushButton methods from method_table");
    window.resize(1400, 900);

    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(scroll);

    window.show();
    return app.exec();
}