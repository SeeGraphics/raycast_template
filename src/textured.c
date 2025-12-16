#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAP_WIDTH 10
#define MAP_HEIGHT 10

#define NUM_TEXTURES 3

typedef struct Texture
{
  int width;
  int height;
  Uint32 *pixels;
} Texture;

static const int g_map[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 2, 2, 2, 0, 0, 0, 1},
    {1, 0, 0, 2, 0, 2, 0, 0, 0, 1},
    {1, 0, 0, 2, 2, 2, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 3, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 3, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 3, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

static bool is_walkable(double x, double y)
{
  int mx = (int)x;
  int my = (int)y;
  if (mx < 0 || mx >= MAP_WIDTH || my < 0 || my >= MAP_HEIGHT)
  {
    return false;
  }
  return g_map[my][mx] == 0;
}

static void fill_background(Uint32 *pixels, Uint32 sky, Uint32 floor)
{
  for (int y = 0; y < SCREEN_HEIGHT; ++y)
  {
    Uint32 color = (y < SCREEN_HEIGHT / 2) ? sky : floor;
    int row_start = y * SCREEN_WIDTH;
    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
      pixels[row_start + x] = color;
    }
  }
}

static bool load_texture(const char *path, Texture *out)
{
  SDL_Surface *surface = IMG_Load(path);
  if (!surface)
  {
    fprintf(stderr, "IMG_Load %s failed: %s\n", path, IMG_GetError());
    return false;
  }

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(surface);
  if (!converted)
  {
    fprintf(stderr, "SDL_ConvertSurfaceFormat %s failed: %s\n", path,
            SDL_GetError());
    return false;
  }

  size_t pixel_count = (size_t)converted->w * (size_t)converted->h;
  out->pixels = malloc(pixel_count * sizeof(Uint32));
  if (!out->pixels)
  {
    fprintf(stderr, "Out of memory while loading %s\n", path);
    SDL_FreeSurface(converted);
    return false;
  }

  memcpy(out->pixels, converted->pixels, pixel_count * sizeof(Uint32));
  out->width = converted->w;
  out->height = converted->h;
  SDL_FreeSurface(converted);
  return true;
}

static void unload_texture(Texture *tex)
{
  free(tex->pixels);
  tex->pixels = NULL;
  tex->width = 0;
  tex->height = 0;
}

