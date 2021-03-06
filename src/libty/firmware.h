/* TyTools - public domain
   Niels Martignène <niels.martignene@protonmail.com>
   https://neodd.com/tytools

   This software is in the public domain. Where that dedication is not
   recognized, you are granted a perpetual, irrevocable license to copy,
   distribute, and modify this file as you see fit.

   See the LICENSE file for more details. */

#ifndef TY_FIRMWARE_H
#define TY_FIRMWARE_H

#include "common.h"
#include "class.h"

TY_C_BEGIN

typedef struct ty_firmware {
    unsigned int refcount;

    char *name;
    char *filename;

    uint8_t *image;
    size_t size;
    size_t alloc_size;
} ty_firmware;

typedef struct ty_firmware_format {
    const char *name;
    const char *ext;

    int (*load)(ty_firmware *fw, const uint8_t *mem, size_t len);
} ty_firmware_format;

extern const ty_firmware_format ty_firmware_formats[];
extern const unsigned int ty_firmware_formats_count;

#define TY_FIRMWARE_MAX_SIZE (1024 * 1024)

int ty_firmware_new(const char *filename, ty_firmware **rfw);
int ty_firmware_load_file(const char *filename, FILE *fp, const char *format_name,
                          ty_firmware **rfw);
int ty_firmware_load_mem(const char *filename, const uint8_t *mem, size_t len,
                         const char *format_name, ty_firmware **rfw);

int ty_firmware_load_elf(ty_firmware *fw, const uint8_t *mem, size_t len);
int ty_firmware_load_ihex(ty_firmware *fw, const uint8_t *mem, size_t len);

ty_firmware *ty_firmware_ref(ty_firmware *fw);
void ty_firmware_unref(ty_firmware *fw);

int ty_firmware_expand_image(ty_firmware *fw, size_t size);

unsigned int ty_firmware_identify(const ty_firmware *fw, ty_model *rmodels,
                                  unsigned int max_models);

TY_C_END

#endif
