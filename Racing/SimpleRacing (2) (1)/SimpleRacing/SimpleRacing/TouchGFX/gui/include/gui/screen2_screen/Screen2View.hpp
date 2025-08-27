#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual bool checkCollision();
protected:
    Unicode::UnicodeChar scoreBuffer[20];
    Unicode::UnicodeChar highScoreBuffer[20];
    int score;
    int highScore;
    int16_t localImageX;
    uint32_t tickCount;
    bool isGameOver;
    int16_t fallSpeed;     // tốc độ rơi của lamb (px mỗi tick)
    int16_t speedLevel;    // cấp độ tốc độ (dùng để tăng dần)
    int16_t carSpeedStep;
    bool seeded = false;
};

#endif // SCREEN2VIEW_HPP
