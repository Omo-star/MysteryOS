#pragma once
#include "kernel/app.h"
#include "fx/glitch.h"

class NotificationManager : public App {
public:
    void render(Kernel& k) override;
    const char* title() const override {return "Notifications";}
};
