if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
    message(FATAL_ERROR "INPUT, OUTPUT and SYMBOL are required")
endif()

file(READ "${INPUT}" HEX_CONTENT HEX)
string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
set(BYTES "")
math(EXPR LAST "${HEX_LENGTH} - 2")
foreach(I RANGE 0 ${LAST} 2)
    string(SUBSTRING "${HEX_CONTENT}" ${I} 2 BYTE)
    string(APPEND BYTES "0x${BYTE},")
endforeach()

file(WRITE "${OUTPUT}" "#pragma once\n#include <cstddef>\n#include <cstdint>\nalignas(4) static constexpr std::uint8_t ${SYMBOL}[] = {${BYTES}};\nstatic constexpr std::size_t ${SYMBOL}Size = sizeof(${SYMBOL});\n")
