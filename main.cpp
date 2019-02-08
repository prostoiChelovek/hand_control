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

int press_minDistChange = 20;
int press_maxDistChange = 50;
int release_minDistChange = 20;
int release_maxDistChange = 50;

string faceCascadePath = "/data/haarcascade_frontalface_alt.xml";
Size minFaceSize = Size(70, 70);

void adjustColorRanges(const string &wName) {
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

bool should_controlMouse = false;
bool should_click = false;
bool should_flipVert = true;
bool should_flipHor = true;
bool should_adjustBox = false;

void flipImg(Mat &img) {
    if (should_flipVert)
        flip(img, img, 0);
    if (should_flipHor)
        flip(img, img, 1);
}

Rect box;
bool mbtnDown = false;

static void adjustBox_cb(int event, int x, int y, int, void *) {
    if (!should_adjustBox) return;
    switch (event) {
        case CV_EVENT_LBUTTONDOWN:
            box.x = x;
            box.y = y;
            mbtnDown = true;
            break;
        case CV_EVENT_LBUTTONUP:
            mbtnDown = false;
            break;
        case CV_EVENT_MOUSEMOVE:
            if (!mbtnDown) break;
            box.width = x;
            box.height = y;
            break;
    }
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
    hd.processNoiseCov = Scalar::all(7);
    hd.measurementNoiseCov = Scalar::all(6e-2);
    hd.errorCovPost = Scalar::all(.3);

    CascadeClassifier faceCascade;
    faceCascade.load(faceCascadePath);
    if (faceCascade.empty())
        cerr << "Could not load face cascade " << faceCascadePath << endl;

    Mat frame, img, img2, mask, bg, imgYCrCb;

    adjustColorRanges("Adjust ranges");
    namedWindow("img");
    setMouseCallback("img", adjustBox_cb);

    int lastDist = 0;

    cap >> bg;
    flipImg(bg);
    while (cap.isOpened()) {
        char key;
        cap >> frame;
        flipImg(frame);
        frame.copyTo(img);
        frame.copyTo(img2);

        hd.deleteBg(frame, bg, img);

        // remove faces
        if (!faceCascade.empty()) {
            vector<Rect> faces;
            faceCascade.detectMultiScale(img, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, minFaceSize);
            for (Rect &f : faces) {
                rectangle(img, f, Scalar(0, 0, 0), CV_FILLED);
            }
        }

        cvtColor(img, imgYCrCb, COLOR_BGR2YCrCb);
        mask = hd.detectHands_range(imgYCrCb, lower, upper);

        hd.getFingers();

        hd.initFilters();
        hd.updateFilters();
        hd.stabilize();

        hd.getCenters();
        hd.getHigherFingers();
        hd.getFarthestFingers();

        rectangle(img2, box, Scalar(0, 255, 0));
        hd.drawHands(img2, Scalar(255, 0, 100), 2);

        if (!hd.hands.empty()) {
            Hand *h;
            Point *p;
            bool found = false;
            for (Hand &hnd : hd.hands) {
                Point &pt = hnd.higherFinger.ptStart;
                if (box.contains(pt)) {
                    h = &hnd;
                    p = &pt;
                    found = true;
                }
            }

            if (should_controlMouse && found) {
                SysInter::moveMouse((p->x - box.x) * (SysInter::winWidth / box.width),
                                    (p->y - box.y) * (SysInter::winHeight / box.height));
            }

            if (should_click && found) {
                int dist = (h->border.x + h->border.width) - p->x;
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
        }

        imshow("img", img2);
        imshow("no bg", img);
        imshow("mask", mask);

        key = waitKey(1);
        if (key != -1) {
            switch (key) {
                case 'q':
                    cout << "Exit" << endl;
                    return EXIT_SUCCESS;
                case 'b':
                    frame.copyTo(bg);
                    cout << "Background captured" << endl;
                    break;
                case 'm':
                    should_controlMouse = !should_controlMouse;
                    break;
                case 'c':
                    should_click = !should_click;
                    break;
                case 'v':
                    should_flipHor = !should_flipVert;
                    break;
                case 'h':
                    should_flipHor = !should_flipHor;
                    break;
                case 'o':
                    should_adjustBox = !should_adjustBox;
                default:
                    cout << "Key presed: " << key << endl;
                    break;
            };
        }
    }

    return EXIT_SUCCESS;
}