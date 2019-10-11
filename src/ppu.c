// PPUCTRL
#define GET_BASE_NAMETABLE_ID() (ppu.ctrl & 0b11)
#define SET_BASE_NAMETABLE_ID(a) (ppu.ctrl = (ppu.ctrl & (~0b11)) | (a & 0b11))
#define NAMETABLE_ID_TO_ADDRS(a) ( \
      a == 0 ? 0x2000 : ( \
      a == 1 ? 0x2400 : ( \
      a == 2 ? 0x2800 : 0x2C00 )) \
    )

#define GET_VRAM_PPU_INCREMENT() (ppu.ctrl & BIT2)
#define SET_VRAM_PPU_INCREMENT(a) (ppu.ctrl = (ppu.ctrl & (~BIT2)) | (a & BIT0))

#define GET_SPRITE_PATTERN_TABLE_ID() (ppu.ctrl & BIT3)
#define SET_SPRITE_PATTERN_TABLE_ID(a) (ppu.ctrl = (ppu.ctrl & (~BIT3)) | (a & BIT0))

#define GET_BACKGROUND_PATTERN_TABLE_ID() (ppu.ctrl & BIT4)
#define SET_BACKGROUND_PATTERN_TABLE_ID(a) (ppu.ctrl = (ppu.ctrl & (~BIT4)) | (a & BIT0))
#define PATTERN_ID_TO_ADDRS(a) (a == 0 ? 0 : 0x1000)

#define GET_SPRITE_SIZE() (ppu.ctrl & BIT5)
#define SET_SPRITE_SIZE(a) (ppu.ctrl = (ppu.ctrl & (~BIT5)) | (a & BIT0))

#define GET_PPU_MASTER_SELECT() (ppu.ctrl & BIT6)
#define SET_PPU_MASTER_SELECT(a) (ppu.ctrl = (ppu.ctrl & (~BIT6) | (a & BIT0)))

#define GET_NMI_ENABLE() (ppu.ctrl & BIT7)
#define SET_NMI_ENABLE(a) (ppu.ctrl = (ppu.ctrl & (~BIT7)) | (a & BIT0))

// PPUMASK
#define GET_GRAYSCALE_ENABLE() (ppu.mask & BIT0)
#define SET_GRAYSCALE_ENABLE(a) (ppu.mask = (ppu.mask & (~BIT0)) | (a & BIT0))

#define GET_BACKGROUND_MASK() (ppu.mask & BIT1)
#define SET_BACKGROUND_MASK(a) (ppu.mask = (ppu.mask & (~BIT1)) | (a & BIT0))

#define GET_SPRITE_MASK() (ppu.mask & BIT2)
#define SET_SPRITE_MASK(a) (ppu.mask = (ppu.mask & (~BIT2)) | (a & BIT0))

#define GET_BACKGROUND_ENABLE() (ppu.mask & BIT3)
#define SET_BACKGROUND_ENABLE(a) (ppu.mask = (ppu.mask & (~BIT3)) | (a & BIT0))

#define GET_SPRITE_ENABLE() (ppu.mask & BIT4)
#define SET_SPRITE_ENABLE(a) (ppu.mask = (ppu.mask & (~BIT4)) | (a & BIT0))

#define GET_RED_FILTER() (ppu.mask & BIT5)
#define SET_RED_FILTER(a) (ppu.mask = (ppu.mask & (~BIT5)) | (a & BIT0))

#define GET_GREEN_FILTER() (ppu.mask & BIT6)
#define SET_GREEN_FILTER(a) (ppu.mask = (ppu.mask & (~BIT6)) | (a & BIT0))

#define GET_BLUE_FILTER() (ppu.mask & BIT7)
#define SET_BLUE_FILTER(a) (ppu.mask = (ppu.mask & (~BIT7)) | (a & BIT0))

// PPUSTATUS
// TODO: Implement the 5 least significant bits
#define GET_SPRITE_OVERFLOW() (ppu.status & BIT5)
#define GET_SPRITE_0_HIT() (ppu.status & BIT6)
#define GET_VBLANK_START() (ppu.status & BIT7)

#define GET_ATTRIBUTETABLE_ADDRS(a) ( \
      a == 0 ? 0x23C0 : ( \
      a == 1 ? 0x27C0 : ( \
      a == 2 ? 0x2BC0 : 0x2FC0 )) \
    )

#define PALETTE_ID_TO_ADDRS(a) ( \
      a == 0 ? 0x3F00 : ( \
      a == 1 ? 0x3F04 : ( \
      a == 2 ? 0x3F08 : 0x3F0C )) \
    )

void oamDMA(uint8_t hibyte) {
  uint16_t addrs = (hibyte << 8);

  for (int i = ppu.oam_addrs; i < 0x100; i++) {
    ppu.oam[i] = readCPUByte(addrs);
  }

  if (cpu.clock_cycles % 2) cpu.clock_cycles += 1;
  cpu.clock_cycles += 513;
}

void decodeTile(uint8_t tile[16], uint8_t decoded_tile[64]) {

  for (int byte = 0; byte < 8; byte++) {
    for (int bit = 0; bit < 8; bit++) {
      decoded_tile[byte*8 + bit] = (tile[byte] & (1 << bit)) | (tile[byte + 8] & (1 << bit)); 
    }
  }
}

uint8_t backgroudPixelColor(uint8_t x, uint8_t y) {

  uint16_t addrs_nametable = NAMETABLE_ID_TO_ADDRS(GET_BASE_NAMETABLE_ID());
  uint8_t tile_x = x/8;
  uint8_t tile_y = y/8;
  uint8_t pattern_id = readPPUByte(addrs_nametable + 32*tile_y + tile_x);
  uint16_t addrs_patterntable = PATTERN_ID_TO_ADDRS(GET_BACKGROUND_PATTERN_TABLE_ID());
  uint8_t tile[16];
  uint8_t decoded_tile[64];

  for (int i = 0; i < 16; i++) {
    tile[i] = readPPUByte(addrs_patterntable + 16*pattern_id + i);
  }
  decodeTile(tile, decoded_tile);

  uint16_t addrs_attributetable = GET_ATTRIBUTETABLE_ADDRS(GET_BASE_NAMETABLE_ID());
  uint8_t attribute_tile_x = tile_x/4;
  uint8_t attribute_tile_y = tile_y/4;
  uint8_t attribute_tile = readPPUByte(addrs_attributetable + 8*attribute_tile_y + attribute_tile_x);
  tile_x = tile_x % 4;
  tile_y = tile_y % 4;

  uint8_t palette_id;

  if (tile_x < 2 && tile_y < 2) palette_id = attribute_tile & 0b11;
  else if (tile_x >= 2 && tile_y < 2) palette_id = attribute_tile & 0b1100;
  else if (tile_x < 2 && tile_y >= 2) palette_id = attribute_tile & 0b110000;
  else palette_id = attribute_tile & 0b11000000;

  uint16_t addrs_palette = PALETTE_ID_TO_ADDRS(palette_id);
  x = x % 8;
  y = y % 8;

  uint8_t pixel_palette = decoded_tile[y*8 + x];

  if (pixel_palette == 0) return readPPUByte(0x3F00);
  return readPPUByte(addrs_palette + pixel_palette);
}
