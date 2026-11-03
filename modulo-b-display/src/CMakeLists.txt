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

# Fontes Orbitron convertidas para LVGL (src/fontes/LEIA-ME.md). Mesmo aviso
# do glob acima: fonte nova na pasta pede Clean.
file(GLOB FONTES_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/fontes/*.c")

idf_component_register(
    SRCS
        "main.c"
        "display_init.c"
        "enlace_espnow.c"
        "estado.c"
        "nvm_config.c"
        "alarme.c"
        "ui_ponte.c"
        "ui_icones.c"
        "ui_detalhe.c"
        "ui_estilo.c"
        "ui_cockpit.c"
        "ui_painel.c"
        "ui_menu.c"
        "ui_tema.c"
        "ui_icones_k.c"
        ${UI_EEZ_SRCS}
        ${FONTES_SRCS}
    INCLUDE_DIRS
        "."
        "ui"
        "../../comum"
)
