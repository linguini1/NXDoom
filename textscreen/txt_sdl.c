/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_sdl.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Text mode emulation in SDL
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"

#include "txt_main.h"
#include "txt_sdl.h"
#include "txt_utf8.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Time between character blinks in ms */

#define BLINK_PERIOD 250

/* XXX: duplicate from doomtype.h */

#define arrlen(array) (sizeof(array) / sizeof(*array))

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  const char *name;
  const uint8_t *data;
  unsigned int w;
  unsigned int h;
} txt_font_t;

/* Fonts: */

#include "fonts/codepage.h"
#include "fonts/large.h"
#include "fonts/normal.h"
#include "fonts/small.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if 0
SDL_Window *TXT_SDLWindow;
static SDL_Surface *screenbuffer;
static SDL_Renderer *renderer;
#endif

static unsigned char *screendata;

#if 0
/* Current input mode. */

static txt_input_mode_t input_mode = TXT_INPUT_NORMAL;

/* Dimensions of the screen image in screen coordinates (not pixels); this
 * is the value that was passed to SDL_CreateWindow().
 */

static int screen_image_w, screen_image_h;

static TxtSDLEventCallbackFunc event_callback;
static void *event_callback_data;

/* Font we are using: */

static const txt_font_t *font;

/* Dummy "font" that means to try highdpi rendering, or fallback to
 * normal_font otherwise.
 */

static const txt_font_t highdpi_font = { "normal-highdpi", NULL, 8, 16 };

/* Mapping from SDL keyboard scancode to internal key code. */

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

/* String names of keys. This is a fallback; we usually use the SDL API. */

static const struct
{
  int key;
  const char *name;
} key_names[] = KEY_NAMES_ARRAY;
#endif

/* Unicode key mapping; see codepage.h. */

static const short g_code_page_to_unicode[] = CODE_PAGE_TO_UNICODE;

