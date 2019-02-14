//
// Created by prostoichelovek on 14.02.19.
//

#ifndef MOUSECONTROL_GUI_HPP
#define MOUSECONTROL_GUI_HPP

#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

#include "handDetector/HandDetector.h"

#include "cvui.h"

using namespace std;
using namespace cv;

namespace GUI {
    String settingsWName = "Settings";
    int width = 200;
    Mat settingsWin(650, width * 2 + 60, CV_8UC1);

    // sorry for this
    int *press_minDistChange, *press_maxDistChange, *release_minDistChange,
            *release_maxDistChange, *gesture_minDistChange, *gestureFrames;
    float *gstDelay;
    bool *should_controlMouse, *should_click, *should_recognizeGestures;

    void init(int *press_minDistChange_, int *press_maxDistChange_,
              int *release_minDistChange_, int *release_maxDistChange_,
              int *gesture_minDistChange_, int *gestureFrames_,
              float *gstDelay_, bool *should_controlMouse_, bool *should_click_,
              bool *should_recognizeGestures_) {
        press_minDistChange = press_minDistChange_;
        press_maxDistChange = press_maxDistChange_;
        release_minDistChange = release_minDistChange_;
        release_maxDistChange = release_maxDistChange_;
        gesture_minDistChange = gesture_minDistChange_;
        gestureFrames = gestureFrames_;
        gstDelay = gstDelay_;
        should_controlMouse = should_controlMouse_;
        should_click = should_click_;
        should_recognizeGestures = should_recognizeGestures_;

        namedWindow(settingsWName, CV_WINDOW_AUTOSIZE);
        cvui::init(settingsWName);
    }

    void displaySettingsGUI(HandDetector &hd, char &key) {
        cvui::context(settingsWName);
        settingsWin = Scalar(49, 52, 49);
        cvui::beginColumn(settingsWin, 20, 20, width, -1, 6);

        cvui::text("blur kernel size");
        cvui::trackbar(width, &hd.blurKsize.width, 1, 100);
        cvui::space(3);

        cvui::text("threshold sens val");
        cvui::trackbar(width, &hd.thresh_sens_val, 1, 100);
        cvui::space(3);

        cvui::text("Max angle");
        cvui::trackbar(width, &hd.maxAngle, 20, 180);
        cvui::space(5);

        cvui::text("Mix distance change for press");
        cvui::trackbar(width, press_minDistChange, 0, 100);
        cvui::space(3);

        cvui::text("Max distance change for press");
        cvui::trackbar(width, press_maxDistChange, *press_minDistChange + 1, 250);
        cvui::space(3);

        cvui::text("Mix distance change for release");
        cvui::trackbar(width, release_minDistChange, 0, 100);
        cvui::space(3);

        cvui::text("Max distance change for press");
        cvui::trackbar(width, release_maxDistChange, *release_minDistChange + 1, 250);
        cvui::space(3);

        cvui::text("Mix distance change for gesture");
        cvui::trackbar(width, gesture_minDistChange, 0, 100);
        cvui::space(3);

        cvui::endColumn();

        cvui::beginColumn(settingsWin, width + 50, 20, width, -1, 6);

        cvui::text("Frames for gesture");
        cvui::trackbar(width, gestureFrames, 0, 20);
        cvui::space(3);

        cvui::text("Gesture delay");
        cvui::trackbar(width, gstDelay, 0.1f, 2.f);
        cvui::space(5);

        cvui::checkbox("Blur", &hd.shouldBlur);
        cvui::space(2);

        cvui::checkbox("Check size", &hd.shouldCheckSize);
        cvui::space(2);

        cvui::checkbox("Check angles", &hd.shouldCheckAngles);
        cvui::space(2);

        cvui::checkbox("Get last fingers", &hd.shouldGetLast);
        cvui::space(2);

        cvui::checkbox("Control mouse", should_controlMouse);
        cvui::space(2);

        cvui::checkbox("Click", should_click);
        cvui::space(2);

        cvui::checkbox("Recognize gestures", should_recognizeGestures);
        cvui::space(5);

        if (cvui::button("Exit (q)"))
            key = 'q';
        if (cvui::button("Change background (b)"))
            key = 'b';
        if (cvui::button("Adjust box"))
            key = 'o';
        if (cvui::button("Flip horizontally (h)"))
            key = 'h';
        if (cvui::button("Flip vertically (v)"))
            key = 'v';
        cvui::endColumn();

        cvui::update();
        cv::imshow(settingsWName, settingsWin);
    }

}

#endif //MOUSECONTROL_GUI_HPP