static void render_frame(Uint32 *pixels, double posX, double posY, double dirX,
                         double dirY, double planeX, double planeY,
                         const Texture *textures, int texture_count)
{
  fill_background(pixels, 0xFF1C1F2B, 0xFF252D2A);

  const Texture *floorTex = (texture_count > 1) ? &textures[1] : NULL;
  const Texture *ceilTex = (texture_count > 2) ? &textures[2] : floorTex;

  for (int x = 0; x < SCREEN_WIDTH; ++x)
  {
    double cameraX = 2.0 * x / (double)SCREEN_WIDTH - 1.0;
    double rayDirX = dirX + planeX * cameraX;
    double rayDirY = dirY + planeY * cameraX;

    int mapX = (int)posX;
    int mapY = (int)posY;

    double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);
    double sideDistX;
    double sideDistY;

    int stepX;
    int stepY;
    if (rayDirX < 0)
    {
      stepX = -1;
      sideDistX = (posX - mapX) * deltaDistX;
    }
    else
    {
      stepX = 1;
      sideDistX = (mapX + 1.0 - posX) * deltaDistX;
    }
    if (rayDirY < 0)
    {
      stepY = -1;
      sideDistY = (posY - mapY) * deltaDistY;
    }
    else
    {
      stepY = 1;
      sideDistY = (mapY + 1.0 - posY) * deltaDistY;
    }

    int side = 0;
    int hit = 0;
    while (!hit)
    {
      if (sideDistX < sideDistY)
      {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      }
      else
      {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
      {
        hit = 1;
      }
      else if (g_map[mapY][mapX] > 0)
      {
        hit = 1;
      }
    }

    double perpWallDist;
    if (side == 0)
    {
      perpWallDist = (mapX - posX + (1 - stepX) / 2.0) / rayDirX;
    }
    else
    {
      perpWallDist = (mapY - posY + (1 - stepY) / 2.0) / rayDirY;
    }

    int lineHeight = (int)(SCREEN_HEIGHT / fmax(perpWallDist, 1e-6));
    int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawStart < 0)
    {
      drawStart = 0;
    }
    int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawEnd >= SCREEN_HEIGHT)
    {
      drawEnd = SCREEN_HEIGHT - 1;
    }

    int tile = (mapY >= 0 && mapY < MAP_HEIGHT && mapX >= 0 && mapX < MAP_WIDTH)
                   ? g_map[mapY][mapX]
                   : 0;
    const Texture *tex =
        (tile > 0 && tile <= texture_count) ? &textures[tile - 1] : NULL;

    Uint32 fallback = 0xFFFFFFFF;
    double wallX;
    if (side == 0)
    {
      wallX = posY + perpWallDist * rayDirY;
    }
    else
    {
      wallX = posX + perpWallDist * rayDirX;
    }
    wallX -= floor(wallX);

    int texX = tex ? (int)(wallX * tex->width) : 0;
    if (tex)
    {
      if (side == 0 && rayDirX > 0)
        texX = tex->width - texX - 1;
      if (side == 1 && rayDirY < 0)
        texX = tex->width - texX - 1;
    }

    double step = tex ? ((double)tex->height / lineHeight) : 0.0;
    double texPos = (drawStart - SCREEN_HEIGHT / 2.0 + lineHeight / 2.0) * step;

    for (int y = drawStart; y <= drawEnd; ++y)
    {
      Uint32 color = fallback;
      if (tex)
      {
        int texY = (int)texPos;
        if (texY < 0)
          texY = 0;
        if (texY >= tex->height)
          texY = tex->height - 1;
        texPos += step;
        color = tex->pixels[texY * tex->width + texX];
      }
      if (side == 1)
      {
        color = ((color & 0xFEFEFE) >> 1) | 0xFF000000;
      }
      pixels[y * SCREEN_WIDTH + x] = color;
    }

    /* Floor & ceiling casting using the hit position for perspective correct
       interpolation. */
    double floorXWall;
    double floorYWall;
    if (side == 0 && rayDirX > 0)
    {
      floorXWall = mapX;
      floorYWall = mapY + wallX;
    }
    else if (side == 0 && rayDirX < 0)
    {
      floorXWall = mapX + 1.0;
      floorYWall = mapY + wallX;
    }
    else if (side == 1 && rayDirY > 0)
    {
      floorXWall = mapX + wallX;
      floorYWall = mapY;
    }
    else
    {
      floorXWall = mapX + wallX;
      floorYWall = mapY + 1.0;
    }

    double distWall = perpWallDist;
    double distPlayer = 0.0;
    int floorStart = drawEnd + 1;
    if (floorStart < 0)
      floorStart = 0;

    for (int y = floorStart; y < SCREEN_HEIGHT; ++y)
    {
      double denom = 2.0 * y - SCREEN_HEIGHT;
      double currentDist = SCREEN_HEIGHT / fmax(denom, 1e-6);
      double weight = (currentDist - distPlayer) / (distWall - distPlayer);
      double currentFloorX = weight * floorXWall + (1.0 - weight) * posX;
      double currentFloorY = weight * floorYWall + (1.0 - weight) * posY;

      Uint32 floorColor = 0xFF444444;
      Uint32 ceilColor = 0xFF222222;
      if (floorTex)
      {
        int texX = (int)(currentFloorX * floorTex->width) % floorTex->width;
        int texY = (int)(currentFloorY * floorTex->height) % floorTex->height;
        if (texX < 0)
          texX += floorTex->width;
        if (texY < 0)
          texY += floorTex->height;
        floorColor = floorTex->pixels[texY * floorTex->width + texX];
      }
      if (ceilTex)
      {
        int texX = (int)(currentFloorX * ceilTex->width) % ceilTex->width;
        int texY = (int)(currentFloorY * ceilTex->height) % ceilTex->height;
        if (texX < 0)
          texX += ceilTex->width;
        if (texY < 0)
          texY += ceilTex->height;
        ceilColor = ceilTex->pixels[texY * ceilTex->width + texX];
      }

      pixels[y * SCREEN_WIDTH + x] = floorColor;
      int ceilY = SCREEN_HEIGHT - y - 1;
      if (ceilY >= 0)
        pixels[ceilY * SCREEN_WIDTH + x] = ceilColor;
    }
  }
}

