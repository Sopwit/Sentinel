# cmake/SentinelQmlHelpers.cmake
# CMake helper functions to automate QML resource alias registration and singleton attributes

function(sentinel_register_qml_files)
    cmake_parse_arguments(ARG "" "" "FILES;SINGLETONS" ${ARGN})

    foreach(qml_file IN LISTS ARG_FILES)
        get_filename_component(file_name "${qml_file}" NAME)

        set_source_files_properties("${qml_file}" PROPERTIES
            QT_RESOURCE_ALIAS "${file_name}"
        )

        if(file_name IN_LIST ARG_SINGLETONS)
            set_source_files_properties("${qml_file}" PROPERTIES
                QT_QML_SINGLETON_TYPE TRUE
            )
        endif()
    endforeach()
endfunction()
