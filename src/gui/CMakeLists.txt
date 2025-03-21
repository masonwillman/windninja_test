# THIS SOFTWARE WAS DEVELOPED AT THE ROCKY MOUNTAIN RESEARCH STATION (RMRS)
# MISSOULA FIRE SCIENCES LABORATORY BY EMPLOYEES OF THE FEDERAL GOVERNMENT
# IN THE COURSE OF THEIR OFFICIAL DUTIES. PURSUANT TO TITLE 17 SECTION 105
# OF THE UNITED STATES CODE, THIS SOFTWARE IS NOT SUBJECT TO COPYRIGHT
# PROTECTION AND IS IN THE PUBLIC DOMAIN. RMRS MISSOULA FIRE SCIENCES
# LABORATORY ASSUMES NO RESPONSIBILITY WHATSOEVER FOR ITS USE BY OTHER
# PARTIES,  AND MAKES NO GUARANTEES, EXPRESSED OR IMPLIED, ABOUT ITS QUALITY,
# RELIABILITY, OR ANY OTHER CHARACTERISTIC.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 2.6)

QT4_WRAP_UI(QTUI_H_SRCS WidgetDownloadDEM.ui
                        setconfigdialog.ui
                        stationFetchWidget.ui)

set(NINJA_GUI_INCLUDES consoleDockWidget.h
                       diurnalInput.h
                       stabilityInput.h
                       fbOutput.h
                       googleOutput.h
                       latLonWidget.h
                       mainWindow.h
                       metaWindWidget.h
                       nativeSolverInput.h
                       ninjafoamInput.h
                       outputHeightWidget.h
                       outputMetaData.h
                       pdfOutput.h
                       pointInput.h
                       shapeOutput.h
                       solvePage.h
                       solveThread.h
                       splash.h
                       surfaceInput.h
                       timeZoneWidget.h
                       #vec3f.h
                       vtkOutput.h
                       weatherModel.h
                       WidgetDownloadDEM.h
                       stationFetchWidget.h
                       windInput.h
                       windInputTable.h
                       WindNinjaTree.h
                       GoogleMapsInterface.h
                       setconfigdialog.h
                       ${QTUI_H_SRCS})

include_directories(${QT_INCLUDES}
                    ${Boost_INCLUDE_DIRS}
                    ${NETCDF_INCLUDES}
                    ${GDAL_SYSTEM_INCLUDE} ${GDAL_INCLUDE_DIR}
                    ${CURL_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/ninja
                    ${CMAKE_CURRENT_BINARY_DIR})

QT4_WRAP_CPP(NINJA_GUI_INCLUDES_MOC ${NINJA_GUI_INCLUDES})
QT4_ADD_RESOURCES(NINJA_GUI_RESOURCES ${PROJECT_SOURCE_DIR}/wn-resources.qrc)

set(NINJA_GUI_SOURCES cmake_gui.cpp
                      ${PROJECT_SOURCE_DIR}/src/ninja/cli.cpp
                      consoleDockWidget.cpp
                      diurnalInput.cpp
                      stabilityInput.cpp
                      fbOutput.cpp
                      googleOutput.cpp
                      latLonWidget.cpp
                      mainWindow.cpp
                      metaWindWidget.cpp
                      outputHeightWidget.cpp
                      outputMetaData.cpp
                      nativeSolverInput.cpp
                      ninjafoamInput.cpp
                      pdfOutput.cpp
                      pointInput.cpp
                      shapeOutput.cpp
                      solvePage.cpp
                      solveThread.cpp
                      splash.cpp
                      surfaceInput.cpp
                      timeZoneWidget.cpp
                      vec3f.cpp
                      vtkOutput.cpp
                      weatherModel.cpp
                      windInput.cpp
                      windInputTable.cpp
                      WindNinjaTree.cpp
                      WidgetDownloadDEM.cpp
                      stationFetchWidget.cpp
                      GoogleMapsInterface.cpp
                      setconfigdialog.cpp
                      ${QTUI_H_SRCS})
if(WIN32)
    set(NINJA_GUI_SOURCES ${NINJA_GUI_SOURCES} 
                          ${PROJECT_SOURCE_DIR}/wn-ms-resource.rc)
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif(WIN32)

set(LINK_LIBS ${QT_LIBRARIES}
              ${GDAL_LIBRARY}
              ${Boost_LIBRARIES}
              ${CURL_LIBRARIES}
              ${NETCDF_LIBRARIES_C})

if(WIN32)
    set(LINK_LIBS ${LINK_LIBS} ${PROJECT_BINARY_DIR}/src/ninja/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}ninja${CMAKE_STATIC_LIBRARY_SUFFIX})
    if(NOT ENABLE_CONSOLE)
        set(LINK_LIBS ${LINK_LIBS} ${QT_QTMAIN_LIBRARY})
        set(GUI_TYPE WIN32)
    else(NOT ENABLE_CONSOLE)
        set(GUI_TYPE "")
    endif(NOT ENABLE_CONSOLE)
else(WIN32)
    set(LINK_LIBS ${LINK_LIBS} ${PROJECT_BINARY_DIR}/src/ninja/${CMAKE_SHARED_LIBRARY_PREFIX}ninja${CMAKE_SHARED_LIBRARY_SUFFIX})
endif(WIN32)

add_executable(WindNinja ${GUI_TYPE}
                         ${NINJA_GUI_INCLUDES_MOC} 
                         ${NINJA_GUI_RESOURCES}
                         ${NINJA_GUI_SOURCES})

target_link_libraries(WindNinja ${LINK_LIBS})
add_dependencies(WindNinja ninja)
install(TARGETS WindNinja DESTINATION bin COMPONENT apps)

