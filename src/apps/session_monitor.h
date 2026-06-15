#pragma once
#include "kernel/app.h"

class SessionMonitor : public App {
public:
    void render(Kernel& k) override;
    const char* title() const override {return "Session Monitor";}
private:
    int selected_pid_ = 7741;
};
