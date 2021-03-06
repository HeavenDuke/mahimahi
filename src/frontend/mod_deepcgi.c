#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"

#define MAX_HEADER_LENGTH 200001

extern const char *replayserver_filename;

static void deepcgi_hooks(apr_pool_t *inpPool);

static int deepcgi_handler(request_rec *inpRequest);

typedef struct {
    const char *working_dir;
    const char *recording_dir;
} deepcgi_config;

static deepcgi_config config;

// ============================================================================
// Methods for reading configuration parameters
// ============================================================================

const char *deepcgi_set_workingdir(cmd_parms *cmd, void *cfg, const char *arg) {
    config.working_dir = arg;
    return NULL;
}

const char *deepcgi_set_recordingdir(cmd_parms *cmd, void *cfg, const char *arg) {
    config.recording_dir = arg;
    return NULL;
}

char* set_push_headers(char* dest) {
    char push_config_file[200] = "", resource[2000] = "", type[20] = "";
    int push = 0;
    strcat(push_config_file, config.working_dir);
    strcat(push_config_file, "/");
    strcat(push_config_file, "push.txt");
    strcpy(dest, "");
    FILE* fp = fopen(push_config_file, "r");
    if (fp != NULL) {
        while(fscanf(fp, "%s %d %s", resource, &push, type) != EOF) {
            strcat(dest, "Link: <");
            strcat(dest, resource);
            strcat(dest, ">; rel=preload");
            if (push == 0) {
                strcat(dest, "; as=");
                strcat(dest, type);
                strcat(dest, ";");
            }
            strcat(dest, "\r\n");
        }
        fclose(fp);
    }
    return dest;
}

// ============================================================================
// Directives to read configuration parameters
// ============================================================================

static const command_rec deepcgi_directives[] =
        {
                AP_INIT_TAKE1("workingDir", deepcgi_set_workingdir, NULL, RSRC_CONF, "Working directory"),
                AP_INIT_TAKE1("recordingDir", deepcgi_set_recordingdir, NULL, RSRC_CONF, "Recording directory"),
                {NULL}
        };

// ============================================================================
// Module definition
// ============================================================================

module AP_MODULE_DECLARE_DATA
deepcgi_module =
        {
                STANDARD20_MODULE_STUFF,
                NULL,
                NULL,
                NULL,
                NULL,
                deepcgi_directives,
                deepcgi_hooks
        };

// ============================================================================
// Module handler function
// ============================================================================

