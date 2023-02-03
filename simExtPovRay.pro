#QT -= core
QT -= gui

TARGET = simExtPovRay
TEMPLATE = lib
DEFINES -= UNICODE
CONFIG += shared plugin

win32 {
    DEFINES += WIN_SIM
    DEFINES += NOMINMAX
    DEFINES += _CRT_SECURE_NO_WARNINGS
 #  remove Zc:strictStrings  in file Qt\QtX.Y.Z\...\msvc20XX_64\mkspecs\common\msvc-version.conf
}

macx {
    INCLUDEPATH += "/usr/local/include"
    DEFINES += MAC_SIM
}

unix:!macx {
    DEFINES += LIN_SIM
}


*-msvc* {
        QMAKE_CXXFLAGS += -O2
        QMAKE_CXXFLAGS += -W3
}
*-g++* {
        QMAKE_CXXFLAGS += -O3
        QMAKE_CXXFLAGS += -Wall
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
INCLUDEPATH += povray
INCLUDEPATH += povray/base
INCLUDEPATH += povray/frontend

SOURCES += \
    ../include/simLib.cpp \
    simExtPovRay.cpp \
    ../include/simMath/MyMath.cpp \
    ../include/simMath/3Vector.cpp \
    ../include/simMath/4Vector.cpp \
    ../include/simMath/7Vector.cpp \
    ../include/simMath/3X3Matrix.cpp \
    ../include/simMath/4X4Matrix.cpp \
    ../include/simMath/MMatrix.cpp \
    povray/base/fileinputoutput.cpp \
    povray/base/povms.cpp \
    povray/base/povmscpp.cpp \
    povray/base/processoptions.cpp \
    povray/base/stringutilities.cpp \
    povray/base/textstream.cpp \
    povray/frontend/defaultplatformbase.cpp \
    povray/frontend/processrenderoptions.cpp \
    povray/atmosph.cpp \
    povray/bbox.cpp \
    povray/bcyl.cpp \
    povray/benchmark.cpp \
    povray/bezier.cpp \
    povray/blob.cpp \
    povray/boxes.cpp \
    povray/bsphere.cpp \
    povray/camera.cpp \
    povray/chi2.cpp \
    povray/colour.cpp \
    povray/colutils.cpp \
    povray/cones.cpp \
    povray/csg.cpp \
    povray/discs.cpp \
    povray/express.cpp \
    povray/fncode.cpp \
    povray/fnintern.cpp \
    povray/fnpovfpu.cpp \
    povray/fnsyntax.cpp \
    povray/fpmetric.cpp \
    povray/fractal.cpp \
    povray/function.cpp \
    povray/hcmplx.cpp \
    povray/hfield.cpp \
    povray/histogra.cpp \
    povray/image.cpp \
    povray/interior.cpp \
    povray/isosurf.cpp \
    povray/lathe.cpp \
    povray/lbuffer.cpp \
    povray/lightgrp.cpp \
    povray/lighting.cpp \
    povray/mathutil.cpp \
    povray/matrices.cpp \
    povray/media.cpp \
    povray/mesh.cpp \
    povray/normal.cpp \
    povray/objects.cpp \
    povray/octree.cpp \
    povray/parse.cpp \
    povray/parsestr.cpp \
    povray/parstxtr.cpp \
    povray/pattern.cpp \
    povray/photons.cpp \
    povray/pigment.cpp \
    povray/planes.cpp \
    povray/point.cpp \
    povray/poly.cpp \
    povray/polygon.cpp \
    povray/polysolv.cpp \
    povray/pov_mem.cpp \
    povray/pov_util.cpp \
    povray/povmsend.cpp \
    povray/povmsrec.cpp \
    povray/povray.cpp \
    povray/prism.cpp \
    povray/quadrics.cpp \
    povray/quatern.cpp \
    povray/rad_data.cpp \
    povray/radiosit.cpp \
    povray/ray.cpp \
    povray/rendctrl.cpp \
    povray/render.cpp \
    povray/sor.cpp \
    povray/spheres.cpp \
    povray/sphsweep.cpp \
    povray/splines.cpp \
    povray/statspov.cpp \
    povray/super.cpp \
    povray/texture.cpp \
    povray/tokenize.cpp \
    povray/torus.cpp \
    povray/triangle.cpp \
    povray/truetype.cpp \
    povray/vbuffer.cpp \
    povray/vlbuffer.cpp \
    povray/warps.cpp \
    povray/renderio.cpp \
    povray/rgbafile.cpp \
    povray/optout.cpp \
    povray/userio.cpp \
    povray/base/textstreambuffer.cpp

HEADERS +=\
    ../include/simLib.h \
    simExtPovRay.h \
    ../include/simMath/MyMath.h \
    ../include/simMath/mathDefines.h \
    ../include/simMath/3Vector.h \
    ../include/simMath/4Vector.h \
    ../include/simMath/7Vector.h \
    ../include/simMath/3X3Matrix.h \
    ../include/simMath/4X4Matrix.h \
    ../include/simMath/MMatrix.h \
    povray/base/configbase.h \
    povray/base/fileinputoutput.h \
    povray/base/platformbase.h \
    povray/base/pointer.h \
    povray/base/pov_err.h \
    povray/base/povms.h \
    povray/base/povmscpp.h \
    povray/base/povmsgid.h \
    povray/base/processoptions.h \
    povray/base/stringutilities.h \
    povray/base/textstream.h \
    povray/frontend/configfrontend.h \
    povray/frontend/defaultplatformbase.h \
    povray/frontend/defaultrenderfrontend.h \
    povray/frontend/messageoutput.h \
    povray/frontend/processrenderoptions.h \
    povray/frontend/renderfrontend.h \
    povray/atmosph.h \
    povray/bbox.h \
    povray/bcyl.h \
    povray/benchmark.h \
    povray/bezier.h \
    povray/blob.h \
    povray/boxes.h \
    povray/bsphere.h \
    povray/camera.h \
    povray/chi2.h \
    povray/colour.h \
    povray/colutils.h \
    povray/cones.h \
    povray/csg.h \
    povray/discs.h \
    povray/express.h \
    povray/fncode.h \
    povray/fnintern.h \
    povray/fnpovfpu.h \
    povray/fnsyntax.h \
    povray/fpmetric.h \
    povray/fractal.h \
    povray/frame.h \
    povray/function.h \
    povray/hcmplx.h \
    povray/hfield.h \
    povray/histogra.h \
    povray/image.h \
    povray/interior.h \
    povray/isosurf.h \
    povray/lathe.h \
    povray/lbuffer.h \
    povray/lightgrp.h \
    povray/lighting.h \
    povray/mathutil.h \
    povray/matrices.h \
    povray/media.h \
    povray/mesh.h \
    povray/normal.h \
    povray/objects.h \
    povray/octree.h \
    povray/parse.h \
    povray/parsestr.h \
    povray/parstxtr.h \
    povray/pattern.h \
    povray/photons.h \
    povray/pigment.h \
    povray/planes.h \
    povray/point.h \
    povray/poly.h \
    povray/polygon.h \
    povray/polysolv.h \
    povray/pov_mem.h \
    povray/pov_util.h \
    povray/povmsend.h \
    povray/povmsrec.h \
    povray/povray.h \
    povray/prism.h \
    povray/quadrics.h \
    povray/quatern.h \
    povray/radiosit.h \
    povray/ray.h \
    povray/rendctrl.h \
    povray/render.h \
    povray/sor.h \
    povray/spheres.h \
    povray/sphsweep.h \
    povray/splines.h \
    povray/statspov.h \
    povray/super.h \
    povray/texture.h \
    povray/tokenize.h \
    povray/torus.h \
    povray/triangle.h \
    povray/truetype.h \
    povray/userio.h \
    povray/vbuffer.h \
    povray/vector.h \
    povray/vlbuffer.h \
    povray/warps.h \
    povray/renderio.h \
    povray/rgbafile.h \
    povray/base/config.h \
    povray/base/textstreambuffer.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

