#ifndef __VERSION_H__
#define __VERSION_H__

#define PLUGIN_NAME "Mupen64Plus Bot Input Plugin"
#define PLUGIN_VERSION 0x000001
#define CORE_API_VERSION 0x020001
#define CONFIG_API_VERSION 0x020301

#define MINIMUM_CORE_VERSION 0x016300
#define INPUT_PLUGIN_API_VERSION 0x020000

#define VERSION_PRINTF_SPLIT(x) (((x) >> 16) & 0xffff), (((x) >> 8) & 0xff), ((x)&0xff)

#endif /* #define __VERSION_H__ */
