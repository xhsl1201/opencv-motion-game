#define NOMINMAX
#include "opencv2/opencv.hpp"
#include <iostream>
#include <ctime>
#include <vector>
#include <windows.h>
#include <algorithm>

struct Ball
{
    cv::Point position;   // 공 중심 좌표
    int radius;           // 공 반지름
    int colorType;        // 0:red, 1:green, 2:blue

    Ball()
    {
        position = cv::Point();
        radius = 20;
        colorType = 0;
    }
};

// 공이 화면 밖으로 나가지 않도록 랜덤 위치 생성
cv::Point getRandomPosition9(int width, int height, int radius)
{
    int x = rand() % (width - 2 * radius) + radius;
    int y = rand() % (height - 2 * radius) + radius;
    return cv::Point(x, y);
}

// 타겟 색 공이 최소 1개 존재하도록 보장
void ensureTargetExists9(std::vector<Ball>& balls, int targetColor)
{
    for (int i = 0; i < balls.size(); i++)
    {
        if (balls[i].colorType == targetColor)
            return;
    }
    balls[0].colorType = targetColor;
}

// 공 리셋 함수
void resetBall9(Ball& b, int width, int height)
{
    b.radius = rand() % 20 + 40;
    b.position = getRandomPosition9(width, height, b.radius);
    b.colorType = rand() % 3;
}

