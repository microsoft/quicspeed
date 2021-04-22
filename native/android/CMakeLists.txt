# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.16)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add_library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

project(quicspeednative)

add_library( Crypto
             STATIC
             IMPORTED )

set_target_properties(Crypto PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/openssl/libcrypto.a)

add_library( OpenSSL
             STATIC
             IMPORTED )

set_target_properties(OpenSSL PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/openssl/libssl.a)


add_library(# Specifies the name of the library.
            msquic

            # Sets the library as a shared library.
            STATIC

            # Provides a relative path to your source file(s).
            ../msquic/src/platform/platform_posix.c 
            ../msquic/src/platform/datapath_epoll.c
            ../msquic/src/platform/hashtable.c
            ../msquic/src/platform/inline.c
            ../msquic/src/platform/pcp.c
            ../msquic/src/platform/platform_posix.c
            ../msquic/src/platform/storage_posix.c
            ../msquic/src/platform/toeplitz.c

            ../msquic/src/platform/tls_openssl.c
            ../msquic/src/platform/cert_capi_openssl.c
            ../msquic/src/platform/selfsign_openssl.c

            ../msquic/src/core/inline.c
            ../msquic/src/core/ack_tracker.c
            ../msquic/src/core/api.c
            ../msquic/src/core/binding.c
            ../msquic/src/core/configuration.c
            ../msquic/src/core/congestion_control.c
            ../msquic/src/core/connection.c
            ../msquic/src/core/crypto.c
            ../msquic/src/core/crypto_tls.c
            ../msquic/src/core/datagram.c
            ../msquic/src/core/frame.c
            ../msquic/src/core/library.c
            ../msquic/src/core/listener.c
            ../msquic/src/core/lookup.c
            ../msquic/src/core/loss_detection.c
            ../msquic/src/core/operation.c
            ../msquic/src/core/packet.c
            ../msquic/src/core/packet_builder.c
            ../msquic/src/core/packet_space.c
            ../msquic/src/core/path.c
            ../msquic/src/core/range.c
            ../msquic/src/core/recv_buffer.c
            ../msquic/src/core/registration.c
            ../msquic/src/core/send.c
            ../msquic/src/core/send_buffer.c
            ../msquic/src/core/sent_packet_metadata.c
            ../msquic/src/core/settings.c
            ../msquic/src/core/stream.c
            ../msquic/src/core/stream_recv.c
            ../msquic/src/core/stream_send.c
            ../msquic/src/core/stream_set.c
            ../msquic/src/core/timer_wheel.c
            ../msquic/src/core/worker.c
            ../msquic/src/core/version_neg.c
            ../msquic/src/core/operation.h
            ../msquic/src/core/stream.h
            ../msquic/src/core/connection.h
             
             )

set(QUIC_COMMON_FLAGS "")
set(QUIC_COMMON_DEFINES _GNU_SOURCE)
set(QUIC_WARNING_FLAGS -Werror -Wall -Wextra -Wformat=2 -Wno-type-limits
    -Wno-unknown-pragmas -Wno-multichar -Wno-missing-field-initializers
    CACHE INTERNAL "")

list(APPEND QUIC_WARNING_FLAGS -Wno-missing-braces -Wno-microsoft-anon-tag -Wno-static-in-inline -Wno-missing-declarations)

list(APPEND QUIC_COMMON_DEFINES QUIC_EVENTS_STUB QUIC_LOGS_STUB)

set(CMAKE_C_FLAGS_DEBUG "-Og -ggdb3")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Ofast ${MARCH} -ggdb3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Ofast ${MARCH} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

list(APPEND QUIC_COMMON_FLAGS -fms-extensions -fPIC)
if (CX_PLATFORM STREQUAL "darwin")
    list(APPEND QUIC_COMMON_DEFINES CX_PLATFORM_DARWIN)
    list(APPEND QUIC_COMMON_FLAGS -Wno-microsoft-anon-tag -Wno-tautological-constant-out-of-range-compare -Wmissing-field-initializers)
else()
    list(APPEND QUIC_COMMON_DEFINES CX_PLATFORM_LINUX)
endif()

set(QUIC_C_FLAGS ${QUIC_COMMON_FLAGS})
set(QUIC_CXX_FLAGS ${QUIC_COMMON_FLAGS})

target_include_directories(msquic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../msquic/src/inc)
target_include_directories (msquic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include)
target_link_libraries(msquic PUBLIC OpenSSL Crypto)

target_compile_features(msquic PUBLIC cxx_std_17)
target_compile_features(msquic PUBLIC c_std_11)

target_compile_definitions(msquic PUBLIC ${QUIC_COMMON_DEFINES})

target_compile_options(msquic PUBLIC ${QUIC_C_FLAGS})
target_compile_options(msquic PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${QUIC_CXX_FLAGS}>)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(msquic PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
target_compile_options(msquic PUBLIC ${QUIC_WARNING_FLAGS})

add_library( # Specifies the name of the library.
             quicspeednative

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ../quicspeed.cpp
             ../linux/init.c )

target_link_libraries(quicspeednative PUBLIC msquic)