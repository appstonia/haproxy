/*
 * Health-checks.
 *
 * Copyright 2008-2009 Krzysztof Piotr Oledzki <ole@ans.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#ifndef _TYPES_CHECKS_H
#define _TYPES_CHECKS_H

#include <sys/time.h>

#include <common/config.h>
#include <common/mini-clist.h>
#include <common/regex.h>
#include <common/buf.h>

#include <types/connection.h>
#include <types/obj_type.h>
#include <types/task.h>

/* enum used by check->result. Must remain in this order, as some code uses
 * result >= CHK_RES_PASSED to declare success.
 */
enum chk_result {
	CHK_RES_UNKNOWN = 0,            /* initialized to this by default */
	CHK_RES_NEUTRAL,                /* valid check but no status information */
	CHK_RES_FAILED,                 /* check failed */
	CHK_RES_PASSED,                 /* check succeeded and server is fully up again */
	CHK_RES_CONDPASS,               /* check reports the server doesn't want new sessions */
};

/* flags used by check->state */
#define CHK_ST_INPROGRESS       0x0001  /* a check is currently running */
#define CHK_ST_CONFIGURED       0x0002  /* this check is configured and may be enabled */
#define CHK_ST_ENABLED          0x0004  /* this check is currently administratively enabled */
#define CHK_ST_PAUSED           0x0008  /* checks are paused because of maintenance (health only) */
#define CHK_ST_AGENT            0x0010  /* check is an agent check (otherwise it's a health check) */
#define CHK_ST_PORT_MISS        0x0020  /* check can't be send because no port is configured to run it */

/* check status */
enum healthcheck_status {
	HCHK_STATUS_UNKNOWN	 = 0,	/* Unknown */
	HCHK_STATUS_INI,		/* Initializing */
	HCHK_STATUS_START,		/* Check started - SPECIAL STATUS */

	/* Below we have finished checks */
	HCHK_STATUS_CHECKED,		/* DUMMY STATUS */

	HCHK_STATUS_HANA,		/* Health analyze detected enough consecutive errors */

	HCHK_STATUS_SOCKERR,		/* Socket error */

	HCHK_STATUS_L4OK,		/* L4 check passed, for example tcp connect */
	HCHK_STATUS_L4TOUT,		/* L4 timeout */
	HCHK_STATUS_L4CON,		/* L4 connection problem, for example: */
					/*  "Connection refused" (tcp rst) or "No route to host" (icmp) */

	HCHK_STATUS_L6OK,		/* L6 check passed */
	HCHK_STATUS_L6TOUT,		/* L6 (SSL) timeout */
	HCHK_STATUS_L6RSP,		/* L6 invalid response - protocol error */

	HCHK_STATUS_L7TOUT,		/* L7 (HTTP/SMTP) timeout */
	HCHK_STATUS_L7RSP,		/* L7 invalid response - protocol error */

	/* Below we have layer 5-7 data available */
	HCHK_STATUS_L57DATA,		/* DUMMY STATUS */
	HCHK_STATUS_L7OKD,		/* L7 check passed */
	HCHK_STATUS_L7OKCD,		/* L7 check conditionally passed */
	HCHK_STATUS_L7STS,		/* L7 response error, for example HTTP 5xx */

	HCHK_STATUS_PROCERR,		/* External process check failure */
	HCHK_STATUS_PROCTOUT,		/* External process check timeout */
	HCHK_STATUS_PROCOK,		/* External process check passed */

	HCHK_STATUS_SIZE
};

/* environment variables memory requirement for different types of data */
#define EXTCHK_SIZE_EVAL_INIT 0                  /* size determined during the init phase,
                                                  * such environment variables are not updatable. */
#define EXTCHK_SIZE_ULONG     20                 /* max string length for an unsigned long value */
#define EXTCHK_SIZE_UINT      11                 /* max string length for an unsigned int value */
#define EXTCHK_SIZE_ADDR      INET6_ADDRSTRLEN+1 /* max string length for an address */

/* external checks environment variables */
enum {
	EXTCHK_PATH = 0,

	/* Proxy specific environment variables */
	EXTCHK_HAPROXY_PROXY_NAME,	/* the backend name */
	EXTCHK_HAPROXY_PROXY_ID,	/* the backend id */
	EXTCHK_HAPROXY_PROXY_ADDR,	/* the first bind address if available (or empty) */
	EXTCHK_HAPROXY_PROXY_PORT,	/* the first bind port if available (or empty) */

	/* Server specific environment variables */
	EXTCHK_HAPROXY_SERVER_NAME,	/* the server name */
	EXTCHK_HAPROXY_SERVER_ID,	/* the server id */
	EXTCHK_HAPROXY_SERVER_ADDR,	/* the server address */
	EXTCHK_HAPROXY_SERVER_PORT,	/* the server port if available (or empty) */
	EXTCHK_HAPROXY_SERVER_MAXCONN,	/* the server max connections */
	EXTCHK_HAPROXY_SERVER_CURCONN,	/* the current number of connections on the server */

