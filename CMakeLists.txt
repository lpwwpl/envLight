cmake_minimum_required(VERSION 3.16)
project(envLight LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 查找 Qt5 和 OpenGL（Qt 建议用 find_package）
find_package(Qt5 5.12.11 REQUIRED COMPONENTS Core Gui Widgets Svg)

# 预编译库的路径（假设位于项目根目录下的 x64/Release）
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/x64/Release")
 
# 导入特定的库文件（非 VTK 部分）
set(QT5_LIB     ${LIB_DIR}/Qt5Core.lib
    ${LIB_DIR}/Qt5Core.lib
    ${LIB_DIR}/Qt5Gui.lib
    ${LIB_DIR}/Qt5Widgets.lib
    ${LIB_DIR}/Qt5Svg.lib
)

# 收集所有 VTK 9.1 的库（基于你提供的列表）
set(VTK_LIBS
    ${LIB_DIR}/vtkWrappingTools-9.1.lib
    ${LIB_DIR}/vtkViewsQt-9.1.lib
    ${LIB_DIR}/vtkViewsInfovis-9.1.lib
    ${LIB_DIR}/vtkViewsContext2D-9.1.lib
    ${LIB_DIR}/vtkTestingRendering-9.1.lib
    ${LIB_DIR}/vtkRenderingQt-9.1.lib
    ${LIB_DIR}/vtkRenderingVolumeOpenGL2-9.1.lib
    ${LIB_DIR}/vtkRenderingLabel-9.1.lib
    ${LIB_DIR}/vtkRenderingLOD-9.1.lib
    ${LIB_DIR}/vtkRenderingImage-9.1.lib
    ${LIB_DIR}/vtkRenderingContextOpenGL2-9.1.lib
    ${LIB_DIR}/vtkIOVeraOut-9.1.lib
    ${LIB_DIR}/vtkIOTecplotTable-9.1.lib
    ${LIB_DIR}/vtkIOSegY-9.1.lib
    ${LIB_DIR}/vtkIOParallelXML-9.1.lib
    ${LIB_DIR}/vtkIOParallel-9.1.lib
    ${LIB_DIR}/vtkIOPLY-9.1.lib
    ${LIB_DIR}/vtkIOOggTheora-9.1.lib
    ${LIB_DIR}/vtktheora-9.1.lib
    ${LIB_DIR}/vtkogg-9.1.lib
    ${LIB_DIR}/vtkIONetCDF-9.1.lib
    ${LIB_DIR}/vtkIOMotionFX-9.1.lib
    ${LIB_DIR}/vtkIOMINC-9.1.lib
    ${LIB_DIR}/vtkIOLSDyna-9.1.lib
    ${LIB_DIR}/vtkIOInfovis-9.1.lib
    ${LIB_DIR}/vtklibxml2-9.1.lib
    ${LIB_DIR}/vtkIOImport-9.1.lib
    ${LIB_DIR}/vtkIOIOSS-9.1.lib
    ${LIB_DIR}/vtkioss-9.1.lib
    ${LIB_DIR}/vtkIOHDF-9.1.lib
    ${LIB_DIR}/vtkIOVideo-9.1.lib
    ${LIB_DIR}/vtkIOMovie-9.1.lib
    ${LIB_DIR}/vtkIOExportPDF-9.1.lib
    ${LIB_DIR}/vtkIOExportGL2PS-9.1.lib
    ${LIB_DIR}/vtkRenderingGL2PSOpenGL2-9.1.lib
    ${LIB_DIR}/vtkgl2ps-9.1.lib
    ${LIB_DIR}/vtkIOExport-9.1.lib
    ${LIB_DIR}/vtkRenderingVtkJS-9.1.lib
    ${LIB_DIR}/vtkIOGeometry-9.1.lib
    ${LIB_DIR}/vtkRenderingSceneGraph-9.1.lib
    ${LIB_DIR}/vtkIOExodus-9.1.lib
    ${LIB_DIR}/vtkIOEnSight-9.1.lib
    ${LIB_DIR}/vtkIOCityGML-9.1.lib
    ${LIB_DIR}/vtkIOChemistry-9.1.lib
    ${LIB_DIR}/vtkIOCONVERGECFD-9.1.lib
    ${LIB_DIR}/vtkIOCGNSReader-9.1.lib
    ${LIB_DIR}/vtkIOAsynchronous-9.1.lib
    ${LIB_DIR}/vtkIOAMR-9.1.lib
    ${LIB_DIR}/vtkInteractionImage-9.1.lib
    ${LIB_DIR}/vtkImagingStencil-9.1.lib
    ${LIB_DIR}/vtkImagingStatistics-9.1.lib
    ${LIB_DIR}/vtkImagingMorphological-9.1.lib
    ${LIB_DIR}/vtkImagingMath-9.1.lib
    ${LIB_DIR}/vtkImagingFourier-9.1.lib
    ${LIB_DIR}/vtkGUISupportQtSQL-9.1.lib
    ${LIB_DIR}/vtkIOSQL-9.1.lib
    ${LIB_DIR}/vtksqlite-9.1.lib
    ${LIB_DIR}/vtkGUISupportQtQuick-9.1.lib
    ${LIB_DIR}/vtkGUISupportQt-9.1.lib
    ${LIB_DIR}/vtkGeovisCore-9.1.lib
    ${LIB_DIR}/vtklibproj-9.1.lib
    ${LIB_DIR}/vtkInfovisLayout-9.1.lib
    ${LIB_DIR}/vtkViewsCore-9.1.lib
    ${LIB_DIR}/vtkInteractionWidgets-9.1.lib
    ${LIB_DIR}/vtkRenderingVolume-9.1.lib
    ${LIB_DIR}/vtkRenderingAnnotation-9.1.lib
    ${LIB_DIR}/vtkImagingHybrid-9.1.lib
    ${LIB_DIR}/vtkImagingColor-9.1.lib
    ${LIB_DIR}/vtkInteractionStyle-9.1.lib
    ${LIB_DIR}/vtkFiltersTopology-9.1.lib
    ${LIB_DIR}/vtkFiltersSelection-9.1.lib
    ${LIB_DIR}/vtkFiltersSMP-9.1.lib
    ${LIB_DIR}/vtkFiltersProgrammable-9.1.lib
    ${LIB_DIR}/vtkFiltersPoints-9.1.lib
    ${LIB_DIR}/vtkFiltersVerdict-9.1.lib
    ${LIB_DIR}/vtkverdict-9.1.lib
    ${LIB_DIR}/vtkFiltersParallelImaging-9.1.lib
    ${LIB_DIR}/vtkFiltersImaging-9.1.lib
    ${LIB_DIR}/vtkImagingGeneral-9.1.lib
    ${LIB_DIR}/vtkFiltersHyperTree-9.1.lib
    ${LIB_DIR}/vtkFiltersGeneric-9.1.lib
    ${LIB_DIR}/vtkFiltersFlowPaths-9.1.lib
    ${LIB_DIR}/vtkFiltersAMR-9.1.lib
    ${LIB_DIR}/vtkFiltersParallel-9.1.lib
    ${LIB_DIR}/vtkFiltersTexture-9.1.lib
    ${LIB_DIR}/vtkFiltersModeling-9.1.lib
    ${LIB_DIR}/vtkFiltersHybrid-9.1.lib
    ${LIB_DIR}/vtkDomainsChemistryOpenGL2-9.1.lib
    ${LIB_DIR}/vtkRenderingOpenGL2-9.1.lib
    ${LIB_DIR}/vtkRenderingUI-9.1.lib
    ${LIB_DIR}/vtkDomainsChemistry-9.1.lib
    ${LIB_DIR}/vtkChartsCore-9.1.lib
    ${LIB_DIR}/vtkInfovisCore-9.1.lib
    ${LIB_DIR}/vtkFiltersExtraction-9.1.lib
    ${LIB_DIR}/vtkParallelDIY-9.1.lib
    ${LIB_DIR}/vtkIOXML-9.1.lib
    ${LIB_DIR}/vtkIOXMLParser-9.1.lib
    ${LIB_DIR}/vtkexpat-9.1.lib
    ${LIB_DIR}/vtkParallelCore-9.1.lib
    ${LIB_DIR}/vtkIOLegacy-9.1.lib
    ${LIB_DIR}/vtkIOCore-9.1.lib
    ${LIB_DIR}/vtkdoubleconversion-9.1.lib
    ${LIB_DIR}/vtklz4-9.1.lib
    ${LIB_DIR}/vtklzma-9.1.lib
    ${LIB_DIR}/vtkFiltersStatistics-9.1.lib
    ${LIB_DIR}/vtkImagingSources-9.1.lib
    ${LIB_DIR}/vtkIOImage-9.1.lib
    ${LIB_DIR}/vtkDICOMParser-9.1.lib
    ${LIB_DIR}/vtkmetaio-9.1.lib
    ${LIB_DIR}/vtktiff-9.1.lib
    ${LIB_DIR}/vtkRenderingContext2D-9.1.lib
    ${LIB_DIR}/vtkRenderingFreeType-9.1.lib
    ${LIB_DIR}/vtkfreetype-9.1.lib
    ${LIB_DIR}/vtkRenderingCore-9.1.lib
    ${LIB_DIR}/vtkFiltersSources-9.1.lib
    ${LIB_DIR}/vtkImagingCore-9.1.lib
    ${LIB_DIR}/vtkFiltersGeometry-9.1.lib
    ${LIB_DIR}/vtkFiltersGeneral-9.1.lib
    ${LIB_DIR}/vtkfmt-9.1.lib
    ${LIB_DIR}/vtkCommonComputationalGeometry-9.1.lib
    ${LIB_DIR}/vtkFiltersCore-9.1.lib
    ${LIB_DIR}/vtkCommonExecutionModel-9.1.lib
    ${LIB_DIR}/vtklibharu-9.1.lib
    ${LIB_DIR}/vtkjsoncpp-9.1.lib
    ${LIB_DIR}/vtkexodusII-9.1.lib
    ${LIB_DIR}/vtknetcdf-9.1.lib
    ${LIB_DIR}/vtkcgns-9.1.lib
    ${LIB_DIR}/vtkhdf5_hl-9.1.lib
    ${LIB_DIR}/vtkhdf5-9.1.lib
    ${LIB_DIR}/vtkglew-9.1.lib
    ${LIB_DIR}/vtkpng-9.1.lib
    ${LIB_DIR}/vtkjpeg-9.1.lib
    ${LIB_DIR}/vtkzlib-9.1.lib
    ${LIB_DIR}/vtkCommonColor-9.1.lib
    ${LIB_DIR}/vtkCommonDataModel-9.1.lib
    ${LIB_DIR}/vtkpugixml-9.1.lib
    ${LIB_DIR}/vtkCommonSystem-9.1.lib
    ${LIB_DIR}/vtkCommonMisc-9.1.lib
    ${LIB_DIR}/vtkCommonTransforms-9.1.lib
    ${LIB_DIR}/vtkCommonMath-9.1.lib
    ${LIB_DIR}/vtkkissfft-9.1.lib
    ${LIB_DIR}/vtkCommonCore-9.1.lib
    ${LIB_DIR}/vtkloguru-9.1.lib
    ${LIB_DIR}/vtksys-9.1.lib
)
set(MINIZ_LIB          ${LIB_DIR}/miniz.lib)

# ---- 源文件（位于 envLight 目录下）----
set(SOURCES     
    envLight/vtk_scene.cpp
    envLight/SunSky.cpp
    envLight/SkyPolarWidget.cpp
    envLight/panorama_processor.cpp
    envLight/mainwindow.cpp
	envLight/main.cpp
    envLight/EpwReader.cpp
    envLight/CIEWidget.cpp
)

# ---- 头文件（位于 envLight 目录下）----
set(HEADERS	
    envLight/VL234f.hpp
    envLight/vtk_scene.h
    envLight/tinyexr.h
    envLight/SunSky.hpp
    envLight/stb_image.h
    envLight/stb_image_write.h
    envLight/SkyPolarWidget.h
    envLight/panorama_processor.h
    envLight/mainwindow.h
    envLight/EpwReader.h
    envLight/EpwData.hpp
    envLight/CIEWidget.h
)

add_executable(envLight WIN32 ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCES})

