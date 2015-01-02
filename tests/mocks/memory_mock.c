#include "memory_mock.h"

uint8_t memory_mock_config1[256];
uint8_t memory_mock_config2[256];
uint8_t memory_mock_app[42];

uint8_t config_page_buffer[256];
const size_t config_page_size = sizeof(config_page_buffer);

void *memory_get_app_addr(void)
{
    return &memory_mock_app[0];
}

void *memory_get_config1_addr(void)
{
    return &memory_mock_config1[0];
}

void *memory_get_config2_addr(void)
{
    return &memory_mock_config2[0];
}

