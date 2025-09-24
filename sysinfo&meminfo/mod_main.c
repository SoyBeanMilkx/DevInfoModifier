#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <hook.h>
#include <syscall.h>
#include <kputils.h>
#include <linux/string.h>
#include "symbols.h"

KPM_NAME("meminfo");
KPM_VERSION("0.0.1");
KPM_AUTHOR("yuuki");
KPM_DESCRIPTION("modify some device info ovo");

static hook_err_t hook_err = HOOK_NOT_HOOK;

/*static void replace_si_meminfo(struct sysinfo *val) {
    backup_si_meminfo(val);

    // to 8GB
    //val->totalram = TARGET_RAM_PAGES;  // total pages

    val->totalram *= 2; 

    if (val->freeram > val->totalram) {
        val->freeram = val->totalram / 2;
    }
}*/

static int replace_do_sysinfo(struct sysinfo *info) {
    backup_do_sysinfo(info);
    info->totalram = 7459020800;
    unsigned long usedram = info->totalram - info->freeram;

    pr_info("[yuuki] =============================================================================\n");
    pr_info("[yuuki] original data:\n");
    pr_info("[yuuki] totalram: %lu (%lu bytes)\n", info->totalram, info->totalram * info->mem_unit);
    pr_info("[yuuki] freeram: %lu (%lu bytes)\n", info->freeram, info->freeram * info->mem_unit);
    pr_info("[yuuki] usedram: %lu (%lu bytes)\n", usedram, usedram * info->mem_unit);

    info->totalram *= 2;

    if (info->freeram > info->totalram) {
        pr_info("[yuuki] freeram (%lu) exceeds totalram (%lu), adjusting freeram\n",
                info->freeram, info->totalram);
        info->freeram = info->totalram / 2;
    }

    usedram = info->totalram - info->freeram;
    if (usedram > info->totalram) {
        pr_err("[yuuki] Error: usedram (%lu) exceeds totalram (%lu)\n", usedram, info->totalram);
    }

    pr_info("[yuuki] modified data:\n");
    pr_info("[yuuki] totalram: %lu (%lu bytes)\n", info->totalram, info->totalram * info->mem_unit);
    pr_info("[yuuki] freeram: %lu (%lu bytes)\n", info->freeram, info->freeram * info->mem_unit);
    pr_info("[yuuki] usedram: %lu (%lu bytes)\n", usedram, usedram * info->mem_unit);

    pr_info("[yuuki] =============================================================================\n");*/
    return 0;
}

void after_statfs(hook_fargs1_t *args, void *udata) {
    if (args->ret != 0) {
        return;
    }

    const char __user *path = (typeof(path))syscall_argn(args, 0);
    struct compat_statfs __user *buf = (typeof(buf))syscall_argn(args, 1);

    char path_name[PATH_MAX];
    memset(path_name, 0, sizeof(path_name));
    compat_strncpy_from_user(path_name, path, sizeof(path_name) - 1);

    if (strcmp(path_name, "/storage/emulated/0") == 0 ||
        strcmp(path_name, "/data") == 0) {

        struct compat_statfs kbuf;
        copy_from_user_nofault(&kbuf, buf, sizeof(kbuf));

        /*pr_info("[yuuki] original data: f_bsize=%u, f_blocks=%llu, f_bfree=%llu, f_bavail=%llu\n",
                kbuf.f_bsize, kbuf.f_blocks, kbuf.f_bfree, kbuf.f_bavail);*/

        // total_bytes = f_blocks * f_bsize
        kbuf.f_blocks *= 2;

        compat_copy_to_user(buf, &kbuf, sizeof(kbuf));
    }
}

static inline bool installHook() {
    bool ret = true;

    /*if (original_si_meminfo) {
        hook_err = hook((void *)original_si_meminfo,
                        (void *)replace_si_meminfo,
                        (void **)&backup_si_meminfo);
        if (hook_err != HOOK_NO_ERR) {
            pr_err("[yuuki] hook si_meminfo failed, error: %d\n", hook_err);
            ret = false;
        } else {
            pr_info("[yuuki] si_meminfo hooked\n");
        }
    } else {
        pr_err("[yuuki] no symbol: si_meminfo\n");
        ret = false;
    }*/

    if (original_do_sysinfo) {
        hook_err = hook((void *)original_do_sysinfo,
                        (void *) replace_do_sysinfo,
                        (void **)&backup_do_sysinfo);
        if (hook_err != HOOK_NO_ERR) {
            pr_err("[yuuki] hook do_sysinfo failed, error: %d\n", hook_err);
            ret = false;
        } else {
            pr_info("[yuuki] do_sysinfo hooked\n");
        }
    } else {
        pr_err("[yuuki] no symbol: do_sysinfo\n");
        ret = false;
    }

    // hook sys_call
    hook_err = inline_hook_syscalln(__NR3264_statfs, 2, 0, after_statfs, 0);
    if (hook_err) {
        pr_err("[yuuki] Failed to hook statfs syscall, error: %d\n", hook_err);
        return false;
    }

    if (ret) {
        pr_info("[yuuki] all hooks installed successfully\n");
    }

    return ret;
}

static inline bool uninstallHook() {
    /*if (original_si_meminfo) {
        unhook((void *)original_si_meminfo);
        pr_info("[yuuki] si_meminfo unhooked\n");
    }*/
    if (original_do_sysinfo) {
        unhook((void *)original_do_sysinfo);
        pr_info("[yuuki] do_sysinfo unhooked\n");
    }

    inline_unhook_syscalln(__NR3264_statfs, 0, after_statfs);
    pr_info("[yuuki] statfs syscall unhooked\n");

    hook_err = HOOK_NOT_HOOK;
    pr_info("[yuuki] all hooks uninstalled\n");
    return true;
}

static inline bool control_internal(bool enable) {
    return enable ? installHook() : uninstallHook();
}

static long mod_init(const char *args, const char *event, void *__user reserved){
    pr_info("[yuuki] Initializing...\n");

    if(!init_symbols()) goto exit;

    control_internal(true);
    exit:
    return 0;
}

static long mod_control0(const char *args, char *__user out_msg, int outlen) {
    pr_info("[yuuki] from meminfo, args: %s\n", args);

    return 0;
}

static long mod_exit(void *__user reserved) {
    control_internal(false);
    pr_info("[yuuki] mod_exit, uninstalled hook.\n");
    return 0;
}

KPM_INIT(mod_init);
KPM_CTL0(mod_control0);
KPM_EXIT(mod_exit);