#if 0
static const SDL_Color ega_colors[] =
{
  {0x00, 0x00, 0x00, 0xff},          /* 0: Black */
  {0x00, 0x00, 0xaa, 0xff},          /* 1: Blue */
  {0x00, 0xaa, 0x00, 0xff},          /* 2: Green */
  {0x00, 0xaa, 0xaa, 0xff},          /* 3: Cyan */
  {0xaa, 0x00, 0x00, 0xff},          /* 4: Red */
  {0xaa, 0x00, 0xaa, 0xff},          /* 5: Magenta */
  {0xaa, 0x55, 0x00, 0xff},          /* 6: Brown */
  {0xaa, 0xaa, 0xaa, 0xff},          /* 7: Grey */
  {0x55, 0x55, 0x55, 0xff},          /* 8: Dark grey */
  {0x55, 0x55, 0xff, 0xff},          /* 9: Bright blue */
  {0x55, 0xff, 0x55, 0xff},          /* 10: Bright green */
  {0x55, 0xff, 0xff, 0xff},          /* 11: Bright cyan */
  {0xff, 0x55, 0x55, 0xff},          /* 12: Bright red */
  {0xff, 0x55, 0xff, 0xff},          /* 13: Bright magenta */
  {0xff, 0xff, 0x55, 0xff},          /* 14: Yellow */
  {0xff, 0xff, 0xff, 0xff},          /* 15: Bright white */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if 0
static inline void update_character(int x, int y)
{
  unsigned char character;
  const uint8_t *p;
  unsigned char *s, *s1;
  unsigned int bit;
  int bg, fg;
  unsigned int x1, y1;

  p = &screendata[(y * TXT_SCREEN_W + x) * 2];
  character = p[0];

  fg = p[1] & 0xf;
  bg = (p[1] >> 4) & 0xf;

  if (bg & 0x8)
    {
      /* blinking */

      bg &= ~0x8;

      if (((SDL_GetTicks() / BLINK_PERIOD) % 2) == 0)
        {
          fg = bg;
        }
    }

  /* How many bytes per line? */

  p = &font->data[(character * font->w * font->h) / 8];
  bit = 0;

  s = ((unsigned char *)screenbuffer->pixels) +
      (y * font->h * screenbuffer->pitch) + (x * font->w);

  for (y1 = 0; y1 < font->h; ++y1)
    {
      s1 = s;

      for (x1 = 0; x1 < font->w; ++x1)
        {
          if (*p & (1 << bit))
            {
              *s1++ = fg;
            }
          else
            {
              *s1++ = bg;
            }

          ++bit;
          if (bit == 8)
            {
              ++p;
              bit = 0;
            }
        }

      s += screenbuffer->pitch;
    }
}

static int limit_to_range(int val, int min, int max)
{
  if (val < min)
    {
      return min;
    }
  else if (val > max)
    {
      return max;
    }
  else
    {
      return val;
    }
}

static void get_dest_rect(SDL_Rect *rect)
{
  int w, h;

  SDL_GetRendererOutputSize(renderer, &w, &h);
  rect->x = (w - screenbuffer->w) / 2;
  rect->y = (h - screenbuffer->h) / 2;
  rect->w = screenbuffer->w;
  rect->h = screenbuffer->h;
}

/* Translates the SDL key */

static int translate_scancode(SDL_Scancode scancode)
{
  switch (scancode)
    {
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
      return KEY_RCTRL;

    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
      return KEY_RSHIFT;

    case SDL_SCANCODE_LALT:
      return KEY_LALT;

    case SDL_SCANCODE_RALT:
      return KEY_RALT;

    default:
      if (scancode < arrlen(scancode_translate_table))
        {
          return scancode_translate_table[scancode];
        }
      else
        {
          return 0;
        }
    }
}

static int translate_keysym(const SDL_Keysym *sym)
{
  int translated;

  /* We cheat here and make use of TranslateScancode. The range of keys
   * associated with printable characters is pretty contiguous, so if it's
   * inside that range we want the localized version of the key instead.
   */

  translated = TranslateScancode(sym->scancode);

  if (translated >= 0x20 && translated < 0x7f)
    {
      return sym->sym;
    }
  else
    {
      return translated;
    }
}

/* Convert an SDL button index to textscreen button index.
 *
 * Note special cases because 2 == mid in SDL, 3 == mid in textscreen/setup
 */

static int sdl_button_to_txt_button(int button)
{
  switch (button)
    {
    case SDL_BUTTON_LEFT:
      return TXT_MOUSE_LEFT;
    case SDL_BUTTON_RIGHT:
      return TXT_MOUSE_RIGHT;
    case SDL_BUTTON_MIDDLE:
      return TXT_MOUSE_MIDDLE;
    default:
      return TXT_MOUSE_BASE + button + 1;
    }
}

/* Convert an SDL wheel motion to a textscreen button index. */

static int sdl_wheel_to_txt_butotn(const SDL_MouseWheelEvent *wheel)
{
  if (wheel->y <= 0)
    {
      return TXT_MOUSE_SCROLLDOWN;
    }
  else
    {
      return TXT_MOUSE_SCROLLUP;
    }
}

static int mouse_has_moved(void)
{
  static int last_x = 0, last_y = 0;
  int x, y;

  txt_get_mouse_position(&x, &y);

  if (x != last_x || y != last_y)
    {
      last_x = x;
      last_y = y;
      return 1;
    }
  else
    {
      return 0;
    }
}

/* Returns true if the given UTF8 key name is printable to the screen. */

static int printable_name(const char *s)
{
  const char *p;
  unsigned int c;

  p = s;
  while (*p != '\0')
    {
      c = txt_decode_utf8(&p);
      if (txt_unicode_character(c) < 0)
        {
          return 0;
        }
    }

  return 1;
}

static const char *name_for_key(int key)
{
  const char *result;
  int i;

  /* Overrides purely for aesthetical reasons, so that default
   * window accelerator keys match those of setup.exe.
   */

  switch (key)
    {
    case KEY_ESCAPE:
      return "ESC";
    case KEY_ENTER:
      return "ENTER";
    default:
      break;
    }

  /* This key presumably maps to a scan code that is listed in the
   * translation table. Find which mapping and once we have a scancode,
   * we can convert it into a virtual key, then a string via SDL.
   */

  for (i = 0; i < arrlen(scancode_translate_table); ++i)
    {
      if (scancode_translate_table[i] == key)
        {
          result = SDL_GetKeyName(SDL_GetKeyFromScancode(i));
          if (txt_utf8_strlen(result) > 6 || !PrintableName(result))
            {
              break;
            }
          return result;
        }
    }

  /* Use US English fallback names, if the localized name is too long,
   * not found in the scancode table, or contains unprintable chars
   * (non-extended ASCII character set):
   */

  for (i = 0; i < arrlen(key_names); ++i)
    {
      if (key_names[i].key == key)
        {
          return key_names[i].name;
        }
    }

  return NULL;
}
#endif

/* Searches the desktop screen buffer to determine whether there are any
 * blinking characters.
 */

static int txt_has_blinking_chars(void)
{
  int x, y;
  unsigned char *p;

  /* Check all characters in screen buffer */

  for (y = 0; y < TXT_SCREEN_H; ++y)
    {
      for (x = 0; x < TXT_SCREEN_W; ++x)
        {
          p = &screendata[(y * TXT_SCREEN_W + x) * 2];

          if (p[1] & 0x80)
            {
              return 1; /* This character is blinking */
            }
        }
    }

  /* None found */

  return 0;
}

static void txt_string_concat(char *dest, const char *src, size_t dest_len)
{
  size_t offset;

  offset = strlen(dest);
  if (offset > dest_len)
    {
      offset = dest_len;
    }

  txt_string_copy(dest + offset, src, dest_len - offset);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Initialize text mode screen
 *
 * Returns 1 if successful, 0 if an error occurred
 */

int txt_init(void)
{
#if 0
    int flags = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return 0;
    }

    ChooseFont();

    screen_image_w = TXT_SCREEN_W * font->w;
    screen_image_h = TXT_SCREEN_H * font->h;

    /* If highdpi_font is selected, try to initialize high dpi rendering. */

    if (font == &highdpi_font)
    {
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }

    TXT_SDLWindow =
        SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         screen_image_w, screen_image_h, flags);

    if (TXT_SDLWindow == NULL)
        return 0;

    renderer = SDL_CreateRenderer(TXT_SDLWindow, -1, SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL)
        renderer = SDL_CreateRenderer(TXT_SDLWindow, -1, SDL_RENDERER_SOFTWARE);

    if (renderer == NULL)
        return 0;

    /* Special handling for OS X retina display. If we successfully set the
     * highdpi flag, check the output size for the screen renderer. If we get
     * the 2x doubled size we expect from a retina display, use the large font
     * for drawing the screen.
     */

    if ((SDL_GetWindowFlags(TXT_SDLWindow) & SDL_WINDOW_ALLOW_HIGHDPI) != 0)
    {
        int render_w, render_h;

        if (SDL_GetRendererOutputSize(renderer, &render_w, &render_h) == 0
         && render_w >= TXT_SCREEN_W * large_font.w
         && render_h >= TXT_SCREEN_H * large_font.h)
        {
            font = &large_font;

            /* Note that we deliberately do not update screen_image_{w,h}
             * since these are the dimensions of textscreen image in screen
             * coordinates, not pixels.
             */
        }
    }

    /* Failed to initialize for high dpi (retina display) rendering? If so
     * then use the normal resolution font instead.
     */

    if (font == &highdpi_font)
    {
        font = &normal_font;
    }

    /* Instead, we draw everything into an intermediate 8-bit surface
     * the same dimensions as the screen. SDL then takes care of all the
     * 8->32 bit (or whatever depth) color conversions for us.
     */

    screenbuffer = SDL_CreateRGBSurface(0,
                                        TXT_SCREEN_W * font->w,
                                        TXT_SCREEN_H * font->h,
                                        8, 0, 0, 0, 0);

    SDL_LockSurface(screenbuffer);
    SDL_SetPaletteColors(screenbuffer->format->palette, ega_colors, 0, 16);
    SDL_UnlockSurface(screenbuffer);

    screendata = malloc(TXT_SCREEN_W * TXT_SCREEN_H * 2);
    memset(screendata, 0, TXT_SCREEN_W * TXT_SCREEN_H * 2);

    return 1;
#endif
  return 1;
}

