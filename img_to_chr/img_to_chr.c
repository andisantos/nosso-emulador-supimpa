#include <stdio.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "md5.h"

// TODO: attribute table, support for 13 colors

int grid_pos = 0;
char *grid;
int num_colors = 0;
int color_map[4] = {-1};
struct hash{
  int h0, h1, h2, h3;
};
char nametable[1024] = {0};

const int nes_palette[64] = {
  0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
  0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
  0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
  0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
  0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
  0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
  0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
  0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000,
};

float color_distance(int a, int b) {
  int ar = (a >> 16) & 0xff;
  int ag = (a >>  8) & 0xff;
  int ab = (a >>  0) & 0xff;

  int br = (b >> 16) & 0xff;
  int bg = (b >>  8) & 0xff;
  int bb = (b >>  0) & 0xff;

  int dist_r = ar - br;
  int dist_g = ag - bg;
  int dist_b = ab - bb;

  return sqrt(dist_r * dist_r + dist_g * dist_g + dist_b * dist_b);
}

int tile_to_grid(unsigned char *data) {
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      int r = data[3*8*j + 3*i + 0];
      int g = data[3*8*j + 3*i + 1];
      int b = data[3*8*j + 3*i + 2];
      int color = (r << 16) | (g << 8) | (b << 0);

      float min_dist = color_distance(color, nes_palette[0]);
      int min_dist_index = 0;
      for (int j = 1; j < 64; j++) {
        float candidate = color_distance(color, nes_palette[j]);
        if (candidate < min_dist) {
          min_dist = candidate;
          min_dist_index = j;
        }
      }
      if (min_dist_index == 0x0D) min_dist_index = 0x0F;

      int index = -1;
      for (int k = 0; k < 4; k++) {
        if (color_map[k] == min_dist_index) {
          index = k;
        }
      }

      if (index == -1) {
        if (num_colors >= 4) {
          fprintf(stderr, "Error: Too many colors. Maximum 4.\n");
          exit(1);
        }
        color_map[num_colors] = min_dist_index;
        index = num_colors;
        num_colors++;
      }

      //printf("%d ", index);

      char lower = 0b01 & index;
      char upper = (0b10 & index) >> 1;

      grid[grid_pos] = (grid[grid_pos] << 1) | lower;
      grid[grid_pos+8] = (grid[grid_pos+8] << 1) | upper;

      if (i % 8 == 7) {
        grid_pos++;
        if (grid_pos % 8 == 0) {
          grid_pos += 8;
        }
      }
    }
    //printf("\n");
  }
  //printf("\n");
}

int main(int argc, char **argv) {

  if (argc < 3) {
    printf("Usage: ./img_to_chr.x input_image output.chr [first_color_hex]\n");
    printf("Options:\n");
    printf("  [first_color_hex]\t\tThis color will be color 0 of the palette\n");
    exit(0);
  }

  if (argc > 3) {
    num_colors = 1;
    int color = strtol(argv[3], NULL, 16);

    float min_dist = color_distance(color, nes_palette[0]);
    int min_dist_index = 0;
    for (int j = 1; j < 64; j++) {
      float candidate = color_distance(color, nes_palette[j]);
      if (candidate < min_dist) {
        min_dist = candidate;
        min_dist_index = j;
      }
    }
    if (min_dist_index == 0x0D) min_dist_index = 0x0F;
    color_map[0] = min_dist_index;
  }

  struct hash hashes[512] = {0};

  int w, h, n;
  unsigned char *data = stbi_load(argv[1], &w, &h, &n, 3);
  if (w > 256 || h > 240) {
    fprintf(stderr, "Error: Image too large. Maximum size is 256x240\n");
    exit(1);
  }

  grid = malloc(w*h/4);

  int current_tile = 0;
  unsigned char tile_data[64*3] = {-1};
  for (int j = 0; j < h; j+=8) {
    for (int i = 0; i < w; i+=8) {
      for (int l = j; l < j+8; l++) {
        for (int k = i; k < i+8; k++) {
          tile_data[(l%8)*8*3+(k%8)*3+0] = data[l*w*3+k*3+0];
          tile_data[(l%8)*8*3+(k%8)*3+1] = data[l*w*3+k*3+1];
          tile_data[(l%8)*8*3+(k%8)*3+2] = data[l*w*3+k*3+2];
        }
      }
      md5(tile_data,64*3);
      int exists = 0;
      for (int l = 0; l < current_tile; l++) {
        if (hashes[l].h0 == h0 && hashes[l].h1 == h1 &&
            hashes[l].h2 == h2 && hashes[l].h3 == h3) {
          exists = 1;
          nametable[(w/8)*(j/8)+i/8] = l;
          break;
        }
      }
      if (!exists) {
        nametable[(w/8)*(j/8)+i/8] = current_tile;
        hashes[current_tile].h0 = h0;
        hashes[current_tile].h1 = h1;
        hashes[current_tile].h2 = h2;
        hashes[current_tile].h3 = h3;
        current_tile++;
        if (current_tile >= 256) {
          fprintf(stderr, "Error: Too many unique tiles.\n");
          exit(1);
        }
        tile_to_grid(tile_data);
      }
    }
  }

  FILE *out = fopen(argv[2], "wb");
  fwrite(grid, current_tile*16, 1, out);
  fclose(out);

  free(grid);
  printf("Palette: \n");
  printf(".byte ");
  for (int i = 0; i < 4; i++) {
    printf("$%02X", color_map[i]);
    if (i != 3) printf(",");
  }
  printf("\n");
  printf("Nametable: \n");
  for (int i = 0; i < w*h/64; i += 16) {
    printf(".byte ");
    for (int j = 0; j < 16; j++) {
      printf("%02X", nametable[i+j]);
      if (j != 15) printf(",");
    }
    printf("\n");
  }

  return 0;
}
