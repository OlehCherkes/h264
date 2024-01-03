idf_component_register(SRCS "ESP_FS.c"
                    INCLUDE_DIRS "."
                    REQUIRES spiffs)
spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)

