#ifndef Settings_h
#define Settings_h

#include <Arduino.h>
#include <ArduinoJson.h>

class Manager;

class Settings {
    public:
        Settings(Manager *_manager);
        ~Settings(void);

        String get(const char *name);
        String get();
        bool set(const char *name, const char *value);

        void reset();

    private:
        Manager *manager;
        
};

#endif