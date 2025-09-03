#QT -= core
QT -= gui

TARGET = simPovRay
TEMPLATE = lib
DEFINES -= UNICODE
CONFIG += shared plugin

win32 {
    DEFINES += WIN_SIM
    DEFINES += NOMINMAX
    DEFINES += _CRT_SECURE_NO_WARNINGS
}

macx {
    INCLUDEPATH += "/usr/local/include"
    DEFINES += MAC_SIM
}

unix:!macx {
    DEFINES += LIN_SIM
}


*-msvc* {
        QMAKE_CXXFLAGS += /O2
        QMAKE_CXXFLAGS += /Zc:strictStrings-
}
*-g++* {
        QMAKE_CXXFLAGS += -O3
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -fvisibility=hidden
        QMAKE_CXXFLAGS += -Wno-unused-parameter
        QMAKE_CXXFLAGS += -Wno-strict-aliasing
        QMAKE_CXXFLAGS += -Wno-empty-body
        QMAKE_CXXFLAGS += -Wno-write-strings
        QMAKE_CXXFLAGS += -Wno-multichar

        QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
        QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
        QMAKE_CXXFLAGS += -Wno-narrowing

        QMAKE_CFLAGS += -O3
        QMAKE_CFLAGS += -Wall
        QMAKE_CFLAGS += -Wno-strict-aliasing
        QMAKE_CFLAGS += -Wno-unused-parameter
        QMAKE_CFLAGS += -Wno-unused-but-set-variable
        QMAKE_CFLAGS += -Wno-unused-local-typedefs
}

INCLUDEPATH += "../include"
INCLUDEPATH += sourceCode
INCLUDEPATH += external
INCLUDEPATH += external/povray
INCLUDEPATH += external/povray/base
INCLUDEPATH += external/povray/frontend

SOURCES += \
    sourceCode/simPovRay.cpp \
    ../include/simLib/simLib.cpp \
    ../include/simMath/mathFuncs.cpp \
    ../include/simMath/3Vector.cpp \
    ../include/simMath/4Vector.cpp \
    ../include/simMath/7Vector.cpp \
    ../include/simMath/3X3Matrix.cpp \
    ../include/simMath/4X4Matrix.cpp \
    ../include/simMath/mXnMatrix.cpp \
    external/povray/base/fileinputoutput.cpp \
    external/povray/base/povms.cpp \
    external/povray/base/povmscpp.cpp \
    external/povray/base/processoptions.cpp \
    external/povray/base/stringutilities.cpp \
    external/povray/base/textstream.cpp \
    external/povray/frontend/defaultplatformbase.cpp \
    external/povray/frontend/processrenderoptions.cpp \
    external/povray/atmosph.cpp \
    external/povray/bbox.cpp \
    external/povray/bcyl.cpp \
    external/povray/benchmark.cpp \
    external/povray/bezier.cpp \
    external/povray/blob.cpp \
    external/povray/boxes.cpp \
    external/povray/bsphere.cpp \
    external/povray/camera.cpp \
    external/povray/chi2.cpp \
    external/povray/colour.cpp \
    external/povray/colutils.cpp \
    external/povray/cones.cpp \
    external/povray/csg.cpp \
    external/povray/discs.cpp \
    external/povray/express.cpp \
    external/povray/fncode.cpp \
    external/povray/fnintern.cpp \
    external/povray/fnpovfpu.cpp \
    external/povray/fnsyntax.cpp \
    external/povray/fpmetric.cpp \
    external/povray/fractal.cpp \
    external/povray/function.cpp \
    external/povray/hcmplx.cpp \
    external/povray/hfield.cpp \
    external/povray/histogra.cpp \
    external/povray/image.cpp \
    external/povray/interior.cpp \
    external/povray/isosurf.cpp \
    external/povray/lathe.cpp \
    external/povray/lbuffer.cpp \
    external/povray/lightgrp.cpp \
    external/povray/lighting.cpp \
    external/povray/mathutil.cpp \
    external/povray/matrices.cpp \
    external/povray/media.cpp \
    external/povray/mesh.cpp \
    external/povray/normal.cpp \
    external/povray/objects.cpp \
    external/povray/octree.cpp \
    external/povray/parse.cpp \
    external/povray/parsestr.cpp \
    external/povray/parstxtr.cpp \
    external/povray/pattern.cpp \
    external/povray/photons.cpp \
    external/povray/pigment.cpp \
    external/povray/planes.cpp \
    external/povray/point.cpp \
    external/povray/poly.cpp \
    external/povray/polygon.cpp \
    external/povray/polysolv.cpp \
    external/povray/pov_mem.cpp \
    external/povray/pov_util.cpp \
    external/povray/povmsend.cpp \
    external/povray/povmsrec.cpp \
    external/povray/povray.cpp \
    external/povray/prism.cpp \
    external/povray/quadrics.cpp \
    external/povray/quatern.cpp \
    external/povray/rad_data.cpp \
    external/povray/radiosit.cpp \
    external/povray/ray.cpp \
    external/povray/rendctrl.cpp \
    external/povray/render.cpp \
    external/povray/sor.cpp \
    external/povray/spheres.cpp \
    external/povray/sphsweep.cpp \
    external/povray/splines.cpp \
    external/povray/statspov.cpp \
    external/povray/super.cpp \
    external/povray/texture.cpp \
    external/povray/tokenize.cpp \
    external/povray/torus.cpp \
    external/povray/triangle.cpp \
    external/povray/truetype.cpp \
    external/povray/vbuffer.cpp \
    external/povray/vlbuffer.cpp \
    external/povray/warps.cpp \
    external/povray/renderio.cpp \
    external/povray/rgbafile.cpp \
    external/povray/optout.cpp \
    external/povray/userio.cpp \
    external/povray/base/textstreambuffer.cpp

