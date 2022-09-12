#include <SDL.h>
#include <SDL_rect.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "pawos.h"

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Rect rect;

SDL_Texture *texture;

static const uint8_t set[] = {1, 2, 4, 8, 16, 32, 64, 128},
                     clr[] = {(uint8_t)~1,  (uint8_t)~2,  (uint8_t)~4,  (uint8_t)~8,
                              (uint8_t)~16, (uint8_t)~32, (uint8_t)~64, (uint8_t)~128};

void redraw()
{
    rect.x = 1;
    rect.y = 1;
    rect.w = 2;
    rect.h = 2;

    SDL_SetRenderDrawColor(renderer, /* RGBA: black */ 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0xc7, 0xc7, 0xc1, 0xFF);
    // SDL_RenderFillRect(renderer, &rect);

    for (int x = 0; x < 128; x++)
    {
        for (int y = 0; y < 128; y++)
        {
            bool en = (display._drawBuffer[(y * (128 + 16) + x + 8) / 8] & set[x & 7]) > 0;

            rect.x = x * 2;
            rect.y = y * 2;
            if (en)
            {
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

uint32_t ticksForNextKeyDown = 0;

bool handle_events()
{
    if (sleepMillisRemain > 0)
    {
        sleepMillisRemain--;
    }
    else
    {
        loop();
    }

    SDL_Event event;
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT)
    {
        return false;
    }
    
    //SDL_KEYUP
    if (event.type == SDL_KEYDOWN)
    {
        uint32_t ticksNow = SDL_GetTicks();
        if (SDL_TICKS_PASSED(ticksNow, ticksForNextKeyDown))
        {
            // Throttle keydown events for 10ms.
            ticksForNextKeyDown = ticksNow + 10;
            switch (event.key.keysym.sym)
            {
            case SDLK_UP:
                keysState |= 0x10;
                break;
            case SDLK_DOWN:
                keysState |= 0x20;
                break;
            case SDLK_RIGHT:
                keysState |= 0x80;
                break;
            case SDLK_LEFT:
                keysState |= 0x40;
                break;
            case SDLK_z:
                keysState |= 0x1;
                break;
            case SDLK_x:
                keysState |= 0x2;
                break;
            case SDLK_c:
                keysState |= 0x4;
                break;
            case SDLK_v:
                keysState |= 0x8;
                sleepMillisRemain = 0;
                break;
            }
            
        }
    }

    redraw();
    SDL_Delay(1);
    return true;
}

void run_main_loop()
{
#ifdef __EMSCRIPTEN__
    printf("ver %f", 1.2);
    emscripten_set_main_loop([]() { handle_events(); }, 0, true);
#else
    while (handle_events())
        ;
#endif


}

int main(int argc, char *argv[])
{
    init();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_CreateWindowAndRenderer(256, 256, 0, &window, &renderer);

    run_main_loop();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}