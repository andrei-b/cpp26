#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QPen>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <cmath>
#include <deque>
#include <memory>
#include <stdexcept>
#include <vector>

#include "signal_slot.hpp"

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class BroadPhaseLayerInterface : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        return inLayer == Layers::NON_MOVING ? BroadPhaseLayers::NON_MOVING : BroadPhaseLayers::MOVING;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        return inLayer == BroadPhaseLayers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
#endif
};

class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        if (inLayer1 == Layers::NON_MOVING)
            return inLayer2 == BroadPhaseLayers::MOVING;
        return true;
    }
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        if (inObject1 == Layers::NON_MOVING)
            return inObject2 == Layers::MOVING;
        return true;
    }
};

class SceneBody {
public:
    SceneBody(QGraphicsScene& scene, QColor color, QString name) {
        body_ = scene.addEllipse(-18, -18, 36, 36, QPen(Qt::black, 2), QBrush(color));
        axis_ = scene.addLine(0, 0, 26, 0, QPen(Qt::black, 3));
        label_ = scene.addSimpleText(name);
        speed_ = scene.addSimpleText("0.0 m/s");
        label_->setBrush(QBrush(Qt::darkBlue));
        speed_->setBrush(QBrush(Qt::darkGray));
        setCenter(QPointF(0, 0));
    }

    // These are intentionally ordinary C++ methods. They are found by method_table.hpp
    // and connected by string using connect_reflected_direct(..., "methodName").
    void setCenter(QPointF center) {
        body_->setPos(center);
        axis_->setPos(center);
        label_->setPos(center + QPointF(-22, -42));
        speed_->setPos(center + QPointF(-30, 24));
    }

    void setAngleDegrees(double angle) {
        body_->setRotation(angle);
        axis_->setRotation(angle);
    }

    void setSpeedText(QString text) {
        speed_->setText(text);
    }

    void setSleeping(bool sleeping) {
        body_->setOpacity(sleeping ? 0.35 : 1.0);
        axis_->setOpacity(sleeping ? 0.35 : 1.0);
    }

private:
    QGraphicsEllipseItem* body_ = nullptr;
    QGraphicsLineItem* axis_ = nullptr;
    QGraphicsSimpleTextItem* label_ = nullptr;
    QGraphicsSimpleTextItem* speed_ = nullptr;
};

struct BodyVisual {
    JPH::BodyID id;
    std::unique_ptr<SceneBody> view;
    Signal<QPointF> positionChanged;
    Signal<double> angleChanged;
    Signal<QString> speedChanged;
    Signal<bool> sleepingChanged;
};

class JoltWorld {
public:
    JoltWorld() {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        job_system_ = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            std::max(1u, std::thread::hardware_concurrency() - 1)
        );

        physics_.Init(
            1024, 0, 1024, 1024,
            broad_phase_layer_interface_,
            object_vs_broadphase_filter_,
            object_pair_filter_
        );