void txt_shutdown(void)
{
#if 0
    free(screendata);
    screendata = NULL;
    SDL_FreeSurface(screenbuffer);
    screenbuffer = NULL;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(TXT_SDLWindow);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
}

void txt_set_colour(txt_color_t color, int r, int g, int b)
{
#if 0
    SDL_Color c = {r, g, b, 0xff};

    SDL_LockSurface(screenbuffer);
    SDL_SetPaletteColors(screenbuffer->format->palette, &c, color, 1);
    SDL_UnlockSurface(screenbuffer);
#endif
}

unsigned char *txt_get_screen_data(void) { return screendata; }

void txt_update_screen_area(int x, int y, int w, int h)
{
#if 0
    SDL_Texture *screentx;
    SDL_Rect rect;
    int x1, y1;
    int x_end;
    int y_end;

    SDL_LockSurface(screenbuffer);

    x_end = LimitToRange(x + w, 0, TXT_SCREEN_W);
    y_end = LimitToRange(y + h, 0, TXT_SCREEN_H);
    x = LimitToRange(x, 0, TXT_SCREEN_W);
    y = LimitToRange(y, 0, TXT_SCREEN_H);

    for (y1=y; y1<y_end; ++y1)
    {
        for (x1=x; x1<x_end; ++x1)
        {
            UpdateCharacter(x1, y1);
        }
    }

    SDL_UnlockSurface(screenbuffer);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    /* TODO: This is currently creating a new texture every time we render
     * the screen; find a more efficient way to do it.
     */

    screentx = SDL_CreateTextureFromSurface(renderer, screenbuffer);

    SDL_RenderClear(renderer);
    GetDestRect(&rect);
    SDL_RenderCopy(renderer, screentx, NULL, &rect);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(screentx);
#endif
}

