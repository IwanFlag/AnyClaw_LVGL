#include "ftp_client.h"

#include <windows.h>
#include <wininet.h>
#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <vector>

#pragma comment(lib, "wininet.lib")

static uint64_t file_size_u64(const std::string& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) return 0;
    ULARGE_INTEGER li{};
    li.LowPart = data.nFileSizeLow;
    li.HighPart = data.nFileSizeHigh;
    return li.QuadPart;
}

bool ftp_transfer_file(const FtpTransferConfig& cfg,
                       std::atomic<bool>& cancel_flag,
                       std::string& error_out,
                       FtpProgressCallback on_progress) {
    error_out.clear();
    if (on_progress) on_progress(3, "Opening internet session");
    HINTERNET hInet = InternetOpenA("AnyClawFTP/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInet) {
        error_out = "InternetOpen failed";
        return false;
    }

    if (on_progress) on_progress(10, "Connecting FTP server");
    HINTERNET hFtp = InternetConnectA(hInet, cfg.host.c_str(), (INTERNET_PORT)cfg.port,
                                      cfg.username.c_str(), cfg.password.c_str(),
                                      INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (!hFtp) {
        error_out = "InternetConnect failed";
        InternetCloseHandle(hInet);
        return false;
    }

    constexpr DWORD chunk = 32 * 1024;
    bool ok = true;
    if (cfg.upload) {
        auto upload_one = [&](const std::string& local_file, const std::string& remote_file, int base_pct, int range_pct) -> bool {
            HANDLE hLocal = CreateFileA(local_file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hLocal == INVALID_HANDLE_VALUE) {
                error_out = "Cannot open local file for upload";
                return false;
            }
            HINTERNET hRemote = FtpOpenFileA(hFtp, remote_file.c_str(), GENERIC_WRITE,
                                             FTP_TRANSFER_TYPE_BINARY | (cfg.use_ftps ? INTERNET_FLAG_SECURE : 0), 0);
            if (!hRemote) {
                CloseHandle(hLocal);
                error_out = "FtpOpenFile upload failed";
                return false;
            }
            uint64_t total = file_size_u64(local_file);
            uint64_t sent = 0;
            char buf[32768];
            bool one_ok = true;
            while (one_ok && !cancel_flag.load()) {
                DWORD readn = 0;
                if (!ReadFile(hLocal, buf, chunk, &readn, nullptr)) { error_out = "Read local file failed"; one_ok = false; break; }
                if (readn == 0) break;
                DWORD writen = 0;
                if (!InternetWriteFile(hRemote, buf, readn, &writen) || writen != readn) { error_out = "InternetWriteFile failed"; one_ok = false; break; }
                sent += writen;
                int pct = (total > 0) ? (int)(base_pct + (sent * range_pct) / total) : (base_pct + range_pct);
                if (pct > 95) pct = 95;
                if (on_progress) on_progress(pct, "Uploading");
            }
            InternetCloseHandle(hRemote);
            CloseHandle(hLocal);
            return one_ok;
        };

        namespace fs = std::filesystem;
        fs::path lp(cfg.local_path);
        if (cfg.recursive_upload && fs::exists(lp) && fs::is_directory(lp)) {
            std::vector<fs::path> files;
            for (auto& p : fs::recursive_directory_iterator(lp)) {
                if (p.is_regular_file()) files.push_back(p.path());
            }
            if (files.empty()) {
                error_out = "Directory has no files";
                ok = false;
            } else {
                int count = (int)files.size();
                for (int i = 0; i < count && ok && !cancel_flag.load(); ++i) {
                    fs::path rel = fs::relative(files[i], lp);
                    std::string remote_file = cfg.remote_path;
                    if (!remote_file.empty() && remote_file.back() != '/') remote_file += "/";
                    remote_file += rel.generic_string();
                    if (on_progress) on_progress(20 + (i * 70) / count, "Uploading directory");
                    ok = upload_one(files[i].string(), remote_file, 20 + (i * 70) / count, std::max(1, 70 / count));
                }
            }
        } else {
            ok = upload_one(cfg.local_path, cfg.remote_path, 20, 75);
        }
    } else {
        if (on_progress) on_progress(20, "Opening remote download file");
        uint64_t remote_total = 0;
        WIN32_FIND_DATAA fd{};
        HINTERNET hFind = FtpFindFirstFileA(hFtp, cfg.remote_path.c_str(), &fd, INTERNET_FLAG_RELOAD, 0);
        if (hFind) {
            ULARGE_INTEGER li{};
            li.LowPart = fd.nFileSizeLow;
            li.HighPart = fd.nFileSizeHigh;
            remote_total = li.QuadPart;
            InternetCloseHandle(hFind);
        }
        HINTERNET hRemote = FtpOpenFileA(hFtp, cfg.remote_path.c_str(), GENERIC_READ,
                                         FTP_TRANSFER_TYPE_BINARY | (cfg.use_ftps ? INTERNET_FLAG_SECURE : 0), 0);
        if (!hRemote) {
            error_out = "FtpOpenFile download failed";
            ok = false;
        } else {
            HANDLE hLocal = CreateFileA(cfg.local_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hLocal == INVALID_HANDLE_VALUE) {
                error_out = "Cannot open local output file";
                ok = false;
            } else {
                uint64_t recv_total = 0;
                char buf[32768];
                while (ok && !cancel_flag.load()) {
                    DWORD got = 0;
                    if (!InternetReadFile(hRemote, buf, chunk, &got)) {
                        error_out = "InternetReadFile failed";
                        ok = false;
                        break;
                    }
                    if (got == 0) break;
                    DWORD writen = 0;
                    if (!WriteFile(hLocal, buf, got, &writen, nullptr) || writen != got) {
                        error_out = "Write local file failed";
                        ok = false;
                        break;
                    }
                    recv_total += got;
                    int pct = 0;
                    if (remote_total > 0) {
                        pct = (int)(20 + (recv_total * 75) / remote_total);
                    } else {
                        pct = (int)(20 + (recv_total / 1024));
                    }
                    if (pct > 95) pct = 95;
                    if (on_progress) on_progress(pct, "Downloading");
                }
                CloseHandle(hLocal);
            }
            InternetCloseHandle(hRemote);
        }
    }

    if (cancel_flag.load()) {
        error_out = "Cancelled by user";
        ok = false;
    }

    InternetCloseHandle(hFtp);
    InternetCloseHandle(hInet);
    if (ok && on_progress) on_progress(100, "Done");
    return ok;
}
