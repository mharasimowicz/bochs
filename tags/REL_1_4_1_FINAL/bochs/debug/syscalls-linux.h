/////////////////////////////////////////////////////////////////////////
// $Id: syscalls-linux.h,v 1.3 2001-10-03 13:10:37 bdenney Exp $
/////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
// Linux system call table
//////////////////////////////////////////////////////////////////////////////
//
// Format for each entry:
//   DEF_LINUX_SYSCALL(syscall_number, "syscall_name")
// This file can be regenerated with the following command:
//
//    ./make-syscalls-linux.pl < /usr/include/asm/unistd.h > syscalls-linux.h
//
DEF_SYSCALL(1, "exit")
DEF_SYSCALL(2, "fork")
DEF_SYSCALL(3, "read")
DEF_SYSCALL(4, "write")
DEF_SYSCALL(5, "open")
DEF_SYSCALL(6, "close")
DEF_SYSCALL(7, "waitpid")
DEF_SYSCALL(8, "creat")
DEF_SYSCALL(9, "link")
DEF_SYSCALL(10, "unlink")
DEF_SYSCALL(11, "execve")
DEF_SYSCALL(12, "chdir")
DEF_SYSCALL(13, "time")
DEF_SYSCALL(14, "mknod")
DEF_SYSCALL(15, "chmod")
DEF_SYSCALL(16, "lchown")
DEF_SYSCALL(17, "break")
DEF_SYSCALL(18, "oldstat")
DEF_SYSCALL(19, "lseek")
DEF_SYSCALL(20, "getpid")
DEF_SYSCALL(21, "mount")
DEF_SYSCALL(22, "umount")
DEF_SYSCALL(23, "setuid")
DEF_SYSCALL(24, "getuid")
DEF_SYSCALL(25, "stime")
DEF_SYSCALL(26, "ptrace")
DEF_SYSCALL(27, "alarm")
DEF_SYSCALL(28, "oldfstat")
DEF_SYSCALL(29, "pause")
DEF_SYSCALL(30, "utime")
DEF_SYSCALL(31, "stty")
DEF_SYSCALL(32, "gtty")
DEF_SYSCALL(33, "access")
DEF_SYSCALL(34, "nice")
DEF_SYSCALL(35, "ftime")
DEF_SYSCALL(36, "sync")
DEF_SYSCALL(37, "kill")
DEF_SYSCALL(38, "rename")
DEF_SYSCALL(39, "mkdir")
DEF_SYSCALL(40, "rmdir")
DEF_SYSCALL(41, "dup")
DEF_SYSCALL(42, "pipe")
DEF_SYSCALL(43, "times")
DEF_SYSCALL(44, "prof")
DEF_SYSCALL(45, "brk")
DEF_SYSCALL(46, "setgid")
DEF_SYSCALL(47, "getgid")
DEF_SYSCALL(48, "signal")
DEF_SYSCALL(49, "geteuid")
DEF_SYSCALL(50, "getegid")
DEF_SYSCALL(51, "acct")
DEF_SYSCALL(52, "umount2")
DEF_SYSCALL(53, "lock")
DEF_SYSCALL(54, "ioctl")
DEF_SYSCALL(55, "fcntl")
DEF_SYSCALL(56, "mpx")
DEF_SYSCALL(57, "setpgid")
DEF_SYSCALL(58, "ulimit")
DEF_SYSCALL(59, "oldolduname")
DEF_SYSCALL(60, "umask")
DEF_SYSCALL(61, "chroot")
DEF_SYSCALL(62, "ustat")
DEF_SYSCALL(63, "dup2")
DEF_SYSCALL(64, "getppid")
DEF_SYSCALL(65, "getpgrp")
DEF_SYSCALL(66, "setsid")
DEF_SYSCALL(67, "sigaction")
DEF_SYSCALL(68, "sgetmask")
DEF_SYSCALL(69, "ssetmask")
DEF_SYSCALL(70, "setreuid")
DEF_SYSCALL(71, "setregid")
DEF_SYSCALL(72, "sigsuspend")
DEF_SYSCALL(73, "sigpending")
DEF_SYSCALL(74, "sethostname")
DEF_SYSCALL(75, "setrlimit")
DEF_SYSCALL(76, "getrlimit")
DEF_SYSCALL(77, "getrusage")
DEF_SYSCALL(78, "gettimeofday")
DEF_SYSCALL(79, "settimeofday")
DEF_SYSCALL(80, "getgroups")
DEF_SYSCALL(81, "setgroups")
DEF_SYSCALL(82, "select")
DEF_SYSCALL(83, "symlink")
DEF_SYSCALL(84, "oldlstat")
DEF_SYSCALL(85, "readlink")
DEF_SYSCALL(86, "uselib")
DEF_SYSCALL(87, "swapon")
DEF_SYSCALL(88, "reboot")
DEF_SYSCALL(89, "readdir")
DEF_SYSCALL(90, "mmap")
DEF_SYSCALL(91, "munmap")
DEF_SYSCALL(92, "truncate")
DEF_SYSCALL(93, "ftruncate")
DEF_SYSCALL(94, "fchmod")
DEF_SYSCALL(95, "fchown")
DEF_SYSCALL(96, "getpriority")
DEF_SYSCALL(97, "setpriority")
DEF_SYSCALL(98, "profil")
DEF_SYSCALL(99, "statfs")
DEF_SYSCALL(100, "fstatfs")
DEF_SYSCALL(101, "ioperm")
DEF_SYSCALL(102, "socketcall")
DEF_SYSCALL(103, "syslog")
DEF_SYSCALL(104, "setitimer")
DEF_SYSCALL(105, "getitimer")
DEF_SYSCALL(106, "stat")
DEF_SYSCALL(107, "lstat")
DEF_SYSCALL(108, "fstat")
DEF_SYSCALL(109, "olduname")
DEF_SYSCALL(110, "iopl")
DEF_SYSCALL(111, "vhangup")
DEF_SYSCALL(112, "idle")
DEF_SYSCALL(113, "vm86old")
DEF_SYSCALL(114, "wait4")
DEF_SYSCALL(115, "swapoff")
DEF_SYSCALL(116, "sysinfo")
DEF_SYSCALL(117, "ipc")
DEF_SYSCALL(118, "fsync")
DEF_SYSCALL(119, "sigreturn")
DEF_SYSCALL(120, "clone")
DEF_SYSCALL(121, "setdomainname")
DEF_SYSCALL(122, "uname")
DEF_SYSCALL(123, "modify_ldt")
DEF_SYSCALL(124, "adjtimex")
DEF_SYSCALL(125, "mprotect")
DEF_SYSCALL(126, "sigprocmask")
DEF_SYSCALL(127, "create_module")
DEF_SYSCALL(128, "init_module")
DEF_SYSCALL(129, "delete_module")
DEF_SYSCALL(130, "get_kernel_syms")
DEF_SYSCALL(131, "quotactl")
DEF_SYSCALL(132, "getpgid")
DEF_SYSCALL(133, "fchdir")
DEF_SYSCALL(134, "bdflush")
DEF_SYSCALL(135, "sysfs")
DEF_SYSCALL(136, "personality")
DEF_SYSCALL(137, "afs_syscall")
DEF_SYSCALL(138, "setfsuid")
DEF_SYSCALL(139, "setfsgid")
DEF_SYSCALL(141, "getdents")
DEF_SYSCALL(143, "flock")
DEF_SYSCALL(144, "msync")
DEF_SYSCALL(145, "readv")
DEF_SYSCALL(146, "writev")
DEF_SYSCALL(147, "getsid")
DEF_SYSCALL(148, "fdatasync")
DEF_SYSCALL(150, "mlock")
DEF_SYSCALL(151, "munlock")
DEF_SYSCALL(152, "mlockall")
DEF_SYSCALL(153, "munlockall")
DEF_SYSCALL(154, "sched_setparam")
DEF_SYSCALL(155, "sched_getparam")
DEF_SYSCALL(156, "sched_setscheduler")
DEF_SYSCALL(157, "sched_getscheduler")
DEF_SYSCALL(158, "sched_yield")
DEF_SYSCALL(159, "sched_get_priority_max")
DEF_SYSCALL(160, "sched_get_priority_min")
DEF_SYSCALL(161, "sched_rr_get_interval")
DEF_SYSCALL(162, "nanosleep")
DEF_SYSCALL(163, "mremap")
DEF_SYSCALL(164, "setresuid")
DEF_SYSCALL(165, "getresuid")
DEF_SYSCALL(166, "vm86")
DEF_SYSCALL(167, "query_module")
DEF_SYSCALL(168, "poll")
DEF_SYSCALL(169, "nfsservctl")
DEF_SYSCALL(170, "setresgid")
DEF_SYSCALL(171, "getresgid")
DEF_SYSCALL(172, "prctl")
DEF_SYSCALL(173, "rt_sigreturn")
DEF_SYSCALL(174, "rt_sigaction")
DEF_SYSCALL(175, "rt_sigprocmask")
DEF_SYSCALL(176, "rt_sigpending")
DEF_SYSCALL(177, "rt_sigtimedwait")
DEF_SYSCALL(178, "rt_sigqueueinfo")
DEF_SYSCALL(179, "rt_sigsuspend")
DEF_SYSCALL(180, "pread")
DEF_SYSCALL(181, "pwrite")
DEF_SYSCALL(182, "chown")
DEF_SYSCALL(183, "getcwd")
DEF_SYSCALL(184, "capget")
DEF_SYSCALL(185, "capset")
DEF_SYSCALL(186, "sigaltstack")
DEF_SYSCALL(187, "sendfile")
DEF_SYSCALL(188, "getpmsg")
DEF_SYSCALL(189, "putpmsg")
DEF_SYSCALL(190, "vfork")
DEF_SYSCALL(191, "ugetrlimit")
DEF_SYSCALL(192, "mmap2")
DEF_SYSCALL(193, "truncate64")
DEF_SYSCALL(194, "ftruncate64")
DEF_SYSCALL(195, "stat64")
DEF_SYSCALL(196, "lstat64")
DEF_SYSCALL(197, "fstat64")
#define N_SYSCALLS 197
