//
// Created by prostoichelovek on 07.02.19.
//

#include <string>
#include <vector>
#include <future>

#include <opencv2/opencv.hpp>

#include "handDetector/HandDetector.h"

#include "SystemInteraction.hpp"

using namespace std;
using namespace cv;

Scalar lower = Scalar(0, 135, 90);
Scalar upper = Scalar(255, 230, 150);

void adjustColorRanges(string wName) {
    namedWindow(wName);
    auto onTrackbarActivity = [](int val, void *data) {
        double &vNum = *(static_cast<double *>(data));
        vNum = double(val);
    };
    int CrMinVal = lower.val[0];
    createTrackbar("CrMin", wName, &CrMinVal, 255, onTrackbarActivity, &lower.val[0]);
    int CrMaxVal = upper.val[0];
    createTrackbar("CrMax", wName, &CrMaxVal, 255, onTrackbarActivity, &upper.val[0]);
    int CbMinVal = lower.val[1];
    createTrackbar("CbMin", wName, &CbMinVal, 255, onTrackbarActivity, &lower.val[1]);
    int CbMaxVal = upper.val[1];
    createTrackbar("CbMax", wName, &CbMaxVal, 255, onTrackbarActivity, &upper.val[1]);
    int YMinVal = lower.val[2];
    createTrackbar("YMin", wName, &YMinVal, 255, onTrackbarActivity, &lower.val[2]);
    int YMaxVal = upper.val[2];
    createTrackbar("YMax", wName, &YMaxVal, 255, onTrackbarActivity, &upper.val[2]);
}

int main() {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Unable to open video capture" << endl;
        return EXIT_FAILURE;
    }

    SysInter::init();

    HandDetector hd;

    hd.shouldCheckAngles = false;

    Mat frame, img, img2, mask, bg, imgYCrCb;
    cap >> bg;

    adjustColorRanges("Adjust ranges");

    int lastDist = 0;
    int press_minDistChange = 20;
    int press_maxDistChange = 50;
    int release_minDistChange = 20;
    int release_maxDistChange = 50;

    while (cap.isOpened()) {
        char key;
        cap >> frame;
        frame.copyTo(img);
        frame.copyTo(img2);

        hd.deleteBg(frame, bg, img);
        cvtColor(img, imgYCrCb, COLOR_BGR2YCrCb);
        mask = hd.detectHands_range(imgYCrCb, lower, upper);

        hd.getFingers();

        hd.initFilters();
        hd.updateFilters();
        hd.stabilize();

        hd.getCenters();
        hd.getHigherFingers();
        hd.getFarthestFingers();

        hd.drawHands(img2, Scalar(255, 0, 100), 2);

        if (!hd.hands.empty()) {
            Hand &h = hd.hands[0];
            Finger &f = h.higherFinger;
            Point &p = f.ptStart;
            SysInter::moveMouse(p.x * (SysInter::winWidth / img.cols),
                                p.y * (SysInter::winHeight / img.rows));

            int dist = (h.border.x + h.border.width) - p.x;
            int distChange = lastDist - dist;
            if (distChange >= press_minDistChange && distChange <= press_maxDistChange) {
                SysInter::mouseClick(Button1, true);
                cout << distChange << " press" << endl;
            }
            if (-distChange >= release_minDistChange && -distChange <= release_maxDistChange) {
                SysInter::mouseClick(Button1, false);
                cout << distChange << " release" << endl;
            }
            lastDist = dist;
        }

        imshow("img", img2);
        imshow("mask", mask);

        key = waitKey(1);
        if (key != -1) {
            switch (key) {
                case 'q':
                    cout << "Exit" << endl;
                    return EXIT_SUCCESS;
                case 'b':
                    cap >> bg;
                    cout << "Background captured" << endl;
                    break;
                default:
                    cout << "Key presed: " << key << endl;
                    break;
            };
        }
    }

    return EXIT_SUCCESS;
}