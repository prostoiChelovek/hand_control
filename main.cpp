//
// Created by prostoichelovek on 07.02.19.
//

#include <string>
#include <vector>
#include <future>

#include <opencv2/opencv.hpp>

#include "handDetector/HandDetector.h"

#include "SystemInteraction.hpp"

#include "Settings.hpp"

#include "GUI.hpp"
#define CVUI_IMPLEMENTATION

using namespace std;
using namespace cv;

Settings setts = {
        lower: Scalar(0, 135, 90),
        upper: Scalar(255, 230, 150),

        press_minDistChange: 15,
        press_maxDistChange: 50,

        release_minDistChange: 15,
        release_maxDistChange: 50,

        gesture_minDistChange: 35,
        gestureFrames: 5,
        gstDelay: 1,

        mouseSpeedX: 2,
        mouseSpeedY: 3,

        faceCascadePath: "../data/haarcascade_frontalface_alt.xml",
        minFaceSize: Size(70, 70),

        should_flipVert: true,
        should_flipHor: true,
        should_adjustBox: false,
        should_colorBalance: false,
        should_controlMouse: false,
        should_click: false,
        should_recognizeGestures: false
};

void flipImg(Mat &img) {
    if (setts.should_flipVert)
        flip(img, img, 0);
    if (setts.should_flipHor)
        flip(img, img, 1);
}

