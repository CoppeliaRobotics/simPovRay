cmake_minimum_required(VERSION 3.15)
project(simExtPovRay)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

find_package(Boost REQUIRED)

if(NOT LIBPLUGIN_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(LIBPLUGIN_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/libPlugin)
    else()
        set(LIBPLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libPlugin)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH ${LIBPLUGIN_DIR}/cmake)
find_package(CoppeliaSim 4.5.0.0 REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

coppeliasim_add_plugin(
    simExtPovRay
    LEGACY
    SOURCES
    simExtPovRay.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/MyMath.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/3Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/4Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/7Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/3X3Matrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/4X4Matrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/MMatrix.cpp
    povray/base/fileinputoutput.cpp
    povray/base/povms.cpp
    povray/base/povmscpp.cpp
    povray/base/processoptions.cpp
    povray/base/stringutilities.cpp
    povray/base/textstream.cpp
    povray/frontend/defaultplatformbase.cpp
    povray/frontend/processrenderoptions.cpp
    povray/atmosph.cpp
    povray/bbox.cpp
    povray/bcyl.cpp
    povray/benchmark.cpp
    povray/bezier.cpp
    povray/blob.cpp
    povray/boxes.cpp
    povray/bsphere.cpp
    povray/camera.cpp
    povray/chi2.cpp
    povray/colour.cpp
    povray/colutils.cpp
    povray/cones.cpp
    povray/csg.cpp
    povray/discs.cpp
    povray/express.cpp
    povray/fncode.cpp
    povray/fnintern.cpp
    povray/fnpovfpu.cpp
    povray/fnsyntax.cpp
    povray/fpmetric.cpp
    povray/fractal.cpp
    povray/function.cpp
    povray/hcmplx.cpp
    povray/hfield.cpp
    povray/histogra.cpp
    povray/image.cpp
    povray/interior.cpp
    povray/isosurf.cpp
    povray/lathe.cpp
    povray/lbuffer.cpp
    povray/lightgrp.cpp
    povray/lighting.cpp
    povray/mathutil.cpp
    povray/matrices.cpp
    povray/media.cpp
    povray/mesh.cpp
    povray/normal.cpp
    povray/objects.cpp
    povray/octree.cpp
    povray/parse.cpp
    povray/parsestr.cpp
    povray/parstxtr.cpp
    povray/pattern.cpp
    povray/photons.cpp
    povray/pigment.cpp
    povray/planes.cpp
    povray/point.cpp
    povray/poly.cpp
    povray/polygon.cpp
    povray/polysolv.cpp
    povray/pov_mem.cpp
    povray/pov_util.cpp
    povray/povmsend.cpp
    povray/povmsrec.cpp
    povray/povray.cpp
    povray/prism.cpp
    povray/quadrics.cpp
    povray/quatern.cpp
    povray/rad_data.cpp
    povray/radiosit.cpp
    povray/ray.cpp
    povray/rendctrl.cpp
    povray/render.cpp
    povray/sor.cpp
    povray/spheres.cpp
    povray/sphsweep.cpp
    povray/splines.cpp
    povray/statspov.cpp
    povray/super.cpp
    povray/texture.cpp
    povray/tokenize.cpp
    povray/torus.cpp
    povray/triangle.cpp
    povray/truetype.cpp
    povray/vbuffer.cpp
    povray/vlbuffer.cpp
    povray/warps.cpp
    povray/renderio.cpp
    povray/rgbafile.cpp
    povray/optout.cpp
    povray/userio.cpp
    povray/base/textstreambuffer.cpp
)
set_property(TARGET simExtPovRay PROPERTY CXX_STANDARD 98)
target_include_directories(simExtPovRay PRIVATE ${COPPELIASIM_ROOT_DIR}/programming/simMath)
target_include_directories(simExtPovRay PRIVATE povray)
target_include_directories(simExtPovRay PRIVATE povray/base)
target_include_directories(simExtPovRay PRIVATE povray/frontend)
target_link_libraries(simExtPovRay Qt5::Core)
