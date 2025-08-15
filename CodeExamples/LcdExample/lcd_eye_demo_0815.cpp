// #include "LcdControl.h"
#include "../Doly/include/LcdControl_x86_sim.h"
#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdlib>

// LCD屏幕参数
const int SCREEN_WIDTH = LCD_WIDTH;
const int SCREEN_HEIGHT = LCD_HEIGHT;
const int SCREEN_CENTER_X = SCREEN_WIDTH / 2;
const int SCREEN_CENTER_Y = SCREEN_HEIGHT / 2;

// 眼睛参数 (根据图片调整)
const int EYE_BACKGROUND_RADIUS = 120;    // 整个眼睛背景半径
const int PUPIL_RADIUS = 75;              // 大的黑色瞳孔
const int IRIS_RING_WIDTH = 12;            // 蓝色虹膜环宽度
const int HIGHLIGHT_RADIUS = 20;          // 白色高光半径
const int HIGHLIGHT_OFFSET_X = -30;        // 高光X偏移
const int HIGHLIGHT_OFFSET_Y = -30;        // 高光Y偏移

// 定义颜色
struct Color {
    uint8_t r, g, b;
};

const Color COLOR_BLACK_BG = {0, 0, 0};           // 黑色屏幕背景
const Color COLOR_WHITE_EYE = {255, 255, 255};    // 白色眼球
const Color COLOR_BLACK_PUPIL = {0, 0, 0};        // 黑色瞳孔
const Color COLOR_BLUE_IRIS = {0, 150, 200};      // 蓝色虹膜
const Color COLOR_WHITE_HIGHLIGHT = {255, 255, 255}; // 白色高光
const Color COLOR_YELLOW_EYELID = {255, 200, 0};  // 黄色眼皮
const Color COLOR_TEAR = {135, 206, 250};         // 淡蓝色泪水
const Color COLOR_ANGRY_RED = {255, 80, 80};      // 愤怒红色

/**
 * @brief 在24位缓冲区中设置像素颜色
 */
void set_pixel_24bit(uint8_t* buffer, int x, int y, const Color& color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        int index = (y * SCREEN_WIDTH + x) * 3;
        buffer[index] = color.r;
        buffer[index + 1] = color.g;
        buffer[index + 2] = color.b;
    }
}

/**
 * @brief 清空整个24位缓冲区
 */
void clear_buffer_24bit(uint8_t* buffer, const Color& color) {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            set_pixel_24bit(buffer, x, y, color);
        }
    }
}

/**
 * @brief 绘制填充的圆形
 */
void draw_filled_circle_24bit(uint8_t* buffer, int center_x, int center_y, int radius, const Color& color) {
    int radius_sq = radius * radius;
    
    for (int y = center_y - radius; y <= center_y + radius; ++y) {
        for (int x = center_x - radius; x <= center_x + radius; ++x) {
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                int dx = x - center_x;
                int dy = y - center_y;
                if (dx * dx + dy * dy <= radius_sq) {
                    set_pixel_24bit(buffer, x, y, color);
                }
            }
        }
    }
}

/**
 * @brief 绘制环形（外圆减去内圆）
 */
void draw_ring_24bit(uint8_t* buffer, int center_x, int center_y, int inner_radius, int outer_radius, const Color& color) {
    int outer_sq = outer_radius * outer_radius;
    int inner_sq = inner_radius * inner_radius;
    
    for (int y = center_y - outer_radius; y <= center_y + outer_radius; ++y) {
        for (int x = center_x - outer_radius; x <= center_x + outer_radius; ++x) {
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                int dx = x - center_x;
                int dy = y - center_y;
                int dist_sq = dx * dx + dy * dy;
                if (dist_sq <= outer_sq && dist_sq > inner_sq) {
                    set_pixel_24bit(buffer, x, y, color);
                }
            }
        }
    }
}

/**
 * @brief 绘制椭圆（用于眨眼效果）
 */
void draw_filled_ellipse_24bit(uint8_t* buffer, int center_x, int center_y, int radius_x, int radius_y, const Color& color) {
    for (int y = center_y - radius_y; y <= center_y + radius_y; ++y) {
        for (int x = center_x - radius_x; x <= center_x + radius_x; ++x) {
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                float dx = (float)(x - center_x) / radius_x;
                float dy = (float)(y - center_y) / radius_y;
                if (dx * dx + dy * dy <= 1.0f) {
                    set_pixel_24bit(buffer, x, y, color);
                }
            }
        }
    }
}