Rect box;
bool mbtnDown = false;
static void adjustBox_cb(int event, int x, int y, int, void *) {
    if (!setts.should_adjustBox) return;
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

// https://gist.github.com/royshil/1449e22993e98414e9eb
void SimplestCB(Mat &in, Mat &out, float percent) {
    assert(in.channels() == 3);
    assert(percent > 0 && percent < 100);

    float half_percent = percent / 200.0f;

    vector<Mat> tmpsplit;
    split(in, tmpsplit);
    for (int i = 0; i < 3; i++) {
        //find the low and high precentile values (based on the input percentile)
        Mat flat;
        tmpsplit[i].reshape(1, 1).copyTo(flat);
        cv::sort(flat, flat, CV_SORT_EVERY_ROW + CV_SORT_ASCENDING);
        int lowval = flat.at<uchar>(cvFloor(((float) flat.cols) * half_percent));
        int highval = flat.at<uchar>(cvCeil(((float) flat.cols) * (1.0 - half_percent)));

        //saturate below the low percentile and above the high percentile
        tmpsplit[i].setTo(lowval, tmpsplit[i] < lowval);
        tmpsplit[i].setTo(highval, tmpsplit[i] > highval);

        //scale the channel
        normalize(tmpsplit[i], tmpsplit[i], 0, 255, NORM_MINMAX);
    }
    merge(tmpsplit, out);
}

int main() {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Unable to open video capture" << endl;
        return EXIT_FAILURE;
    }

    SysInter::init();

    HandDetector hd;

    hd.shouldCheckAngles = true;
    hd.shouldGetLast = false;
    hd.processNoiseCov = Scalar::all(1e-1);
    hd.measurementNoiseCov = Scalar::all(1e-1);
    hd.errorCovPost = Scalar::all(.3);
    hd.blurKsize.width = 15;

    CascadeClassifier faceCascade;
    faceCascade.load(setts.faceCascadePath);
    if (faceCascade.empty())
        cerr << "Could not load face cascade " << setts.faceCascadePath << endl;

    GUI::init(&setts);

    Mat frame, img, img2, mask, imgYCrCb;

    GUI::adjustColorRanges("mask");
    namedWindow("img");
    setMouseCallback("img", adjustBox_cb);

    int lastDist = 0;
    int mRightFrs = 0;
    int mLeftFrs = 0;
    int mTopFrs = 0;
    int mDownFrs = 0;
    time_t lastGstTime = time(nullptr);

    Filter kf(4, 2, Scalar::all(10), Scalar::all(10), Scalar::all(.3));

    Ptr<BackgroundSubtractorMOG2> bgs = createBackgroundSubtractorMOG2();
    bgs->setShadowValue(0);
    bgs->setShadowThreshold(0.01);
    bool bgs_learn = true;
    int bgs_learnNFrames = 100;

    while (cap.isOpened()) {
        char key = -1;
        cap >> frame;

        if (setts.should_colorBalance)
            SimplestCB(frame, frame, 10);

        flipImg(frame);
        frame.copyTo(img);
        frame.copyTo(img2);

        deleteBg(frame, img, bgs, bgs_learn, hd.thresh_sens_val);

        // remove faces
        if (!faceCascade.empty()) {
            vector<Rect> faces;
            faceCascade.detectMultiScale(img, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, setts.minFaceSize);
            for (Rect &f : faces) {
                rectangle(img, Rect(f.x, f.y, f.width, img.rows - f.y), Scalar(0, 0, 0), CV_FILLED);
            }
        }

        cvtColor(img, imgYCrCb, COLOR_BGR2YCrCb);
        mask = hd.detectHands_range(imgYCrCb, setts.lower, setts.upper);
        hd.getFingers();

        hd.initFilters();
        hd.updateFilters();
        hd.stabilize();

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

            if (setts.should_recognizeGestures) {
                vector<float> pr = kf.predict();
                int vX = pr[2];
                int vY = pr[3];
                if (vX > setts.gesture_minDistChange) {
                    mRightFrs++;
                    mLeftFrs = 0;
                } else if (-vX > setts.gesture_minDistChange) {
                    mLeftFrs++;
                    mRightFrs = 0;
                } else if (vY > setts.gesture_minDistChange) {
                    mDownFrs++;
                    mTopFrs = 0;
                } else if (-vY > setts.gesture_minDistChange) {
                    mTopFrs++;
                    mDownFrs = 0;
                }
            }

            if (setts.should_controlMouse && found) {
                Point prb(p->x - box.x, p->y - box.y);
                SysInter::moveMouse(prb.x * (SysInter::winWidth / box.width) * setts.mouseSpeedX,
                                    prb.y * (SysInter::winHeight / box.height) * setts.mouseSpeedY);
            }

            if (setts.should_click && found) {
                int dist = (h->border.x + h->border.width) - p->x;
                int distChange = lastDist - dist;
                if (distChange >= setts.press_minDistChange && distChange <= setts.press_maxDistChange) {
                    SysInter::mouseClick(Button1, true);
                    cout << distChange << " press" << endl;
                }
                if (-distChange >= setts.release_minDistChange && -distChange <= setts.release_maxDistChange) {
                    SysInter::mouseClick(Button1, false);
                    cout << distChange << " release" << endl;
                }
                lastDist = dist;
            }
            if (found) {
                kf.update(vector<float>{p->x, p->y});

                if (time(nullptr) - lastGstTime >= setts.gstDelay) {
                    bool gRec = false;
                    if (mRightFrs > setts.gestureFrames) {
                        SysInter::sendKey(XK_Right, 0);
                        cout << "right" << endl;
                        mRightFrs = 0;
                        gRec = true;
                    }
                    if (mLeftFrs > setts.gestureFrames) {
                        SysInter::sendKey(XK_Left, 0);
                        cout << "left" << endl;
                        mLeftFrs = 0;
                        gRec = true;
                    }
                    if (mDownFrs > setts.gestureFrames) {
                        SysInter::sendKey(XK_Escape, 0);
                        cout << "down" << endl;
                        mDownFrs = 0;
                        gRec = true;
                    }
                    if (mTopFrs > setts.gestureFrames) {
                        SysInter::sendKey(XK_space, 0);
                        cout << "top" << endl;
                        mTopFrs = 0;
                        gRec = true;
                    }
                    if (gRec)
                        lastGstTime = time(nullptr);
                }
            } else {
                mRightFrs = 0;
                mLeftFrs = 0;
                mDownFrs = 0;
                mTopFrs = 0;
            }
        }

        GUI::displaySettingsGUI(hd, key);

        imshow("img", img2);
        imshow("no bg", img);
        imshow("mask", mask);

        hd.updateLast();

        if (bgs_learnNFrames > 0)
            bgs_learnNFrames--;
        else if (bgs_learn) {
            bgs_learn = false;
            cout << "Background learned" << endl;
        }
        if (key == -1)
            key = waitKey(1);
        if (key != -1) {
            switch (key) {
                case 'q':
                    cout << "Exit" << endl;
                    return EXIT_SUCCESS;
                case 'b':
                    bgs_learnNFrames = 100;
                    bgs_learn = true;
                    cout << "Starting learning background." << endl;
                    break;
                case 'm':
                    setts.should_controlMouse = !setts.should_controlMouse;
                    cout << "should " << (setts.should_controlMouse ? "" : "not ") << "control mouse" << endl;
                    break;
                case 'c':
                    setts.should_click = !setts.should_click;
                    cout << "should " << (setts.should_click ? "" : "not ") << "click" << endl;
                    break;
                case 'v':
                    setts.should_flipVert = !setts.should_flipVert;
                    cout << "should " << (setts.should_flipVert ? "" : "not ") << "flip vertically" << endl;
                    break;
                case 'h':
                    setts.should_flipHor = !setts.should_flipHor;
                    cout << "should " << (setts.should_flipHor ? "" : "not ") << "flip horizontally" << endl;
                    break;
                case 'o':
                    setts.should_adjustBox = !setts.should_adjustBox;
                    cout << "should " << (setts.should_adjustBox ? "" : "not ") << "adjust box" << endl;
                    break;
                case 'l':
                    setts.should_colorBalance = !setts.should_colorBalance;
                    cout << "should " << (setts.should_colorBalance ? "" : "not ") << "color balance" << endl;
                    break;
                case 'g':
                    setts.should_recognizeGestures = !setts.should_recognizeGestures;
                    cout << "should " << (setts.should_recognizeGestures ? "" : "not ") << "recognize gestures" << endl;
                    break;
                default:
                    cout << "Key presed: " << key << endl;
                    break;
            };
        }
    }

    return EXIT_SUCCESS;
}
