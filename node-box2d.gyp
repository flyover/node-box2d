{
    'variables': {
        'OS%': 'ios',
        'NODE_PATH': '../../../node',
    },
    'xcode_settings': {
        'ALWAYS_SEARCH_USER_PATHS': 'NO',
        'USE_HEADERMAP': 'NO',
    },
    'conditions': [
        [ 'OS=="ios"', {
            'xcode_settings': {
                'SDKROOT': 'iphoneos',
                'ARCHS': '$(ARCHS_STANDARD)',
                'TARGETED_DEVICE_FAMILY': '1,2',
                'CODE_SIGN_IDENTITY': 'iPhone Developer',
            }
        }],
        [ 'OS=="osx"', {
            'xcode_settings': {
                'SDKROOT': 'macosx',
                'ARCHS': '$(ARCHS_STANDARD_32_64_BIT)',
            }
        }],
    ],
    'target_defaults': {
        'defines': [
            '__POSIX__',
            '_LARGEFILE_SOURCE',
            '_LARGEFILE64_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_DARWIN_USE_64_BIT_INODE=1',
        ],
        'configurations': {
            'Debug': {
                'defines': [ '_DEBUG', 'DEBUG=1' ],
                'cflags': [
                    '-g',
                    '-O0',
                    '-fno-strict-aliasing'
                    '-fwrapv'
                ],
            },
            'Release': {
                'defines': [ 'NDEBUG=1' ],
                'cflags': [
                    '-O3',
                    '-fstrict-aliasing',
                    '-fomit-frame-pointer',
                    '-fdata-sections',
                    '-ffunction-sections',
                ],
            },
        },
        'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',         # -std=c99
            'GCC_CW_ASM_SYNTAX': 'NO',                # No -fasm-blocks
            'GCC_DYNAMIC_NO_PIC': 'NO',               # No -mdynamic-no-pic (Equivalent to -fPIC)
            'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',        # -fno-exceptions
            'GCC_ENABLE_CPP_RTTI': 'NO',              # -fno-rtti
            'GCC_ENABLE_PASCAL_STRINGS': 'NO',        # No -mpascal-strings
            'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',  # -fvisibility-inlines-hidden
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',      # -fvisibility=hidden
            'GCC_THREADSAFE_STATICS': 'NO',           # -fno-threadsafe-statics
            'GCC_WARN_NON_VIRTUAL_DESTRUCTOR': 'YES', # -Wnon-virtual-dtor
            'PREBINDING': 'NO',                       # No -Wl,-prebind
            'WARNING_CFLAGS': [
                '-Wall',
                '-Wendif-labels',
                '-W',
                '-Wno-unused-parameter',
            ],
            'OTHER_CFLAGS[arch=armv7]': [ '-marm' ],
            'OTHER_CFLAGS[arch=armv7s]': [ '-marm' ],
            'OTHER_CFLAGS[arch=arm64]': [ '-marm' ],
        },
    },
    'targets': [
        {
            'target_name': 'libnode-box2d-<(OS)',
            'type': 'static_library',
			'variables': {
				'BOX2D_PATH': 'box2d/Box2D'
			},
            'defines': [
                'NODE_WANT_INTERNALS=1',
            ],
            'include_dirs': [
                '.',
                '<!(node -e "require(\'nan\')")',
                '<(NODE_PATH)/src',
                '<(NODE_PATH)/deps/uv/include',
                '<(NODE_PATH)/deps/v8/include',
                '<(NODE_PATH)/deps/debugger-agent/include',
                '<(NODE_PATH)/deps/cares/include',
                '<(BOX2D_PATH)',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    '.',
                    '<!(node -e "require(\'nan\')")',
                ]
            },
            'dependencies': [
            ],
            'sources': [
				'node-box2d.h',
				'node-box2d.cc',
				'<(BOX2D_PATH)/Box2D/Collision/b2BroadPhase.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2CollideCircle.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2CollideEdge.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2CollidePolygon.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2Collision.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2Distance.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2DynamicTree.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/b2TimeOfImpact.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/Shapes/b2ChainShape.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/Shapes/b2CircleShape.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/Shapes/b2EdgeShape.cpp',
				'<(BOX2D_PATH)/Box2D/Collision/Shapes/b2PolygonShape.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2BlockAllocator.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2Draw.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2FreeList.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2Math.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2Settings.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2StackAllocator.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2Stat.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2Timer.cpp',
				'<(BOX2D_PATH)/Box2D/Common/b2TrackedBlock.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2Body.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2ContactManager.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2Fixture.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2Island.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2World.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/b2WorldCallbacks.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2CircleContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2Contact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2ContactSolver.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Contacts/b2PolygonContact.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2DistanceJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2FrictionJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2GearJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2Joint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2MotorJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2MouseJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2PulleyJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2RopeJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2WeldJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Dynamics/Joints/b2WheelJoint.cpp',
				'<(BOX2D_PATH)/Box2D/Particle/b2Particle.cpp',
				'<(BOX2D_PATH)/Box2D/Particle/b2ParticleAssembly.cpp',
				'<(BOX2D_PATH)/Box2D/Particle/b2ParticleGroup.cpp',
				'<(BOX2D_PATH)/Box2D/Particle/b2ParticleSystem.cpp',
				'<(BOX2D_PATH)/Box2D/Particle/b2VoronoiDiagram.cpp',
				'<(BOX2D_PATH)/Box2D/Rope/b2Rope.cpp'
            ],
        },
    ],
}
