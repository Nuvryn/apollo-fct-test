/** @file shell.c @brief Zephyr-only command and output adapter. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "fct/fct.h"
#include "fct/can_test.h"
#include "fct/shell.h"

void fct_shell_output(const char *text)
{
    const struct shell *shell = shell_backend_uart_get_ptr();
    if (shell != NULL)
    {
        shell_print(shell, "%s", text);
    }
}

static int fct_shell_result(const struct shell *shell, int result)
{
    if (result != 0)
    {
        shell_error(shell, "error %d (%s)", result,
            result == -EBUSY ? "test session busy" : result == -ENOENT ? "unknown test"
                                                 : result == -EALREADY ? "already idle"
                                                                       : "command rejected or device unavailable");
    }
    return result;
}

static int cmd_list(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        const fct_case_info_t *info = fct_info(idx);
        shell_print(shell, "%s [%s]: %s", info->id,
            info->interactive ? "interactive" : "automatic", info->description);
    }
    return 0;
}

static int cmd_info(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        const fct_case_info_t *info = fct_info(idx);
        if (strcmp(argv[1], info->id) == 0)
        {
            shell_print(shell, "%s: %s; timeout=%u ms", info->id, info->description,
                (unsigned int) info->timeout_ms);
            return 0;
        }
    }
    return fct_shell_result(shell, -ENOENT);
}

static int cmd_run(const struct shell *shell, size_t argc, char **argv)
{
    int result;
    (void) argc;
    result = fct_start(argv[1]);
    if (result == 0)
    {
        shell_print(shell, "Accepted; use fct results / fct stop");
    }
    return fct_shell_result(shell, result);
}

static int cmd_status(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    shell_print(shell, "%s", fct_busy() ? "busy" : "idle");
    return 0;
}

static int cmd_results(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        fct_result_t result;
        int status = fct_result(idx, &result);
        if (status != 0)
        {
            return fct_shell_result(shell, status);
        }
        shell_print(shell, "%s %s %u ms: %s", fct_info(idx)->id,
            fct_state_name(result.state), (unsigned int) result.elapsed_ms, result.reason);
    }
    return 0;
}

static int cmd_stop(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    return fct_shell_result(shell, fct_stop());
}

static int cmd_confirm(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    if (strcmp(argv[1], "pass") != 0 && strcmp(argv[1], "fail") != 0)
    {
        return fct_shell_result(shell, -EINVAL);
    }
    return fct_shell_result(shell, fct_confirm(strcmp(argv[1], "pass") == 0));
}

static int cmd_probe(const struct shell *shell, size_t argc, char **argv)
{
    char *end;
    unsigned long address;
    int result;
    (void) argc;
    errno = 0;
    address = strtoul(argv[1], &end, 0);
    if (errno != 0 || end == argv[1] || *end != '\0' || address < 0x08UL || address > 0x77UL)
    {
        return fct_shell_result(shell, -EINVAL);
    }
    result = fct_i2c_probe((uint16_t) address);
    shell_print(shell, "0x%02lx: %s (error %d); ACK does not prove device function", address,
        result == 0 ? "ACK" : "no ACK / transfer error", result);
    return result;
}

static int cmd_can_submit(const struct shell *shell, char **argv, bool transmit)
{
    fct_can_request_t request;
    int result = fct_can_parse(transmit, argv[1], argv[2], argv[3], argv[4], &request);
    if (result == 0)
    {
        result = fct_start_can(&request);
    }
    if (result == 0)
    {
        shell_print(shell, "Accepted; P10=CAN, shared OTG USB unused; use fct results / fct stop");
    }
    return fct_shell_result(shell, result);
}
static int cmd_can_tx(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    return cmd_can_submit(shell, argv, true);
}
static int cmd_can_rx(const struct shell *shell, size_t argc, char **argv)
{
    (void) argc;
    return cmd_can_submit(shell, argv, false);
}
SHELL_STATIC_SUBCMD_SET_CREATE(s_can_commands,
    SHELL_CMD_ARG(tx, NULL, "<bitrate> <std|ext> <id> <hex|->; peer ACK required", cmd_can_tx, 5, 0),
    SHELL_CMD_ARG(rx, NULL, "<bitrate> <std|ext> <id> <wait_ms:10..5000>; normal mode with ACK", cmd_can_rx, 5, 0),
    SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(s_i2c_commands,
    SHELL_CMD_ARG(probe, NULL, "Address-only probe, e.g. 0x50; no data write", cmd_probe, 2, 0),
    SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(s_fct_commands,
    SHELL_CMD_ARG(list, NULL, "List implemented tests", cmd_list, 1, 0),
    SHELL_CMD_ARG(info, NULL, "Describe one test", cmd_info, 2, 0),
    SHELL_CMD_ARG(run, NULL, "Run <id|all>; all excludes interactive tests", cmd_run, 2, 0),
    SHELL_CMD_ARG(status, NULL, "Worker state", cmd_status, 1, 0),
    SHELL_CMD_ARG(results, NULL, "RAM-only results", cmd_results, 1, 0),
    SHELL_CMD_ARG(stop, NULL, "Request cancellation", cmd_stop, 1, 0),
    SHELL_CMD_ARG(confirm, NULL, "Confirm current observation: pass|fail", cmd_confirm, 2, 0),
    SHELL_CMD(can, &s_can_commands, "External classic-CAN tests; P10/USB conflict", NULL),
    SHELL_CMD(i2c, &s_i2c_commands, "Manual I2C diagnostics", NULL),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(fct, &s_fct_commands, "Apollo V1 functional tests", NULL);
