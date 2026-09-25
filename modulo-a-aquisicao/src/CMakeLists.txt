# Componente principal do Módulo A.
# O header comum (contrato) entra tanto aqui quanto via build_flags no
# platformio.ini — redundância intencional para o build funcionar igual
# sob PIO e sob idf.py puro.
idf_component_register(
    SRCS
        "main.c"
        "can_obd2.c"
        "pids.c"
        "consumo.c"
        "enlace_espnow.c"
        "energia.c"
    INCLUDE_DIRS
        "."
        "../../comum"
)
