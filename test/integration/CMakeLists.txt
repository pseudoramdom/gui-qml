add_library(bitcoinqml_test_application STATIC fixture/test_application.cpp)
target_include_directories(bitcoinqml_test_application PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/bitcoin/src
  ${PROJECT_BINARY_DIR}/bitcoin/src
)
target_link_libraries(bitcoinqml_test_application PUBLIC bitcoinqml bitcoinqt Qt6::Test)

add_executable(bitcoinqml_integration_tests
  runner/test_main.cpp
  runner/test_cases.cpp
  fixture/test_process.cpp
  stage1_foundation/test_lifecycle.cpp
  stage2_node_setup/test_navigation.cpp
  stage2_node_setup/test_blockclock.cpp
  stage2_node_setup/test_settings.cpp
  stage3_node_tools/test_node.cpp
  stage3_node_tools/test_debug_log.cpp
  stage3_node_tools/test_console.cpp
  stage3_node_tools/test_blocksonly.cpp
  stage4_wallet_management/test_wallet.cpp
  stage5_wallet_activity/test_receive.cpp
  stage6_wallet_sending/test_send.cpp
  stage7_psbt/test_psbt.cpp
  stage8_signers/test_watchonly.cpp
  stage9_payment_import/test_payment_import.cpp
)
target_link_libraries(bitcoinqml_integration_tests PRIVATE bitcoinqml_test_application)
foreach(stage RANGE 1 9)
  add_test(NAME bitcoinqml_integration_stage${stage} COMMAND bitcoinqml_integration_tests
    --stage=${stage}
    --profile-dir=${PROJECT_BINARY_DIR}/test-profiles
    -o ${PROJECT_BINARY_DIR}/test-reports/integration-stage${stage}.xml,junitxml -o -,txt
  )
  set_tests_properties(bitcoinqml_integration_stage${stage} PROPERTIES
    TIMEOUT 1500
    LABELS "integration;stage${stage}"
    ENVIRONMENT "${gui_test_environment}"
  )
endforeach()

if(qt_lib_type STREQUAL "STATIC_LIBRARY")
  qt6_import_plugins(bitcoinqml_integration_tests INCLUDE Qt6::QMinimalIntegrationPlugin Qt6::QSvgPlugin Qt6::QSvgIconPlugin)
  target_link_libraries(bitcoinqml_integration_tests PRIVATE Qt6::LabsPlatformplugin Qt6::qtquickdialogsplugin)
  if(Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
    target_link_libraries(bitcoinqml_integration_tests PRIVATE Qt6::qtqmlcoreplugin)
  else()
    target_link_libraries(bitcoinqml_integration_tests PRIVATE Qt6::qmlsettingsplugin)
  endif()
endif()

add_custom_target(bitcoinqml_integration
  DEPENDS bitcoinqml_integration_tests)