/**
 * @brief 绘制四角星型高光
 */
void draw_star_highlight_24bit(uint8_t* buffer, int center_x, int center_y, int size, const Color& color) {
    // 四角星由四个三角形组成，每个三角形指向不同方向
    int half_size = size / 2;
    
    // 绘制四个三角形，形成四角星
    // 1. 向上的三角形
    for (int y = center_y - half_size; y <= center_y; ++y) {
        int width = (y - (center_y - half_size)) * 2 + 1;
        for (int x = center_x - width/2; x <= center_x + width/2; ++x) {
            set_pixel_24bit(buffer, x, y, color);
        }
    }
    
    // 2. 向下的三角形
    for (int y = center_y; y <= center_y + half_size; ++y) {
        int width = ((center_y + half_size) - y) * 2 + 1;
        for (int x = center_x - width/2; x <= center_x + width/2; ++x) {
            set_pixel_24bit(buffer, x, y, color);
        }
    }
    
    // 3. 向左的三角形
    for (int x = center_x - half_size; x <= center_x; ++x) {
        int height = (x - (center_x - half_size)) * 2 + 1;
        for (int y = center_y - height/2; y <= center_y + height/2; ++y) {
            set_pixel_24bit(buffer, x, y, color);
        }
    }
    
    // 4. 向右的三角形
    for (int x = center_x; x <= center_x + half_size; ++x) {
        int height = ((center_x + half_size) - x) * 2 + 1;
        for (int y = center_y - height/2; y <= center_y + height/2; ++y) {
            set_pixel_24bit(buffer, x, y, color);
        }
    }
}

/**
 * @brief 绘制完整的卡通眼睛（根据图片风格）
 */
void draw_cartoon_eye_24bit(uint8_t* buffer, int pupil_offset_x = 0, int pupil_offset_y = 0,
                           const Color& iris_color = COLOR_BLUE_IRIS, bool show_highlight = true, bool star_highlight = false) {
    // 1. 清空为黑色背景
    clear_buffer_24bit(buffer, COLOR_BLACK_BG);
    
    // 2. 绘制白色眼球背景
    draw_filled_circle_24bit(buffer, SCREEN_CENTER_X, SCREEN_CENTER_Y, EYE_BACKGROUND_RADIUS, COLOR_WHITE_EYE);
    
    // 3. 绘制黑色大瞳孔
    draw_filled_circle_24bit(buffer, SCREEN_CENTER_X + pupil_offset_x, SCREEN_CENTER_Y + pupil_offset_y, 
                            PUPIL_RADIUS, COLOR_BLACK_PUPIL);
    
    // 4. 绘制蓝色虹膜环
    draw_ring_24bit(buffer, SCREEN_CENTER_X + pupil_offset_x, SCREEN_CENTER_Y + pupil_offset_y,
                   PUPIL_RADIUS, PUPIL_RADIUS + IRIS_RING_WIDTH, iris_color);
    
    // 5. 绘制高光点
    if (show_highlight) {
        if (star_highlight) {
            // 四角星型高光
            draw_star_highlight_24bit(buffer, 
                                    SCREEN_CENTER_X + pupil_offset_x + HIGHLIGHT_OFFSET_X, 
                                    SCREEN_CENTER_Y + pupil_offset_y + HIGHLIGHT_OFFSET_Y, 
                                    HIGHLIGHT_RADIUS * 2, COLOR_WHITE_HIGHLIGHT);
        } else {
            // 圆形高光
            draw_filled_circle_24bit(buffer, 
                                    SCREEN_CENTER_X + pupil_offset_x + HIGHLIGHT_OFFSET_X, 
                                    SCREEN_CENTER_Y + pupil_offset_y + HIGHLIGHT_OFFSET_Y, 
                                    HIGHLIGHT_RADIUS, COLOR_WHITE_HIGHLIGHT);
        }
    }
}

/**
 * @brief 绘制眨眼状态 - 黄色眼皮覆盖，保持四角星型高光
 */
