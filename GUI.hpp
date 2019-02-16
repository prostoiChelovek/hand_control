//
// Created by prostoichelovek on 14.02.19.
//

#ifndef MOUSECONTROL_GUI_HPP
#define MOUSECONTROL_GUI_HPP

#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

#include "handDetector/HandDetector.h"

#include "Settings.hpp"

#include "cvui.h"

using namespace std;
using namespace cv;

namespace GUI {
    String settingsWName = "Settings";
    int width = 200;
    Mat settingsWin(650, width * 3 + 100, CV_8UC1);
    Settings *setts;

    void init(Settings *setts_) {
        setts = setts_;

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
        cvui::trackbar(width, &setts->press_minDistChange, 0, 100);
        cvui::space(3);

        cvui::text("Max distance change for press");
        cvui::trackbar(width, &setts->press_maxDistChange, setts->press_minDistChange + 1, 250);
        cvui::space(3);

        cvui::text("Mix distance change for release");
        cvui::trackbar(width, &setts->release_minDistChange, 0, 100);
        cvui::space(3);

        cvui::text("Max distance change for press");
        cvui::trackbar(width, &setts->release_maxDistChange, setts->release_minDistChange + 1, 250);
        cvui::space(3);

        cvui::text("Mix distance change for gesture");
        cvui::trackbar(width, &setts->gesture_minDistChange, 0, 100);
        cvui::space(3);
        cvui::endColumn();

        cvui::beginColumn(settingsWin, width + 50, 20, width, -1, 6);
        cvui::text("Frames for gesture");
        cvui::trackbar(width, &setts->gestureFrames, 0, 20);
        cvui::space(3);

        cvui::text("Gesture delay");
        cvui::trackbar(width, &setts->gstDelay, 0.1f, 3.f);

        cvui::text("Mouse speed X");
        cvui::trackbar(width, &setts->mouseSpeedX, .0f, 10.f);
        cvui::space(3);

        cvui::text("Mouse speed Y");
        cvui::trackbar(width, &setts->mouseSpeedY, .0f, 10.f);
        cvui::space(5);

        cvui::checkbox("Blur", &hd.shouldBlur);
        cvui::space(2);

        cvui::checkbox("Check size", &hd.shouldCheckSize);
        cvui::space(2);

        cvui::checkbox("Check angles", &hd.shouldCheckAngles);
        cvui::space(2);

        cvui::checkbox("Get last fingers", &hd.shouldGetLast);
        cvui::space(2);

        cvui::checkbox("Remove faces", &setts->should_removeFaces);
        cvui::space(2);

        cvui::checkbox("Control mouse", &setts->should_controlMouse);
        cvui::space(2);

        cvui::checkbox("Click", &setts->should_click);
        cvui::space(2);

        cvui::checkbox("Recognize gestures", &setts->should_recognizeGestures);
        cvui::space(2);

        cvui::checkbox("Auto adjust dist change", &setts->should_adjustDstCh);
        cvui::space(5);
        cvui::endColumn();

        cvui::beginColumn(settingsWin, (width + 50) * 2, 20, width, -1, 6);
        if (cvui::button("Exit (q)"))
            key = 'q';
        if (cvui::button("Change background (b)"))
            key = 'b';
        if (cvui::button("Adjust box (o)"))
            key = 'o';
        if (cvui::button("Flip horizontally (h)"))
            key = 'h';
        if (cvui::button("Flip vertically (v)"))
            key = 'v';
        cvui::endColumn();

        cvui::update();
        cv::imshow(settingsWName, settingsWin);
    }

    void adjustColorRanges(const string &wName) {
        namedWindow(wName);
        auto onTrackbarActivity = [](int val, void *data) {
            double &vNum = *(static_cast<double *>(data));
            vNum = val;
        };
        int CrMinVal = setts->lower.val[0];
        createTrackbar("CrMin", wName, &CrMinVal, 255, onTrackbarActivity, &setts->lower.val[0]);
        int CrMaxVal = setts->upper.val[0];
        createTrackbar("CrMax", wName, &CrMaxVal, 255, onTrackbarActivity, &setts->upper.val[0]);
        int CbMinVal = setts->lower.val[1];
        createTrackbar("CbMin", wName, &CbMinVal, 255, onTrackbarActivity, &setts->lower.val[1]);
        int CbMaxVal = setts->upper.val[1];
        createTrackbar("CbMax", wName, &CbMaxVal, 255, onTrackbarActivity, &setts->upper.val[1]);
        int YMinVal = setts->lower.val[2];
        createTrackbar("YMin", wName, &YMinVal, 255, onTrackbarActivity, &setts->lower.val[2]);
        int YMaxVal = setts->upper.val[2];
        createTrackbar("YMax", wName, &YMaxVal, 255, onTrackbarActivity, &setts->upper.val[2]);
    }

}

#endif //MOUSECONTROL_GUI_HPP