void runProject9()
{
    srand((unsigned int)time(0));

    int combo = 0;
    int comboTimer = 0;
    int wrongFlashTimer = 0;

    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "웹캠이 없습니다.\n";
        return;
    }

    cv::Mat imgRed = cv::imread("red2.png", cv::IMREAD_UNCHANGED);
    cv::Mat imgGreen = cv::imread("green.png", cv::IMREAD_UNCHANGED);
    cv::Mat imgBlue = cv::imread("blue.png", cv::IMREAD_UNCHANGED);

    if (imgRed.empty() || imgGreen.empty() || imgBlue.empty())
    {
        std::cerr << "이미지 로드 실패\n";
        return;
    }

    int width = cvRound(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = cvRound(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    cv::Mat prev_gray;

    // 공 생성
    std::vector<Ball> balls;
    int ballCount = 5;
    int scores[3] = { 0,0,0 };
    int targetColor = rand() % 3;

    for (int i = 0; i < ballCount; i++)
    {
        Ball b;
        b.radius = rand() % 45 + 59;
        b.colorType = rand() % 3;
        b.position = getRandomPosition9(width, height, b.radius);
        balls.push_back(b);
    }
    ensureTargetExists9(balls, targetColor);

    // 게임 시간
    int experimentTime = 30;
    int64 startTick = cv::getTickCount();
    double freq = cv::getTickFrequency();
    bool finished = false;

    while (true)
    {
        cv::Mat frame, gray, diff, thresh;
        cap >> frame;
        if (frame.empty()) break;

        cv::flip(frame, frame, 1);

        if (!finished)
        {
            double passed = (cv::getTickCount() - startTick) / freq;
            int remainTime = experimentTime - (int)passed;
            if (remainTime <= 0)
            {
                remainTime = 0;
                finished = true;
            }

            // 움직임 감지
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray, gray, cv::Size(15, 15), 0);

            if (prev_gray.empty())
            {
                gray.copyTo(prev_gray);
                continue;
            }

            cv::absdiff(prev_gray, gray, diff);
            cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);

            cv::Mat kernel = cv::getStructuringElement(
                cv::MORPH_RECT, cv::Size(5, 5));
            cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);

            // 공 터치 판정
            for (int i = 0; i < balls.size(); i++)
            {
                Ball& b = balls[i];

                int x1 = std::max(0, b.position.x - b.radius);
                int y1 = std::max(0, b.position.y - b.radius);
                int x2 = std::min(width, b.position.x + b.radius);
                int y2 = std::min(height, b.position.y + b.radius);

                cv::Rect ballRect(x1, y1, x2 - x1, y2 - y1);
                cv::Mat roi = thresh(ballRect);

                int movePixels = cv::countNonZero(roi);
                int area = (b.radius * 2) * (b.radius * 2);

                if (movePixels > area * 0.1)
                {
                    cv::circle(frame, b.position, b.radius + 5,
                        cv::Scalar(0, 255, 255), 3);
                    cv::putText(frame, "HIT", b.position,
                        cv::FONT_HERSHEY_PLAIN, 2,
                        cv::Scalar(0, 255, 255), 2);

                    if (b.colorType == targetColor)
                    {
                        Beep(1200, 50);
                        combo++;
                        scores[targetColor] += (combo >= 2 ? combo : 1);
                        comboTimer = 30;
                        targetColor = rand() % 3;
                    }
                    else
                    {
                        Beep(400, 80);
                        scores[targetColor]--;
                        combo = 0;
                        comboTimer = 0;
                        wrongFlashTimer = 6;
                    }

                    resetBall9(b, width, height);
                    ensureTargetExists9(balls, targetColor);
                }
            }

            gray.copyTo(prev_gray);

            // UI
            cv::putText(frame, "TIME: " + std::to_string(remainTime),
                cv::Point(20, 30), cv::FONT_HERSHEY_PLAIN, 2,
                cv::Scalar(0, 255, 255), 2);

            std::string colorName[3] = { "RED","GREEN","BLUE" };
            cv::Scalar colorDraw[3] = {
                cv::Scalar(0,0,255),
                cv::Scalar(0,255,0),
                cv::Scalar(255,0,0)
            };

            cv::putText(frame, "TARGET: " + colorName[targetColor],
                cv::Point(width - 260, 40),
                cv::FONT_HERSHEY_PLAIN, 2.5,
                colorDraw[targetColor], 3);

            for (int i = 0; i < 3; i++)
            {
                cv::putText(frame,
                    colorName[i] + ": " + std::to_string(scores[i]),
                    cv::Point(20, 70 + i * 30),
                    cv::FONT_HERSHEY_PLAIN, 1.5,
                    colorDraw[i], 2);
            }

            if (wrongFlashTimer > 0)
            {
                cv::Mat redOverlay(frame.size(), frame.type(),
                    cv::Scalar(0, 0, 255));
                cv::addWeighted(redOverlay, 0.3, frame, 0.7, 0, frame);
                wrongFlashTimer--;
            }

            if (combo >= 2 && comboTimer > 0)
            {
                cv::putText(frame,
                    std::to_string(combo) + " COMBO!",
                    cv::Point(width / 2 - 100, height / 2),
                    cv::FONT_HERSHEY_PLAIN, 3,
                    cv::Scalar(0, 255, 255), 3);
                comboTimer--;
            }
        }
        else
        {
            frame = frame * 0.5;
            int total = scores[0] + scores[1] + scores[2];

            cv::putText(frame, "GAME OVER",
                cv::Point(width / 2 - 200, height / 2 - 40),
                cv::FONT_HERSHEY_PLAIN, 3,
                cv::Scalar(0, 0, 255), 3);

            cv::putText(frame, "TOTAL: " + std::to_string(total),
                cv::Point(width / 2 - 120, height / 2 + 40),
                cv::FONT_HERSHEY_PLAIN, 2,
                cv::Scalar(255, 255, 255), 2);
        }

        // 공 그리기
        for (int i = 0; i < balls.size(); i++)
        {
            cv::Mat* img =
                (balls[i].colorType == 0) ? &imgRed :
                (balls[i].colorType == 1) ? &imgGreen : &imgBlue;

            cv::Mat resized;
            cv::resize(*img, resized,
                cv::Size(balls[i].radius * 2, balls[i].radius * 2));

            int x = balls[i].position.x - balls[i].radius;
            int y = balls[i].position.y - balls[i].radius;

            cv::Rect roiRect(x, y, resized.cols, resized.rows);
            cv::Rect frameRect(0, 0, frame.cols, frame.rows);
            cv::Rect validRect = roiRect & frameRect;

            if (validRect.width > 0 && validRect.height > 0)
            {
                cv::Mat roi = frame(validRect);
                cv::Mat imgROI = resized(
                    cv::Rect(validRect.x - roiRect.x,
                        validRect.y - roiRect.y,
                        validRect.width,
                        validRect.height));

                if (imgROI.channels() == 4)
                {
                    std::vector<cv::Mat> ch;
                    cv::split(imgROI, ch);

                    std::vector<cv::Mat> rgbChannels;
                    rgbChannels.push_back(ch[0]);
                    rgbChannels.push_back(ch[1]);
                    rgbChannels.push_back(ch[2]);

                    cv::Mat rgb;
                    cv::merge(rgbChannels, rgb);

                    rgb.copyTo(roi, ch[3]);
                }
                else
                {
                    imgROI.copyTo(roi);
                }
            }
        }

        cv::imshow("PROJECT", frame);
        if (cv::waitKey(10) == 27) break;
    }

    cap.release();
    cv::destroyAllWindows();
}