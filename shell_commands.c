#include <string.h>
#include <stdlib.h>
#include "shell.h"

#include "sys_timer.h"
#include "version.h"

#include "hardware/clocks.h"
#include "pico/malloc.h"
#include "motor_demo.h"
#include "dht11.h"
#include "mansour_foc.h"

static void shell_command_help(ShellIntf* intf, int argc, const char** argv);
static void shell_command_version(ShellIntf* intf, int argc, const char** argv);
static void shell_command_uptime(ShellIntf* intf, int argc, const char** argv);
static void shell_command_sysinfo(ShellIntf* intf, int argc, const char** argv);
static void shell_command_renc(ShellIntf* intf, int argc, const char** argv);
static void shell_command_dht11(ShellIntf* intf, int argc, const char** argv);
static void shell_command_as5600(ShellIntf* intf, int argc, const char** argv);
static void shell_command_foc(ShellIntf* intf, int argc, const char** argv);

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
  {
    "renc",
    "show rotary encoder info",
    shell_command_renc,
  },
  {
    "dht11",
    "show dht11 temperature and humidity",
    shell_command_dht11,
  },
  {
    "as5600",
    "read as5600 angle",
    shell_command_as5600,
  },
  {
    "foc",
    "show/set foc parameters",
    shell_command_foc,
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

static void
shell_command_renc(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "Rotary Encoder   : %d \r\n", motor_rotary_encoder_get());
}

static void
shell_command_dht11(ShellIntf* intf, int argc, const char** argv)
{
  float t;
  uint8_t h;
  uint32_t attempt, fail;

  dht11_get(&t, &h, &attempt, &fail);
  shell_printf(intf, "\r\n");
  shell_printf(intf, "DHT11 Temperature: %.1f \r\n", t);
  shell_printf(intf, "DHT11 Humidity   : %d %\r\n", h);
  shell_printf(intf, "DHT11 Attempt    : %ld %\r\n", attempt);
  shell_printf(intf, "DHT11 Fail       : %ld %\r\n", fail);
}

static void
shell_command_as5600(ShellIntf* intf, int argc, const char** argv)
{
  float a;

  a = mansour_foc_get_as5600();
  shell_printf(intf, "\r\n");
  shell_printf(intf, "AS5600 Angle     : %.1f \r\n", a);
}

static void
shell_command_foc(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  if(argc <= 1)
  {
    goto invalid_cmd;
  }

  if(strcmp(argv[1], "info") == 0)
  {
    uint16_t  hz;
    float     t_angle, t_actual,
              p_gain, d_gain, i_gain,
              v_limit;
    mansour_foc_get_foc_info(&hz, &t_angle, &t_actual, &p_gain, &d_gain, &i_gain, &v_limit);
    shell_printf(intf, "FOC Loop Hz      : %ld Hz\r\n", hz);
    shell_printf(intf, "Target Angle     : %.1f\r\n", t_angle);
    shell_printf(intf, "Target Actual    : %.1f\r\n", t_actual);
    shell_printf(intf, "P Gain           : %.4f\r\n", p_gain);
    shell_printf(intf, "I Gain           : %.4f\r\n", i_gain);
    shell_printf(intf, "D Gain           : %.4f\r\n", d_gain);
    shell_printf(intf, "Voltate Limit    : %.1f%\r\n", v_limit);
  }
  else if(strcmp(argv[1], "target") == 0 && argc == 3)
  {
    float t_angle = atof(argv[2]);

    if(t_angle < 0) t_angle = 0;
    if(t_angle > 360) t_angle = 0;

    shell_printf(intf,"Setting target angle to %.1f\r\n", t_angle);
    mansour_foc_set_target_angle(t_angle);
  }
  else if(strcmp(argv[1], "vlimit") == 0 && argc == 3)
  {
    float vlimit = atof(argv[2]);

    if(vlimit < 0) vlimit = 0;
    if(vlimit > 100) vlimit = 100;

    shell_printf(intf,"Setting voltage limit to %.1f\r\n", vlimit);
    mansour_foc_set_vlimit(vlimit);
  }
  else if(strcmp(argv[1], "gain") == 0 && argc == 5)
  {
    float p = atof(argv[2]);
    float i = atof(argv[3]);
    float d = atof(argv[4]);

    if(p < 0) p = 0;
    if(i < 0) i = 0;
    if(d < 0) d = 0;

    shell_printf(intf,"Setting gains to %.4f %.4f %.4f\r\n", p, i, d);
    mansour_foc_set_gain(p, i, d);
  }
  else
  {
    goto invalid_cmd;
  }
  return;

invalid_cmd:
  shell_printf(intf, "Invalid Command\r\n");
  shell_printf(intf, "foc info             -- show foc parameters\r\n");
  shell_printf(intf, "foc target <angle>   -- set foc target angle\r\n");
  shell_printf(intf, "foc vlimit <percent> -- set foc PWM voltage limit\r\n");
  shell_printf(intf, "foc gain <p> <i> <d> -- set foc gain\r\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// init
//
////////////////////////////////////////////////////////////////////////////////
void
shell_commands_init(void)
{
  shell_init(_commands,
      sizeof(_commands)/sizeof(ShellCommand),
      "\r\nChinese Puppy> ");
}