int deepcgi_handler(request_rec *inpRequest) {
    if (!inpRequest->handler || strcmp(inpRequest->handler, "deepcgi-handler")) {
        return DECLINED;
    }

    const char *request_method = inpRequest->method;
    const char *request_uri = inpRequest->unparsed_uri;
    const char *protocol = inpRequest->protocol;
    const char *http_host = inpRequest->hostname;
    const char *user_agent = apr_table_get(inpRequest->headers_in, "User-Agent");

    setenv("MAHIMAHI_CHDIR", config.working_dir, TRUE);
    setenv("MAHIMAHI_RECORD_PATH", config.recording_dir, TRUE);
    setenv("REQUEST_METHOD", request_method, TRUE);
    setenv("REQUEST_URI", request_uri, TRUE);
    setenv("SERVER_PROTOCOL", protocol, TRUE);
    setenv("HTTP_HOST", http_host, TRUE);
    if (user_agent != NULL) {
        setenv("HTTP_USER_AGENT", user_agent, TRUE);
    }

    /* check if connection is HTTPS */
    /* see bug report at http://modpython.org/pipermail/mod_python/2006-February/020197.html */
    /* taken from fix at https://issues.apache.org/jira/secure/attachment/12321011/requestobject.c.patch */
    APR_DECLARE_OPTIONAL_FN(int, ssl_is_https, (conn_rec * ));
    APR_OPTIONAL_FN_TYPE(ssl_is_https) * optfn_is_https
            = APR_RETRIEVE_OPTIONAL_FN(ssl_is_https);

    if (optfn_is_https) {
        if (optfn_is_https(inpRequest->connection)) {
            setenv("HTTPS", "1", TRUE);
        }
    }

    FILE *fp = popen(replayserver_filename, "r");

    if (fp == NULL) {
        // "Error encountered while running script"
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    char line[HUGE_STRING_LEN], replacement[MAX_HEADER_LENGTH] = "";
    struct ap_filter_t *cur, *start;

    // Only Append Server Push Header to the response of root document
    if (strcmp(request_uri, "/") == 0 && strcmp(request_method, "GET") == 0) {
        set_push_headers(replacement);
    }

    // Get rid of all filters up through protocol...since we
    // haven't parsed off the headers, there is no way they can
    // work
    cur = inpRequest->proto_output_filters;
    start = inpRequest->proto_output_filters;
    while (cur && cur->frec->ftype < AP_FTYPE_CONNECTION) {
        cur = cur->next;
    }
    inpRequest->output_filters = inpRequest->proto_output_filters = cur;

    // Write headers + body
    int num_bytes_read, len = strlen(replacement), inserted = 0;
    do {
        num_bytes_read = fread(line, sizeof(char), HUGE_STRING_LEN, fp);
        if (inserted == 0) {
            char* result = strstr(line, "\r\n\r\n");
            if (result != NULL) {
                // write data before inserted data
                int num_bytes = result - line + 2;
                int num_bytes_left = num_bytes;
                while (num_bytes_left > 0) {
                    int offset = num_bytes - num_bytes_left;
                    int num_bytes_written = ap_rwrite(line + offset, num_bytes_left, inpRequest);
                    if (num_bytes_written == -1) {
                        // "Error encountered while writing"
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }
                    num_bytes_left -= num_bytes_written;
                }
                // write inserted headers
                num_bytes_left = len;
                while (num_bytes_left > 0) {
                    int offset = len - num_bytes_left;
                    int num_bytes_written = ap_rwrite(replacement + offset, num_bytes_left, inpRequest);
                    if (num_bytes_written == -1) {
                        // "Error encountered while writing"
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }
                    num_bytes_left -= num_bytes_written;
                }
                // write data after inserted data
                num_bytes_left = num_bytes_read - num_bytes;
                while (num_bytes_left > 0) {
                    int offset = num_bytes_read - num_bytes_left;
                    int num_bytes_written = ap_rwrite(line + offset, num_bytes_left, inpRequest);
                    if (num_bytes_written == -1) {
                        // "Error encountered while writing"
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }
                    num_bytes_left -= num_bytes_written;
                }
                inserted = 1;
            }
            else {
                int num_bytes_left = num_bytes_read;
                while (num_bytes_left > 0) {
                    int offset = num_bytes_read - num_bytes_left;
                    int num_bytes_written = ap_rwrite(line + offset, num_bytes_left, inpRequest);
                    if (num_bytes_written == -1) {
                        // "Error encountered while writing"
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }
                    num_bytes_left -= num_bytes_written;
                }
            }
        }
        else {
            int num_bytes_left = num_bytes_read;
            while (num_bytes_left > 0) {
                int offset = num_bytes_read - num_bytes_left;
                int num_bytes_written = ap_rwrite(line + offset, num_bytes_left, inpRequest);
                if (num_bytes_written == -1) {
                    // "Error encountered while writing"
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                num_bytes_left -= num_bytes_written;
            }
        }
    } while (num_bytes_read == HUGE_STRING_LEN);

    // To ensure that connection is kept-alive
    ap_set_keepalive(inpRequest);

    pclose(fp);

    return OK;
}

// ============================================================================
// Definition of hook for handler
// ============================================================================

void deepcgi_hooks(apr_pool_t *inpPool) {
    ap_hook_handler(deepcgi_handler, NULL, NULL, APR_HOOK_LAST);
}