int main(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }

  int img_flags = IMG_INIT_PNG;
  if ((IMG_Init(img_flags) & img_flags) != img_flags)
  {
    fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
    SDL_Quit();
    return 1;
  }

  Texture textures[NUM_TEXTURES] = {0};
  const char *texture_files[NUM_TEXTURES] = {
      "assets/sides/brick.png",
      "assets/sides/wood.png",
      "assets/sides/eagle.png",
  };
  for (int i = 0; i < NUM_TEXTURES; ++i)
  {
    if (!load_texture(texture_files[i], &textures[i]))
    {
      fprintf(stderr, "Failed to load texture %s\n", texture_files[i]);
      for (int j = 0; j <= i; ++j)
      {
        unload_texture(&textures[j]);
      }
      IMG_Quit();
      SDL_Quit();
      return 1;
    }
  }

  SDL_Window *window = SDL_CreateWindow(
      "Raycaster (Textured)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
  if (!window)
  {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    for (int i = 0; i < NUM_TEXTURES; ++i)
      unload_texture(&textures[i]);
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
  {
    fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    for (int i = 0; i < NUM_TEXTURES; ++i)
      unload_texture(&textures[i]);
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Texture *framebuffer =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                        SCREEN_HEIGHT);
  if (!framebuffer)
  {
    fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    for (int i = 0; i < NUM_TEXTURES; ++i)
      unload_texture(&textures[i]);
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  Uint32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

  double posX = 2.5;
  double posY = 2.5;
  double dirX = 1.0;
  double dirY = 0.0;
  double planeX = 0.0;
  double planeY = 0.66;

  Uint32 last_ticks = SDL_GetTicks();
  bool running = true;
  while (running)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
      else if (event.type == SDL_KEYDOWN &&
               event.key.keysym.sym == SDLK_ESCAPE)
      {
        running = false;
      }
    }

    Uint32 now = SDL_GetTicks();
    double frameTime = (now - last_ticks) / 1000.0;
    last_ticks = now;

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    double moveSpeed = 2.5 * frameTime;
    double rotSpeed = 1.5 * frameTime;

    if (state[SDL_SCANCODE_UP])
    {
      double newX = posX + dirX * moveSpeed;
      double newY = posY + dirY * moveSpeed;
      if (is_walkable(newX, posY))
        posX = newX;
      if (is_walkable(posX, newY))
        posY = newY;
    }
    if (state[SDL_SCANCODE_DOWN])
    {
      double newX = posX - dirX * moveSpeed;
      double newY = posY - dirY * moveSpeed;
      if (is_walkable(newX, posY))
        posX = newX;
      if (is_walkable(posX, newY))
        posY = newY;
    }
    if (state[SDL_SCANCODE_RIGHT])
    {
      double oldDirX = dirX;
      dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
      dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
      double oldPlaneX = planeX;
      planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
      planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
    }
    if (state[SDL_SCANCODE_LEFT])
    {
      double oldDirX = dirX;
      dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
      dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
      double oldPlaneX = planeX;
      planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
      planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
    }

    render_frame(pixels, posX, posY, dirX, dirY, planeX, planeY, textures,
                 NUM_TEXTURES);

    SDL_UpdateTexture(framebuffer, NULL, pixels,
                      SCREEN_WIDTH * (int)sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(framebuffer);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  for (int i = 0; i < NUM_TEXTURES; ++i)
    unload_texture(&textures[i]);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
