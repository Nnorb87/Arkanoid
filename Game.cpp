#include "Engine.h"
#include <stdlib.h>
#include <memory.h>

#include <windows.h> // OutputDebugString()
#include <cstdio>   // sprintf()
#include <cstdlib>  // rand()
#include <vector> // store falling brick
#include <string>

const uint32_t WHITE = 0xFFFFFFFF;
const uint32_t RED = 0xFFFF0000;
const uint32_t GREEN = 0xFF00FF00;
const uint32_t BLUE = 0xFF0000FF;
const uint32_t PURPLE = 0xFF800080;
const uint32_t PINK = 0xFFFFC0CB;
const uint32_t YELLOW = 0xFFFFFF00;
const uint32_t ORANGE = 0xFFFFA500;
const uint32_t GRAY = 0x808080;

struct vector2 {
    float x, y;
};

struct rectangle {
    rectangle() {};
    rectangle(float _x, float _y, float _width, float _height, bool _isVisible, uint32_t _color) :
    x(_x), y(_y), width(_width), height(_height), isVisible(_isVisible), color(_color)
    {};

    float x{}, y{}, width{}, height{};
    bool isVisible{ true };
    uint32_t color{ WHITE };
};

struct ballRect : public rectangle {
    ballRect(float _x, float _y, float _width, float _height, bool _isVisible, uint32_t _color, vector2 _velocity) :
        rectangle(_x, _y, _width, _height, _isVisible, _color), velocity(_velocity), originalVelocity(_velocity) {};

    vector2 velocity{};
    vector2 const originalVelocity{};
    vector2 lastPosition{};
    vector2 maxVelocity{700.f, 700.f};
};

int playerScore{};
int bulletCount{};

rectangle paddle{ SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 200, 20, true, WHITE };
float paddleSpeed{ 800.f };
float paddleStrength{ 100.f };  // effect on the balls x velocity after hit

ballRect ball{ SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT / 2 - 20, 20, 20, true, PURPLE, vector2{ 250.f, 250.f } };
ballRect ball2{ SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT / 2 - 20, 20, 20, true, YELLOW, vector2{ 200.f, 200.f } };
ballRect* balls[2]{ &ball, &ball2 };

rectangle bricks[3][10];
std::vector<rectangle>* fallingBricks{};
std::vector<rectangle>* bullets{};
float brickFallSpeed{100.f};
float bulletSpeed{ 300.f };

float timer{};

char char_buffer[100];       // utility to convert floats to string in debugging
bool mouseControl{ true };

void resetGame() {
    playerScore = 0;
    bulletCount = 0;

    for (ballRect* ball : balls) {
        ball->isVisible = true;
        ball->velocity = ball->originalVelocity;
        int randomVelocityMultiplier[2]{ 1,-1 };
        ball->velocity.x *= randomVelocityMultiplier[rand() % 2];

        // reset ball position
        ball->x = SCREEN_WIDTH / 2 - 20;
        ball->y = SCREEN_HEIGHT / 2 - 20;
    }

    // reset bricks visibility
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            if (!bricks[i][j].isVisible) {
                bricks[i][j].isVisible = true;
            }
        }
    }

    // clear falling bricks
    for (auto it = fallingBricks->begin(); it != fallingBricks->end();) {
        it = fallingBricks->erase(it);
        }

    // clear remaining bullets
    for (auto it = bullets->begin(); it != bullets->end();) {
        it = bullets->erase(it);
    }
}

bool checkCollision(const rectangle& rec1, const rectangle& rec2) {
    // Calculate the edges of each rectangle
    float left1 = rec1.x;
    float right1 = rec1.x + rec1.width;
    float top1 = rec1.y;
    float bottom1 = rec1.y + rec1.height;

    float left2 = rec2.x;
    float right2 = rec2.x + rec2.width;
    float top2 = rec2.y;
    float bottom2 = rec2.y + rec2.height;

    // Check for intersection
    if (right1 >= left2 && left1 <= right2 && bottom1 >= top2 && top1 <= bottom2) {
        return true; // Rectangles intersect
    }
    return false; // No collision
}

