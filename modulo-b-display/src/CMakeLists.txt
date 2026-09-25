# Componente principal do Módulo B.
#
# Os fontes de src/ui/ são GERADOS pelo EEZ Studio e o conjunto de arquivos
# muda conforme o projeto (images.c, styles.c, fontes...). Por isso entram
# por glob com CONFIGURE_DEPENDS: arquivo novo exportado pelo EEZ é visto no
# próximo build sem mexer neste CMakeLists.
file(GLOB UI_EEZ_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.c")

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
