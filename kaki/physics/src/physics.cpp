//
// Created by felix on 27/03/2022.
//

#include <physics.h>

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <kaki/transform.h>

namespace kaki {

    struct PhysicsContext {
        btBroadphaseInterface* broadphase;
        btDefaultCollisionConfiguration* collisionConfiguration;
        btCollisionDispatcher* collisionDispatcher;
        btSequentialImpulseConstraintSolver* solver;
        btDiscreteDynamicsWorld* world;
    };

    struct PhysicsBody {
        btCollisionShape* shape;
        btRigidBody* body;
    };

    struct MotionState : btMotionState {
        flecs::entity entity;

        explicit MotionState(flecs::entity e) : entity(e) {
        };

        void getWorldTransform(btTransform& worldTrans) const override {
            const auto* t = entity.get<Transform>();
            worldTrans.setOrigin(btVector3(t->position.x, t->position.y, t->position.z));
            worldTrans.setRotation(btQuaternion(t->orientation.x, t->orientation.y, t->orientation.z, t->orientation.w));
        }

        void setWorldTransform(const btTransform& worldTrans) override {
            auto* t = entity.get_mut<Transform>();
            t->position.x = worldTrans.getOrigin().x();
            t->position.y = worldTrans.getOrigin().y();
            t->position.z = worldTrans.getOrigin().z();
            t->orientation.x = worldTrans.getRotation().x();
            t->orientation.y = worldTrans.getRotation().y();
            t->orientation.z = worldTrans.getRotation().z();
            t->orientation.w = worldTrans.getRotation().w();
            entity.modified<Transform>();
        };
    };

    void setBody(flecs::entity e, const RigidBody& r, const Transform& transform) {

        btCollisionShape* shape;
        if (r.sphere) {
            shape = new btSphereShape(r.extent.x);
        } else {
            shape = new btBoxShape(btVector3(r.extent.x, r.extent.y, r.extent.z) / 2.0f);
        }
        auto orientation = btQuaternion(transform.orientation.x, transform.orientation.y, transform.orientation.z, transform.orientation.w);
        auto position = btVector3(transform.position.x, transform.position.y, transform.position.z);

        auto* motionState = new MotionState(e);

        btVector3 bodyInertia;
        shape->calculateLocalInertia(r.mass, bodyInertia);

        btRigidBody::btRigidBodyConstructionInfo bodyInfo (r.mass, motionState, shape, bodyInertia);

        auto body = new btRigidBody(bodyInfo);

        //body->setSleepingThresholds(0.002, body->getAngularSleepingThreshold());

        body->setRestitution(0.3);

        e.set<PhysicsBody>(PhysicsBody {
            shape,
            body
        });

        e.world().get<PhysicsContext>()->world->addRigidBody(body);
    }

    kaki::Physics::Physics(flecs::world &world) {
        world.module<kaki::Physics>();
        world.component<PhysicsContext>();

        world.system<const RigidBody, const Transform>()
                .term<PhysicsBody>().oper(flecs::Not).inout(flecs::Out)
                .kind(flecs::PreUpdate)
                .each(setBody);

        world.system<Impulse>().kind(flecs::OnUpdate)
        .term<PhysicsBody>().inout(flecs::InOut).set(flecs::Nothing)
        .each([](flecs::entity e, const Impulse& imp) {
            auto pb = imp.target.get<PhysicsBody>();
            if (pb) {
                pb->body->applyCentralImpulse(btVector3(imp.impulse.x, imp.impulse.y, imp.impulse.z));
            }
            e.destruct();
        });

        world.system<PhysicsContext>().kind(flecs::OnUpdate)
        .each([](flecs::entity e, PhysicsContext& ctx) {
            ctx.world->stepSimulation(e.world().delta_time());
        });

        world.system<PhysicsBody>().kind(flecs::OnStore).each([](PhysicsBody& b) {
            btTransform t;
            b.body->getMotionState()->getWorldTransform(t);
            auto o = t.getOrigin();
        });
    }

    void initPhysics(flecs::world &world) {
        PhysicsContext ctx {};

        ctx.broadphase = new btDbvtBroadphase();
        ctx.collisionConfiguration = new btDefaultCollisionConfiguration();
        ctx.collisionDispatcher = new btCollisionDispatcher(ctx.collisionConfiguration);
        ctx.solver = new btSequentialImpulseConstraintSolver();

        ctx.world = new btDiscreteDynamicsWorld( ctx.collisionDispatcher, ctx.broadphase, ctx.solver, ctx.collisionConfiguration);

        ctx.world->setGravity(btVector3(0, -9.81, 0));


        world.set<PhysicsContext>(ctx);
    }
}