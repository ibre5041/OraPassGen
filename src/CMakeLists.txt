
add_executable (dbpass dbpass.cpp dbutils.cpp common.cpp)
target_link_libraries(dbpass ${OPENSSL_LIBRARIES} trotl)

add_executable (genn genn.cpp common.cpp)
target_link_libraries(genn ${OPENSSL_LIBRARIES})