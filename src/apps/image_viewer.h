#pragma once
#include "kernel/app.h"
#include <string>

class ImageViewer : public App {
    public:
        explicit ImageViewer(const std::string& path);
        ~ImageViewer();
        void render(Kernel& k) override;
        const char* title() const override;
    private:
        std::string path_, title_;
        unsigned int texture_ = 0;
        int tex_w_ = 0, tex_h_ = 0;
        bool loaded_ = false;
        bool counted_ = false;
        void try_load(Kernel& k);
        void gen_glitch(bool is_anomaly);
};
