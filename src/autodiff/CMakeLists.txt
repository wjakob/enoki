set(ENOKI_AUTODIFF_VARIANTS enoki-autodiff-scalar-f32:float enoki-autodiff-scalar-f64:double)

include_directories(
  ../../ext/enoki-jit/ext/robin_map/include
  ../../ext/enoki-jit/ext/lz4
)

if (ENOKI_ENABLE_JIT)
  set(ENOKI_AUTODIFF_VARIANTS ${ENOKI_AUTODIFF_VARIANTS}
    enoki-autodiff-cuda-f32:CUDAArray<float>
    enoki-autodiff-cuda-f64:CUDAArray<double>
    enoki-autodiff-llvm-f32:LLVMArray<float>
    enoki-autodiff-llvm-f64:LLVMArray<double>)
endif()

foreach(ENOKI_AUTODIFF_VARIANT ${ENOKI_AUTODIFF_VARIANTS})
  string(REPLACE ":" ";" ENOKI_AUTODIFF_VARIANT ${ENOKI_AUTODIFF_VARIANT})
  list(GET ENOKI_AUTODIFF_VARIANT 0 ENOKI_AUTODIFF_VARIANT_NAME)
  list(GET ENOKI_AUTODIFF_VARIANT 1 ENOKI_AUTODIFF_VARIANT_VALUE)
  add_library(${ENOKI_AUTODIFF_VARIANT_NAME} OBJECT
    ../../include/enoki/autodiff.h
    autodiff.cpp
  )
  set_target_properties(${ENOKI_AUTODIFF_VARIANT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set(ENOKI_AUTODIFF_OBJECTS ${ENOKI_AUTODIFF_OBJECTS} $<TARGET_OBJECTS:${ENOKI_AUTODIFF_VARIANT_NAME}>)
  string(REPLACE "enoki-autodiff-" "" ENOKI_AUTODIFF_VARIANT_NAME_U ${ENOKI_AUTODIFF_VARIANT_NAME})
  string(REPLACE "-" "_" ENOKI_AUTODIFF_VARIANT_NAME_U ${ENOKI_AUTODIFF_VARIANT_NAME_U})
  target_compile_definitions(${ENOKI_AUTODIFF_VARIANT_NAME} PRIVATE
    -DENOKI_AUTODIFF_NAME=${ENOKI_AUTODIFF_VARIANT_NAME_U}
    -DENOKI_AUTODIFF_VALUE=${ENOKI_AUTODIFF_VARIANT_VALUE}
    -DENOKI_BUILD_AUTODIFF=1
  )
  if (NOT MSVC)
    target_compile_options(${ENOKI_AUTODIFF_VARIANT_NAME} PRIVATE -fvisibility=hidden)
  endif()
  target_link_libraries(${ENOKI_AUTODIFF_VARIANT_NAME} PRIVATE enoki)
  if (ENOKI_ENABLE_JIT)
    target_link_libraries(${ENOKI_AUTODIFF_VARIANT_NAME} PRIVATE enoki-jit)
    target_compile_definitions(${ENOKI_AUTODIFF_VARIANT_NAME} PRIVATE -DENOKI_ENABLE_JIT=1)
  endif()
endforeach()

add_library(enoki-autodiff SHARED
  ${ENOKI_AUTODIFF_OBJECTS}
  common.h common.cpp
)
target_link_libraries(enoki-autodiff PUBLIC enoki)

if (NOT MSVC)
  target_compile_options(enoki-autodiff PRIVATE -fvisibility=hidden)
endif()

target_compile_definitions(enoki-autodiff PRIVATE -DENOKI_BUILD_AUTODIFF=1)

if (ENOKI_ENABLE_JIT)
  target_link_libraries(enoki-autodiff PRIVATE enoki-jit)
  target_compile_definitions(enoki-autodiff PRIVATE -DENOKI_ENABLE_JIT=1)
endif()
