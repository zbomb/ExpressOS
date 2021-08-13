/*==============================================================
    Axon Kernel - PC Screen Font Format
    2021, Zachary Berry
    axon/private/axon/gfx/psf1.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"


/*
    Constants
*/
#define PSF1_FONT_MAGIC 0x0436

/*
    Structures
*/
struct axk_psf1_header_t
{
    uint16_t magic;
    uint8_t mode;
    uint8_t glyph_sz;
};

struct axk_psf1_t
{
    struct axk_psf1_header_t header;
    uint8_t* glyph_data;
};