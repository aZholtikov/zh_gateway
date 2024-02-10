idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(SRCS "zh_gateway.c" INCLUDE_DIRS "" EMBED_TXTFILES ${project_dir}/certificate.pem)