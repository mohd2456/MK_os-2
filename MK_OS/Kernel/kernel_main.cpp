#ifndef MK_KERNEL_MAIN_CPP
#define MK_KERNEL_MAIN_CPP

#include <iostream>
#include <string>
#include <algorithm>

// Core kernel subsystems
#include "core/process.cpp"
#include "core/syscalls.cpp"
#include "core/panic.cpp"
#include "mm/pager.cpp"
#include "sched/sched.cpp"
#include "driver/console.cpp"
#include "driver/disk.cpp"
#include "driver/timer.cpp"

// ─────────────────────────────────────────────
//  MK KERNEL COMMAND SHELL
//  Accepts typed commands from the console and
//  dispatches them to kernel subsystems.
// ─────────────────────────────────────────────
void mkShell(MKConsoleDriver& console,
             MKProcessTable&  procs,
             MKKernelScheduler& sched,
             MKPager&         pager,
             MKDiskDriver&    disk,
             MKTimerDriver&   timer,
             MKSyscalls&      sys,
             MKPanic&         panic) {

    console.log("MK Shell active. Type 'help' for commands.");
    console.divider();

    while(true) {
        std::string input = console.read("mk> ");

        // Normalize input
        std::string cmd = input;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if(cmd == "help") {
            std::cout << "\nMK Shell Commands:\n"
                      << "  ps         — list all processes\n"
                      << "  sched      — show scheduler stats\n"
                      << "  mem        — show memory/page stats\n"
                      << "  disk       — show disk stats\n"
                      << "  timer      — show timer uptime\n"
                      << "  tick       — fire one timer tick\n"
                      << "  flush      — flush disk cache\n"
                      << "  panic      — trigger test kernel panic\n"
                      << "  halt       — shutdown MK\n"
                      << "  help       — show this menu\n\n";

        } else if(cmd == "ps") {
            procs.list();

        } else if(cmd == "sched") {
            sched.stats();

        } else if(cmd == "mem") {
            pager.stats();

        } else if(cmd == "disk") {
            disk.stats();

        } else if(cmd == "timer") {
            timer.stats();

        } else if(cmd == "tick") {
            timer.tick();
            procs.tick();
            sched.runNext();

        } else if(cmd == "flush") {
            disk.flush();

        } else if(cmd == "halt") {
            sys.halt();
            console.log("MK shutting down. Goodbye.");
            disk.flush();
            break;

        } else if(cmd == "panic") {
            panic.panic("User triggered test panic from shell.",
                        MKPanicCode::ASSERT_FAIL);

        } else if(cmd.empty()) {
            // Just re-prompt

        } else {
            console.log("Unknown command: '" + input + "' — try 'help'",
                        MKLogLevel::WARN);
        }
    }
}

// ─────────────────────────────────────────────
//  MK KERNEL ENTRY POINT
// ─────────────────────────────────────────────
int main() {

    // ── Phase 1: Hardware Drivers ──
    MKConsoleDriver console;
    MKDiskDriver    disk;
    MKTimerDriver   timer;

    console.banner();
    console.log("Phase 1: Hardware drivers online.");

    // ── Phase 2: Kernel Subsystems ──
    MKProcessTable    procs;
    MKSyscalls        sys;
    MKPanic           panic;
    MKPager           pager(128);
    MKKernelScheduler sched;

    console.log("Phase 2: Kernel subsystems initialized.");

    // ── Phase 3: Timer Setup ──
    timer.init();
    timer.registerEvent("sched_tick",    10, [&](){ sched.runNext(); });
    timer.registerEvent("proc_tick",     10, [&](){ procs.tick(); });
    timer.registerEvent("disk_flush",   100, [&](){ disk.flush(); });
    timer.registerEvent("reap_zombies",  50, [&](){ procs.reap(); });

    // ── Phase 4: Memory Map ──
    pager.mapPage(1, "kernel_text",  MKPageProt::READ_ONLY,  true);
    pager.mapPage(2, "kernel_data",  MKPageProt::READ_WRITE, true);
    pager.mapPage(3, "kernel_stack", MKPageProt::READ_WRITE, true);
    pager.mapPage(4, "ai_core_heap", MKPageProt::READ_WRITE, false);
    pager.mapPage(5, "mk_brain_buf", MKPageProt::READ_WRITE, false);

    console.log("Phase 3: Memory pages mapped.");

    // ── Phase 5: Boot Processes ──
    procs.create("init",        0); // PID 1 — always first
    procs.create("mk_ai_core",  1); // PID 2 — AI engine
    procs.create("mk_brain",    2); // PID 3 — knowledge store
    procs.create("mk_scheduler",3); // PID 4 — task manager
    procs.list();

    // ── Phase 6: Seed Scheduler ──
    sched.add("init_task",      MKTaskPriority::HIGH,     1);
    sched.add("ai_core_boot",   MKTaskPriority::REALTIME, 3);
    sched.add("brain_load",     MKTaskPriority::HIGH,     2);
    sched.add("network_init",   MKTaskPriority::NORMAL,   2);
    sched.add("plugin_loader",  MKTaskPriority::LOW,      1);

    // Drain boot tasks
    console.log("Phase 4: Running boot task queue...");
    sched.drain();

    // ── Phase 7: Boot Disk Signature ──
    disk.writeBlock(0, "MK_BOOT_SIG_V1");
    disk.writeBlock(1, "kernel_main_loaded");
    disk.flush();

    // ── Phase 8: System Ready ──
    sys.log("MK Kernel boot complete.");
    console.divider('=');
    console.log("MK OS is online. All subsystems nominal.");
    console.log("Uptime: " + std::to_string(timer.wallMs()) + "ms");
    console.divider('=');

    // ── Phase 9: Enter Interactive Shell ──
    mkShell(console, procs, sched, pager, disk, timer, sys, panic);

    return 0;
}

#endif // MK_KERNEL_MAIN_CPP