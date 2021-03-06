LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := node-box2d
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/box2d/Box2D
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(subst \,/,$(shell cd $(LOCAL_PATH) && node -e "require('nan')"))
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_SRC_FILES := node-box2d.cc
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2BroadPhase.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2CollideCircle.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2CollideEdge.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2CollidePolygon.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2Collision.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2Distance.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2DynamicTree.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/b2TimeOfImpact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/Shapes/b2ChainShape.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/Shapes/b2CircleShape.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/Shapes/b2EdgeShape.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Collision/Shapes/b2PolygonShape.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2BlockAllocator.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2Draw.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2FreeList.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2Math.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2Settings.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2StackAllocator.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2Stat.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2Timer.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Common/b2TrackedBlock.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2Body.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2ContactManager.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2Fixture.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2Island.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2World.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/b2WorldCallbacks.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2CircleContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2Contact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2ContactSolver.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Contacts/b2PolygonContact.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2DistanceJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2FrictionJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2GearJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2Joint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2MotorJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2MouseJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2PulleyJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2RopeJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2WeldJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Dynamics/Joints/b2WheelJoint.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2Particle.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2ParticleAssembly.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2ParticleGroup.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2ParticleSystem.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2VoronoiDiagram.cpp
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Rope/b2Rope.cpp
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += '-DLIQUIDFUN_SIMD_NEON' -mfloat-abi=softfp -mfpu=neon
LOCAL_SRC_FILES += box2d/Box2D/Box2D/Particle/b2ParticleAssembly.neon.s
endif
LOCAL_STATIC_LIBRARIES := node
include $(BUILD_STATIC_LIBRARY)

