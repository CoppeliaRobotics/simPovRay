cmake_minimum_required(VERSION 3.16.3)
project(simPovRay)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

find_package(Boost REQUIRED)

if(NOT COPPELIASIM_INCLUDE_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(COPPELIASIM_INCLUDE_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/include)
    else()
        set(COPPELIASIM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH ${COPPELIASIM_INCLUDE_DIR}/cmake)
find_package(CoppeliaSim 4.5.1.7 REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

coppeliasim_add_plugin(
    simPovRay
    LEGACY
    SOURCES
    simPovRay.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mathFuncs.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/7Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3X3Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4X4Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mXnMatrix.cpp
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
set_property(TARGET simPovRay PROPERTY CXX_STANDARD 98)
target_include_directories(simPovRay PRIVATE ${COPPELIASIM_INCLUDE_DIR}/simMath)
target_include_directories(simPovRay PRIVATE povray)
target_include_directories(simPovRay PRIVATE povray/base)
target_include_directories(simPovRay PRIVATE povray/frontend)
target_link_libraries(simPovRay PRIVATE Qt5::Core)
coppeliasim_add_lua(simPovRay.lua)
