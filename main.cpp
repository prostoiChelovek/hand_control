//
// Created by prostoichelovek on 07.02.19.
//

#include <algorithm>
#include <string>
#include <vector>
#include <future>
#include <map>

#include <opencv2/opencv.hpp>

#include "handDetector/HandDetector.h"

#include "SystemInteraction.hpp"

#include "Settings.hpp"

#include "GUI.hpp"
#define CVUI_IMPLEMENTATION

#include "cvui.h"

using namespace std;
using namespace cv;

Settings setts = {
        lower: Scalar(0, 135, 90),
        upper: Scalar(255, 230, 150),

        press_minDistChange: 15,
        press_maxDistChange: 50,

        release_minDistChange: 15,
        release_maxDistChange: 50,

        gestureSpeeds: map<GestureDir, int>{
                {RIGHT, 35},
                {LEFT,  35},
                {UP,    30},
                {DOWN,  25}
        },
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
        should_removeFaces: true,
        should_controlMouse: false,
        should_click: false,
        should_recognizeGestures: false,
        should_adjustDstCh: false,
        should_adjustGstSpeed: false,

        gstCmds: map<GestureDir, string>{
                {RIGHT, "xdotool key Right"},
                {LEFT,  "xdotool key Left"},
                {UP,    "xdotool key space"},
                {DOWN,  "xdotool key Escape"}
        }
};

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
void ColorBalance(Mat &in, Mat &out, float percent) {
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

void flipImg(Mat &img) {
    if (setts.should_flipVert)
        flip(img, img, 0);
    if (setts.should_flipHor)
        flip(img, img, 1);
}

void autoAdjDstCh(vector<int> &dists) {
    vector<vector<int>> seqs;
    for (auto d = dists.begin() + 2; d < dists.end(); d++) {
        if (*d >= -6 && *d <= 10) {
            seqs.emplace_back(vector<int>{});
            continue;
        }
        if (!seqs.empty())
            seqs[seqs.size() - 1].emplace_back(*d);
    }
    vector<int> posMaxs, posMins, negMaxs, negMins;
    for (vector<int> &seq : seqs) {
        if (seq.empty())
            continue;

        int maxEl = *max_element(seq.begin(), seq.end());
        int minEl = *min_element(seq.begin(), seq.end());
        if (seq[0] > 0) {
            posMaxs.emplace_back(maxEl);
            posMins.emplace_back(minEl);
        } else {
            negMaxs.emplace_back(minEl);
            negMins.emplace_back(maxEl);
        }
    }

    int posMaxAvg = accumulate(posMaxs.begin(), posMaxs.end(), 0.0) / posMaxs.size();
    int posMinAvg = accumulate(posMins.begin(), posMins.end(), 0.0) / posMins.size();
    int negMaxAvg = accumulate(negMaxs.begin(), negMaxs.end(), 0.0) / negMaxs.size();
    int negMinAvg = accumulate(negMins.begin(), negMins.end(), 0.0) / negMins.size();
    setts.press_minDistChange = posMinAvg;
    setts.press_maxDistChange = posMaxAvg;
    setts.release_minDistChange = abs(negMinAvg);
    setts.release_maxDistChange = abs(negMaxAvg);
    cout << posMaxAvg << " " << posMinAvg << endl;
    cout << negMaxAvg << " " << negMinAvg << endl;
    dists.clear();
    setts.should_adjustDstCh = false;
}

void autoAdjGstSpeeds(vector<Vec2i> &speeds, GestureDir &currentGst) {
    vector<vector<int>> seqs;
    for (auto s = speeds.begin() + 2; s < speeds.end(); s++) {
        int v = 0;
        if (currentGst == RIGHT || currentGst == LEFT)
            v = (*s)[0];
        else
            v = (*s)[1];
        if ((currentGst == RIGHT || currentGst == DOWN) && v < 0)
            continue;
        if ((currentGst == LEFT || currentGst == UP) && v > 0)
            continue;
        if (v >= -18 && v <= 18) {
            seqs.emplace_back(vector<int>{});
            continue;
        }
        if (!seqs.empty())
            seqs[seqs.size() - 1].emplace_back(v);
    }

    vector<int> mins;
    for (vector<int> &seq : seqs) {
        if (seq.empty())
            continue;
        int minEl = *min_element(seq.begin(), seq.end());
        mins.emplace_back(minEl);
    }
    int minAvg = accumulate(mins.begin(), mins.end(), 0.0) / mins.size();
    cout << minAvg << endl;
    setts.gestureSpeeds[currentGst] = abs(minAvg);

    speeds.clear();
    currentGst = GestureDir((int) currentGst + 1);
    if (currentGst == NONE) {
        setts.should_adjustGstSpeed = false;
        currentGst = RIGHT;
        cout << "Gestures was adjusted!" << endl;
    } else {
        cout << "Now: " << gesture2str(currentGst) << endl;
    }
}

void processGesture(GestureDir gd) {
    if (setts.gstCmds.count(gd) == 1) {
        system((setts.gstCmds[gd] + " &").c_str());
    }
    cout << gesture2str(gd) << endl;
}

int main(int argc, char **argv) {
    int capDev = 0;

    if (argc > 1)
        capDev = stoi(argv[1]);

    VideoCapture cap(capDev);
    if (!cap.isOpened()) {
        cerr << "Unable to open video capture on " << capDev << endl;
        return EXIT_FAILURE;
    }

    SysInter::init();

    GUI::init(&setts);
    GUI::adjustColorRanges("mask");
    namedWindow("img");
    setMouseCallback("img", adjustBox_cb);

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

    Mat frame, img, img2, mask, imgYCrCb;

    int lastDist = 0;
    int mRightFrs = 0;
    int mLeftFrs = 0;
    int mUpFrs = 0;
    int mDownFrs = 0;
    time_t lastGstTime = time(nullptr);

    vector<int> dists;
    vector<Vec2i> speeds;
    GestureDir currentGst = RIGHT;


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
            ColorBalance(frame, frame, 10);

        flipImg(frame);
        frame.copyTo(img);
        frame.copyTo(img2);

        deleteBg(frame, img, bgs, bgs_learn, hd.thresh_sens_val);

        // remove faces
        if (!faceCascade.empty() && setts.should_removeFaces) {
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
                if (vX > setts.gestureSpeeds[RIGHT]) {
                    mRightFrs++;
                } else if (-vX > setts.gestureSpeeds[LEFT]) {
                    mLeftFrs++;
                } else if (vY > setts.gestureSpeeds[DOWN]) {
                    mDownFrs++;
                } else if (-vY > setts.gestureSpeeds[UP]) {
                    mUpFrs++;
                }
                if (setts.should_adjustGstSpeed)
                    speeds.emplace_back(vX, vY);
            }

            if (found) {
                bool clicked = false;
                if (setts.should_click) {
                    int dist = (h->border.x + h->border.width) - p->x;
                    int distChange = lastDist - dist;

                    if (setts.should_adjustDstCh)
                        dists.emplace_back(distChange);

                    if (distChange >= setts.press_minDistChange && distChange <= setts.press_maxDistChange) {
                        SysInter::mouseClick(Button1, true);
                        clicked = true;
                    }
                    if (-distChange >= setts.release_minDistChange && -distChange <= setts.release_maxDistChange) {
                        SysInter::mouseClick(Button1, false);
                        clicked = true;
                    }
                    lastDist = dist;
                }

                if (setts.should_controlMouse && !clicked) {
                    Point prb(p->x - box.x, p->y - box.y);
                    SysInter::moveMouse(prb.x * (SysInter::winWidth / box.width) * setts.mouseSpeedX,
                                        prb.y * (SysInter::winHeight / box.height) * setts.mouseSpeedY);
                }

                kf.update(vector<float>{p->x, p->y});

                if (time(nullptr) - lastGstTime >= setts.gstDelay) {
                    GestureDir gd = NONE;
                    if (mRightFrs > setts.gestureFrames)
                        gd = RIGHT;
                    if (mLeftFrs > setts.gestureFrames)
                        gd = LEFT;
                    if (mDownFrs > setts.gestureFrames)
                        gd = DOWN;
                    if (mUpFrs > setts.gestureFrames)
                        gd = UP;
                    if (gd != NONE) {
                        lastGstTime = time(nullptr);
                        processGesture(gd);
                        mRightFrs = 0;
                        mLeftFrs = 0;
                        mDownFrs = 0;
                        mUpFrs = 0;
                    }
                }
            }
        }

        GUI::displaySettingsGUI(hd, key, currentGst);

        imshow("img", img2);
        imshow("no bg", img);
        imshow("mask", mask);

        hd.updateLast();

        if (dists.size() > 200) {
            autoAdjDstCh(dists);
        }
        if (speeds.size() > 300) {
            autoAdjGstSpeeds(speeds, currentGst);
        }

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
                    cout << "control mouse: " << setts.should_controlMouse << endl;
                    break;
                case 'c':
                    setts.should_click = !setts.should_click;
                    cout << "click: " << setts.should_click << endl;
                    break;
                case 'g':
                    setts.should_recognizeGestures = !setts.should_recognizeGestures;
                    cout << "Recognize gestures: " << setts.should_recognizeGestures << endl;
                    break;
                case 'v':
                    setts.should_flipVert = !setts.should_flipVert;
                    cout << "Flip vertically: " << setts.should_flipVert << endl;
                    break;
                case 'h':
                    setts.should_flipHor = !setts.should_flipHor;
                    cout << "Flip horizontally: " << setts.should_flipHor << endl;
                    break;
                case 'o':
                    setts.should_adjustBox = !setts.should_adjustBox;
                    cout << "Adjust box: " << setts.should_adjustBox << endl;
                    break;
                case 'l':
                    setts.should_colorBalance = !setts.should_colorBalance;
                    cout << "Color balance: " << setts.should_colorBalance << endl;
                    break;
                case 'a':
                    setts.should_adjustDstCh = !setts.should_adjustDstCh;
                    cout << "Auto adjust click dist change: " << setts.should_adjustDstCh << endl;
                    break;
                case 'd':
                    setts.should_adjustGstSpeed = !setts.should_adjustGstSpeed;
                    cout << "Auto adjust gesture speed: " << setts.should_adjustGstSpeed << endl;
                    break;
                default:
                    cout << "Key presed: " << key << endl;
                    break;
            };
        }
    }

    return EXIT_SUCCESS;
}