HEADERS +=\
    sourceCode/simPovRay.h \
    ../include/simLib/simLib.h \
    ../include/simMath/mathFuncs.h \
    ../include/simMath/mathDefines.h \
    ../include/simMath/3Vector.h \
    ../include/simMath/4Vector.h \
    ../include/simMath/7Vector.h \
    ../include/simMath/3X3Matrix.h \
    ../include/simMath/4X4Matrix.h \
    ../include/simMath/mXnMatrix.h \
    external/povray/base/configbase.h \
    external/povray/base/fileinputoutput.h \
    external/povray/base/platformbase.h \
    external/povray/base/pointer.h \
    external/povray/base/pov_err.h \
    external/povray/base/povms.h \
    external/povray/base/povmscpp.h \
    external/povray/base/povmsgid.h \
    external/povray/base/processoptions.h \
    external/povray/base/stringutilities.h \
    external/povray/base/textstream.h \
    external/povray/frontend/configfrontend.h \
    external/povray/frontend/defaultplatformbase.h \
    external/povray/frontend/defaultrenderfrontend.h \
    external/povray/frontend/messageoutput.h \
    external/povray/frontend/processrenderoptions.h \
    external/povray/frontend/renderfrontend.h \
    external/povray/atmosph.h \
    external/povray/bbox.h \
    external/povray/bcyl.h \
    external/povray/benchmark.h \
    external/povray/bezier.h \
    external/povray/blob.h \
    external/povray/boxes.h \
    external/povray/bsphere.h \
    external/povray/camera.h \
    external/povray/chi2.h \
    external/povray/colour.h \
    external/povray/colutils.h \
    external/povray/cones.h \
    external/povray/csg.h \
    external/povray/discs.h \
    external/povray/express.h \
    external/povray/fncode.h \
    external/povray/fnintern.h \
    external/povray/fnpovfpu.h \
    external/povray/fnsyntax.h \
    external/povray/fpmetric.h \
    external/povray/fractal.h \
    external/povray/frame.h \
    external/povray/function.h \
    external/povray/hcmplx.h \
    external/povray/hfield.h \
    external/povray/histogra.h \
    external/povray/image.h \
    external/povray/interior.h \
    external/povray/isosurf.h \
    external/povray/lathe.h \
    external/povray/lbuffer.h \
    external/povray/lightgrp.h \
    external/povray/lighting.h \
    external/povray/mathutil.h \
    external/povray/matrices.h \
    external/povray/media.h \
    external/povray/mesh.h \
    external/povray/normal.h \
    external/povray/objects.h \
    external/povray/octree.h \
    external/povray/parse.h \
    external/povray/parsestr.h \
    external/povray/parstxtr.h \
    external/povray/pattern.h \
    external/povray/photons.h \
    external/povray/pigment.h \
    external/povray/planes.h \
    external/povray/point.h \
    external/povray/poly.h \
    external/povray/polygon.h \
    external/povray/polysolv.h \
    external/povray/pov_mem.h \
    external/povray/pov_util.h \
    external/povray/povmsend.h \
    external/povray/povmsrec.h \
    external/povray/povray.h \
    external/povray/prism.h \
    external/povray/quadrics.h \
    external/povray/quatern.h \
    external/povray/radiosit.h \
    external/povray/ray.h \
    external/povray/rendctrl.h \
    external/povray/render.h \
    external/povray/sor.h \
    external/povray/spheres.h \
    external/povray/sphsweep.h \
    external/povray/splines.h \
    external/povray/statspov.h \
    external/povray/super.h \
    external/povray/texture.h \
    external/povray/tokenize.h \
    external/povray/torus.h \
    external/povray/triangle.h \
    external/povray/truetype.h \
    external/povray/userio.h \
    external/povray/vbuffer.h \
    external/povray/vector.h \
    external/povray/vlbuffer.h \
    external/povray/warps.h \
    external/povray/renderio.h \
    external/povray/rgbafile.h \
    external/povray/base/config.h \
    external/povray/base/textstreambuffer.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