        physics_.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
        createFloor();
    }

    ~JoltWorld() {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    BodyVisual& addBall(QGraphicsScene& scene, float x, float y, QColor color, QString name) {
        auto shape = new JPH::SphereShape(0.35f);
        JPH::BodyCreationSettings settings(
            shape,
            JPH::RVec3(x, y, 0.0f),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Layers::MOVING
        );
        settings.mRestitution = 0.65f;
        settings.mFriction = 0.35f;

        JPH::BodyInterface& bodies = physics_.GetBodyInterface();
        JPH::Body* body = bodies.CreateBody(settings);
        bodies.AddBody(body->GetID(), JPH::EActivation::Activate);

        BodyVisual visual;
        visual.id = body->GetID();
        visual.view = std::make_unique<SceneBody>(scene, color, name);

        visuals_.push_back(std::move(visual));
        return visuals_.back();
    }

    void kickBodies() {
        auto& bodies = physics_.GetBodyInterface();
        for (std::size_t i = 0; i < visuals_.size(); ++i) {
            const float impulse_x = (i % 2 == 0) ? 3.5f : -2.25f;
            bodies.AddImpulse(visuals_[i].id, JPH::Vec3(impulse_x, 5.0f, 0.0f));
        }
    }

    void step() {
        constexpr float dt = 1.0f / 60.0f;
        physics_.Update(dt, 1, temp_allocator_.get(), job_system_.get());

        auto& bodies = physics_.GetBodyInterface();
        for (BodyVisual& visual : visuals_) {
            const JPH::RVec3 p = bodies.GetPosition(visual.id);
            const JPH::Quat q = bodies.GetRotation(visual.id);
            const JPH::Vec3 v = bodies.GetLinearVelocity(visual.id);

            // Map Jolt meters to Qt pixels. Qt Y goes down, physics Y goes up.
            const QPointF scene_pos(p.GetX() * 90.0, 260.0 - p.GetY() * 90.0);
            const double angle_deg = -q.GetEulerAngles().GetZ() * 180.0 / 3.14159265358979323846;
            const double speed = std::sqrt(double(v.GetX() * v.GetX() + v.GetY() * v.GetY()));

            visual.positionChanged.emit_direct(scene_pos);
            visual.angleChanged.emit_direct(angle_deg);
            visual.speedChanged.emit_direct(QString::number(speed, 'f', 2) + " m/s");
            visual.sleepingChanged.emit_direct(!bodies.IsActive(visual.id));
        }
    }

private:
    void createFloor() {
        auto floor_shape = new JPH::BoxShape(JPH::Vec3(6.0f, 0.20f, 0.5f));
        JPH::BodyCreationSettings floor_settings(
            floor_shape,
            JPH::RVec3(0.0f, -0.25f, 0.0f),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING
        );
        JPH::BodyInterface& bodies = physics_.GetBodyInterface();
        JPH::Body* floor = bodies.CreateBody(floor_settings);
        bodies.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    }

    BroadPhaseLayerInterface broad_phase_layer_interface_;
    ObjectVsBroadPhaseFilter object_vs_broadphase_filter_;
    ObjectLayerPairFilter object_pair_filter_;

    JPH::PhysicsSystem physics_;
    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator_;
    std::unique_ptr<JPH::JobSystemThreadPool> job_system_;
    std::deque<BodyVisual> visuals_;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto* scene = new QGraphicsScene();
    scene->setSceneRect(-460, -80, 920, 420);
    scene->addRect(-540, 278, 1080, 18, QPen(Qt::darkGreen), QBrush(Qt::darkGreen));
    scene->addSimpleText("Jolt Physics -> custom Signal<T...> -> reflected string slots -> QGraphicsScene")
         ->setPos(-430, -60);

    auto* view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setMinimumSize(940, 520);

    auto* window = new QWidget();
    auto* layout = new QVBoxLayout(window);
    layout->addWidget(view);
    window->setWindowTitle("Jolt + Qt GraphicsScene + reflected signal slots");
    window->show();

    JoltWorld world;

    BodyVisual& red = world.addBall(*scene, -2.0f, 3.0f, QColor(240, 80, 80), "red");
    BodyVisual& blue = world.addBall(*scene, 1.0f, 5.0f, QColor(80, 120, 250), "blue");

    // The important lines: all of these use method names supplied as strings.
    red.positionChanged.connect_reflected_direct(*red.view, "setCenter");
    red.angleChanged.connect_reflected_direct(*red.view, "setAngleDegrees");
    red.speedChanged.connect_reflected_direct(*red.view, "setSpeedText");
    red.sleepingChanged.connect_reflected_direct(*red.view, "setSleeping");

    blue.positionChanged.connect_reflected_direct(*blue.view, "setCenter");
    blue.angleChanged.connect_reflected_direct(*blue.view, "setAngleDegrees");
    blue.speedChanged.connect_reflected_direct(*blue.view, "setSpeedText");
    blue.sleepingChanged.connect_reflected_direct(*blue.view, "setSleeping");

    world.kickBodies();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&world] { world.step(); });
    timer.start(16);

    return app.exec();
}