void DrawRectangle(rectangle& rect) {
    if (!rect.isVisible) return;
    if (rect.y + rect.height > SCREEN_HEIGHT || rect.x + rect.width > SCREEN_WIDTH) return;     // prevents drawing outside the window

    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            buffer[y][x] = rect.color;
        }
    }
}

void DrawChar(vector2 _pivot, float _letterHeight, uint32_t color, bool showTop, bool showTopLeft, bool showTopRight, bool showMiddle, bool showBottom, bool showBottomLeft, bool showBottomRight, bool _needRightOffset = false) {
    
    float letterWidth = 40;
    float lineThickness = 10;
    float letterHeight = _letterHeight;
    float rightOffset = _needRightOffset ? 5 : 0;
    vector2 pivot = _pivot;

    rectangle top = (rectangle{ pivot.x, SCREEN_HEIGHT - letterHeight, letterWidth, lineThickness, showTop, color });
    rectangle top_left = (rectangle{ pivot.x, SCREEN_HEIGHT - letterHeight, lineThickness, letterHeight/2, showTopLeft, color });
    rectangle top_right = (rectangle{ pivot.x + letterWidth - lineThickness + rightOffset, SCREEN_HEIGHT - letterHeight, lineThickness, letterHeight/2, showTopRight, color });

    rectangle middle = (rectangle{ pivot.x, SCREEN_HEIGHT - letterHeight/2 -lineThickness/2, letterWidth, lineThickness, showMiddle, color });

    rectangle bottom = (rectangle{ pivot.x, SCREEN_HEIGHT - lineThickness, letterWidth, lineThickness, showBottom, color });
    rectangle bottom_left = (rectangle{ pivot.x, SCREEN_HEIGHT - letterHeight/2, lineThickness, letterHeight/2, showBottomLeft, color });
    rectangle bottom_right = (rectangle{ pivot.x + letterWidth - lineThickness, SCREEN_HEIGHT - letterHeight/2, lineThickness, letterHeight/2, showBottomRight, color });

    DrawRectangle(top);
    DrawRectangle(top_left);
    DrawRectangle(top_right);
    DrawRectangle(middle);
    DrawRectangle(bottom);
    DrawRectangle(bottom_left);
    DrawRectangle(bottom_right);
}

void DrawPoints(std::string stringScore, float scoreStart_X, float scorePadding, float letterHeight) {

    for (size_t i = 0; i < stringScore.length(); ++i) {
        switch (stringScore[i]) {
        case '0':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, true, false, true, true, true);
            break;
        case '1':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, false, false, true, false, false, false, true);
            break;
        case '2':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, false, true, true, true, true, false);
            break;
        case '3':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, false, true, true, true, false, true);
            break;
        case '4':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, false, true, true, true, false, false, true);
            break;
        case '5':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, false, true, true, false, true);
            break;
        case '6':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, false, true, true, true, true);
            break;
        case '7':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, true, false, false, false, true);
            break;
        case '8':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, true, true, true, true, true);
            break;
        case '9':
            DrawChar({ scoreStart_X + i * scorePadding, SCREEN_HEIGHT - letterHeight }, letterHeight, YELLOW, true, true, true, true, true, false, true);
            break;

        }
    }
}
//
//  You are free to modify this file
//

//  is_key_pressed(int button_vk_code) - check if a key is pressed,
//                                       use keycodes (VK_SPACE, VK_RIGHT, VK_LEFT, VK_UP, VK_DOWN, 'A', 'B')
//
//  get_cursor_x(), get_cursor_y() - get mouse cursor position
//  is_mouse_button_pressed(int button) - check if mouse button is pressed (0 - left button, 1 - right button)
//  clear_buffer() - set all pixels in buffer to 'black'
//  is_window_active() - returns true if window is active
//  schedule_quit_game() - quit game after act()

