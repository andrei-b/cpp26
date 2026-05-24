#include <QApplication>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QLineF>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTimer>

#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>

#include "signal_slot.hpp"

// A plain non-QObject controller. It emits your custom signals.
// Several signals are connected to QGraphicsItem methods by STRING name.
class AnimationBus {
public:
    Signal<qreal>  rotateItem;
    Signal<qreal>  scaleItem;
    Signal<QPointF> moveItem;
    Signal<QRectF> changeEllipseRect;
    Signal<QLineF> changeLine;
    Signal<QString> changeLabel;
    Signal<bool> setLineVisible;

    void tick() {
        ++frame_;
        const qreal t = frame_ / 30.0;

        rotateItem.emit_direct(std::fmod(frame_ * 3.0, 360.0));
        scaleItem.emit_direct(1.0 + 0.25 * std::sin(t));
        moveItem.emit_direct(QPointF(30.0 * std::sin(t), 18.0 * std::cos(t * 0.7)));

        const qreal r = 35.0 + 12.0 * std::sin(t * 1.4);
        changeEllipseRect.emit_direct(QRectF(-r, -r, 2.0 * r, 2.0 * r));

        changeLine.emit_direct(QLineF(-130.0, 90.0, 130.0 * std::cos(t), 90.0 + 55.0 * std::sin(t)));
        changeLabel.emit_direct(QString("frame %1: slots are Qt item methods resolved by string").arg(frame_));
        setLineVisible.emit_direct((frame_ / 45) % 2 == 0);
    }

private:
    int frame_ = 0;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(-260, -180, 520, 360);

    auto* rect = scene.addRect(QRectF(-50, -35, 100, 70), QPen(Qt::darkBlue, 2), QBrush(QColor("#9ecbff")));
    rect->setTransformOriginPoint(0, 0);
    rect->setFlag(QGraphicsItem::ItemIsMovable);

    auto* ellipse = scene.addEllipse(QRectF(-35, -35, 70, 70), QPen(Qt::darkRed, 2), QBrush(QColor("#ffc8c8")));
    ellipse->setPos(150, -45);

    auto* line = scene.addLine(QLineF(-130, 90, 130, 90), QPen(Qt::darkGreen, 4));

    auto* label = scene.addText("starting...");
    label->setPos(-230, -155);
    label->setDefaultTextColor(Qt::black);

    AnimationBus bus;
    std::vector<ConnectionToken> connections;

    // --- String-based slot connections using your reflection path ---
    // These items do NOT need QObject or Q_OBJECT for these connections.

    // Method declared on QGraphicsItem. Use a QGraphicsItem base reference so the
    // reflection table is created for the class where setRotation/setScale/setPos live.
    QGraphicsItem& rectAsItem = *rect;
    connections.push_back(bus.rotateItem.connect_reflected_direct(rectAsItem, "setRotation"));
    connections.push_back(bus.scaleItem.connect_reflected_direct(rectAsItem, "setScale"));
    connections.push_back(bus.moveItem.connect_reflected_direct(rectAsItem, "setPos"));

    // Method declared on QGraphicsEllipseItem.
    connections.push_back(bus.changeEllipseRect.connect_reflected_direct(*ellipse, "setRect"));

    // Method declared on QGraphicsLineItem.
    connections.push_back(bus.changeLine.connect_reflected_direct(*line, "setLine"));

    // Method declared on QGraphicsItem.
    QGraphicsItem& lineAsItem = *line;
    connections.push_back(bus.setLineVisible.connect_reflected_direct(lineAsItem, "setVisible"));

    // Method declared on QGraphicsTextItem.
    connections.push_back(bus.changeLabel.connect_reflected_direct(*label, "setPlainText"));

    std::cout << "Connected " << connections.size() << " reflected string-name slots.\n";
    std::cout << "Examples: \"setRotation\", \"setScale\", \"setPos\", \"setRect\", \"setLine\", \"setVisible\", \"setPlainText\"\n";

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&bus] { bus.tick(); });
    timer.start(33);

    QGraphicsView view(&scene);
    view.setWindowTitle("QGraphicsScene + custom string-name signal/slot connections");
    view.setRenderHint(QPainter::Antialiasing);
    view.resize(760, 520);
    view.show();

    return app.exec();
}
