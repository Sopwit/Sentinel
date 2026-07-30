# cmake/SentinelSigning.cmake
# Sentinel Code Signing Module for Windows and macOS

if(WIN32)
    set(SENTINEL_SIGN_THUMBPRINT "" CACHE STRING "SHA-1 Thumbprint of code signing certificate in Windows Certificate Store")
    set(SENTINEL_SIGN_PFX_FILE "" CACHE STRING "Path to the code signing certificate PFX file")
    set(SENTINEL_SIGN_PFX_PASSWORD "" CACHE STRING "Password for the PFX certificate file")
    set(SENTINEL_TIMESTAMP_URL "http://timestamp.digicert.com" CACHE STRING "Timestamp server URL")

    # Dynamic search for signtool using Windows SDK environment variables & standard installation paths
    set(SIGNTOOL_SEARCH_HINTS)
    if(DEFINED ENV{WindowsSdkDir} AND DEFINED ENV{WindowsSDKVersion})
        list(APPEND SIGNTOOL_SEARCH_HINTS "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64")
        list(APPEND SIGNTOOL_SEARCH_HINTS "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x86")
    endif()

    find_program(SIGNTOOL_EXECUTABLE signtool
        HINTS
            ${SIGNTOOL_SEARCH_HINTS}
            "C:/Program Files (x86)/Windows Kits/10/bin/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
        PATHS
            "C:/Program Files (x86)/Windows Kits/10/bin"
            "C:/Program Files (x86)/Windows Kits/11/bin"
            "$ENV{ProgramFiles}/Windows Kits/10/bin"
            "$ENV{ProgramFiles}/Windows Kits/11/bin"
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin"
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/11/bin"
    )

    if(SIGNTOOL_EXECUTABLE)
        set(SIGN_ARGS sign /fd SHA256 /tr "${SENTINEL_TIMESTAMP_URL}" /td SHA256)
        set(CAN_SIGN FALSE)
        if(SENTINEL_SIGN_THUMBPRINT)
            list(APPEND SIGN_ARGS /sha1 "${SENTINEL_SIGN_THUMBPRINT}")
            set(CAN_SIGN TRUE)
        elseif(SENTINEL_SIGN_PFX_FILE)
            list(APPEND SIGN_ARGS /f "${SENTINEL_SIGN_PFX_FILE}")
            if(SENTINEL_SIGN_PFX_PASSWORD)
                list(APPEND SIGN_ARGS /p "${SENTINEL_SIGN_PFX_PASSWORD}")
            endif()
            set(CAN_SIGN TRUE)
        endif()

        if(CAN_SIGN)
            message(STATUS "Windows code signing configured. SignTool executable: ${SIGNTOOL_EXECUTABLE}")

            if(TARGET sentinel-desktop)
                add_custom_command(TARGET sentinel-desktop POST_BUILD
                    COMMAND "${SIGNTOOL_EXECUTABLE}" ${SIGN_ARGS} "$<TARGET_FILE:sentinel-desktop>"
                    COMMENT "Signing sentinel-desktop.exe to prevent SmartScreen warnings"
                )
            endif()

            add_custom_target(sign_packages
                COMMAND ${CMAKE_COMMAND}
                    -DSIGNTOOL_EXECUTABLE="${SIGNTOOL_EXECUTABLE}"
                    -DSIGN_ARGS="${SIGN_ARGS}"
                    -DBINARY_DIR="${CMAKE_BINARY_DIR}"
                    -P "${CMAKE_CURRENT_LIST_DIR}/SentinelSignPackages.cmake"
                COMMENT "Signing generated NSIS/WiX installer files safely"
            )
        else()
            message(STATUS "signtool found but no certificate thumbprint or PFX file provided. Code signing bypassed.")
        endif()
    else()
        message(STATUS "signtool.exe not found. Automatic Windows code signing bypassed.")
    endif()

elseif(APPLE)
    set(SENTINEL_MACOS_CODESIGN_IDENTITY "" CACHE STRING "Developer ID Application certificate identity for macOS signing")
    set(SENTINEL_MACOS_NOTARY_PROFILE "" CACHE STRING "notarytool profile name for macOS notarization")

    find_program(CODESIGN_EXECUTABLE codesign)

    if(CODESIGN_EXECUTABLE AND SENTINEL_MACOS_CODESIGN_IDENTITY)
        message(STATUS "macOS code signing configured with identity: ${SENTINEL_MACOS_CODESIGN_IDENTITY}")

        if(TARGET sentinel-desktop)
            add_custom_command(TARGET sentinel-desktop POST_BUILD
                COMMAND "${CODESIGN_EXECUTABLE}" --force --deep --options runtime
                        --sign "${SENTINEL_MACOS_CODESIGN_IDENTITY}"
                        "$<TARGET_FILE:sentinel-desktop>"
                COMMENT "Signing macOS bundle target sentinel-desktop"
            )
        endif()
    else()
        message(STATUS "macOS code signing identity not set. App bundle will be unsigned / ad-hoc signed.")
    endif()
endif()
