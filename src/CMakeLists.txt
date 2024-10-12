# set the project name
set ( TARGET main )

# add the executable
add_executable(${TARGET}.elf startup.cpp
                            example_simple.cpp
                            example_timer.cpp
                            example_irq.cpp ${TARGET}.cpp)

set_target_properties(${TARGET}.elf PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")

# Post processing command to create a disassembly file
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -S  ${TARGET}.elf > ${TARGET}.disasm
        COMMENT "Invoking: Disassemble")

# Post processing command to create a hex file
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex  ${TARGET}.elf  ${TARGET}.hex
        COMMENT "Invoking: Hexdump")
