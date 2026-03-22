#include <QApplication>
#include <QPainter>
#include <QPushButton>
#include <QAbstractButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>
#include <QPen>
#include <QBrush>
#include <QPolygon>

#include <algorithm>
#include <string>
#include <tuple>
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
static QStringList reflectedMethodLines()
{
    QStringList lines;
    const auto table = get_method_table<T>();

    for (const auto& [name, entry] : table) {
        lines << formatMethodLine(name, entry);
    }

    lines.removeDuplicates();
    std::sort(lines.begin(), lines.end());
    return lines;
}

template<typename T>
static UmlClassInfo makeClassInfo()
{
    UmlClassInfo info;
    info.className = QString::fromStdString(std::string(type_name<T>()));
    info.methods = reflectedMethodLines<T>();
    return info;
}

class UmlDiagramWidget : public QWidget
{
public:
    explicit UmlDiagramWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_classes.push_back(makeClassInfo<QPushButton>());
        m_classes.push_back(makeClassInfo<QAbstractButton>());
        m_classes.push_back(makeClassInfo<QWidget>());
        m_classes.push_back(makeClassInfo<QObject>());

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

        drawInheritanceArrows(p);

        for (const auto& cls : m_classes) {
            drawClassBox(p, cls);
        }
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
        constexpr int classSpacing = 80;
        constexpr int minWidth = 700;

        int y = outerMargin;
        int maxWidth = 0;

        for (auto& cls : m_classes) {
            int width = titleFm.horizontalAdvance(cls.className);

            for (const auto& line : cls.methods) {
                width = std::max(width, bodyFm.horizontalAdvance(line));
            }

            width += 2 * innerPad + 20;
            width = std::max(width, minWidth);

            const int titleHeight = titleFm.height() + 2 * innerPad;

            const int methodsHeight =
                static_cast<int>(cls.methods.size()) * (bodyFm.height() + 4) + 2 * innerPad;

            const int height = titleHeight + methodsHeight;

            cls.rect = QRect(outerMargin, y, width, height);
            y += height + classSpacing;
            maxWidth = std::max(maxWidth, width);
        }

        for (auto& cls : m_classes) {
            cls.rect.moveLeft(outerMargin + (maxWidth - cls.rect.width()) / 2);
        }

        m_totalSize = QSize(maxWidth + 2 * outerMargin, y);
    }

    void drawInheritanceArrows(QPainter& p)
    {
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(Qt::white);

        for (int i = 0; i + 1 < static_cast<int>(m_classes.size()); ++i) {
            const QRect& child = m_classes[i].rect;
            const QRect& base  = m_classes[i + 1].rect;

            const QPoint start(child.center().x(), child.bottom());
            const QPoint end(base.center().x(), base.top());

            p.drawLine(start, end);

            const int aw = 16;
            const int ah = 14;

            QPolygon triangle;
            triangle << QPoint(end.x(), end.y())
                     << QPoint(end.x() - aw / 2, end.y() + ah)
                     << QPoint(end.x() + aw / 2, end.y() + ah);

            p.drawPolygon(triangle);
        }
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
    std::vector<UmlClassInfo> m_classes;
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
    window.setWindowTitle("UML Inheritance Diagram - QPushButton");
    window.resize(1200, 900);

    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(scroll);

    window.show();
    return app.exec();
}