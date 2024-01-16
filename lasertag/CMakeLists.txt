add_executable(lasertag.elf
main.c
queue.c
# filter.c
# isr.c
# trigger.c
# transmitter.c
# hitLedTimer.c
# lockoutTimer.c
# buffer.c
# detector.c
# game.c
)

include_directories(. sound)
include_directories(. support)
add_subdirectory(sound)
add_subdirectory(support)
target_link_libraries(lasertag.elf ${330_LIBS} lasertag sound support)
set_target_properties(lasertag.elf PROPERTIES LINKER_LANGUAGE CXX)
