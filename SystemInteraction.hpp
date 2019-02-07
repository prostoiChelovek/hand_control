//
// Created by prostoichelovek on 07.02.19.
//

#ifndef MOUSECONTROL_SYSTEMINTERACTION_HPP
#define MOUSECONTROL_SYSTEMINTERACTION_HPP


#include <unistd.h>

#if defined(unix) || defined(__unix__) || defined(__unix)
#define OS_UNIX

#include <SerialStream.h>

#endif

namespace SysInter {

#ifdef OS_UNIX
    // https://bharathisubramanian.wordpress.com/2010/04/01/x11-fake-mouse-events-generation-using-xtest/

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/XTest.h>

    Display *disp;
    int scr;
    int winHeight;
    int winWidth;

    void init() {
        disp = XOpenDisplay(nullptr);
        scr = XDefaultScreen(disp);
        winHeight = DisplayHeight(disp, scr);
        winWidth = DisplayWidth(disp, scr);
    }

    void mouseClick(int button = Button1, bool press = true) {
        XTestFakeButtonEvent(disp, button, press, CurrentTime);
    }

    void moveMouse(int x, int y) {
        XTestFakeMotionEvent(disp, 0, x, y, CurrentTime);
        XSync(disp, 0);
    }


#endif // is unix


#ifdef _WIN32
#pragma message ( "Windows interaction  is not implemented!" )
#endif // _WIN32

}

#endif //MOUSECONTROL_SYSTEMINTERACTION_HPP
