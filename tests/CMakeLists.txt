find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

function(hatsapp_add_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        hatsapp_core
        Qt6::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

hatsapp_add_test(test_url_policy test_url_policy.cpp)
hatsapp_add_test(test_protocol_router test_protocol_router.cpp)
hatsapp_add_test(test_notification_throttler test_notification_throttler.cpp)
hatsapp_add_test(test_unread_badge_parser test_unread_badge_parser.cpp)
