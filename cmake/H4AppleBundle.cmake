set(H4_BUNDLE_ID  "org.openh4.openh4" CACHE STRING "The app's bundle identifier")
set(H4_APPLE_TEAM ""                  CACHE STRING "Apple development team ID Xcode signs for")

function(h4_apple_bundle target)
  if(IOS)
    set(plist ${PROJECT_SOURCE_DIR}/apps/openh4/apple/Info-ios.plist.in)
  else()
    set(plist ${PROJECT_SOURCE_DIR}/apps/openh4/apple/Info-macos.plist.in)
  endif()
  set(catalog ${PROJECT_SOURCE_DIR}/apps/openh4/apple/Assets.xcassets)
  target_sources(${target} PRIVATE ${catalog})
  set_target_properties(${target} PROPERTIES
    RESOURCE                            ${catalog}
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
    MACOSX_BUNDLE                       TRUE
    MACOSX_BUNDLE_INFO_PLIST            ${plist}
    MACOSX_BUNDLE_BUNDLE_NAME           ${target}
    MACOSX_BUNDLE_GUI_IDENTIFIER        ${H4_BUNDLE_ID}
    MACOSX_BUNDLE_SHORT_VERSION_STRING  ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION        ${PROJECT_VERSION}
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${H4_BUNDLE_ID}
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE     Automatic
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
  if(H4_APPLE_TEAM)
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM    ${H4_APPLE_TEAM}
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY  "Apple Development")
  else()
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")
  endif()
endfunction()
