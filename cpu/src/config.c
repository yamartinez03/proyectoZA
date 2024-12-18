#include "config.h"


t_cpu_config *create_cpu_config(char *path_a_config){
    t_cpu_config *cpu_config = malloc(sizeof(t_cpu_config));
    cpu_config->config = config_create(path_a_config);
    cpu_config->memory_ip = config_get_string_value(cpu_config->config,"IP_MEMORIA");
    cpu_config->memory_port = config_get_string_value(cpu_config->config,"PUERTO_MEMORIA");
    cpu_config->listen_dispatch_port = config_get_string_value(cpu_config->config,"PUERTO_ESCUCHA_DISPATCH");
    cpu_config->listen_interrupt_port = config_get_string_value(cpu_config->config,"PUERTO_ESCUCHA_INTERRUPT");
    cpu_config->log_level = config_get_string_value(cpu_config->config,"LOG_LEVEL");

    return cpu_config;
}

void destroy_cpu_config(t_cpu_config *cpu_config){
    free(cpu_config->memory_ip);
    free(cpu_config->memory_port);
    free(cpu_config->listen_dispatch_port);
    free(cpu_config->listen_interrupt_port);
    free(cpu_config->log_level);
    config_destroy(cpu_config->config);
    free(cpu_config->config);
}