void txt_update_screen(void)
{
  txt_update_screen_area(0, 0, TXT_SCREEN_W, TXT_SCREEN_H);
}

void txt_get_mouse_position(int *x, int *y)
{
#if 0
    int window_w, window_h;
    int origin_x, origin_y;

    SDL_GetMouseState(x, y);

    /* Translate mouse position from 'pixel' position into character position.
     * We are working here in screen coordinates and not pixels, since this is
     * what SDL_GetWindowSize() returns; we must calculate and subtract the
     * origin position since we center the image within the window.
     */

    SDL_GetWindowSize(TXT_SDLWindow, &window_w, &window_h);
    origin_x = (window_w - screen_image_w) / 2;
    origin_y = (window_h - screen_image_h) / 2;
    *x = ((*x - origin_x) * TXT_SCREEN_W) / screen_image_w;
    *y = ((*y - origin_y) * TXT_SCREEN_H) / screen_image_h;

    if (*x < 0)
    {
        *x = 0;
    }
    else if (*x >= TXT_SCREEN_W)
    {
        *x = TXT_SCREEN_W - 1;
    }
    if (*y < 0)
    {
        *y = 0;
    }
    else if (*y >= TXT_SCREEN_H)
    {
        *y = TXT_SCREEN_H - 1;
    }
#endif
}

signed int txt_getchar(void)
{
#if 0
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        /* If there is an event callback, allow it to intercept this
         * event.
         */

        if (event_callback != NULL)
        {
            if (event_callback(&ev, event_callback_data))
            {
                continue;
            }
        }

        /* Process the event. */

        switch (ev.type)
        {
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button < TXT_MAX_MOUSE_BUTTONS)
                {
                    return SDLButtonToTXTButton(ev.button.button);
                }
                break;

            case SDL_MOUSEWHEEL:
                return SDLWheelToTXTButton(&ev.wheel);

            case SDL_KEYDOWN:
                switch (input_mode)
                {
                    case TXT_INPUT_RAW:
                        return TranslateScancode(ev.key.keysym.scancode);
                    case TXT_INPUT_NORMAL:
                        return TranslateKeysym(&ev.key.keysym);
                    case TXT_INPUT_TEXT:

                        /* We ignore key inputs in this mode, except for a
                         * few special cases needed during text input:
                         */

                        if (ev.key.keysym.sym == SDLK_ESCAPE
                         || ev.key.keysym.sym == SDLK_BACKSPACE
                         || ev.key.keysym.sym == SDLK_RETURN)
                        {
                            return TranslateKeysym(&ev.key.keysym);
                        }
                        break;
                }
                break;

            case SDL_TEXTINPUT:
                if (input_mode == TXT_INPUT_TEXT)
                {
                    /* TODO: Support input of more than just the first char? */

                    const char *p = ev.text.text;
                    int result = txt_decode_utf8(&p);

                    /* 0-127 is ASCII, but we map non-ASCII Unicode chars into
                     * a higher range to avoid conflicts with special keys.
                     */

                    return TXT_UNICODE_TO_KEY(result);
                }
                break;

            case SDL_QUIT:
                return 27; /* Quit = escape */

            case SDL_MOUSEMOTION:
                if (MouseHasMoved())
                {
                    return 0;
                }

            default:
                break;
        }
    }

    return -1;
#endif
  return -1;
}

