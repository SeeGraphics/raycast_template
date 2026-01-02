#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAP_WIDTH 10
#define MAP_HEIGHT 10

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
      pixels[row_start + x] = color; /* 1D indexing into the pixel buffer */
    }
  }
}

static void render_frame(Uint32 *pixels, double posX, double posY, double dirX,
                         double dirY, double planeX, double planeY)
{
  fill_background(pixels, 0xFF1C1F2B, 0xFF252D2A);

  const Uint32 wall_colors[] = {
      0xFF9B1B30, /* red */
      0xFF2F80ED, /* blue */
      0xFF00B894, /* teal */
      0xFFF2C94C  /* yellow */
  };

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
    Uint32 color = wall_colors[(tile - 1) % 4];
    if (side == 1)
    {
      color = ((color & 0xFEFEFE) >> 1) |
              0xFF000000; /* simple shading for y side */
    }

    for (int y = drawStart; y <= drawEnd; ++y)
    {
      pixels[y * SCREEN_WIDTH + x] =
          color; /* 1D indexing into the pixel buffer */
    }
  }
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
      SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
  if (!window)
  {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
  {
    fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!texture)
  {
    fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
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

    render_frame(pixels, posX, posY, dirX, dirY, planeX, planeY);

    SDL_UpdateTexture(texture, NULL, pixels,
                      SCREEN_WIDTH * (int)sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