void draw_blinking_eye_24bit(uint8_t* buffer, float blink_progress, bool star_highlight = true) {
    // blink_progress: 0.0 = 完全睁开, 1.0 = 完全闭上
    
    // 先绘制正常眼睛，使用四角星型高光
    draw_cartoon_eye_24bit(buffer, 0, 0, COLOR_BLUE_IRIS, true, star_highlight);
    
    // 计算眼皮覆盖的高度
    int eyelid_height = (int)(blink_progress * EYE_BACKGROUND_RADIUS * 2);
    
    // 从上方绘制黄色眼皮覆盖
    for (int y = SCREEN_CENTER_Y - EYE_BACKGROUND_RADIUS; y < SCREEN_CENTER_Y - EYE_BACKGROUND_RADIUS + eyelid_height; ++y) {
        for (int x = SCREEN_CENTER_X - EYE_BACKGROUND_RADIUS; x <= SCREEN_CENTER_X + EYE_BACKGROUND_RADIUS; ++x) {
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                // 检查是否在眼睛圆形区域内
                int dx = x - SCREEN_CENTER_X;
                int dy = y - SCREEN_CENTER_Y;
                if (dx * dx + dy * dy <= EYE_BACKGROUND_RADIUS * EYE_BACKGROUND_RADIUS) {
                    set_pixel_24bit(buffer, x, y, COLOR_YELLOW_EYELID);
                }
            }
        }
    }
}

/**
 * @brief 绘制完全闭眼状态
 */
void draw_closed_eye_24bit(uint8_t* buffer) {
    clear_buffer_24bit(buffer, COLOR_BLACK_BG);
    // 绘制黄色椭圆表示闭眼
    draw_filled_ellipse_24bit(buffer, SCREEN_CENTER_X, SCREEN_CENTER_Y, 
                             EYE_BACKGROUND_RADIUS, 8, COLOR_YELLOW_EYELID);
}

/**
 * @brief 绘制泪滴
 */
void draw_tear_24bit(uint8_t* buffer, int x, int y, int size = 8) {
    // 泪滴主体
    draw_filled_circle_24bit(buffer, x, y, size, COLOR_TEAR);
    // 泪滴尖端
    for (int i = 1; i <= size/2; ++i) {
        int tear_width = size - i;
        for (int dx = -tear_width/2; dx <= tear_width/2; ++dx) {
            set_pixel_24bit(buffer, x + dx, y + size + i, COLOR_TEAR);
        }
    }
}

/**
 * @brief 将24位缓冲区写入LCD
 */
void write_eye_to_lcd(const std::vector<uint8_t>& temp_24bit_buffer, LcdData* frame_data) {
    int lcd_buffer_size = LcdControl::getBufferSize();
    std::vector<uint8_t> lcd_buffer(lcd_buffer_size);
    
    LcdControl::LcdBufferFrom24Bit(lcd_buffer.data(), 
                                  const_cast<uint8_t*>(temp_24bit_buffer.data()));
    
    std::memcpy(frame_data->buffer, lcd_buffer.data(), lcd_buffer_size);
    
    int result = LcdControl::writeLcd(frame_data);
    if (result != 0) {
        std::cerr << "Write LCD failed: " << (int)result << std::endl;
    }
}

/**
 * @brief 开心表情动画 - 正常眼睛 + 眨眼 + 眼球微动
 */
