//
// Created by prostoichelovek on 15.02.19.
//

#ifndef MOUSECONTROL_SETTINGS_HPP
#define MOUSECONTROL_SETTINGS_HPP

#include <string>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

enum GestureDir {
    RIGHT = 0,
    LEFT = 1,
    UP = 2,
    DOWN = 3,
    NONE = 4
};

string gesture2str(const GestureDir &gd) {
    switch (gd) {
        case RIGHT:
            return "Right";
        case LEFT:
            return "Left";
        case UP:
            return "Up";
        case DOWN:
            return "Down";
        case NONE:
            return "NONE";
        default:
            return "";
    };
}

struct Settings {
    Scalar lower, upper;

    int press_minDistChange;
    int press_maxDistChange;

    int release_minDistChange;
    int release_maxDistChange;

    map<GestureDir, int> gestureSpeeds;
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
    bool should_removeFaces;
    bool should_controlMouse;
    bool should_click;
    bool should_recognizeGestures;
    bool should_adjustDstCh;
    bool should_adjustGstSpeed;

    map<GestureDir, string> gstCmds;
};


#endif //MOUSECONTROL_SETTINGS_HPP
