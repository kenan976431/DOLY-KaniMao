#pragma once
#include <stdint.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <SDL2/SDL.h>

// 模拟LCD屏幕参数
#define LCD_WIDTH 240
#define LCD_HEIGHT 240

// 日志级别定义
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

// 当前日志级别（可以调整）
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO

// 日志宏
#define LOG_ERROR(msg) if(CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR) std::cerr << "[ERROR] " << msg << std::endl
#define LOG_WARN(msg)  if(CURRENT_LOG_LEVEL >= LOG_LEVEL_WARN)  std::cout << "[WARN]  " << msg << std::endl
#define LOG_INFO(msg)  if(CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO)  std::cout << "[INFO]  " << msg << std::endl
#define LOG_DEBUG(msg) if(CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) std::cout << "[DEBUG] " << msg << std::endl

enum LcdColorDepth :uint8_t
{
    // 0x06 = 18 bit,  0x03 = 12 bit
    LCD_12BIT = 0x03,
    LCD_18BIT = 0x06,
};

enum LcdSide :uint8_t
{
    LcdLeft = 0,
    LcdRight = 1,
};

struct LcdData
{
    uint8_t side;
    uint8_t* buffer;
};

namespace LcdControl
{
    // 模拟LCD状态
    static bool lcd_initialized = false;
    static LcdColorDepth current_depth = LCD_12BIT;
    static uint8_t current_brightness = 7;
    
    // SDL2相关变量
    static SDL_Window* window = nullptr;
    static SDL_Renderer* renderer = nullptr;
    static SDL_Texture* texture = nullptr;
    static bool sdl_initialized = false;
    
    // 初始化SDL2
    static bool initSDL() {
        if (sdl_initialized) return true;
        
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_ERROR("SDL could not initialize! SDL_Error: " << SDL_GetError());
            return false;
        }
        
        // 创建窗口，缩小到1/8大小并在屏幕右下角显示
        int window_width = LCD_WIDTH / 4;   // 240/4 = 60
        int window_height = LCD_HEIGHT / 4; // 240/4 = 60
        
        // 获取屏幕尺寸
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        int screen_width = DM.w;
        int screen_height = DM.h;
        
        // 计算右下角位置
        int window_x = screen_width - window_width - 80;  // 距离右边缘20像素
        int window_y = screen_height - window_height - 80; // 距离下边缘20像素
        
        window = SDL_CreateWindow("LCD Eye Demo (X86 Simulated)", 
                                window_x, window_y,
                                window_width, window_height, 
                                SDL_WINDOW_SHOWN);
        if (!window) {
            LOG_ERROR("Window could not be created! SDL_Error: " << SDL_GetError());
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            LOG_ERROR("Renderer could not be created! SDL_Error: " << SDL_GetError());
            return false;
        }
        
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, 
                                  SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
        if (!texture) {
            LOG_ERROR("Texture could not be created! SDL_Error: " << SDL_GetError());
            return false;
        }
        
        sdl_initialized = true;
        LOG_INFO("SDL2 initialized successfully for visual display!");
        return true;
    }
    
    // 清理SDL2资源
    static void cleanupSDL() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        if (sdl_initialized) {
            SDL_Quit();
            sdl_initialized = false;
        }
    }
    
    // Initialize lcd - 模拟初始化
    inline int8_t init(LcdColorDepth depth = LCD_12BIT) {
        if (lcd_initialized) {
            LOG_WARN("LCD already initialized!");
            return 1;
        }
        
        // 初始化SDL2
        if (!initSDL()) {
            LOG_ERROR("Failed to initialize SDL2!");
            return -1;
        }
        
        lcd_initialized = true;
        current_depth = depth;
        LOG_INFO("LCD initialized successfully! (Simulated with SDL2 display)");
        LOG_INFO("Color depth: " << (depth == LCD_12BIT ? "12-bit" : "18-bit"));
        return 0;
    }

    // release file descriptor and deinitialize
    inline int8_t release() {
        if (!lcd_initialized) {
            LOG_WARN("LCD already closed or not opened!");
            return 1;
        }
        
        lcd_initialized = false;
        cleanupSDL();
        LOG_INFO("LCD released successfully! (Simulated)");
        return 0;
    }

    // returns state of lcds
    inline bool isActive() {
        return lcd_initialized;
    }

    // fill lcd with RGB - 模拟填充颜色
    inline void LcdColorFill(LcdSide side, uint8_t R, uint8_t G, uint8_t B) {
        if (!lcd_initialized) {
            LOG_ERROR("LCD not initialized!");
            return;
        }
        
        LOG_DEBUG("Filling LCD with RGB(" << (int)R << "," << (int)G << "," << (int)B << ")");
    }

    // write buffer data to lcd - 模拟写入数据，并显示到SDL窗口
    inline int8_t writeLcd(LcdData* frame_data) {
        if (!lcd_initialized) {
            LOG_ERROR("LCD not initialized!");
            return -2;
        }
        
        LOG_DEBUG("Writing to LCD");
        
        // 更新SDL纹理并显示
        if (texture && frame_data->buffer) {
            SDL_UpdateTexture(texture, nullptr, frame_data->buffer, LCD_WIDTH * 3);
            
            // 清空渲染器
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            // 渲染纹理，缩放到当前窗口大小
            int current_width, current_height;
            SDL_GetWindowSize(window, &current_width, &current_height);
            SDL_Rect dest_rect = {0, 0, current_width, current_height};
            SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);
            
            // 显示到屏幕
            SDL_RenderPresent(renderer);
            
            // 处理SDL事件（保持窗口响应）
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    // 用户关闭窗口，退出程序
                    exit(0);
                }
            }
        }
        
        return 0;
    }

    // return lcd buffer size
    inline int getBufferSize() {
        // 240*240*3 = 172800 bytes for 24-bit color
        return LCD_WIDTH * LCD_HEIGHT * 3;
    }

    // returns lcd color depth
    inline LcdColorDepth getColorDepth() {
        return current_depth;
    }

    // brightness value min = 0, max = 10
    inline int8_t setBrightness(uint8_t value) {
        if (value > 10) {
            LOG_ERROR("Brightness value out of range! Max is 10.");
            return -1;
        }
        
        current_brightness = value;
        LOG_DEBUG("Brightness set to " << (int)value);
        return 0;
    }

    // converts 24 bit image to lcd image depth - 模拟转换
    inline void LcdBufferFrom24Bit(uint8_t* output, uint8_t* input) {
        // 在模拟环境中，我们直接复制数据
        // 在实际ARM环境中，这里会进行颜色深度转换
        int buffer_size = LCD_WIDTH * LCD_HEIGHT * 3;
        std::memcpy(output, input, buffer_size);
        LOG_DEBUG("Converted 24-bit buffer to LCD format");
    }
};