void animate_happy_face(LcdData* frame_data_left, LcdData* frame_data_right,
                       std::vector<uint8_t>& temp_buffer_left,
                       std::vector<uint8_t>& temp_buffer_right) {
    std::cout << "😊 开始开心表情..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 眼球微动模式 - 表现兴奋状态，进一步减小幅度，增加频率
    int eye_movements[][2] = {
        {0, 0},     // 正中
        {-1, -1},   // 左上
        {1, -1},    // 右上
        {-1, 1},    // 左下
        {1, 1},     // 右下
        {0, 0}      // 回正中
    };
    int movement_count = sizeof(eye_movements) / sizeof(eye_movements[0]);
    int current_movement = 0;
    
    for (int i = 0; i < 80; ++i) {
        // 获取当前眼球位置
        int offset_x = eye_movements[current_movement][0];
        int offset_y = eye_movements[current_movement][1];
        
        // 正常睁开的眼睛，使用四角星型高光
        draw_cartoon_eye_24bit(temp_buffer_left.data(), offset_x, offset_y, COLOR_BLUE_IRIS, true, true);
        draw_cartoon_eye_24bit(temp_buffer_right.data(), offset_x, offset_y, COLOR_BLUE_IRIS, true, true);
        
        write_eye_to_lcd(temp_buffer_left, frame_data_left);
        write_eye_to_lcd(temp_buffer_right, frame_data_right);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        
        // 每3帧切换眼球位置，进一步增加微动频率
        if (i % 3 == 2) {
            current_movement = (current_movement + 1) % movement_count;
        }
        
        // 每40帧眨眼一次
        if (i % 40 == 35) {
            // 眨眼动画序列，保持四角星型高光
            float blink_steps[] = {0.3f, 0.7f, 1.0f, 0.7f, 0.3f};
            int step_count = sizeof(blink_steps) / sizeof(blink_steps[0]);
            
            for (int step = 0; step < step_count; ++step) {
                draw_blinking_eye_24bit(temp_buffer_left.data(), blink_steps[step], true);
                draw_blinking_eye_24bit(temp_buffer_right.data(), blink_steps[step], true);
                
                write_eye_to_lcd(temp_buffer_left, frame_data_left);
                write_eye_to_lcd(temp_buffer_right, frame_data_right);
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "😊 开心表情完成 - 实际运行" << (duration.count() / 1000.0) << "秒" << std::endl;
}

/**
 * @brief 悲伤表情动画 - 向下看 + 流泪
 */
void animate_sad_face(LcdData* frame_data_left, LcdData* frame_data_right,
                     std::vector<uint8_t>& temp_buffer_left,
                     std::vector<uint8_t>& temp_buffer_right) {
    std::cout << "😢 开始悲伤表情..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    const int pupil_offset_y = 12; // 眼球向下看
    
    // 泪水从眼睛下方流下
    for (int tear_y = SCREEN_CENTER_Y + EYE_BACKGROUND_RADIUS + 15; 
         tear_y < SCREEN_HEIGHT - 30; tear_y += 6) {
        
        draw_cartoon_eye_24bit(temp_buffer_left.data(), 0, pupil_offset_y);
        draw_cartoon_eye_24bit(temp_buffer_right.data(), 0, pupil_offset_y);
        
        // 左眼流泪
        draw_tear_24bit(temp_buffer_left.data(), SCREEN_CENTER_X - 30, tear_y);
        // 右眼流泪
        draw_tear_24bit(temp_buffer_right.data(), SCREEN_CENTER_X + 30, tear_y);
        
        write_eye_to_lcd(temp_buffer_left, frame_data_left);
        write_eye_to_lcd(temp_buffer_right, frame_data_right);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    
    // 保持悲伤表情
    for (int i = 0; i < 30; ++i) {
        draw_cartoon_eye_24bit(temp_buffer_left.data(), 0, pupil_offset_y);
        draw_cartoon_eye_24bit(temp_buffer_right.data(), 0, pupil_offset_y);
        write_eye_to_lcd(temp_buffer_left, frame_data_left);
        write_eye_to_lcd(temp_buffer_right, frame_data_right);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "😢 悲伤表情完成 - 实际运行" << (duration.count() / 1000.0) << "秒" << std::endl;
}

/**
 * @brief 愤怒表情动画 - 红色虹膜 + 眯眼
 */
void animate_angry_face(LcdData* frame_data_left, LcdData* frame_data_right,
                       std::vector<uint8_t>& temp_buffer_left,
                       std::vector<uint8_t>& temp_buffer_right) {
    std::cout << "😠 开始愤怒表情..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 50; ++i) {
        // 绘制愤怒的眼睛 - 红色虹膜，无高光显得更凶
        draw_cartoon_eye_24bit(temp_buffer_left.data(), 0, 0, COLOR_ANGRY_RED, false);
        draw_cartoon_eye_24bit(temp_buffer_right.data(), 0, 0, COLOR_ANGRY_RED, false);
        
        write_eye_to_lcd(temp_buffer_left, frame_data_left);
        write_eye_to_lcd(temp_buffer_right, frame_data_right);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        
        // 偶尔眯眼增加愤怒效果
        if (i % 15 == 10) {
            draw_blinking_eye_24bit(temp_buffer_left.data(), 0.4f);
            draw_blinking_eye_24bit(temp_buffer_right.data(), 0.4f);
            write_eye_to_lcd(temp_buffer_left, frame_data_left);
            write_eye_to_lcd(temp_buffer_right, frame_data_right);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "😠 愤怒表情完成 - 实际运行" << (duration.count() / 1000.0) << "秒" << std::endl;
}

/**
 * @brief 静止眨眼动画 - 眼球移动 + 自然眨眼
 */
void animate_idle_blink(LcdData* frame_data_left, LcdData* frame_data_right,
                       std::vector<uint8_t>& temp_buffer_left,
                       std::vector<uint8_t>& temp_buffer_right) {
    std::cout << "😐 开始静止状态..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 眼球移动模式
    int eye_movements[][2] = {
        {0, 0},     // 正中
        {-8, -5},   // 左上
        {8, -5},    // 右上
        {0, 8},     // 向下
        {-12, 0},   // 左
        {12, 0},    // 右
        {0, 0}      // 回正中
    };
    int movement_count = sizeof(eye_movements) / sizeof(eye_movements[0]);
    
    for (int cycle = 0; cycle < 2; ++cycle) {
        for (int move = 0; move < movement_count; ++move) {
            int offset_x = eye_movements[move][0];
            int offset_y = eye_movements[move][1];
            
            for (int frame = 0; frame < 20; ++frame) {
                draw_cartoon_eye_24bit(temp_buffer_left.data(), offset_x, offset_y);
                draw_cartoon_eye_24bit(temp_buffer_right.data(), offset_x, offset_y);
                
                write_eye_to_lcd(temp_buffer_left, frame_data_left);
                write_eye_to_lcd(temp_buffer_right, frame_data_right);
                std::this_thread::sleep_for(std::chrono::milliseconds(70));
                
                // 随机眨眼
                if (frame == 15 && move % 4 == 1) {
                    draw_blinking_eye_24bit(temp_buffer_left.data(), 1.0f, true);
                    draw_blinking_eye_24bit(temp_buffer_right.data(), 1.0f, true);
                    write_eye_to_lcd(temp_buffer_left, frame_data_left);
                    write_eye_to_lcd(temp_buffer_right, frame_data_right);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "😐 静止状态完成 - 实际运行" << (duration.count() / 1000.0) << "秒" << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << "=== 眼睛动画系统启动 ===" << std::endl;
    
    // 初始化LCD
    int8_t init_result = LcdControl::init(LCD_12BIT);
    if (init_result != 0) {
        std::cerr << "LCD初始化失败! 错误: " << (int)init_result << std::endl;
        return -1;
    }
    
    // 设置中等亮度
    LcdControl::setBrightness(7);
    
    // 获取缓冲区信息
    int lcd_buffer_size = LcdControl::getBufferSize();
    if (lcd_buffer_size <= 0) {
        std::cerr << "无效的LCD缓冲区大小: " << lcd_buffer_size << std::endl;
        LcdControl::release();
        return -1;
    }
    
    std::cout << "LCD初始化成功!" << std::endl;
    
    // 创建缓冲区
    std::vector<uint8_t> left_lcd_buffer(lcd_buffer_size);
    std::vector<uint8_t> right_lcd_buffer(lcd_buffer_size);
    
    int temp_buffer_size = SCREEN_WIDTH * SCREEN_HEIGHT * 3; // 24位缓冲区
    std::vector<uint8_t> temp_buffer_left(temp_buffer_size);
    std::vector<uint8_t> temp_buffer_right(temp_buffer_size);
    
    LcdData frame_data_left = { LcdLeft, left_lcd_buffer.data() };
    LcdData frame_data_right = { LcdRight, right_lcd_buffer.data() };
    
    // 检查LCD状态
    if (!LcdControl::isActive()) {
        std::cerr << "LCD未激活!" << std::endl;
        LcdControl::release();
        return -1;
    }
    
    std::cout << "开始眼睛动画..." << std::endl;
    
    // 主动画循环
    int animation_cycle = 0;
    while (true) {
        std::cout << "\n--- 第 " << ++animation_cycle << " 轮动画 ---" << std::endl;
        
        animate_happy_face(&frame_data_left, &frame_data_right, temp_buffer_left, temp_buffer_right);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // animate_idle_blink(&frame_data_left, &frame_data_right, temp_buffer_left, temp_buffer_right);
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // animate_sad_face(&frame_data_left, &frame_data_right, temp_buffer_left, temp_buffer_right);
        // std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // animate_angry_face(&frame_data_left, &frame_data_right, temp_buffer_left, temp_buffer_right);
        // std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 可以添加退出条件
        if (animation_cycle >= 3) {
            std::cout << "演示完成！退出程序..." << std::endl;
            break;
        }
    }
    
    // 清理资源
    LcdControl::release();
    std::cout << "动画系统关闭完成。" << std::endl;
    return 0;
}