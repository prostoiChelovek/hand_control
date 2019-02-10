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

int press_minDistChange = 15;
int press_maxDistChange = 50;
int release_minDistChange = 15;
int release_maxDistChange = 50;
int gesture_minDistChange = 45;
int gestureFrames = 5;
float gstDelay = 1;

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
bool should_colorBalance = false;

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

    hd.shouldCheckAngles = false;
    hd.processNoiseCov = Scalar::all(1e-1);
    hd.measurementNoiseCov = Scalar::all(1e-1);
    hd.errorCovPost = Scalar::all(.3);
    hd.blurKsize.width = 20;

    CascadeClassifier faceCascade;
    faceCascade.load(faceCascadePath);
    if (faceCascade.empty())
        cerr << "Could not load face cascade " << faceCascadePath << endl;

    Mat frame, img, img2, mask, bg, imgYCrCb;

    adjustColorRanges("mask");
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
        char key;
        cap >> frame;

        if (should_colorBalance)
            SimplestCB(frame, frame, 10);

        flipImg(frame);
        frame.copyTo(img);
        frame.copyTo(img2);

        deleteBg(frame, img, bgs, bgs_learn, 127);

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

            vector<float> pr = kf.predict();
            int vX = pr[2];
            int vY = pr[3];
            if (vX > gesture_minDistChange) {
                mRightFrs++;
                mLeftFrs = 0;
            } else if (-vX > gesture_minDistChange) {
                mLeftFrs++;
                mRightFrs = 0;
            } else if (vY > gesture_minDistChange) {
                mDownFrs++;
                mTopFrs = 0;
            } else if (-vY > gesture_minDistChange) {
                mTopFrs++;
                mDownFrs = 0;
            } else {
                mRightFrs = 0;
                mLeftFrs = 0;
                mDownFrs = 0;
                mTopFrs = 0;
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
            if (found) {
                kf.update(vector<float>{p->x, p->y});

                if (time(nullptr) - lastGstTime >= gstDelay) {
                    bool gRec = false;
                    if (mRightFrs > gestureFrames) {
                        SysInter::sendKey(XK_Right, 0);
                        cout << "right" << endl;
                        mRightFrs = 0;
                        gRec = true;
                    }
                    if (mLeftFrs > gestureFrames) {
                        SysInter::sendKey(XK_Left, 0);
                        cout << "left" << endl;
                        mLeftFrs = 0;
                        gRec = true;
                    }
                    if (mDownFrs > gestureFrames) {
                        SysInter::sendKey(XK_Escape, 0);
                        cout << "down" << endl;
                        mDownFrs = 0;
                        gRec = true;
                    }
                    if (mTopFrs > gestureFrames) {
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
                    should_controlMouse = !should_controlMouse;
                    cout << "should " << (should_controlMouse ? "" : "not ") << "control mouse\"" << endl;
                    break;
                case 'c':
                    should_click = !should_click;
                    cout << "should " << (should_click ? "" : "not ") << "click" << endl;
                    break;
                case 'v':
                    should_flipVert = !should_flipVert;
                    cout << "should " << (should_flipVert ? "" : "not ") << "flip vertically" << endl;
                    break;
                case 'h':
                    should_flipHor = !should_flipHor;
                    cout << "should " << (should_flipHor ? "" : "not ") << "flip horizontally" << endl;
                    break;
                case 'o':
                    should_adjustBox = !should_adjustBox;
                    cout << "should " << (should_adjustBox ? "" : "not ") << "adjust box" << endl;
                    break;
                case 'g':
                    should_colorBalance = !should_colorBalance;
                    cout << "should " << (should_colorBalance ? "" : "not ") << "color balance" << endl;
                    break;
                default:
                    cout << "Key presed: " << key << endl;
                    break;
            };
        }
    }

    return EXIT_SUCCESS;
}