int txt_get_modifier_state(txt_modifier_t mod)
{
#if 0
    SDL_Keymod state;

    state = SDL_GetModState();

    switch (mod)
    {
        case TXT_MOD_SHIFT:
            return (state & KMOD_SHIFT) != 0;
        case TXT_MOD_CTRL:
            return (state & KMOD_CTRL) != 0;
        case TXT_MOD_ALT:
            return (state & KMOD_ALT) != 0;
        default:
            return 0;
    }
#endif
  return 0;
}

int txt_unicode_character(unsigned int c)
{
  unsigned int i;

  /* Check the code page mapping to see if this character maps
   * to anything.
   */

  for (i = 0; i < arrlen(g_code_page_to_unicode); ++i)
    {
      if (g_code_page_to_unicode[i] == c)
        {
          return i;
        }
    }

  return -1;
}

void txt_get_key_description(int key, char *buf, size_t buf_len)
{
#if 0
  const char *keyname;
  int i;

  keyname = name_for_key(key);

  if (keyname != NULL)
    {
      txt_string_copy(buf, keyname, buf_len);

      /* Key description should be all-uppercase to match setup.exe. */

      for (i = 0; buf[i] != '\0'; ++i)
        {
          buf[i] = toupper(buf[i]);
        }
    }
  else
    {
      txt_snprintf(buf, buf_len, "??%i", key);
    }
#endif
}

/* Sleeps until an event is received, the screen needs to be redrawn,
 * or until timeout expires (if timeout != 0)
 */

void txt_sleep(int timeout)
{
#if 0
    unsigned int start_time;

    if (TXT_ScreenHasBlinkingChars())
    {
        int time_to_next_blink;

        time_to_next_blink = BLINK_PERIOD - (SDL_GetTicks() % BLINK_PERIOD);

        /* There are blinking characters on the screen, so we 
         * must time out after a while
         */
       
        if (timeout == 0 || timeout > time_to_next_blink)
        {
            /* Add one so it is always positive */

            timeout = time_to_next_blink + 1;
        }
    }

    if (timeout == 0)
    {
        /* We can just wait forever until an event occurs */

        SDL_WaitEvent(NULL);
    }
    else
    {
        /* Sit in a busy loop until the timeout expires or we have to
         * redraw the blinking screen
         */

        start_time = SDL_GetTicks();

        while (SDL_GetTicks() < start_time + timeout)
        {
            if (SDL_PollEvent(NULL) != 0)
            {
                /* Received an event, so stop waiting */

                break;
            }

            /* Don't hog the CPU */

            SDL_Delay(1);
        }
    }
#endif
}

void txt_set_input_mode(txt_input_mode_t mode)
{
#if 0
    if (mode == TXT_INPUT_TEXT && !SDL_IsTextInputActive())
    {
        SDL_StartTextInput();
    }
    else if (SDL_IsTextInputActive() && mode != TXT_INPUT_TEXT)
    {
        SDL_StopTextInput();
    }

    input_mode = mode;
#endif
}

void txt_set_window_title(const char *title)
{
#if 0
    SDL_SetWindowTitle(TXT_SDLWindow, title);
#endif
}

void txt_sdl_set_event_callback(void *user_data)
{
#if 0
  /* First argument was TxtSDLEventCallbackFunc callback */

  event_callback = callback;
  event_callback_data = user_data;
#endif
}

/* Safe string functions. */

void txt_string_copy(char *dest, const char *src, size_t dest_len)
{
  if (dest_len < 1)
    {
      return;
    }

  dest[dest_len - 1] = '\0';
  strncpy(dest, src, dest_len - 1);
}

/* Safe, portable vsnprintf(). */

int txt_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args)
{
  int result;

  if (buf_len < 1)
    {
      return 0;
    }

  /* Windows (and other OSes?) has a vsnprintf() that doesn't always
   * append a trailing \0. So we must do it, and write into a buffer
   * that is one byte shorter; otherwise this function is unsafe.
   */

  result = vsnprintf(buf, buf_len, s, args);

  /* If truncated, change the final char in the buffer to a \0.
   * A negative result indicates a truncated buffer on Windows.
   */

  if (result < 0 || result >= buf_len)
    {
      buf[buf_len - 1] = '\0';
      result = buf_len - 1;
    }

  return result;
}

/* Safe, portable snprintf(). */

int txt_snprintf(char *buf, size_t buf_len, const char *s, ...)
{
  va_list args;
  int result;
  va_start(args, s);
  result = txt_vsnprintf(buf, buf_len, s, args);
  va_end(args);
  return result;
}
