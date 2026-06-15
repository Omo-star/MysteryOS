#include "text_editor.h"
#include "kernel/kernel.h"
#include "kernel/vfs.h"
#include "fx/glitch.h"
#include "imgui.h"

using namespace std;

TextEditor::TextEditor(const string& path) : path_(path) {
    auto slash = path.rfind('/');
    title_ = string("Text Editor — ") + (slash == string::npos ? path : path.substr(slash + 1));
}

const char* TextEditor::title() const { return title_.c_str(); }

void TextEditor::render(Kernel& k) {
    if (!counted_) { counted_ = true; k.record_file_open(path_); }
    VFSNode* node = k.vfs().get(path_);
    if (!node) {ImGui::Text("[File not found: %s]", path_.c_str()); return;}
    if (node->is_dir) {ImGui::Text("[Cannot open: is a directory]"); return;}
    string content = node->corrupted ? Glitch::mangle(node->content) : node->content;
    ImGui::BeginChild("##text", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(content.c_str());
    ImGui::EndChild();
}
