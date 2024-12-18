#include "config.h"

t_filesystem_config *create_filesystem_config(char *path_a_config){
    t_filesystem_config *filesystem_config = malloc(sizeof(t_filesystem_config));
    filesystem_config->config = config_create(path_a_config);
    filesystem_config->puerto_esucha = config_get_string_value(filesystem_config->config,"PUERTO_ESCUCHA");
    filesystem_config->mount_dir = config_get_string_value(filesystem_config->config,"MOUNT_DIR");
    filesystem_config->block_size = config_get_int_value(filesystem_config->config,"BLOCK_SIZE");
    filesystem_config->block_count = config_get_int_value(filesystem_config->config,"BLOCK_COUNT");
    filesystem_config->retardo_acceso_bloque = config_get_int_value(filesystem_config->config,"RETARDO_ACCESO_BLOQUE");
    filesystem_config->log_level = config_get_string_value(filesystem_config->config,"LOG_LEVEL");
    filesystem_config->path_bitmap = config_get_string_value(filesystem_config->config,"PATH_BITMAP");
    filesystem_config->path_bloques = config_get_string_value(filesystem_config->config,"PATH_BLOQUES");
    return filesystem_config;
}

void destroy_filesystem_config(t_filesystem_config *filesystem_config){
    free(filesystem_config->puerto_esucha);
    free(filesystem_config->mount_dir);
    free(filesystem_config->log_level);
    config_destroy(filesystem_config->config);
    free(filesystem_config->config);
    free(filesystem_config);
}