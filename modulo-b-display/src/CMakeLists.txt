# Componente principal do Módulo B.
#
# Os fontes de src/ui/ são GERADOS pelo EEZ Studio e o conjunto de arquivos
# muda conforme o projeto (images.c, styles.c, fontes...). Por isso entram
# por glob, sem listar um a um.
#
# Sem CONFIGURE_DEPENDS: o ESP-IDF também avalia este arquivo em modo script
# (para levantar dependências), onde essa opção é inválida. Consequência:
# quando o EEZ criar um .c NOVO, rode "Clean" no PlatformIO antes do build
# para o CMake reler a pasta. Alterar arquivos que já existem não precisa.
file(GLOB UI_EEZ_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.c")

idf_component_register(
    SRCS
        "main.c"
        "display_init.c"
        "enlace_espnow.c"
        "estado.c"
        "nvm_config.c"
        "alarme.c"
        "ui_ponte.c"
        ${UI_EEZ_SRCS}
    INCLUDE_DIRS
        "."
        "ui"
        "../../comum"
)
