#include "file_explorer.h"
#include "kernel/kernel.h"
#include "imgui.h"

using namespace std;

static bool is_image_file(const string& name) {
    auto dot = name.rfind('.');
    if (dot == string::npos) return false;
    string ext = name.substr(dot);
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp";
}

void FileExplorer::render(Kernel& k){
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::BeginChild("##tree", {w * 0.35f, 0}, true);
    render_tree(k, k.vfs().root(), "/");
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##files", {0, 0}, true);
    VFSNode* dir = k.vfs().get(selected_dir_);
    if (dir) render_contents(k, dir, selected_dir_);
    ImGui::EndChild();
}

void FileExplorer::render_tree(Kernel& k, VFSNode* node, const string& path){
    if (!node || !node->is_dir) return;
    for (auto& [name, child] : node->children){
        if (child->hidden) continue;
        if (!child->is_dir) continue;
        string child_path = (path == "/" ? "/" : path + "/") + name;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (selected_dir_ == child_path) flags |= ImGuiTreeNodeFlags_Selected;
        bool open = ImGui::TreeNodeEx(name.c_str(), flags);
        if (ImGui::IsItemClicked()) selected_dir_ = child_path;
        if (open){
            render_tree(k, child.get(), child_path);
            ImGui::TreePop();
        }
    }
}

void FileExplorer::render_contents(Kernel& k, VFSNode* dir, const string& path){
    for (auto& [name, child] : dir->children){
        if (child->hidden) continue;
        string child_path = (path == "/" ? "/" : path + "/") + name;
        if (child->is_dir) {
            if (child->locked) {
                ImGui::TextDisabled("[locked] %s/", name.c_str());
                if (ImGui::IsItemClicked()) k.launch("password_dialog", child_path);
            } else {
                ImGui::Text("[dir] %s/", name.c_str());
                if (ImGui::IsItemClicked()) selected_dir_ = child_path;
            }
        } else {
            if (child->locked) {
                ImGui::TextDisabled("[locked] %s", name.c_str());
                if (ImGui::IsItemClicked()) k.launch("password_dialog", child_path);
            } else if (child->corrupted) {
                ImGui::TextColored({1, 0.4f, 0.4f, 1}, "%s", name.c_str());
                if (ImGui::IsItemClicked()) k.launch(is_image_file(name) ? "image_viewer" : "text_editor", child_path);
            } else {
                ImGui::Text("%s", name.c_str());
                if (ImGui::IsItemClicked()) k.launch(is_image_file(name) ? "image_viewer" : "text_editor", child_path);
            }
        }
    }
}
