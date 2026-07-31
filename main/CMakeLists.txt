# Registers every source file in this component so ESP-IDF's build
# system picks them up. Add new .c files here as you create them.

idf_component_register(
    SRCS
        "app_main.c"
        "tasks/sensor_task.c"
        "tasks/radio_task.c"
        "tasks/power_task.c"
    INCLUDE_DIRS
        "."
        "tasks"
        "common"
)