	EXTCHK_SIZE
};


/* health status for response tracking */
enum {
	HANA_STATUS_UNKNOWN	= 0,

	HANA_STATUS_L4_OK,		/* L4 successful connection */
	HANA_STATUS_L4_ERR,		/* L4 unsuccessful connection */

	HANA_STATUS_HTTP_OK,		/* Correct http response */
	HANA_STATUS_HTTP_STS,		/* Wrong http response, for example HTTP 5xx */
	HANA_STATUS_HTTP_HDRRSP,	/* Invalid http response (headers) */
	HANA_STATUS_HTTP_RSP,		/* Invalid http response */

	HANA_STATUS_HTTP_READ_ERROR,	/* Read error */
	HANA_STATUS_HTTP_READ_TIMEOUT,	/* Read timeout */
	HANA_STATUS_HTTP_BROKEN_PIPE,	/* Unexpected close from server */

	HANA_STATUS_SIZE
};

enum {
	HANA_ONERR_UNKNOWN	= 0,

	HANA_ONERR_FASTINTER,		/* Force fastinter*/
	HANA_ONERR_FAILCHK,		/* Simulate a failed check */
	HANA_ONERR_SUDDTH,		/* Enters sudden death - one more failed check will mark this server down */
	HANA_ONERR_MARKDWN,		/* Mark this server down, now! */
};

enum {
	HANA_ONMARKEDDOWN_NONE	= 0,
	HANA_ONMARKEDDOWN_SHUTDOWNSESSIONS,	/* Shutdown peer sessions */
};

enum {
	HANA_ONMARKEDUP_NONE	= 0,
	HANA_ONMARKEDUP_SHUTDOWNBACKUPSESSIONS,	/* Shutdown peer sessions */
};

enum {
	HANA_OBS_NONE		= 0,

	HANA_OBS_LAYER4,		/* Observe L4 - for example tcp */
	HANA_OBS_LAYER7,		/* Observe L7 - for example http */

	HANA_OBS_SIZE
};

struct check {
	struct session *sess;			/* Health check session. */
	struct vars vars;			/* Health check dynamic variables. */
	struct xprt_ops *xprt;			/* transport layer operations for health checks */
	struct conn_stream *cs;			/* conn_stream state for health checks */
	struct buffer bi, bo;			/* input and output buffers to send/recv check */
	struct task *task;			/* the task associated to the health check processing, NULL if disabled */
	struct timeval start;			/* last health check start time */
	long duration;				/* time in ms took to finish last health check */
	short status, code;			/* check result, check code */
	unsigned short port;			/* the port to use for the health checks */
	char desc[HCHK_DESC_LEN];		/* health check description */
	char use_ssl;				/* use SSL for health checks (1: on, 0: server mode, -1: off) */
	int send_proxy;				/* send a PROXY protocol header with checks */
	struct tcpcheck_rules *tcpcheck_rules;	/* tcp-check send / expect rules */
	struct tcpcheck_rule *current_step;     /* current step when using tcpcheck */
	int inter, fastinter, downinter;        /* checks: time in milliseconds */
	enum chk_result result;                 /* health-check result : CHK_RES_* */
	int state;				/* state of the check : CHK_ST_*   */
	int health;				/* 0 to rise-1 = bad;
						 * rise to rise+fall-1 = good */
	int rise, fall;				/* time in iterations */
	int type;				/* Check type, one of PR_O2_*_CHK */
	int send_string_len;			/* length of agent command string */
	char *send_string;			/* optionally send a string when connecting to the agent */
	struct server *server;			/* back-pointer to server */
	struct proxy *proxy;                    /* proxy to be used */
	char **argv;				/* the arguments to use if running a process-based check */
	char **envp;				/* the environment to use if running a process-based check */
	struct pid_list *curpid;		/* entry in pid_list used for current process-based test, or -1 if not in test */
	struct sockaddr_storage addr;   	/* the address to check */
	struct wait_event wait_list;            /* Waiting for I/O events */
	char *sni;				/* Server name */
	char *alpn_str;                         /* ALPN to use for checks */
	int alpn_len;                           /* ALPN string length */

	int via_socks4;                         /* check the connection via socks4 proxy */
};

struct check_status {
	short result;			/* one of SRV_CHK_* */
	char *info;			/* human readable short info */
	char *desc;			/* long description */
};

struct extcheck_env {
	char *name;	/* environment variable name */
	int vmaxlen;	/* value maximum length, used to determine the required memory allocation */
};

struct analyze_status {
	char *desc;				/* description */
	unsigned char lr[HANA_OBS_SIZE];	/* result for l4/l7: 0 = ignore, 1 - error, 2 - OK */
};

