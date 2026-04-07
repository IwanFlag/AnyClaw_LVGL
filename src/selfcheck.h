#ifndef SELFCHECK_H
#define SELFCHECK_H

/* Self-check result for a single item */
struct SelfCheckItem {
    bool ok;
    char message[256];
};

/* Full self-check result */
struct SelfCheckResult {
    SelfCheckItem nodejs;       /* Node.js installed */
    SelfCheckItem npm;          /* npm available */
    SelfCheckItem network;      /* Network connectivity */
    SelfCheckItem config_dir;   /* Config directory writable */
    bool all_ok;
};

/* Run all self-checks. Returns result struct.
   Writes findings to log via ui_log(). */
SelfCheckResult selfcheck_run();

/* Attempt to fix issues. Returns true if all fixed. */
bool selfcheck_fix(SelfCheckResult& result);

/* Convenience: run + fix + log */
bool selfcheck_run_and_fix();

#endif /* SELFCHECK_H */
