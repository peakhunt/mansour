#include "shell.h"

#include "sys_timer.h"
#include "version.h"

#include "hardware/clocks.h"
#include "pico/malloc.h"

static void shell_command_help(ShellIntf* intf, int argc, const char** argv);
static void shell_command_version(ShellIntf* intf, int argc, const char** argv);
static void shell_command_uptime(ShellIntf* intf, int argc, const char** argv);
static void shell_command_sysinfo(ShellIntf* intf, int argc, const char** argv);

static const ShellCommand _commands[] = 
{
  {
    "help",
    "show this command",
    shell_command_help,
  },
  {
    "version",
    "show version",
    shell_command_version,
  },
  {
    "uptime",
    "show system uptime",
    shell_command_uptime,
  },
  {
    "sysinfo",
    "show system info",
    shell_command_sysinfo,
  },
};

////////////////////////////////////////////////////////////////////////////////
//
// shell command handlers
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_command_help(ShellIntf* intf, int argc, const char** argv)
{
  size_t i;

  shell_printf(intf, "\r\n");

  for(i = 0; i < sizeof(_commands)/sizeof(ShellCommand); i++)
  {
    shell_printf(intf, "%-20s: ", _commands[i].command);
    shell_printf(intf, "%s\r\n", _commands[i].description);
  }
}

static void
shell_command_version(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "%s\r\n", VERSION);
}

static void
shell_command_uptime(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "System Uptime    : %lu sec\r\n", __uptime);
}

static void
shell_command_sysinfo(ShellIntf* intf, int argc, const char** argv)
{
  uint32_t hz = clock_get_hz(clk_sys);

  // 2. Memory Info (RP2040 has 264KB total SRAM)
  // We can report total SRAM and a rough estimate of free heap
  extern char __StackLimit; // Provided by the linker script
  extern char __bss_end__;
  uint32_t total_ram = 264 * 1024;
  uint32_t free_approx = (uint32_t)&__StackLimit - (uint32_t)&__bss_end__;

  shell_printf(intf, "\r\n--- System Information ---\r\n");
  shell_printf(intf, "CPU Speed      : %lu MHz\r\n", hz / 1000000);
  shell_printf(intf, "Total SRAM     : %lu Bytes\r\n", total_ram);
  shell_printf(intf, "Approx. Free   : %lu Bytes\r\n", free_approx);
}

////////////////////////////////////////////////////////////////////////////////
//
// init
//
////////////////////////////////////////////////////////////////////////////////
void
shell_commands_init(void)
{
  shell_init(_commands, sizeof(_commands)/sizeof(ShellCommand));
}
