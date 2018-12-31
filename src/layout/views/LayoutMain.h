#ifndef LAYOUTMAIN_H
#define LAYOUTMAIN_H

#include "../Render.h"
#include "../Display.h"
#include "../helpers/LayoutBase.h"

#include <MenuBackend.h>

class LayoutMain: public LayoutBase {
    public:
        LayoutMain(Display *_display);
        ~LayoutMain(void);

    private:
        void initMenu();

        void leftButtonClicked();
        void rightButtonClicked();
        void okButtonClicked();
        
        static void menuChangeEventStatic(MenuChangeEvent changed, void *context);
        void menuChangeEvent(MenuChangeEvent changed);

        MenuBackend *menuRoot;

        MenuItem send = MenuItem("Send");
        
        MenuItem receive = MenuItem("Receive");
            MenuItem receive_menu = MenuItem("Receive_Menu");

        MenuItem setup = MenuItem("Setup");
            MenuItem wifi = MenuItem("Wifi");
                MenuItem wifi_menu = MenuItem("Wifi_Menu");
            MenuItem wallet = MenuItem("Wallet");
            MenuItem setup_back = MenuItem("Back");

        // Layout main boundaries (not including date)
        int x = 0;
        int y = 0;
        int w = 128;
        int h = 128;

};

#endif