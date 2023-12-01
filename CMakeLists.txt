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
    sourceCode/simPovRay.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mathFuncs.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/7Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3X3Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4X4Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mXnMatrix.cpp
    external/povray/base/fileinputoutput.cpp
    external/povray/base/povms.cpp
    external/povray/base/povmscpp.cpp
    external/povray/base/processoptions.cpp
    external/povray/base/stringutilities.cpp
    external/povray/base/textstream.cpp
    external/povray/frontend/defaultplatformbase.cpp
    external/povray/frontend/processrenderoptions.cpp
    external/povray/atmosph.cpp
    external/povray/bbox.cpp
    external/povray/bcyl.cpp
    external/povray/benchmark.cpp
    external/povray/bezier.cpp
    external/povray/blob.cpp
    external/povray/boxes.cpp
    external/povray/bsphere.cpp
    external/povray/camera.cpp
    external/povray/chi2.cpp
    external/povray/colour.cpp
    external/povray/colutils.cpp
    external/povray/cones.cpp
    external/povray/csg.cpp
    external/povray/discs.cpp
    external/povray/express.cpp
    external/povray/fncode.cpp
    external/povray/fnintern.cpp
    external/povray/fnpovfpu.cpp
    external/povray/fnsyntax.cpp
    external/povray/fpmetric.cpp
    external/povray/fractal.cpp
    external/povray/function.cpp
    external/povray/hcmplx.cpp
    external/povray/hfield.cpp
    external/povray/histogra.cpp
    external/povray/image.cpp
    external/povray/interior.cpp
    external/povray/isosurf.cpp
    external/povray/lathe.cpp
    external/povray/lbuffer.cpp
    external/povray/lightgrp.cpp
    external/povray/lighting.cpp
    external/povray/mathutil.cpp
    external/povray/matrices.cpp
    external/povray/media.cpp
    external/povray/mesh.cpp
    external/povray/normal.cpp
    external/povray/objects.cpp
    external/povray/octree.cpp
    external/povray/parse.cpp
    external/povray/parsestr.cpp
    external/povray/parstxtr.cpp
    external/povray/pattern.cpp
    external/povray/photons.cpp
    external/povray/pigment.cpp
    external/povray/planes.cpp
    external/povray/point.cpp
    external/povray/poly.cpp
    external/povray/polygon.cpp
    external/povray/polysolv.cpp
    external/povray/pov_mem.cpp
    external/povray/pov_util.cpp
    external/povray/povmsend.cpp
    external/povray/povmsrec.cpp
    external/povray/povray.cpp
    external/povray/prism.cpp
    external/povray/quadrics.cpp
    external/povray/quatern.cpp
    external/povray/rad_data.cpp
    external/povray/radiosit.cpp
    external/povray/ray.cpp
    external/povray/rendctrl.cpp
    external/povray/render.cpp
    external/povray/sor.cpp
    external/povray/spheres.cpp
    external/povray/sphsweep.cpp
    external/povray/splines.cpp
    external/povray/statspov.cpp
    external/povray/super.cpp
    external/povray/texture.cpp
    external/povray/tokenize.cpp
    external/povray/torus.cpp
    external/povray/triangle.cpp
    external/povray/truetype.cpp
    external/povray/vbuffer.cpp
    external/povray/vlbuffer.cpp
    external/povray/warps.cpp
    external/povray/renderio.cpp
    external/povray/rgbafile.cpp
    external/povray/optout.cpp
    external/povray/userio.cpp
    external/povray/base/textstreambuffer.cpp
)
set_property(TARGET simPovRay PROPERTY CXX_STANDARD 98)
target_include_directories(simPovRay PRIVATE ${COPPELIASIM_INCLUDE_DIR}/simMath)
target_include_directories(simPovRay PRIVATE sourceCode)
target_include_directories(simPovRay PRIVATE external)
target_include_directories(simPovRay PRIVATE external/povray)
target_include_directories(simPovRay PRIVATE external/povray/base)
target_include_directories(simPovRay PRIVATE external/povray/frontend)
target_link_libraries(simPovRay PRIVATE Qt5::Core)
coppeliasim_add_lua(lua/simPovRay.lua)
