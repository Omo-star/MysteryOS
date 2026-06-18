#pragma once

#include <map>
#include <string>
#include <vector>

class PlayerProfile {
    public:
        void record_command(const std::string& command);
        void record_file_open(const std::string& path, const std::string& source);
        void record_search(const std::string& command, const std::string& query);
        void record_anomaly_talk();

        std::vector<std::string> observations(int stage) const;
        std::string live_profile_content(int stage) const;
        const std::vector<std::string>& command_history() const { return command_history_; }

        int terminal_reads() const { return terminal_reads_; }
        int gui_reads() const { return gui_reads_; }
        int deleted_reads() const { return deleted_reads_; }
        int model_reads() const { return model_reads_; }

    private:
        std::map<std::string, int> open_counts_;
        std::map<std::string, int> search_counts_;
        std::vector<std::string> command_history_;
        int terminal_reads_ = 0;
        int gui_reads_ = 0;
        int deleted_reads_ = 0;
        int model_reads_ = 0;
        int player_folder_reads_ = 0;
        int anomaly_talks_ = 0;
};