set_target_properties(envLight PROPERTIES AUTOGEN_SOURCE_GROUP "moc")



# 包含头文件路径（请根据实际目录调整）
target_include_directories(envLight PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/envLight
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyexr-3.0.0
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vtk
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyexr-3.0.0/deps/miniz
)
include_directories(${QT5_INCLUDE_DIR})
target_compile_definitions(envLight PRIVATE
	TINYEXR_USE_PIZ
	_CRT_SECURE_NO_WARNINGS
	STB_IMAGE_WRITE_IMPLEMENTATION
	STB_IMAGE_IMPLEMENTATION
    _WINDOWS
    UNICODE
    _UNICODE
    WIN32
    WIN64
)

# ---- 编译选项 ----
target_compile_options(envLight PRIVATE
    /openmp
    /utf-8
    /wd4577
    /wd4467
)

# 设置子系统为控制台
target_link_options(envLight PRIVATE /SUBSYSTEM:CONSOLE)
# 针对 Release 模式添加优化链接选项
target_link_options(envLight PRIVATE
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
)

# 链接库
target_link_libraries(envLight
        Qt5::Core
        Qt5::Widgets
        Qt5::Svg
    ${VTK_LIBS}
	${MINIZ_LIB}
)

# 设置输出目录
set_target_properties(envLight PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set(QT_BIN_DIR ${LIB_DIR} CACHE PATH "Directory containing qt.dll")
set(VTK_BIN_DIR ${LIB_DIR} CACHE PATH "Directory containing vtk.dll")

if(WIN32)
    # ------------------- Qt5 DLL -------------------
if(EXISTS ${QT_BIN_DIR})
    install(FILES
        ${QT_BIN_DIR}/Qt5Core.dll
        ${QT_BIN_DIR}/Qt5Gui.dll
        ${QT_BIN_DIR}/Qt5OpenGL.dll
        ${QT_BIN_DIR}/Qt5Widgets.dll
        ${QT_BIN_DIR}/Qt5Svg.dll
        DESTINATION bin
    )
endif()

install(TARGETS envLight
    RUNTIME DESTINATION bin   # 安装到 ${CMAKE_INSTALL_PREFIX}/bin
)

    # ------------------- VTK 9.1 DLL -------------------
    # 假设 VTK 的 DLL 位于某个标准路径（如 VTK 安装目录下的 bin）
if(EXISTS ${VTK_BIN_DIR})
    set(VTK_DLLS_TO_INSTALL "")
    foreach(vtk_lib ${VTK_LIBS})
        # 获取文件名，例如 vtkCommonCore-9.1.lib
        get_filename_component(lib_name ${vtk_lib} NAME)
        # 将扩展名 .lib 替换为 .dll
        string(REPLACE ".lib" ".dll" dll_name ${lib_name})
        set(dll_path "${VTK_BIN_DIR}/${dll_name}")
        if(EXISTS ${dll_path})
            list(APPEND VTK_DLLS_TO_INSTALL ${dll_path})
        else()
            # 可能是静态库或命名不匹配，仅打印状态（不中断）
            message(STATUS "VTK DLL not found (skip): ${dll_path}")
        endif()
    endforeach()

    if(VTK_DLLS_TO_INSTALL)
        install(FILES ${VTK_DLLS_TO_INSTALL} DESTINATION bin)
    else()
        message(WARNING "No VTK DLLs found to install in ${VTK_BIN_DIR}")
    endif()
else()
    message(WARNING "VTK bin directory not found: ${VTK_BIN_DIR}")
endif()


	
    set(QT_PLUGIN_SUBDIRS iconengines imageformats platforms styles)

    foreach(subdir ${QT_PLUGIN_SUBDIRS})
        set(src_dir "${LIB_DIR}/${subdir}")
        if(EXISTS ${src_dir})
            install(DIRECTORY ${src_dir} DESTINATION bin)
        else()
            message(WARNING "Qt plugin directory not found: ${src_dir}")
        endif()
    endforeach()
endif()

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")


# 安装 miniz 头文件
if(EXISTS "${THIRDPARTY_DIR}/tinyexr-3.0.0/")
    install(DIRECTORY "${THIRDPARTY_DIR}/tinyexr-3.0.0/"
        DESTINATION include/tinyexr-3.0.0
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
    )
endif()

# 安装 vtk 头文件
if(EXISTS "${THIRDPARTY_DIR}/vtk/")
    install(DIRECTORY "${THIRDPARTY_DIR}/vtk/"
        DESTINATION include/vtk
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
    )
endif()

install(FILES ${VTK_LIBS} ${MINIZ_LIB} DESTINATION lib)


