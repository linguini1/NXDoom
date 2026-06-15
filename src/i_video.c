/****************************************************************************
 * apps/games/NXDoom/src/i_video.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 * DESCRIPTION:
 * DOOM graphics stuff for SDL.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "config.h"
#include "d_loop.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_diskicon.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define RESIZE_DELAY 500

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct graphics_state_s
{
  int fd; /* File descriptor handle to frame buffer */

  /* The 320x200x32 RGBA intermediate buffer is what we blit the former
   * buffer to. On NuttX, this is the frame buffer memory `fbmem`. It may not
   * have 32-bit depth, but if it doesn't, the code is adjusted accordingly.
   */

  FAR void *fbmem;

  /* 8-bit depth screen buffer (320x200x8) that we draw to (i.e. the one that
   * holds i_video_buffer)
   */

  pixel_t *scrnbuf;

  /* Information about the frame buffer needed for rendering. */

  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;

  /* Scale multiplier for rendering large image */

  uint8_t scale;

  bool inited; /* Track initialization */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NuttX graphics state */

static struct graphics_state_s g_graphics_state =
{
  0
};

/* Window title */

static const char *g_window_title = "";

/* Colour palette map from 8-bit colour to 32-bit */

static struct argbcolor_s g_palette[256];

static boolean palette_to_set;

/* disable mouse? */

static boolean nomouse = false;

/* Maximum number of pixels to use for intermediate scale buffer. */

static int max_scaling_buffer_pixels = 16000000;

/* Time to wait for the screen to settle on startup before starting the game
 * (ms)
 */

static int startup_delay = 1000;

/* Grab the mouse? (int type for config code). nograbmouse_override allows
 * this to be temporarily disabled via the command line.
 */

static int grabmouse = true;
static boolean nograbmouse_override = false;

/* If true, we display dots at the bottom of the screen to
 * indicate FPS.
 */

static boolean display_fps_dots;

/* If this is true, the screen is rendered but not blitted to the
 * video buffer.
 */

static boolean noblit;

/* Callback function to invoke to determine whether to grab the
 * mouse pointer.
 */

static grabmouse_callback_t grabmouse_callback = NULL;

/* Does the window currently have focus? */

static boolean window_focused = true;

/* Window resize state. */

#if 0
static boolean need_resize = false;
static unsigned int last_resize_time;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

int usemouse = 1;

/* Save screenshots in PNG format. */

int png_screenshots = 0;

/* SDL video driver name */

char *video_driver = "";

/* Window position: */

char *window_position = "center";

/* SDL display number on which to run. */

int video_display = 0;

/* Screen width and height, from configuration file. */

int window_width = 320;
int window_height = 200;

/* Fullscreen mode, 0x0 for SDL_WINDOW_FULLSCREEN_DESKTOP. */

int fullscreen_width = 0;
int fullscreen_height = 0;

/* Run in full screen mode?  (int type for config code) */

int fullscreen = true;

/* Smooth pixel scaling */

int smooth_pixel_scaling = true;

/* Force integer scales for resolution-independent rendering */

int integer_scaling = false;

/* VGA Porch palette change emulation */

int vga_porch_flash = false;

/* Force software rendering, for systems which lack effective hardware
 * acceleration
 */

int force_software_renderer = false;

/* The screen buffer; this is modified to draw things to the screen */

pixel_t *i_video_buffer = NULL;

/* If true, game is running as a screensaver */

boolean screensaver_mode = false;

/* Flag indicating whether the screen is currently visible:
 * when the screen isn't visible, don't render the screen
 */

boolean screenvisible = true;

/* Gamma correction level to use */

int usegamma = 0;

/* Joystick/gamepad hysteresis */

unsigned int joywait = 0;

/* TODO: I'm sure more of the variables above can be private */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: blit_screen
 *
 * Description:
 *   Blit the 8-bit depth buffer that DOOM renders to onto the frame buffer
 *   in a higher colour depth.
 *
 ****************************************************************************/

static void blit_screen(void)
{
  uint8_t p_idx;
  void *fbptr;

  /* TODO: It would be best to do this more efficiently/with less memory.
   * It also would be good if we could handle the palette translation here
   * such that DOOM can be played on frame buffers with differing bit depths
   * and pixel formats.
   */

  fbptr = g_graphics_state.fbmem;
  for (unsigned y = 0; y < SCREENHEIGHT * g_graphics_state.scale; y++)
    {
      for (unsigned x = 0; x < SCREENWIDTH * g_graphics_state.scale; x++)
        {
          p_idx = g_graphics_state
                      .scrnbuf[(y / g_graphics_state.scale) * SCREENWIDTH +
                               (x / g_graphics_state.scale)];

          ((uint32_t *)(fbptr))[x] =
              ARGBTO32(g_palette[p_idx].a, g_palette[p_idx].r,
                       g_palette[p_idx].g, g_palette[p_idx].b);
        }

      fbptr += g_graphics_state.pinfo.stride;
    }
}

