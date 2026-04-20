/* Stub for FTP client — compiled unconditionally, active only when ENABLE_FTP is OFF */
#ifndef ENABLE_FTP

#include "ftp_client.h"
#include <atomic>
#include <string>

bool ftp_transfer_file(const FtpTransferConfig&,
                       std::atomic<bool>&,
                       std::string& error_out,
                       FtpProgressCallback) {
    error_out = "FTP is disabled in this build";
    return false;
}

#endif /* !ENABLE_FTP */