#define TCPCHK_OPT_NONE            0x0000  /* no options specified, default */
#define TCPCHK_OPT_SEND_PROXY      0x0001  /* send proxy-protocol string */
#define TCPCHK_OPT_SSL             0x0002  /* SSL connection */
#define TCPCHK_OPT_LINGER          0x0004  /* Do not RST connection, let it linger */
#define TCPCHK_OPT_DEFAULT_CONNECT 0x0008  /* Do a connect using server params */
#define TCPCHK_OPT_IMPLICIT        0x0010  /* Implicit connect */
#define TCPCHK_OPT_SOCKS4          0x0020  /* check the connection via socks4 proxy */

struct tcpcheck_connect {
	char *sni;                     /* server name to use for SSL connections */
	char *alpn;                    /* ALPN to use for the SSL connection */
	int alpn_len;                  /* ALPN string length */
	uint16_t options;              /* options when setting up a new connection */
	uint16_t port;                 /* port to connect to */
	struct sample_expr *port_expr; /* sample expr to determine the port, may be NULL */
	struct sockaddr_storage addr;  /* the address to the connect */
};

enum tcpcheck_send_type {
	TCPCHK_SEND_UNDEF = 0,  /* Send is not parsed. */
	TCPCHK_SEND_STRING,     /* Send an ASCII string. */
	TCPCHK_SEND_BINARY,     /* Send a binary sequence. */
	TCPCHK_SEND_STRING_LF, /* Send an ASCII log-format string. */
	TCPCHK_SEND_BINARY_LF, /* Send a binary log-format sequence. */
};

struct tcpcheck_send {
	enum tcpcheck_send_type type;
	union {
		struct ist  data; /* an ASCII string or a binary sequence */
		struct list fmt;  /* an ASCII or hexa log-format string */
	};
};

enum tcpcheck_expect_type {
	TCPCHK_EXPECT_UNDEF = 0, /* Match is not used. */
	TCPCHK_EXPECT_STRING, /* Matches a string. */
	TCPCHK_EXPECT_REGEX, /* Matches a regular pattern. */
	TCPCHK_EXPECT_REGEX_BINARY, /* Matches a regular pattern on a hex-encoded text. */
	TCPCHK_EXPECT_BINARY, /* Matches a binary sequence. */
};

struct tcpcheck_expect {
	enum tcpcheck_expect_type type; /* Type of pattern used for matching. */
	union {
		char *string;           /* Matching a literal string / binary anywhere in the response. */
		struct my_regex *regex; /* Matching a regex pattern. */
	};
	struct tcpcheck_rule *head;     /* first expect of a chain. */
	int length;                     /* Size in bytes of the pattern referenced by string / binary. */
	int inverse;                    /* Match is inversed. */
	int with_capture;               /* Match will store captured groups for back-reference in comment. */
	int min_recv;                   /* Minimum amount of data before an expect can be applied. (default: -1, ignored) */
	struct list onerror_fmt;        /* log-format string to use as comment on error */
	struct list onsuccess_fmt;      /* log-format string to use as comment on success (if last rule) */
	enum healthcheck_status err_status;  /* The healthcheck status to use on error (default: L7RSP) */
	enum healthcheck_status tout_status; /* The healthcheck status to use on timeout (default: L7TOUT) */
	struct sample_expr *status_expr; /* sample expr to determine the check status code */
};

struct tcpcheck_action_kw {
	struct act_rule *rule;
};

/* possible actions for tcpcheck_rule->action */
enum tcpcheck_rule_type {
	TCPCHK_ACT_SEND = 0, /* send action, regular string format */
	TCPCHK_ACT_EXPECT, /* expect action, either regular or binary string */
	TCPCHK_ACT_CONNECT, /* connect action, to probe a new port */
	TCPCHK_ACT_COMMENT, /* no action, simply a comment used for logs */
	TCPCHK_ACT_ACTION_KW, /* custom registered action_kw rule. */
};

struct tcpcheck_rule {
	struct list list;                       /* list linked to from the proxy */
	enum tcpcheck_rule_type action;         /* type of the rule. */
	int index;                              /* Index within the list. Starts at 0. */
	char *comment;				/* comment to be used in the logs and on the stats socket */
	union {
		struct tcpcheck_connect connect; /* Connect rule. */
		struct tcpcheck_send send;      /* Send rule. */
		struct tcpcheck_expect expect;  /* Expected pattern. */
		struct tcpcheck_action_kw action_kw;  /* Custom action. */
	};
};

#define TCPCHK_RULES_NONE   0x00000000
#define TCPCHK_RULES_SHARED 0x00000001 /* Set for shared list of tcp-check rules */
#define TCPCHK_RULES_DEF    0x00000002 /* Ruleset inherited from the default section */

/* a list of tcp-check rules */
struct tcpcheck_rules {
	unsigned int flags; /* flags applied to the rules */
	struct list *list; /* the list of tcpcheck_rules */
};

/* A list of tcp-check rules with a name */
struct tcpcheck_ruleset {
	char *name;         /* the ruleset name */
	struct list rules;  /* the list of tcpcheck_rule */
	struct list list;   /* used to chain rulesets */
};


#endif /* _TYPES_CHECKS_H */
