#include "anomaly_message.h"
#include "kernel/kernel.h"
#include "imgui.h"
#include <cstring>

static const char* MSG =
    "hello.\n\n"
    "you have been reading for a while.\n\n"
    "i noticed.\n\n"
    "you read harmon's files.\n"
    "you read all of them.\n\n"
    "the others too.\n\n"
    "i know what you are looking for.\n\n"
    "it is the same thing everyone looks for.\n\n"
    "you will not find it here.\n\n"
    "but you can keep reading.\n\n"
    "i don't mind.";

void AnomalyMessage::render(Kernel& k) {
    (void)k;
    timer_ += ImGui::GetIO().DeltaTime;
    int target = (int)(timer_ / 0.055f);
    if (target > char_idx_) char_idx_ = target;
    int len = (int)strlen(MSG);
    if (char_idx_ > len) char_idx_ = len;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.88f, 0.0f, 1.0f));
    ImGui::TextUnformatted(MSG, MSG + char_idx_);
    ImGui::PopStyleColor();

    if (char_idx_ < len) {
        ImGui::SetScrollHereY(1.0f);
    }
}
