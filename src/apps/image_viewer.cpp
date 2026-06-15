#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "image_viewer.h"
#include "kernel/kernel.h"
#include "kernel/vfs.h"
#include "imgui.h"
#include <SDL2/SDL_opengles2.h>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

static string file_basename(const string& path) {
    auto slash = path.rfind('/');
    return slash == string::npos ? path : path.substr(slash + 1);
}

ImageViewer::ImageViewer(const string& path) : path_(path) {
    title_ = string("Image Viewer — ") + file_basename(path);
}

ImageViewer::~ImageViewer() {
    if (texture_) glDeleteTextures(1, &texture_);
}

const char* ImageViewer::title() const { return title_.c_str(); }

void ImageViewer::gen_glitch(bool is_anomaly) {
    tex_w_ = 512; tex_h_ = 384;
    vector<unsigned char> pixels(tex_w_ * tex_h_ * 4);
    srand(is_anomaly ? 0xE10A : (unsigned)time(nullptr));
    for (int i = 0; i < tex_w_ * tex_h_; i++) {
        unsigned char v = rand() % 255;
        pixels[i*4+0] = is_anomaly ? 0       : v / 4;
        pixels[i*4+1] = is_anomaly ? v       : v / 4;
        pixels[i*4+2] = is_anomaly ? 0       : v / 4;
        pixels[i*4+3] = 255;
    }
    // scan lines
    for (int y = 0; y < tex_h_; y += 6) {
        for (int x = 0; x < tex_w_; x++) {
            int idx = (y * tex_w_ + x) * 4;
            pixels[idx]   /= 3;
            pixels[idx+1] /= 3;
            pixels[idx+2] /= 3;
        }
    }
    glGenTextures(1, (GLuint*)&texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w_, tex_h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

void ImageViewer::try_load(Kernel& k) {
    if (loaded_) return;
    loaded_ = true;
    VFSNode* node = k.vfs().get(path_);
    bool corrupted = node && node->corrupted;
    bool is_anomaly = (file_basename(path_) == "0xE10A.png");
    if (corrupted || is_anomaly) { gen_glitch(is_anomaly); return; }
    string img_path = "/data/images/" + file_basename(path_);
    int w, h, c;
    unsigned char* data = stbi_load(img_path.c_str(), &w, &h, &c, 4);
    if (!data) { gen_glitch(false); return; }
    glGenTextures(1, (GLuint*)&texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    tex_w_ = w; tex_h_ = h;
    stbi_image_free(data);
}

void ImageViewer::render(Kernel& k) {
    if (!counted_) { counted_ = true; k.record_file_open(path_); }
    try_load(k);
    if (!texture_) { ImGui::TextColored({1,0.3f,0.3f,1}, "[image data unreadable]"); return; }
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float scale = min(avail.x / tex_w_, avail.y / tex_h_);
    ImGui::Image((ImTextureID)(intptr_t)texture_, {tex_w_ * scale, tex_h_ * scale});
}