// initialize game data in this function
void initialize() {
    OutputDebugString("GAME INIT\n");

    fallingBricks = new std::vector<rectangle>();
    bullets = new std::vector<rectangle>();


    float padding = 10;
    float brick_width = SCREEN_WIDTH / 10 - padding;
    float brick_height = 30;
    uint32_t colorArray[]{ GREEN, BLUE, RED };

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            bricks[i][j] = rectangle{ j * (brick_width + padding) + padding , i * (brick_height + padding) + padding, brick_width, brick_height, true, colorArray[i]};
        }
    }


}

// this function is called to update game data,[F
// dt - time elapsed since the previous update (in seconds)

void act(float dt) {
    timer += dt;
    // quit game on hitting escape
    if (is_key_pressed(VK_ESCAPE)) {
        schedule_quit_game();
    }

    // handle paddle control
    if (mouseControl) {
        // mouse
        float cursor_x = get_cursor_x();
        if (cursor_x >= paddle.width / 2 && cursor_x <= SCREEN_WIDTH - paddle.width / 2) {
            paddle.x = get_cursor_x() - paddle.width / 2;
        }
    }
    else {
        // keyboard
        if (is_key_pressed(VK_LEFT) && paddle.x >= 0) {
            paddle.x -= paddleSpeed * dt;
        }
        else if (is_key_pressed(VK_RIGHT) && paddle.x <= SCREEN_WIDTH - paddle.width) {
            paddle.x += paddleSpeed * dt;
        }
    }

    // shoot
    if ((is_mouse_button_pressed(0) || is_key_pressed(VK_SPACE)) && bulletCount && (timer > .1f)) {
        bullets->push_back({paddle.x + paddle.width/2, paddle.y - 10, 5, 10, true, WHITE});
        bulletCount--;
        timer = 0.f;
    }

    // move bullets
    for (rectangle& bullet : *bullets) {
        if (bullet.y < 0) bullet.isVisible = false;
        bullet.y -= bulletSpeed * dt;
    }

    // handle bullet collisions
    for (rectangle& bullet : *bullets) {
        if (!bullet.isVisible) continue;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 10; j++) {
                rectangle* brick = &bricks[i][j];
                if (brick->isVisible) {
                    if (checkCollision(bullet, *brick)) {
                        brick->isVisible = false;
                        bullet.isVisible = false;
                        switch (brick->color) {
                        case RED:
                            playerScore++;
                            break;
                        case BLUE:
                            playerScore += 2;
                            break;
                        case GREEN:
                            playerScore += 3;
                            break;
                        }
                    }
                }
            }
        }
    }

    // update ball position
    for (ballRect* ball : balls) {
        ball->x += ball->velocity.x * dt;
        ball->y += ball->velocity.y * dt;

        // handle ball collision against paddle and wall
        bool paddleCollision = checkCollision(paddle, *ball);

        if (paddleCollision) {
            // if collided with overlap put back to the last safe position to save program from crash
            if ((ball->y + ball->height) > paddle.y) {
                ball->y = paddle.y -ball->height - 1;
            }

            ball->velocity.y *= -1;
            bool isGoingLeft = ball->velocity.x < 0;

            if (ball->x + ball->width/2 < paddle.x + paddle.width / 2) {
                // left side hit
                if (!isGoingLeft) ball->velocity.x *= -1;

                if (ball->x + ball->width / 2 < paddle.x + paddle.width / 5) {
                    // outter hit increase velocity
                    if ((std::abs(ball->velocity.x) < ball->maxVelocity.x) && (std::abs(ball->velocity.y) < ball->maxVelocity.y)) {
                        ball->velocity.x -= paddleStrength;
                        ball->velocity.y -= paddleStrength;
                    }
                    OutputDebugString("BALL SPEED INCREASED\n");
                }
                else if (ball->x + ball->width / 2 < paddle.x + 2 * (paddle.width / 5)){
                    // inner hit reduce velocity
                    if ((std::abs(ball->velocity.x) > ball->originalVelocity.x) && (std::abs(ball->velocity.y) > ball->originalVelocity.y)) {
                        ball->velocity.x += paddleStrength;
                        ball->velocity.y += paddleStrength;
                    };
                    OutputDebugString("BALL SPEED DECREASED\n");
  
                }
                else {
                    // center hit no effect on velocity
                    OutputDebugString("NO CHANGE IN SPEED\n");
                }
            }
            else {
                // right side hit
                if (isGoingLeft) ball->velocity.x *= -1;

                if (ball->x + ball->width / 2 > paddle.x + 4 * (paddle.width / 5)) {
                    // outter hit increase velocity
                    if ((std::abs(ball->velocity.x) < ball->maxVelocity.x) && (std::abs(ball->velocity.y) < ball->maxVelocity.y)) {
                        ball->velocity.x += paddleStrength;
                        ball->velocity.y -= paddleStrength;
                    }
                    OutputDebugString("BALL SPEED INCREASED\n");
                }
                else if (ball->x + ball->width / 2 > paddle.x + 3 * (paddle.width / 5)) {
                    // inner hit reduce velocity
                    if ((std::abs(ball->velocity.x) > ball->originalVelocity.x) && (std::abs(ball->velocity.y) > ball->originalVelocity.y)) {
                        ball->velocity.x -= paddleStrength;
                        ball->velocity.y += paddleStrength;
                    }
                    OutputDebugString("BALL SPEED DECREASED\n");
                }
                else {
                    // center hit no effect on velocity
                    OutputDebugString("NO CHANGE IN SPEED\n");
            
                }
            }
        }
        else if (ball->x <= 0 || ball->x + ball->width >= SCREEN_WIDTH) {
            ball->velocity.x *= -1;
        }
        else if (ball->y <= 0.f) {
            ball->velocity.y *= -1;
        }
        else if (ball->y >= SCREEN_HEIGHT - ball->height) {
            ball->isVisible = false;
        }

        // handle ball collision against bricks
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 10; j++) {
                rectangle* brick = &bricks[i][j];
                if (brick->isVisible) {
                    if (checkCollision(*ball, *brick)) {
                        brick->isVisible = false;

                        // bounce back the ball from the bricks
                        ball->velocity.y *= -1;

                        // calulcate score based on color
                        switch (brick->color) {
                        case RED:
                            playerScore++;
                            break;
                        case BLUE:
                            playerScore += 2;
                            break;
                        case GREEN:
                            playerScore += 3;
                            break;
                        }
                        // spawn falling bricks with 50% chance
                        if (rand() % 10 > 4) {
                            fallingBricks->push_back(rectangle{ brick->x, brick->y, brick->width, brick->height, true, brick->color });
                        }
                    }
                }

            }
        }

        bool isEveryBallInvisible{ true };

        for (ballRect* ball : balls) {
            if (ball->isVisible) {
                isEveryBallInvisible = false;
                break;
            }
        }

        if (isEveryBallInvisible) resetGame();

        ball->lastPosition.x = ball->x;
        ball->lastPosition.y = ball->y;
    }

    // handle bricks falling
    for (auto it = fallingBricks->begin(); it != fallingBricks->end();) {
        if (it->y >= SCREEN_HEIGHT - it->height) {
            it = fallingBricks->erase(it);
        }
        else {
            it->y += brickFallSpeed * dt;
            ++it;
        }
    }

    // bullets cleanup
    for (auto it = bullets->begin(); it != bullets->end();) {
        if (!it->isVisible) {
            it = bullets->erase(it);
        }
        else {
            ++it;
        }

    }

    // handle scores from falling bricks
    for (rectangle& fBrick : *fallingBricks) {
        if (fBrick.isVisible) {
            if (checkCollision(fBrick, paddle)) {
                fBrick.isVisible = false;
                switch (fBrick.color) {
                case RED:
                    playerScore += 2;
                    bulletCount++;
                    break;
                case BLUE:
                    playerScore += 4;
                    bulletCount += 2;
                    break;
                case GREEN:
                    playerScore += 6;
                    bulletCount += 3;
                    break;
                }
            }
        }
    }

    // check if every bricks are gone
    bool isEveryBricksGone{ true };

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            if (bricks[i][j].isVisible) {
                isEveryBricksGone = false;
                break;
            }
        }
        if (!isEveryBricksGone) break;
    }


    if (isEveryBricksGone && fallingBricks->size() == 0) resetGame();
}

