#include <../include/config.h>

//pene erecto
t_kernel_config *create_kernel_config(char *path_a_config){
    t_kernel_config *kernel_config = malloc(sizeof(t_kernel_config));
    kernel_config->config = config_create(path_a_config);
    kernel_config->memory_ip = config_get_string_value(kernel_config->config,"IP_MEMORIA");
    kernel_config->memory_port = config_get_string_value(kernel_config->config,"MEMORY_PORT");
    kernel_config->cpu_ip = config_get_string_value(kernel_config->config,"IP_CPU");
    kernel_config->cpu_dispatch_port = config_get_string_value(kernel_config->config,"CPU_DISPATCH_PORT");
    kernel_config->cpu_interrupt_port = config_get_string_value(kernel_config->config,"CPU_INTERRUPT_PORT");
    kernel_config->planning_algorithm = config_get_string_value(kernel_config->config,"ALGORITMO_PLANIFICACION");
    kernel_config->quantum = config_get_int_value(kernel_config->config,"QUANTUM");
    kernel_config->log_level = config_get_string_value(kernel_config->config,"LOG_LEVEL");

    return kernel_config;
}
//pene erecto
void destroy_kernel_config(t_kernel_config *kernel_config){
    free(kernel_config->memory_ip);
    free(kernel_config->memory_port);
    free(kernel_config->cpu_dispatch_port);
    free(kernel_config->cpu_interrupt_port);
    free(kernel_config->log_level);
    free(kernel_config->cpu_ip);
    free(kernel_config->planning_algorithm);
    config_destroy(kernel_config->config);
    free(kernel_config->config);
}