#if 0
static boolean mouse_should_be_grabbed(void)
{
  /* never grab the mouse when in screensaver mode */

  if (screensaver_mode) return false;

  /* if the window doesn't have focus, never grab it */

  if (!window_focused) return false;

  /* always grab the mouse when full screen (dont want to
   * see the mouse pointer)
   */

  if (fullscreen) return true;

  /* Don't grab the mouse if mouse input is disabled */

  if (!usemouse || nomouse) return false;

  /* if we specify not to grab the mouse, never grab */

  if (nograbmouse_override || !grabmouse) return false;

  /* Invoke the grabmouse callback function to determine whether
   * the mouse should be grabbed
   */

  if (grabmouse_callback != NULL)
    {
      return grabmouse_callback();
    }
  else
    {
      return true;
    }
}
#endif

#if 0
static void set_show_cursor(boolean show)
{
  if (!screensaver_mode)
    {
      /* When the cursor is hidden, grab the input.
       * Relative mode implicitly hides the cursor.
       */

      SDL_SetRelativeMouseMode(!show);
      SDL_GetRelativeMouseState(NULL, NULL);
    }
}

/* Adjust window_width / window_height variables to be an an aspect
 * ratio consistent with the aspect_ratio_correct variable.
 */

static void adjust_window_size(void)
{
  if (aspect_ratio_correct || integer_scaling)
    {
      if (window_width * actualheight <= window_height * SCREENWIDTH)
        {
          /* We round up window_height if the ratio is not exact; this leaves
           * the result stable.
           */

          window_height =
              (window_width * actualheight + SCREENWIDTH - 1) / SCREENWIDTH;
        }
      else
        {
          window_width = window_height * SCREENWIDTH / actualheight;
        }
    }
}

static void handle_window_event(SDL_WindowEvent *event)
{
  int i;

  switch (event->event)
    {
#if 0 /* SDL2-TODO */
     case SDL_ACTIVEEVENT:

       /* need to update our focus state */

       UpdateFocus();
       break;
#endif
    case SDL_WINDOWEVENT_EXPOSED:
      palette_to_set = true;
      break;

    case SDL_WINDOWEVENT_RESIZED:
      need_resize = true;
      last_resize_time = SDL_GetTicks();
      break;

      /* Don't render the screen when the window is minimized: */

    case SDL_WINDOWEVENT_MINIMIZED:
      screenvisible = false;
      break;

    case SDL_WINDOWEVENT_MAXIMIZED:
    case SDL_WINDOWEVENT_RESTORED:
      screenvisible = true;
      break;

      /* Update the value of window_focused when we get a focus event
       *
       * We try to make ourselves be well-behaved: the grab on the mouse
       * is removed if we lose focus (such as a popup window appearing),
       * and we dont move the mouse around if we aren't focused either.
       */

    case SDL_WINDOWEVENT_FOCUS_GAINED:
      window_focused = true;
      break;

    case SDL_WINDOWEVENT_FOCUS_LOST:
      window_focused = false;
      break;

      /* We want to save the user's preferred monitor to use for running the
       * game, so that next time we're run we start on the same display. So
       * every time the window is moved, find which display we're now on and
       * update the video_display config variable.
       */

    case SDL_WINDOWEVENT_MOVED:
      i = SDL_GetWindowDisplayIndex(screen);
      if (i >= 0)
        {
          video_display = i;
        }
      break;

    default:
      break;
    }
}

static boolean toggle_fullscreen_keyshortcut(char key)
{
  /* Argument was SDL_Keysym *sym */

  Uint16 flags = (KMOD_LALT | KMOD_RALT);
  return (sym->scancode == SDL_SCANCODE_RETURN ||
          sym->scancode == SDL_SCANCODE_KP_ENTER) &&
         (sym->mod & flags) != 0;
}

