# CMakeList.txt: PAK-server에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 3.20)

# 지원되는 경우 MSVC 컴파일러에 대해 핫 다시 로드 사용하도록 설정합니다.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PAK-server" LANGUAGES CXX)

# CRT MTd로
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain이 연결되어 있다는 전제에서, Config package를 찾는다.
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(asio-grpc CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system)

# .proto 파일들
set(PROTO_IMPORT_DIR ${CMAKE_CURRENT_LIST_DIR}/proto)

# 생성 파일을 둘 디렉토리(빌드 트리 내부)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# proto + grpc 생성물을 담을 라이브러리 타겟
add_library(PAK_proto STATIC)

# 생성 헤더 include 경로
target_include_directories(PAK_proto PUBLIC ${PROTO_BINARY_DIR})

# 프로젝트를 링크하는 쪽(서버/클라)이 필요한 의존성까지 같이 가져가도록 PUBLIC
target_link_libraries(PAK_proto
  PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# proto 파일 전체 수집 (서브디렉토리 포함)
file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto"
)

# asio-grpc 헬퍼로 proto 코드 생성
# - GENERATE_GRPC: *.grpc.pb.{h,cc}도 함께 생성
# - TARGET: 생성된 소스들을 PAK-server 타겟에 자동으로 붙임
# - OUT_DIR: 생성 파일 위치
asio_grpc_protobuf_generate(
  GENERATE_GRPC
  TARGET PAK_proto
  IMPORT_DIRS ${PROTO_IMPORT_DIR}
  OUT_DIR ${PROTO_BINARY_DIR}
  PROTOS ${PROTO_FILES}
)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (PAK-server "src/PAK-server.cpp" "src/PAK-server.h")

target_link_libraries(PAK-server
  PRIVATE
    PAK_proto
    gRPC::grpc++
    absl::flags
    absl::flags_parse
    asio-grpc::asio-grpc #Boost.Asio 백엔드
)

add_executable(test-client "test/client/test-client.cpp" "test/client/test-client.h")

target_link_libraries(test-client
  PRIVATE
    PAK_proto
    gRPC::grpc++
    absl::flags
    absl::flags_parse
    absl::log_initialize
    absl::check
)

add_executable(udp_test_client "test/client/udp_test_client.cpp")

target_link_libraries(udp_test_client
  PRIVATE
    Boost::system
)

add_executable(udp_test_server "test/server/udp_test_server.cpp")

target_link_libraries(udp_test_server
  PRIVATE
    Boost::system
    PAK_proto
)
