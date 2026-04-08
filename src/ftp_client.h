#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include <atomic>
#include <functional>
#include <string>

struct FtpTransferConfig {
    std::string host;
    int port = 21;
    std::string username;
    std::string password;
    std::string remote_path;
    std::string local_path;
    bool upload = false;
    bool use_ftps = false;
    bool recursive_upload = false;
};

using FtpProgressCallback = std::function<void(int percent, const char* step)>;

bool ftp_transfer_file(const FtpTransferConfig& cfg,
                       std::atomic<bool>& cancel_flag,
                       std::string& error_out,
                       FtpProgressCallback on_progress);

#endif
