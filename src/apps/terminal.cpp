#include "terminal.h"
#include "kernel/kernel.h"
#include "kernel/vfs.h"
#include "fx/glitch.h"
#include "imgui.h"
#include <sstream>
#include <cstring>
#include <cstdlib>

using namespace std;

void Terminal::print(const string& text) {
    lines_.push_back(text);
    scroll_to_bottom_ = true;
}

void Terminal::render(Kernel& k) {
    auto anomaly_responses = k.drain_anomaly_responses();
    for (const auto& response : anomaly_responses) {
        print(response);
    }

    float h = ImGui::GetContentRegionAvail().y - 32;
    ImGui::BeginChild("##out", {0, h}, false);
    for (auto& line : lines_) {
        string displayed = (Glitch::level() >= 4 && (rand() % 40 == 0)) ? Glitch::mangle(line) : line;
        ImGui::TextUnformatted(displayed.c_str());
    }
    if (Glitch::level() >= 4 && (rand() % 200 == 0)) ImGui::TextColored({0.5f, 0, 0, 1}, "ERR: unauthorized process detected");
    if (scroll_to_bottom_) { ImGui::SetScrollHereY(1.0f); scroll_to_bottom_ = false; }
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::SetNextItemWidth(-60);
    bool run = ImGui::InputText("##cmd", input_, sizeof(input_), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    run |= ImGui::Button("Run");
    if (run && input_[0] != '\0') {
        print(string("> ") + input_);
        execute(input_, k);
        memset(input_, 0, sizeof(input_));
        ImGui::SetKeyboardFocusHere(-1);
    }
}

void Terminal::execute(const string& raw, Kernel& k) {
    istringstream ss(raw);
    string cmd, arg;
    ss >> cmd >> arg;
    if (cmd == "help") {
        print("Commands: help  ls [path]  cat <file>  unlock <password>  whoami  ps  ping <host>  kill <pid>  decode <file>  monitor  trace <pid>  talk 7741 <message>");
        if (k.puzzle().stage() >= 1) print("  [hint] try: cat /System/Archive/classified.txt");
        return;
    }
    if (cmd == "whoami"){
        print("evoss");
        return;
    }
    if (cmd == "ps") {
        print("  PID   USER     %CPU  COMMAND");
        print("    1   root      0.0  init");
        print("  312   evoss     0.0  file_explorer");
        print("  441   evoss     0.1  terminal");
        if (k.puzzle().stage() >= 2)
            print(" 7741   -        31.4  [UNKNOWN]");
        else
            print(" 7741   -         0.0  [no name]");
        return;
    }
    if (cmd == "ping") {
        if (arg.empty()) { print("ping: missing host"); return; }
        if (arg == "10.0.0.47") {
            print("PING 10.0.0.47: 56 data bytes");
            print("64 bytes from 10.0.0.47: icmp_seq=0 ttl=64 time=0.000 ms");
            print("64 bytes from 10.0.0.47: icmp_seq=1 ttl=64 time=0.000 ms");
            print("64 bytes from 10.0.0.47: icmp_seq=2 ttl=64 time=0.000 ms");
            print("--- 10.0.0.47 ping statistics ---");
            print("3 packets transmitted, 3 received, 0% packet loss");
            print("round-trip min/avg/max = 0.000/0.000/0.000 ms");
        } else {
            print("ping: " + arg + ": Request timeout for icmp_seq 0");
        }
        return;
    }
    if (cmd == "kill") {
        if (arg.empty()) { print("kill: missing PID"); return; }
        if (arg == "7741")
            print("kill: (7741): Operation not permitted");
        else
            print("kill: (" + arg + "): No such process");
        return;
    }
    if (cmd == "ls") {
        VFSNode* dir = k.vfs().get(arg.empty() ? "/" : arg);
        if (!dir || !dir->is_dir) { print("ls: not a directory: " + arg); return; }
        for (auto& [name, child] : dir->children)
            print("  " + name + (child->is_dir ? "/" : "") + (child->locked ? "  [locked]" : ""));
        return;
    }
    if (cmd == "cat") {
        if (arg.empty()) { print("cat: missing path"); return; }
        VFSNode* node = k.vfs().get(arg);
        if (!node)        { print("cat: not found: " + arg); return; }
        if (node->locked) { print("cat: permission denied"); return; }
        if (node->is_dir) { print("cat: is a directory"); return; }
        print(node->corrupted ? Glitch::mangle(node->content) : node->content);
        return;
    }
    if (cmd == "unlock") {
        if (arg.empty()) { print("unlock: missing password"); return; }
        if (k.puzzle().try_password(arg, k.vfs()))
            print("Access granted. Stage " + to_string(k.puzzle().stage()) + " unlocked.");
        else
            print("unlock: incorrect password.");
        return;
    }
    if (cmd == "run") {
        if (arg.empty()) { print("run: missing app name"); return; }
        k.launch(arg);
        print("Launching " + arg + "...");
        return;
    }
    if (cmd == "monitor") {
        if (k.puzzle().stage() < 2) {
            print("monitor: access denied");
        } else {
            k.launch("session_monitor");
            print("Launching Session Monitor...");
        }
        return;
    }
    if (cmd == "trace") {
        if (arg.empty()) { print("trace: missing PID"); return; }
        if (arg != "7741") { print("trace: (" + arg + "): no anomaly detected"); return; }
        int stage = k.puzzle().stage();
        if (stage < 2) print("trace: insufficient privileges");
        else if (stage == 2) print("PID 7741 is not registered in process table.");
        else if (stage == 3) print("PID 7741 is bound to active user session: evoss.");
        else if (stage == 4) print("PID 7741 has written files without user input.");
        else print("PID 7741 is observing current session.");
        return;
    }
    if (cmd == "talk") {
        string message;
        getline(ss, message);
        if (!message.empty() && message[0] == ' ') message.erase(0, 1);
        if (arg != "7741") {
            print("talk: usage: talk 7741 <message>");
            return;
        }
        if (message.empty()) {
            print("talk: missing message");
            return;
        }
        print("7741: [listening]");
        k.request_anomaly(message);
        return;
    }
    if (cmd == "decode") {
        if (arg.empty()) { print("decode: missing path"); return; }
        VFSNode* node = k.vfs().get(arg);
        if (!node)        { print("decode: not found: " + arg); return; }
        if (node->locked) { print("decode: permission denied"); return; }
        if (arg == "/Users/cshin/Documents/response_1byte.bin") {
            print("Decoding " + arg + "...");
            print("Content (1 byte):");
            print("");
            print("  Hex:   0x3F");
            print("  ASCII: ?");
            print("  Value: 63");
        } else {
            print("decode: " + arg + ": binary decode not supported");
        }
        return;
    }
    print("Unknown command: " + cmd + ". Type 'help' for a list.");
}
