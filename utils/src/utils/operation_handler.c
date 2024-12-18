
#include "operation_handler.h"

const char *translate_header(uint8_t operation_code)
{
  switch (operation_code)
  {
  case KERNEL:
    return "KERNEL";
    break;
  case CPU:
    return "CPU";
    break;
  case FILESYSTEM:
    return "FILESYSTEM";
    break;
  case MEMORY:
    return "MEMORY";
    break;
  default:
    return "UNKNOWN_OPEARATION";
    break;
  }
}