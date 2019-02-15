//
// Created by prostoichelovek on 15.02.19.
//

#ifndef MOUSECONTROL_SETTINGS_HPP
#define MOUSECONTROL_SETTINGS_HPP

#include <string>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

struct Settings {
    Scalar lower, upper;

    int press_minDistChange;
    int press_maxDistChange;

    int release_minDistChange;
    int release_maxDistChange;

    int gesture_minDistChange;
    int gestureFrames;
    float gstDelay;

    float mouseSpeedX;
    float mouseSpeedY;

    string faceCascadePath;
    Size minFaceSize;

    bool should_flipVert;
    bool should_flipHor;
    bool should_adjustBox;
    bool should_colorBalance;
    bool should_controlMouse;
    bool should_click;
    bool should_recognizeGestures;
};


#endif //MOUSECONTROL_SETTINGS_HPP
