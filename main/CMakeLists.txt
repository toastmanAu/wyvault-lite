idf_component_register(
    SRCS
        "main.c"
        "crypto.c"
        "storage.c"
        "fingerprint.c"
        "ui.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_hw_support
        esp_system
        mbedtls
        driver
        freertos
        log
        bootloader_support
)
