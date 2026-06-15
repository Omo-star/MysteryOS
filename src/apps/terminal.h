#pragma once
#include "kernel/app.h"
#include <string>
#include <vector>
class Terminal : public App{
    public:
        Terminal();
        void render(Kernel& k) override;
        const char* title() const override {return "Terminal";}
    private:
        int terminal_id_ = 0;
        std::vector<std::string> lines_;
        char input_[256] = {};
        bool scroll_to_bottom_ = false;
        void execute(const std::string& cmd, Kernel& k);
        void print(const std::string& text);
};
