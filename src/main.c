#include <stdint.h>
#include "SMSlib.h"
#ifdef USEPSGLIB // Set USEPSGLIB := true in Makefile to build with PSGlib for music and sounds
#include "PSGlib.h"
#endif
#include "assets.generated.h"

SMS_EMBED_SEGA_ROM_HEADER(0, 0); //  includes the TMR SEGA header in the ROM image

#define BOTTOM_BORDER 0x01
#define RIGHT_BORDER 0x02
#define UNVISITED 0x04
#define MAZE_GENERATION_TRACKBACK 0
#define MAZE_GENERATION_RANDOM 1
#define MAZE_GENERATION_RESTART 2
#define MAZE_GENERATION MAZE_GENERATION_TRACKBACK

uint8_t paused = 0;
uint8_t player_x = 0;
uint8_t player_y = 11;
uint8_t maze[32][24];

uint16_t get_random(void)
{
  static uint16_t random = 1;
  uint8_t lsb = random & 1;
  random >>= 1;
  if (lsb == 1)
    random ^= 0xb400u;
  return random;
}

void generate_maze(void)
{
  unsigned int move_count = 0;
  uint16_t moves[768];
  uint16_t move_idx = 0;
  uint16_t goal_idx = move_idx;
  uint16_t random;

  for (int x = 0; x < 32; x++)
  {
    for (int y = 0; y < 24; y++)
    {
      maze[x][y] = 3 | UNVISITED;
      SMS_setTileatXY(x, y, maze[x][y]);
    }
  }

  player_x = 0;
  player_y = 11;

  moves[move_idx] = player_x + (32 * player_y);
  move_idx += 1;

  while (1)
  {
    // check if we are stuck and if so backtrack
    if (!(
            ((player_y > 0) && (maze[player_x][player_y - 1] & UNVISITED)) ||
            ((player_y < 23) && (maze[player_x][player_y + 1] & UNVISITED)) ||
            ((player_x > 0) && (maze[player_x - 1][player_y] & UNVISITED)) ||
            ((player_x < 31) && (maze[player_x + 1][player_y] & UNVISITED))))
    {
      if (move_idx == 0)
        break;

#if MAZE_GENERATION == MAZE_GENERATION_RANDOM
      random = get_random() % move_idx;
      player_x = moves[random] % 32;
      player_y = moves[random] / 32;
      moves[random] = moves[move_idx];
      move_idx -= 1;
#elif MAZE_GENERATION == MAZE_GENERATION_RESTART
      player_x = moves[0] % 32;
      player_y = moves[0] / 32;
      moves[0] = moves[move_idx];
      move_idx -= 1;
#else
      move_idx -= 1;
      player_x = moves[move_idx] % 32;
      player_y = moves[move_idx] / 32;
#endif
    }

    random = get_random() % 4;

    switch (random)
    {
    case 0: // move up
      if (!((player_y > 0) && (maze[player_x][player_y - 1] & UNVISITED)))
        continue;
      player_y -= 1;
      maze[player_x][player_y] &= ~BOTTOM_BORDER;
      SMS_setTileatXY(player_x, player_y, maze[player_x][player_y]);
      break;
    case 1: // move down
      if (!((player_y < 23) && (maze[player_x][player_y + 1] & UNVISITED)))
        continue;
      maze[player_x][player_y] &= ~BOTTOM_BORDER;
      SMS_setTileatXY(player_x, player_y, maze[player_x][player_y]);
      player_y += 1;
      break;
    case 2: // move left
      if (!((player_x > 0) && (maze[player_x - 1][player_y] & UNVISITED)))
        continue;
      player_x -= 1;
      maze[player_x][player_y] &= ~RIGHT_BORDER;
      SMS_setTileatXY(player_x, player_y, maze[player_x][player_y]);
      break;
    case 3: // move right
      if (!((player_x < 31) && (maze[player_x + 1][player_y] & UNVISITED)))
        continue;
      maze[player_x][player_y] &= ~RIGHT_BORDER;
      SMS_setTileatXY(player_x, player_y, maze[player_x][player_y]);
      player_x += 1;
      break;
    }
    moves[move_idx] = player_x + (32 * player_y);
    move_idx += 1;
    if (move_idx > goal_idx)
    {
      goal_idx = move_idx - 1;
    }
    maze[player_x][player_y] &= ~UNVISITED;
    SMS_setTileatXY(player_x, player_y, maze[player_x][player_y]);
    SMS_initSprites();
    SMS_addSprite(player_x * 8, player_y * 8, 0);
    // SMS_waitForVBlank();
    SMS_copySpritestoSAT();
  }
  // display a token tile near the bottom of the screen
  SMS_setTileatXY(moves[goal_idx] % 32, moves[goal_idx] / 32, 4);

  player_x = 0;
  player_y = 11;
}

void main(void)
{
  uint8_t keys;

  // load the background tiles
  SMS_loadBGPalette(background_palette);
  SMS_loadTiles(background_tiles, 0, background_tiles_size);
  // clear the screen
  SMS_setNextTileatLoc(0);

  // load the sprite tiles
  SMS_loadSpritePalette(sprites_palette);
  SMS_loadTiles(sprites_tiles, 256, sprites_tiles_size);

  SMS_setBackdropColor(1);

  SMS_displayOn();
  generate_maze();

  // main game loop
  while (1)
  {
    SMS_waitForVBlank();

#ifdef USEPSGLIB
    if (!paused)
    {
      PSGFrame(); // if using PSGlib you must call PSGFrame at *regular* intervals, e.g. here
    }
#endif
    SMS_copySpritestoSAT();
    if (SMS_queryPauseRequested())
    {
      paused = !paused;
      SMS_resetPauseRequest();
    }

    if (!paused)
    {
      keys = SMS_getKeysPressed();

      if ((keys & PORT_A_KEY_LEFT) && (player_x > 0) && !(maze[player_x - 1][player_y] & RIGHT_BORDER))
        player_x -= 1;
      else if (keys & PORT_A_KEY_RIGHT && (player_x < 31) && !(maze[player_x][player_y] & RIGHT_BORDER))
        player_x += 1;

      if (keys & PORT_A_KEY_UP && (player_y > 0) && !(maze[player_x][player_y - 1] & BOTTOM_BORDER))
        player_y -= 1;
      else if (keys & PORT_A_KEY_DOWN && (player_y < 23) && !(maze[player_x][player_y] & BOTTOM_BORDER))
        player_y += 1;

      // draw the sprites
      SMS_initSprites();
      SMS_addSprite(player_x * 8, player_y * 8, 0);
    }
  }
}
