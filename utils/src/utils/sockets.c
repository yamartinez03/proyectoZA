#include "sockets.h"

int waiting_client(int server_socket)
{
  struct sockaddr_in client_dir;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int client_socket = accept(server_socket, (void *)&client_dir, &addr_size);
  return client_socket;
}

int server_listen(t_log *logger, char *identifier, int server_socket, void (*handler_connection)(void))
{
  while (1)
  {
    int client_socket = waiting_client(server_socket);

    if (client_socket == -1)
    {
      log_error(logger, "%s Error aceptting client", identifier);
      destroy_socket(server_socket);
      return EXIT_FAILURE;
    }

    t_handler_connection_args *args = malloc(sizeof(t_handler_connection_args));
    args->client_socket = client_socket;
    args->logger = logger;
    args->socket_name = identifier;

    pthread_t service_thread;
    pthread_create(&service_thread, NULL, (void *)handler_connection, (void *)args);
    pthread_detach(service_thread);
  }
  return EXIT_SUCCESS;
}

int create_connection(t_log *logger, const char *identifier, char *ip, char *port, t_socket_type socket_type)
{
  struct addrinfo hints;
  struct addrinfo *server_info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(ip, port, &hints, &server_info);

  int created_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

  if (socket_type == TYPE_SOCKET_SERVER)
  {
    setsockopt(created_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    bind(created_socket, server_info->ai_addr, server_info->ai_addrlen);
    listen(created_socket, SOMAXCONN);
  }
  else if (connect(created_socket, server_info->ai_addr, server_info->ai_addrlen) == -1)
  {
    log_error(logger, "Error trying to connect %s", identifier);
    freeaddrinfo(server_info);
    return EXIT_FAILURE;
  }

  log_info(logger, "%s connected to %s:%s\n", identifier, ip, port);
  freeaddrinfo(server_info);
  return created_socket;
}

void destroy_socket(int socket_to_destroy)
{
  close(socket_to_destroy);
}