/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <cyaml/cyaml.h>
#include <syslog.h>
#include "config.h"

const cyaml_schema_field_t gpio_fields_schema[] = {
    CYAML_FIELD_UINT("port",            CYAML_FLAG_DEFAULT, config_gpio_t, port),
    CYAML_FIELD_UINT("pin",             CYAML_FLAG_DEFAULT, config_gpio_t, pin),
    CYAML_FIELD_END
};

const cyaml_schema_field_t send_fields_schema[] = {
    CYAML_FIELD_UINT("sig_freq",        CYAML_FLAG_DEFAULT, config_send_t, sig_freq),
    CYAML_FIELD_UINT("modem_bytes",     CYAML_FLAG_DEFAULT, config_send_t, modem_bytes),
    CYAML_FIELD_UINT("modem_wait",      CYAML_FLAG_DEFAULT, config_send_t, modem_wait),
    CYAML_FIELD_UINT("sinad_start",     CYAML_FLAG_DEFAULT, config_send_t, sinad_start),
    CYAML_FIELD_UINT("sinad_stop",      CYAML_FLAG_DEFAULT, config_send_t, sinad_stop),
    CYAML_FIELD_END
};

const cyaml_schema_field_t modem_fields_schema[] = {
    CYAML_FIELD_UINT("fsk",             CYAML_FLAG_DEFAULT, config_modem_t, fsk),
    CYAML_FIELD_UINT("rate",            CYAML_FLAG_DEFAULT, config_modem_t, rate),
    CYAML_FIELD_UINT("first_tone",      CYAML_FLAG_DEFAULT, config_modem_t, first_tone),
    CYAML_FIELD_UINT("tone_spacing",    CYAML_FLAG_DEFAULT, config_modem_t, tone_spacing),
    CYAML_FIELD_STRING_PTR("codename",  CYAML_FLAG_POINTER, config_modem_t, codename, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

const cyaml_schema_field_t config_fields_schema[] = {
    CYAML_FIELD_MAPPING("ptt",          CYAML_FLAG_DEFAULT, config_t, ptt, gpio_fields_schema),
    CYAML_FIELD_UINT("tcp_port",        CYAML_FLAG_DEFAULT, config_t, tcp_port),
    CYAML_FIELD_MAPPING("modem",        CYAML_FLAG_DEFAULT, config_t, modem, modem_fields_schema),
    CYAML_FIELD_MAPPING("send",         CYAML_FLAG_DEFAULT, config_t, send, send_fields_schema),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t config_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, config_t, config_fields_schema)
};

static const cyaml_config_t cyaml_config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

config_t *config;

bool config_load(const char *filename) {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &cyaml_config, &config_schema, (void **) &config, NULL);

    if (err != CYAML_OK) {
        syslog(LOG_ERR, "%s", cyaml_strerror(err));
        return false;
    }

    return true;
}