static void i_toggle_fullscreen(void)
{
  unsigned int flags = 0;

  /* TODO: Consider implementing fullscreen toggle for SDL_WINDOW_FULLSCREEN
   * (mode-changing) setup. This is hard because we have to shut down and
   * restart again.
   */

  if (fullscreen_width != 0 || fullscreen_height != 0)
    {
      return;
    }

  fullscreen = !fullscreen;

  if (fullscreen)
    {
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

  SDL_SetWindowFullscreen(screen, flags);

  if (!fullscreen)
    {
      AdjustWindowSize();
      SDL_SetWindowSize(screen, window_width, window_height);
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_set_grab_mouse_callback(grabmouse_callback_t func)
{
  grabmouse_callback = func;
}

/* Set the variable controlling FPS dots. */

void i_display_fps_dots(boolean dots_on)
{
  display_fps_dots = dots_on;
}

void i_shutdown_graphics(void)
{
  if (!g_graphics_state.inited)
    {
      return;
    }

  close(g_graphics_state.fd);
  munmap(g_graphics_state.fbmem, g_graphics_state.pinfo.fblen);
  free(g_graphics_state.scrnbuf);
  g_graphics_state.inited = false;
}

void i_start_frame(void)
{
  /* er? */
}

void I_GetEvent(void)
{
  int err;
#if CONFIG_GAMES_NXDOOM_KEYBOARD
  struct keyboard_event_s kbdevent;

  while ((err = get_kbd_event(&kbdevent)) == 0)
    {
      switch (kbdevent.type)
        {
        case KEYBOARD_PRESS:
#if 0
          if (toggle_fullscreen_key_shortcut(&sdlevent.key.keysym))
            {
              i_toggle_fullscreen();
              break;
            }

#endif
          /* deliberate fall-though */

        case KEYBOARD_RELEASE:
          i_handle_keyboard_event(&kbdevent);
          break;

#if 0
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
          if (usemouse && !nomouse && window_focused)
            {
              i_handle_mouse_event(&sdlevent);
            }
          break;

        case SDL_QUIT:
          if (screensaver_mode)
            {
              i_quit();
            }
          else
            {
              event_t event;
              event.type = ev_quit;
              d_post_event(&event);
            }
          break;

        case SDL_WINDOWEVENT:
          if (sdlevent.window.windowID == SDL_GetWindowID(screen))
            {
              HandleWindowEvent(&sdlevent.window);
            }
          break;
#endif
        default:
          break;
        }
    }
#endif
}

static void update_grab(void)
{
#if 0
  static boolean currently_grabbed = false;
  boolean grab;

  grab = mouse_should_be_grabbed();

  if (screensaver_mode)
    {
      /* Hide the cursor in screensaver mode */

      set_show_cursor(false);
    }
  else if (grab && !currently_grabbed)
    {
      set_show_cursor(false);
    }
  else if (!grab && currently_grabbed)
    {
      int screen_w;
      int screen_h;

      set_show_cursor(true);

      /* When releasing the mouse from grab, warp the mouse cursor to
       * the bottom-right of the screen. This is a minimally distracting
       * place for it to appear - we may only have released the grab
       * because we're at an end of level intermission screen, for
       * example.
       */

      SDL_GetWindowSize(screen, &screen_w, &screen_h);
      SDL_WarpMouseInWindow(screen, screen_w - 16, screen_h - 16);
      SDL_GetRelativeMouseState(NULL, NULL);
    }

  currently_grabbed = grab;
#endif
}

#if 0
static void limit_texture_size(int *w_upscale, int *h_upscale)
{
  SDL_RendererInfo rinfo;
  int orig_w;
  int orig_h;

  orig_w = *w_upscale;
  orig_h = *h_upscale;

  /* Query renderer and limit to maximum texture dimensions of hardware: */

  if (SDL_GetRendererInfo(renderer, &rinfo) != 0)
    {
      i_error("CreateUpscaledTexture: SDL_GetRendererInfo() call failed: %s",
              SDL_GetError());
    }

  while (*w_upscale * SCREENWIDTH > rinfo.max_texture_width)
    {
      --*w_upscale;
    }
  while (*h_upscale * SCREENHEIGHT > rinfo.max_texture_height)
    {
      --*h_upscale;
    }

  if ((*w_upscale < 1 && rinfo.max_texture_width > 0) ||
      (*h_upscale < 1 && rinfo.max_texture_height > 0))
    {
      i_error("CreateUpscaledTexture: Can't create a texture big enough for "
              "the whole screen! Maximum texture size %dx%d",
              rinfo.max_texture_width, rinfo.max_texture_height);
    }

  /* We limit the amount of texture memory used for the intermediate buffer,
   * since beyond a certain point there are diminishing returns. Also,
   * depending on the hardware there may be performance problems with very
   * huge textures, so the user can use this to reduce the maximum texture
   * size if desired.
   */

  if (max_scaling_buffer_pixels < SCREENWIDTH * SCREENHEIGHT)
    {
      i_error("CreateUpscaledTexture: max_scaling_buffer_pixels too small "
              "to create a texture buffer: %d < %d",
              max_scaling_buffer_pixels, SCREENWIDTH * SCREENHEIGHT);
    }

  while (*w_upscale * *h_upscale * SCREENWIDTH * SCREENHEIGHT >
         max_scaling_buffer_pixels)
    {
      if (*w_upscale > *h_upscale)
        {
          --*w_upscale;
        }
      else
        {
          --*h_upscale;
        }
    }

  if (*w_upscale != orig_w || *h_upscale != orig_h)
    {
      printf("CreateUpscaledTexture: Limited texture size to %dx%d "
             "(max %d pixels, max texture size %dx%d)\n",
             *w_upscale * SCREENWIDTH, *h_upscale * SCREENHEIGHT,
             max_scaling_buffer_pixels, rinfo.max_texture_width,
             rinfo.max_texture_height);
    }
}

static void create_upscaled_texture(boolean force)
{
  int w;
  int h;
  int h_upscale;
  int w_upscale;
  static int h_upscale_old;
  static int w_upscale_old;

  SDL_Texture *new_texture;
  SDL_Texture *old_texture;

  /* Get the size of the renderer output. The units this gives us will be
   * real world pixels, which are not necessarily equivalent to the screen's
   * window size (because of highdpi).
   */

  if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0)
    {
      i_error("Failed to get renderer output size: %s", SDL_GetError());
    }

  w = fbstate.vinfo.xres;
  h = fbstate.vinfo.yres;

  /* When the screen or window dimensions do not match the aspect ratio
   * of the texture, the rendered area is scaled down to fit. Calculate
   * the actual dimensions of the rendered area.
   */

  if (w * actualheight < h * SCREENWIDTH)
    {
      /* Tall window. */

      h = w * actualheight / SCREENWIDTH;
    }
  else
    {
      /* Wide window. */

      w = h * SCREENWIDTH / actualheight;
    }

  /* Pick texture size the next integer multiple of the screen dimensions.
   * If one screen dimension matches an integer multiple of the original
   * resolution, there is no need to overscale in this direction.
   */

  w_upscale = (w + SCREENWIDTH - 1) / SCREENWIDTH;
  h_upscale = (h + SCREENHEIGHT - 1) / SCREENHEIGHT;

  /* Minimum texture dimensions of 320x200. */

  if (w_upscale < 1)
    {
      w_upscale = 1;
    }

  if (h_upscale < 1)
    {
      h_upscale = 1;
    }

  limit_texture_size(&w_upscale, &h_upscale);

  /* Create a new texture only if the upscale factors have actually changed.
   */

  if (h_upscale == h_upscale_old && w_upscale == w_upscale_old && !force)
    {
      return;
    }

  h_upscale_old = h_upscale;
  w_upscale_old = w_upscale;

  /* Set the scaling quality for rendering the upscaled texture to "linear",
   * which looks much softer and smoother than "nearest" but does a better
   * job at downscaling from the upscaled texture to screen.
   */

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

  new_texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
      w_upscale * SCREENWIDTH, h_upscale * SCREENHEIGHT);

  old_texture = texture_upscaled;
  texture_upscaled = new_texture;

  if (old_texture != NULL)
    {
      SDL_DestroyTexture(old_texture);
    }
}
#endif

static void set_video_mode(void)
{
#if 0
  int w;
  int h;
  int x;
  int y;
  int window_flags = 0;
  int renderer_flags = 0;
  SDL_DisplayMode mode;

  w = window_width;
  h = window_height;

  /* In windowed mode, the window can be resized while the game is
   * running.
   */

  window_flags = SDL_WINDOW_RESIZABLE;

  /* Set the highdpi flag - this makes a big difference on Macs with
   * retina displays, especially when using small window sizes.
   */

  window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

  if (fullscreen)
    {
      if (fullscreen_width == 0 && fullscreen_height == 0)
        {
          /* This window_flags means "Never change the screen resolution!
           * Instead, draw to the entire screen by scaling the texture
           * appropriately".
           */

          window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
      else
        {
          w = fullscreen_width;
          h = fullscreen_height;
          window_flags |= SDL_WINDOW_FULLSCREEN;
        }
    }

  /* Running without window decorations is potentially useful if you're
   * playing in three window mode and want to line up three game windows
   * next to each other on a single desktop.
   * Deliberately not documented because I'm not sure how useful this is yet.
   */

  if (m_parm_exists("-borderless"))
    {
      window_flags |= SDL_WINDOW_BORDERLESS;
    }

  i_get_window_position(&x, &y, w, h);

  /* Create window and renderer contexts. We set the window title
   * later anyway and leave the window position "undefined". If
   * "window_flags" contains the fullscreen flag (see above), then
   * w and h are ignored.
   */

  if (screen == NULL)
    {
      screen = SDL_CreateWindow(NULL, x, y, w, h, window_flags);

      if (screen == NULL)
        {
          i_error("Error creating window for video startup: %s",
                  SDL_GetError());
        }

      SDL_SetWindowMinimumSize(screen, SCREENWIDTH, actualheight);
    }

  /* The SDL_RENDERER_TARGETTEXTURE flag is required to render the
   * intermediate texture into the upscaled texture.
   */

  renderer_flags = SDL_RENDERER_TARGETTEXTURE;

  if (SDL_GetCurrentDisplayMode(video_display, &mode) != 0)
    {
      i_error("Could not get display mode for video display #%d: %s",
              video_display, SDL_GetError());
    }

  /* Turn on vsync if we aren't in a -timedemo */

  if (!singletics && mode.refresh_rate > 0)
    {
      renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }

  if (force_software_renderer)
    {
      renderer_flags |= SDL_RENDERER_SOFTWARE;
      renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;
    }

  if (renderer != NULL)
    {
      SDL_DestroyRenderer(renderer);

      /* all associated textures get destroyed */

      texture = NULL;
      texture_upscaled = NULL;
    }

  renderer = SDL_CreateRenderer(screen, -1, renderer_flags);

  /* If we could not find a matching render driver,
   * try again without hardware acceleration.
   */

  if (renderer == NULL && !force_software_renderer)
    {
      renderer_flags |= SDL_RENDERER_SOFTWARE;
      renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;

      renderer = SDL_CreateRenderer(screen, -1, renderer_flags);

      /* If this helped, save the setting for later. */

      if (renderer != NULL)
        {
          force_software_renderer = 1;
        }
    }

  if (renderer == NULL)
    {
      i_error("Error creating renderer for screen window: %s",
              SDL_GetError());
    }

  /* Important: Set the "logical size" of the rendering context. At the same
   * time this also defines the aspect ratio that is preserved while scaling
   * and stretching the texture into the window.
   */

  if (aspect_ratio_correct || integer_scaling)
    {
      SDL_RenderSetLogicalSize(renderer, SCREENWIDTH, actualheight);
    }

  /* Force integer scales for resolution-independent rendering. */

  SDL_RenderSetIntegerScale(renderer, integer_scaling);

  /* Blank out the full screen area in case there is any junk in
   * the borders that won't otherwise be overwritten.
   */

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  /* Create the 8-bit paletted and the 32-bit RGBA screenbuffer surfaces. */

  if (screenbuffer != NULL)
    {
      SDL_FreeSurface(screenbuffer);
      screenbuffer = NULL;
    }

  if (screenbuffer == NULL)
    {
      screenbuffer =
          SDL_CreateRGBSurface(0, SCREENWIDTH, SCREENHEIGHT, 8, 0, 0, 0, 0);
      SDL_FillRect(screenbuffer, NULL, 0);
    }

  /* Format of argbbuffer must match the screen pixel format because we
   * import the surface data into the texture.
   */

  if (argbbuffer != NULL)
    {
      SDL_FreeSurface(argbbuffer);
      argbbuffer = NULL;
    }

  if (argbbuffer == NULL)
    {
      /* pixels and pitch will be filled with the texture's values
       * in i_finish_update()
       */

      argbbuffer = SDL_CreateRGBSurfaceWithFormatFrom(
          NULL, w, h, 0, 0, SDL_PIXELFORMAT_ARGB8888);
    }

  if (texture != NULL)
    {
      SDL_DestroyTexture(texture);
    }

  /* Set the scaling quality for rendering the intermediate texture into
   * the upscaled texture to "nearest", which is gritty and pixelated and
   * resembles software scaling pretty well.
   */

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  /* Create the intermediate texture that the RGBA surface gets loaded into.
   * The SDL_TEXTUREACCESS_STREAMING flag means that this texture's content
   * is going to change frequently.
   */

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH,
                              SCREENHEIGHT);

  /* Initially create the upscaled texture for rendering to screen */

  CreateUpscaledTexture(true);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_start_tic(void)
{
  if (!g_graphics_state.inited)
    {
      return;
    }

  I_GetEvent();

  if (usemouse && !nomouse && window_focused)
    {
      i_read_mouse();
    }

  if (joywait < i_get_time())
    {
      i_update_joystick();
    }
}

void i_update_no_blit(void)
{
  /* what is this? */
}

void i_finish_update(void)
{
  static int lasttic;
  int tics;
  int i;

  if (!g_graphics_state.inited) return;

  if (noblit) return;

#if 0
  if (need_resize)
    {
      if (SDL_GetTicks() > last_resize_time + RESIZE_DELAY)
        {
          int flags;

          /* When the window is resized (we're not in fullscreen mode),
           * save the new window size.
           */

          flags = SDL_GetWindowFlags(screen);
          if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
            {
              SDL_GetWindowSize(screen, &window_width, &window_height);

              /* Adjust the window by resizing again so that the window
               * is the right aspect ratio.
               */

              AdjustWindowSize();
              SDL_SetWindowSize(screen, window_width, window_height);
            }

          CreateUpscaledTexture(false);
          need_resize = false;
          palette_to_set = true;
        }
      else
        {
          return;
        }
    }

  update_grab();
#endif

#if 0 /* SDL2-TODO */

  /* Don't update the screen if the window isn't visible.
   * Not doing this breaks under Windows when we alt-tab away
   * while fullscreen.
   */

  if (!(SDL_GetAppState() & SDL_APPACTIVE)) return;
#endif

  /* draws little dots on the bottom of the screen */

  if (display_fps_dots)
    {
      i = i_get_time();
      tics = i - lasttic;
      lasttic = i;
      if (tics > 20) tics = 20;

      for (i = 0; i < tics * 4; i += 4)
        i_video_buffer[(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0xff;
      for (; i < 20 * 4; i += 4)
        i_video_buffer[(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0x0;
    }

  /* Draw disk icon before blit, if necessary. */

  v_draw_disk_icon();

  if (palette_to_set)
    {
      palette_to_set = false;
    }

  blit_screen();

#if 0
  SDL_LockTexture(texture, &blit_rect, &argbbuffer->pixels,
                  &argbbuffer->pitch);
  SDL_LowerBlit(screenbuffer, &blit_rect, argbbuffer, &blit_rect);
  SDL_UnlockTexture(texture);

  /* Make sure the pillarboxes are kept clear each frame. */

  SDL_RenderClear(renderer);

  if (smooth_pixel_scaling && !force_software_renderer)
    {
      /* Render this intermediate texture into the upscaled texture
       * using "nearest" integer scaling.
       */

      SDL_SetRenderTarget(renderer, texture_upscaled);
      SDL_RenderCopy(renderer, texture, NULL, NULL);

      /* Finally, render this upscaled texture to screen using linear
       * scaling.
       */

      SDL_SetRenderTarget(renderer, NULL);
      SDL_RenderCopy(renderer, texture_upscaled, NULL, NULL);
    }
  else
    {
      SDL_SetRenderTarget(renderer, NULL);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
    }
#endif

  /* Draw! */

#if 0
  SDL_RenderPresent(renderer);
#endif

  /* Restore background and undo the disk indicator, if it was drawn. */

  v_restore_disk_background();
}

void i_read_screen(pixel_t *scr)
{
  memcpy(scr, i_video_buffer, SCREENWIDTH * SCREENHEIGHT * sizeof(*scr));
}

/****************************************************************************
 * Name: i_set_palette
 ****************************************************************************/

void i_set_palette(byte *doompalette)
{
  for (int i = 0; i < 256; ++i)
    {
      /* Zero out the bottom two bits of each channel - the PC VGA
       * controller only supports 6 bits of accuracy.
       */

      g_palette[i].a = 0xffu;
      g_palette[i].r = gammatable[usegamma][*doompalette++] & ~3;
      g_palette[i].g = gammatable[usegamma][*doompalette++] & ~3;
      g_palette[i].b = gammatable[usegamma][*doompalette++] & ~3;
    }

  palette_to_set = true;
}

/****************************************************************************
 * Name: i_get_palette_index
 *
 * Description:
 *  Given an RGB value, find the closest matching palette index.
 *
 * Return:
 *   An index into the palette lookup table for the best match.
 *
 ****************************************************************************/

int i_get_palette_index(int r, int g, int b)
{
  int best = 0;
  int best_diff = INT_MAX;
  int diff;

  for (int i = 0; i < 256; ++i)
    {
      diff = (r - g_palette[i].r) * (r - g_palette[i].r) +
             (g - g_palette[i].g) * (g - g_palette[i].g) +
             (b - g_palette[i].b) * (b - g_palette[i].b);

      if (diff < best_diff)
        {
          best = i;
          best_diff = diff;
        }

      if (diff == 0)
        {
          break;
        }
    }

  return best;
}

/****************************************************************************
 * Name: i_set_window_title
 *
 * Description:
 *  Set the window title internally.
 *
 ****************************************************************************/

void i_set_window_title(const char *title)
{
  g_window_title = title;
}

/****************************************************************************
 * Name: i_init_window_title
 *
 * Description:
 *   Actually cause the window title to update with whatever window title was
 *   last set via i_set_window_title.
 *
 ****************************************************************************/

void i_init_window_title(void)
{
#if 0
  char *buf;

  buf = m_string_join(g_window_title, " - ", PACKAGE_STRING, NULL);
  SDL_SetWindowTitle(screen, buf);
  free(buf);
#endif
}

/****************************************************************************
 * Name: i_set_window_title
 ****************************************************************************/

void i_graphics_check_commandline(void)
{
  int i;

  /* @category video
   * @vanilla
   *
   * Disable blitting the screen.
   */

  noblit = m_check_parm("-noblit");

  /* @category video
   *
   * Don't grab the mouse when running in windowed mode.
   */

  nograbmouse_override = m_parm_exists("-nograbmouse");

  /* default to fullscreen mode, allow override with command line
   * nofullscreen because we love prboom
   */

  /* @category video
   *
   * Run in a window.
   */

  if (m_check_parm("-window") || m_check_parm("-nofullscreen"))
    {
      fullscreen = false;
    }

  /* @category video
   *
   * Run in fullscreen mode.
   */

  if (m_check_parm("-fullscreen"))
    {
      fullscreen = true;
    }

  /* @category video
   *
   * Disable the mouse.
   */

  nomouse = m_check_parm("-nomouse") > 0;

  /* @category video
   * @arg <W>
   *
   * Specify the screen width, in pixels.  Implies -window.
   */

  i = m_check_parm_with_args("-width", 1);

  if (i > 0)
    {
      window_width = atoi(myargv[i + 1]);
      fullscreen = false;
    }

  /* @category video
   * @arg <H>
   *
   * Specify the screen height, in pixels.  Implies -window.
   */

  i = m_check_parm_with_args("-height", 1);

  if (i > 0)
    {
      window_height = atoi(myargv[i + 1]);
      fullscreen = false;
    }

  /* @category video
   * @arg <WxH>
   *
   * Specify the dimensions of the window.  Implies -window.
   */

  i = m_check_parm_with_args("-geometry", 1);

  if (i > 0)
    {
      int w;
      int h;
      int s;

      s = sscanf(myargv[i + 1], "%ix%i", &w, &h);
      if (s == 2)
        {
          window_width = w;
          window_height = h;
          fullscreen = false;
        }
    }

  /* @category video
   * @arg <x>
   *
   * Specify the display number on which to show the screen.
   */

  i = m_check_parm_with_args("-display", 1);

  if (i > 0)
    {
      int display = atoi(myargv[i + 1]);
      if (display >= 0)
        {
          video_display = display;
        }
    }
}

/* Check if we have been invoked as a screensaver by xscreensaver. */

void i_check_is_screensaver(void)
{
  char *env;

  env = getenv("XSCREENSAVER_WINDOW");

  if (env != NULL)
    {
      screensaver_mode = true;
    }
}

#if 0
static void set_sdl_video_driver(void)
{
  /* Allow a default value for the SDL video driver to be specified
   * in the configuration file.
   */

  if (strcmp(video_driver, "") != 0)
    {
      char *env_string;

      env_string = m_string_join("SDL_VIDEODRIVER=", video_driver, NULL);
      putenv(env_string);
      free(env_string);
    }
}
#endif

/* Check the display bounds of the display referred to by 'video_display' and
 * set x and y to a location that places the window in the center of that
 * display.
 */

static void center_window(int *x, int *y, int w, int h)
{
#if 0
  SDL_Rect bounds;

  if (SDL_GetDisplayBounds(video_display, &bounds) < 0)
    {
      fprintf(stderr,
              "CenterWindow: Failed to read display bounds "
              "for display #%d!\n",
              video_display);
      return;
    }

  *x = bounds.x + SDL_max((bounds.w - w) / 2, 0);
  *y = bounds.y + SDL_max((bounds.h - h) / 2, 0);
#endif
  *x = MAX((g_graphics_state.vinfo.xres - w) / 2, 0);
  *y = MAX((g_graphics_state.vinfo.yres - h) / 2, 0);
}

void i_get_window_position(int *x, int *y, int w, int h)
{
#if 0
  /* Check that video_display corresponds to a display that really exists,
   * and if it doesn't, reset it.
   */

  if (video_display < 0 || video_display >= SDL_GetNumVideoDisplays())
    {
      fprintf(
          stderr,
          "i_get_window_position: We were configured to run on display #%d, "
          "but it no longer exists (max %d). Moving to display 0.\n",
          video_display, SDL_GetNumVideoDisplays() - 1);
      video_display = 0;
    }
#endif

  /* in fullscreen mode, the window "position" still matters, because
   * we use it to control which display we run fullscreen on.
   */

  if (fullscreen)
    {
      center_window(x, y, w, h);
      return;
    }

#if 0

  /* in windowed mode, the desired window position can be specified
   * in the configuration file.
   */

  if (window_position == NULL || !strcmp(window_position, ""))
    {
      *x = *y = SDL_WINDOWPOS_UNDEFINED;
    }
  else if (!strcmp(window_position, "center"))
    {
      /* Note: SDL has a SDL_WINDOWPOS_CENTER, but this is useless for our
       * purposes, since we also want to control which display we appear on.
       * So we have to do this ourselves.
       */

      CenterWindow(x, y, w, h);
    }
  else if (sscanf(window_position, "%i,%i", x, y) != 2)
    {
      /* invalid format: revert to default */

      fprintf(stderr,
              "i_get_window_position: invalid window_position setting\n");
      *x = *y = SDL_WINDOWPOS_UNDEFINED;
    }
#endif
}

void i_init_graphics(void)
{
  uint8_t xscale;
  uint8_t yscale;
  int err;
  byte *doompal;

  /* Open frame buffer */

  g_graphics_state.fd = open(CONFIG_GAMES_NXDOOM_FBPATH, O_RDWR);
  if (g_graphics_state.fd < 0)
    {
      i_error("Failed to open frame buffer: %d", errno);
    }

  /* Get frame buffer characteristics */

  err = ioctl(g_graphics_state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)(uintptr_t)&g_graphics_state.vinfo);
  if (err < 0)
    {
      close(g_graphics_state.fd);
      i_error("Failed to get video info: %d", errno);
    }

  /* Here, we check the dimensions of the frame buffer. If we have enough
   * space to scale up the rendered image in both width and height, record
   * that so we can make use of it elsewhere.
   *
   * If we don't have enough frame buffer space for the game, quit!
   */

  if (g_graphics_state.vinfo.xres < SCREENWIDTH)
    {
      i_error("Resolution width of %u px < minimum of %u px\n",
              g_graphics_state.vinfo.xres, SCREENWIDTH);
    }

  if (g_graphics_state.vinfo.yres < SCREENHEIGHT)
    {
      i_error("Resolution height of %u px < minimum of %u px\n",
              g_graphics_state.vinfo.yres, SCREENHEIGHT);
    }

  xscale = g_graphics_state.vinfo.xres / SCREENWIDTH;
  yscale = g_graphics_state.vinfo.yres / SCREENHEIGHT;
  g_graphics_state.scale = xscale > yscale ? yscale : xscale;

  /* Get frame buffer plane info */

  if (ioctl(g_graphics_state.fd, FBIOGET_PLANEINFO,
            (unsigned long)((uintptr_t)&g_graphics_state.pinfo)) < 0)
    {
      i_error("ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno);
    }

  /* Initialize frame buffer memory for actual rendering */

  g_graphics_state.fbmem =
      mmap(NULL, g_graphics_state.pinfo.fblen, PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_FILE, g_graphics_state.fd, 0);
  if (g_graphics_state.fbmem == MAP_FAILED)
    {
      i_error("mmap() of frame buffer failed: %d\n", errno);
    }

  /* Create an 8-bit depth screen buffer for DOOM to render to */

  g_graphics_state.scrnbuf = malloc(SCREENWIDTH * SCREENHEIGHT);
  if (g_graphics_state.scrnbuf == NULL)
    {
      i_error("Couldn't allocate screen buffer: %d\n", errno);
    }

  /* Create the game window; this may switch graphic modes depending
   * on configuration.
   * AdjustWindowSize();
   */

  set_video_mode();

  /* Start with a clear black screen
   * (screen will be flipped after we set the palette)
   */

  memset(g_graphics_state.scrnbuf, 0, SCREENHEIGHT * SCREENWIDTH);

  /* Set the palette */

  doompal = w_cache_lump_name(("PLAYPAL"), PU_CACHE);
  i_set_palette(doompal);

  update_grab();

  /* On some systems, it takes a second or so for the screen to settle
   * after changing modes.  We include the option to add a delay when
   * setting the screen mode, so that the game doesn't start immediately
   * with the player unable to see anything.
   */

  if (fullscreen && !screensaver_mode)
    {
      usleep(startup_delay * 1000);
    }

  /* The actual 320x200 canvas that we draw to. This is the pixel buffer of
   * the 8-bit paletted screen buffer that gets blit on an intermediate
   * 32-bit RGBA screen buffer that gets loaded into a texture that gets
   * finally rendered into our window or full screen in i_finish_update().
   */

  i_video_buffer = g_graphics_state.scrnbuf;
  v_restore_buffer();

  /* Clear the screen to black. */

  memset(i_video_buffer, 0,
         SCREENWIDTH * SCREENHEIGHT * sizeof(*i_video_buffer));

  /* clear out any events waiting at the start and center the mouse */

#if 0
  while (SDL_PollEvent(&dummy))
    {
    };
#endif

  g_graphics_state.inited = true;

  /* Call i_shutdown_graphics on quit */

  i_at_exit(i_shutdown_graphics, true);
}

/* Bind all variables controlling video options into the configuration
 * file system.
 */

void i_bind_video_variables(void)
{
  m_bind_int_variable("use_mouse", &usemouse);
  m_bind_int_variable("fullscreen", &fullscreen);
  m_bind_int_variable("video_display", &video_display);
  m_bind_int_variable("integer_scaling", &integer_scaling);
  m_bind_int_variable("smooth_pixel_scaling", &smooth_pixel_scaling);
  m_bind_int_variable("vga_porch_flash", &vga_porch_flash);
  m_bind_int_variable("startup_delay", &startup_delay);
  m_bind_int_variable("fullscreen_width", &fullscreen_width);
  m_bind_int_variable("fullscreen_height", &fullscreen_height);
  m_bind_int_variable("force_software_renderer", &force_software_renderer);
  m_bind_int_variable("max_scaling_buffer_pixels",
                      &max_scaling_buffer_pixels);
  m_bind_int_variable("window_width", &window_width);
  m_bind_int_variable("window_height", &window_height);
  m_bind_int_variable("grabmouse", &grabmouse);
  m_bind_string_variable("video_driver", &video_driver);
  m_bind_string_variable("window_position", &window_position);
  m_bind_int_variable("usegamma", &usegamma);
  m_bind_int_variable("png_screenshots", &png_screenshots);
}
