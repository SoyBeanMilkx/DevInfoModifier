#ifndef MEMINFO_SYMBOLS_H
#define MEMINFO_SYMBOLS_H

#include <asm/current.h>
#include <taskext.h>
#include <linux/cred.h>

#define TARGET_RAM_KB 8388608UL  // 8GB in KB
#define TARGET_RAM_PAGES (TARGET_RAM_KB * 1024 / 4096)  // convert to pages

#define PATH_MAX 4096

/*struct sysinfo {
    long            uptime;
    unsigned long   loads[3];
    unsigned long   totalram;
    unsigned long   freeram;
    unsigned long   sharedram;
    unsigned long   bufferram;
    unsigned long   totalswap;
    unsigned long   freeswap;
    unsigned short  procs;
    unsigned short  pad;
    unsigned long   totalhigh;
    unsigned long   freehigh;
    unsigned int    mem_unit;
    char _f[20-2*sizeof(unsigned long)-sizeof(unsigned int)];
};*/

struct sysinfo {
    __kernel_long_t uptime;		/* Seconds since boot */
    __kernel_ulong_t loads[3];	/* 1, 5, and 15 minute load averages */
    __kernel_ulong_t totalram;	/* Total usable main memory size */
    __kernel_ulong_t freeram;	/* Available memory size */
    __kernel_ulong_t sharedram;	/* Amount of shared memory */
    __kernel_ulong_t bufferram;	/* Memory used by buffers */
    __kernel_ulong_t totalswap;	/* Total swap space size */
    __kernel_ulong_t freeswap;	/* swap space still available */
    __u16 procs;		   	/* Number of current processes */
    __u16 pad;		   	/* Explicit padding for m68k */
    __kernel_ulong_t totalhigh;	/* Total high memory size */
    __kernel_ulong_t freehigh;	/* Available high memory size */
    __u32 mem_unit;			/* Memory unit size in bytes */
    char _f[20-2*sizeof(__kernel_ulong_t)-sizeof(__u32)];	/* Padding: libc5 uses this.. */
};



typedef struct {
    int	val[2];
} kernel_fsid_t;

// 这个结构体有问题，也可能是我的设备问题，你需要自己对着你的内核源码找偏移，如果你要精准的修改成目标值的话
struct compat_statfs {
    __u32	f_type;
    __u32	f_bsize;
    __u32	f_frsize;	/* Fragment size - unsupported */
    __u32	__pad;
    __u64	f_blocks;
    __u64	f_bfree;
    __u64	f_files;
    __u64	f_ffree;
    __u64	f_bavail;
    __kernel_fsid_t f_fsid;
    __u32	f_namelen;
    __u32	f_flags;
    __u32	f_spare[5];
};

void (*original_si_meminfo)(struct sysinfo * val) = NULL;
void (*backup_si_meminfo)(struct sysinfo * val) = NULL;

int (*original_do_sysinfo)(struct sysinfo *info) = NULL;
int (*backup_do_sysinfo)(struct sysinfo *info) = NULL;

long (*copy_from_user_nofault)(void *dst, const void __user *src, size_t size) = NULL;

inline bool init_symbols() {
    original_si_meminfo = (typeof(original_si_meminfo))kallsyms_lookup_name("si_meminfo");
    if (!original_si_meminfo) {
        pr_info("[yuuki] kernel func: 'si_meminfo' does not exist!\n");
        goto exit;
    }

    original_do_sysinfo = (typeof(original_do_sysinfo))kallsyms_lookup_name("do_sysinfo");
    if (!original_do_sysinfo) {
        pr_info("[yuuki] kernel func: 'do_sysinfo' does not exist!\n");
        goto exit;
    }

    copy_from_user_nofault = (typeof(copy_from_user_nofault))kallsyms_lookup_name("copy_from_user_nofault");
    if (!copy_from_user_nofault) {
        pr_info("[yuuki] kernel func: 'copy_from_user_nofault' does not exist!\n");
        goto exit;
    }

    return true;
    exit:
    return false;
}

#endif
