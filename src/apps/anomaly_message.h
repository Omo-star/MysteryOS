#pragma once
#include "kernel/app.h"

class AnomalyMessage : public App {
public:
    void render(Kernel& k) override;
    const char* title() const override { return "SYSTEM MESSAGE"; }
private:
    float timer_ = 0.0f;
    int char_idx_ = 0;
};
