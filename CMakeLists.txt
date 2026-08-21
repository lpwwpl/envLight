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

# 查找 Qt5
find_package(Qt5 5.12.11 REQUIRED COMPONENTS Core Gui Widgets Svg)

# ========== 修改开始：配置 Debug/Release 库目录 ==========
set(LIB_DIR_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/x64/Release")
set(LIB_DIR_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/x64/Debug")

# ---------- VTK 库基础名称（全部从原列表中提取，去掉 “-9.1.lib”） ----------
set(VTK_BASENAMES
    vtkWrappingTools
    vtkViewsQt
    vtkViewsInfovis
    vtkViewsContext2D
    vtkTestingRendering
    vtkRenderingQt
    vtkRenderingVolumeOpenGL2
    vtkRenderingLabel
    vtkRenderingLOD
    vtkRenderingImage
    vtkRenderingContextOpenGL2
    vtkIOVeraOut
    vtkIOTecplotTable
    vtkIOSegY
    vtkIOParallelXML
    vtkIOParallel
    vtkIOPLY
    vtkIOOggTheora
    vtktheora
    vtkogg
    vtkIONetCDF
    vtkIOMotionFX
    vtkIOMINC
    vtkIOLSDyna
    vtkIOInfovis
    vtklibxml2
    vtkIOImport
    vtkIOIOSS
    vtkioss
    vtkIOHDF
    vtkIOVideo
    vtkIOMovie
    vtkIOExportPDF
    vtkIOExportGL2PS
    vtkRenderingGL2PSOpenGL2
    vtkgl2ps
    vtkIOExport
    vtkRenderingVtkJS
    vtkIOGeometry
    vtkRenderingSceneGraph
    vtkIOExodus
    vtkIOEnSight
    vtkIOCityGML
    vtkIOChemistry
    vtkIOCONVERGECFD
    vtkIOCGNSReader
    vtkIOAsynchronous
    vtkIOAMR
    vtkInteractionImage
    vtkImagingStencil
    vtkImagingStatistics
    vtkImagingMorphological
    vtkImagingMath
    vtkImagingFourier
    vtkGUISupportQtSQL
    vtkIOSQL
    vtksqlite
    vtkGUISupportQtQuick
    vtkGUISupportQt
    vtkGeovisCore
    vtklibproj
    vtkInfovisLayout
    vtkViewsCore
    vtkInteractionWidgets
    vtkRenderingVolume
    vtkRenderingAnnotation
    vtkImagingHybrid
    vtkImagingColor
    vtkInteractionStyle
    vtkFiltersTopology
    vtkFiltersSelection
    vtkFiltersSMP
    vtkFiltersProgrammable
    vtkFiltersPoints
    vtkFiltersVerdict
    vtkverdict
    vtkFiltersParallelImaging
    vtkFiltersImaging
    vtkImagingGeneral
    vtkFiltersHyperTree
    vtkFiltersGeneric
    vtkFiltersFlowPaths
    vtkFiltersAMR
    vtkFiltersParallel
    vtkFiltersTexture
    vtkFiltersModeling
    vtkFiltersHybrid
    vtkDomainsChemistryOpenGL2
    vtkRenderingOpenGL2
    vtkRenderingUI
    vtkDomainsChemistry
    vtkChartsCore
    vtkInfovisCore
    vtkFiltersExtraction
    vtkParallelDIY
    vtkIOXML
    vtkIOXMLParser
    vtkexpat
    vtkParallelCore
    vtkIOLegacy
    vtkIOCore
    vtkdoubleconversion
    vtklz4
    vtklzma
    vtkFiltersStatistics
    vtkImagingSources
    vtkIOImage
    vtkDICOMParser
    vtkmetaio
    vtktiff
    vtkRenderingContext2D
    vtkRenderingFreeType
    vtkfreetype
    vtkRenderingCore
    vtkFiltersSources
    vtkImagingCore
    vtkFiltersGeometry
    vtkFiltersGeneral
    vtkfmt
    vtkCommonComputationalGeometry
    vtkFiltersCore
    vtkCommonExecutionModel
    vtklibharu
    vtkjsoncpp
    vtkexodusII
    vtknetcdf
    vtkcgns
    vtkhdf5_hl
    vtkhdf5
    vtkglew
    vtkpng
    vtkjpeg
    vtkzlib
    vtkCommonColor
    vtkCommonDataModel
    vtkpugixml
    vtkCommonSystem
    vtkCommonMisc
    vtkCommonTransforms
    vtkCommonMath
    vtkkissfft
    vtkCommonCore
    vtkloguru
    vtksys
)

# 生成带配置的 VTK 库路径
set(VTK_LIBS "")
foreach(basename ${VTK_BASENAMES})
    list(APPEND VTK_LIBS
        "$<IF:$<CONFIG:Debug>,${LIB_DIR_DEBUG}/${basename}-9.1d.lib,${LIB_DIR_RELEASE}/${basename}-9.1.lib>"
    )
endforeach()

# ---------- 单独处理 miniz ----------
set(MINIZ_LIB_DEBUG   "${LIB_DIR_DEBUG}/miniz.lib")
set(MINIZ_LIB_RELEASE "${LIB_DIR_RELEASE}/miniz.lib")
set(MINIZ_LIB "$<IF:$<CONFIG:Debug>,${MINIZ_LIB_DEBUG},${MINIZ_LIB_RELEASE}>")

# 原 QT5_LIB 变量未使用，可注释或删除
# ========== 修改结束 ==========

# ---- 源文件 ----
set(SOURCES     
    envLight/vtk_scene.cpp
    envLight/SunSky.cpp
    envLight/SkyPolarWidget.cpp
    envLight/panorama_processor.cpp
    envLight/mainwindow.cpp
    envLight/main.cpp
    envLight/WeatherEffects.cpp
    envLight/SkyPerspectiveWidget.cpp
    envLight/EpwReader.cpp
    envLight/CIEWidget.cpp
)

# ---- 头文件 ----
set(HEADERS	
    envLight/VL234f.hpp
    envLight/vtk_scene.h
    envLight/coordinate_system.h
    envLight/tinyexr.h
    envLight/SunSky.hpp
    envLight/stb_image.h
    envLight/stb_image_write.h
    envLight/WeatherEffects.h
    envLight/SkyPolarWidget.h
    envLight/panorama_processor.h
    envLight/SkyPerspectiveWidget.h
    envLight/mainwindow.h
    envLight/EpwReader.h
    envLight/EpwData.hpp
    envLight/CIEWidget.h
)

add_executable(envLight WIN32 ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCES})

set_target_properties(envLight PROPERTIES AUTOGEN_SOURCE_GROUP "moc")

# 包含头文件路径
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

# 编译选项
target_compile_options(envLight PRIVATE
    /openmp
    /utf-8
    /wd4577
    /wd4467
)

# 设置子系统为控制台
target_link_options(envLight PRIVATE /SUBSYSTEM:CONSOLE)
target_link_options(envLight PRIVATE
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
)

# 链接库（现在 VTK_LIBS 和 MINIZ_LIB 会自动选择配置）
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

# ---- 安装部分（未改动，仍默认使用 Release 的 DLL） ----
set(QT_BIN_DIR ${LIB_DIR_RELEASE} CACHE PATH "Directory containing qt.dll")
set(VTK_BIN_DIR ${LIB_DIR_RELEASE} CACHE PATH "Directory containing vtk.dll")

if(WIN32)
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
        RUNTIME DESTINATION bin
    )

    # 安装 VTK DLL（同样使用 Release 目录）
    if(EXISTS ${VTK_BIN_DIR})
        set(VTK_DLLS_TO_INSTALL "")
        # 注意：此处使用原来的 VTK_BASENAMES 生成 DLL 名称，但只拷贝 Release 的 .dll
        foreach(basename ${VTK_BASENAMES})
            set(dll_path "${VTK_BIN_DIR}/${basename}-9.1.dll")
            if(EXISTS ${dll_path})
                list(APPEND VTK_DLLS_TO_INSTALL ${dll_path})
            endif()
        endforeach()
        if(VTK_DLLS_TO_INSTALL)
            install(FILES ${VTK_DLLS_TO_INSTALL} DESTINATION bin)
        endif()
    endif()

    # Qt 插件
    set(QT_PLUGIN_SUBDIRS iconengines imageformats platforms styles)
    foreach(subdir ${QT_PLUGIN_SUBDIRS})
        set(src_dir "${LIB_DIR_RELEASE}/${subdir}")
        if(EXISTS ${src_dir})
            install(DIRECTORY ${src_dir} DESTINATION bin)
        endif()
    endforeach()
endif()

# 安装第三方头文件和库（沿用原逻辑）
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")

if(EXISTS "${THIRDPARTY_DIR}/tinyexr-3.0.0/")
    install(DIRECTORY "${THIRDPARTY_DIR}/tinyexr-3.0.0/"
        DESTINATION include/tinyexr-3.0.0
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
    )
endif()

if(EXISTS "${THIRDPARTY_DIR}/vtk/")
    install(DIRECTORY "${THIRDPARTY_DIR}/vtk/"
        DESTINATION include/vtk
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
    )
endif()

# 注意：此处安装的是 Release 库文件（若需安装 Debug 库可另行处理）
install(FILES ${VTK_LIBS} ${MINIZ_LIB} DESTINATION lib)   # 这里的 ${VTK_LIBS} 是生成器表达式，在安装时会被展开为当前配置的路径，可能不适用。建议改为显式指定 Release 库。