// fill buffer in this function
// uint32_t buffer[SCREEN_HEIGHT][SCREEN_WIDTH] - is an array of 32-bit colors (8 bits per R, G, B)

void draw() {
    // clear backbuffer
    memset(buffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint32_t));

    // paddle
    rectangle paddle_section_0{ paddle.x,paddle.y,paddle.width / 5, paddle.height, true, GREEN };
    rectangle paddle_section_1{ paddle.x + paddle.width / 5,paddle.y,paddle.width / 5, paddle.height, true, RED };
    rectangle paddle_section_2{ paddle.x + 2 * paddle.width / 5,paddle.y,paddle.width / 5, paddle.height, true, WHITE };
    rectangle paddle_section_3{ paddle.x + 3 * paddle.width / 5,paddle.y,paddle.width / 5, paddle.height, true, RED };
    rectangle paddle_section_4{ paddle.x + 4 * paddle.width / 5,paddle.y,paddle.width / 5, paddle.height, true, GREEN };

    DrawRectangle(paddle_section_0);
    DrawRectangle(paddle_section_1);
    DrawRectangle(paddle_section_2);
    DrawRectangle(paddle_section_3);
    DrawRectangle(paddle_section_4);

    // draw balls
    for (ballRect* ball : balls) {
        DrawRectangle(*ball);
    }

    // Draw fix bricks
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            DrawRectangle(bricks[i][j]);
        }
    }

    //Draw falling bricks
    for (rectangle& fBrick : *fallingBricks) {
        DrawRectangle(fBrick);
    }

    // Draw Score
    float letterHeight = 80;
    DrawChar({ 10.f, SCREEN_HEIGHT - letterHeight }, letterHeight, RED, true, true, false, true, true, false, true);
    DrawChar({ 60.f, SCREEN_HEIGHT - letterHeight }, letterHeight, RED, true, true, false, false, true, true, false);
    DrawChar({ 110.f, SCREEN_HEIGHT - letterHeight }, letterHeight, RED, true, true, true, false, true, true, true);
    DrawChar({ 160.f, SCREEN_HEIGHT - letterHeight }, letterHeight, RED, true, true, true, true, false, true, true, true);
    DrawChar({ 210.f, SCREEN_HEIGHT - letterHeight }, letterHeight, RED, true, true, false, true, true, true, false);

    std::string stringScore = std::to_string(playerScore);

    float scorePadding = 45;
    float scoreStart_X = 300.f;

    DrawPoints(stringScore, scoreStart_X, scorePadding, letterHeight);

    // Draw bullet icon
    rectangle bulletIcon_body{ SCREEN_WIDTH -200, SCREEN_HEIGHT - 40, 20, 40, true, ORANGE };
    rectangle bulletIcon_tip{ SCREEN_WIDTH -199, SCREEN_HEIGHT - 55, 18, 15, true, GRAY };
    DrawRectangle(bulletIcon_body);
    DrawRectangle(bulletIcon_tip);

    // Draw bulletcount
    std::string bulletCountString = std::to_string(bulletCount);
    float bulletCounterStart_X = SCREEN_WIDTH - 150;

    DrawPoints(bulletCountString, bulletCounterStart_X, scorePadding, letterHeight);

    // Draw bullets

    for (rectangle& bullet : *bullets) {
        DrawRectangle(bullet);
    }
}

// free game data in this function
void finalize()
{
    delete fallingBricks;
    delete bullets;

}