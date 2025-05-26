/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <nrf_rpc.h>
#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_tr.h>
#include <mock_nrf_rpc_transport.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <zcbor_decode.h>

LOG_MODULE_REGISTER(serialize_suite, CONFIG_LOG_DEFAULT_LEVEL);

extern const struct nrf_rpc_tr mock_nrf_rpc_tr;

/* Define a mock group for testing */
NRF_RPC_GROUP_DEFINE(test_group, "test", &mock_nrf_rpc_tr, NULL, NULL, NULL);



/* Error handler for tests */
static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
    zassert_ok(report->code);
}

/* Error handler for invalid state tests */
static void nrf_rpc_err_handler_invalid(const struct nrf_rpc_err_report *report)
{
    static bool first_error = true;

    if (first_error) {
        /* First error is from group registration, should succeed */
        zassert_ok(report->code);
        first_error = false;
    } else {
        /* Second error is from our test, should be -12 */
        zassert_equal(report->code, -12, "Expected error code -12, got %d", report->code);
    }
}

static uint8_t group_reg[] = {0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 0x74, 0x65, 0x73, 0x74};
static uint8_t group_reg_response_data[] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x65, 0x73, 0x74};
static mock_nrf_rpc_pkt_t group_reg_pkt = {
    .data = group_reg,
    .len = sizeof(group_reg)
};
static mock_nrf_rpc_pkt_t group_reg_response = {
    .data = group_reg_response_data,
    .len = sizeof(group_reg_response_data)
};

/* Setup and teardown for encode tests */
static void *setup(void)
{
	LOG_INF("Setting up test suite");
    /* Reset mock transport before each test */
    mock_nrf_rpc_tr_expect_reset();
    /* Add group registration packet */
    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    /* Initialize the mock transport */
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	LOG_DBG("Initialized nRF RPC");
	return NULL;
}

static void teardown(void *fixture)
{
	LOG_INF("Tearing down test suite");
    /* Reset mock transport after each test */
    mock_nrf_rpc_tr_expect_reset();
    LOG_DBG("Reset mock transport");
}

/* Setup and teardown for decode tests */
static void *setup_decode(void)
{
	LOG_INF("Setting up decode test suite");
    /* Reset mock transport before each test */
    mock_nrf_rpc_tr_expect_reset();
    /* Add group registration packet */
    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    /* Initialize the mock transport */
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	LOG_DBG("Initialized nRF RPC for decode tests");
	return NULL;
}

static void teardown_decode(void *fixture)
{
	LOG_INF("Tearing down decode test suite");
    /* Reset mock transport after each test */
    mock_nrf_rpc_tr_expect_reset();
    LOG_DBG("Reset mock transport for decode tests");
}

/* Setup and teardown for invalid state tests */
static void *setup_invalid(void)
{
    LOG_INF("Setting up invalid state test suite");
    /* Reset mock transport before each test */
    mock_nrf_rpc_tr_expect_reset();
    /* Add group registration packet */
    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    /* Initialize the mock transport */
    zassert_ok(nrf_rpc_init(nrf_rpc_err_handler_invalid));
    LOG_DBG("Initialized nRF RPC for invalid state tests");
    return NULL;
}

static void teardown_invalid(void *fixture)
{
	LOG_INF("Tearing down invalid state test suite");
    /* Reset mock transport after each test */
    mock_nrf_rpc_tr_expect_reset();
    LOG_DBG("Reset mock transport for invalid state tests");
}

ZTEST(serialize_suite, test_encode_basic_types)
{
	struct nrf_rpc_cbor_ctx ctx;

	LOG_INF("Starting test_encode_basic_types");

	
	/* Test data that is being sent with nrf rpc */
	uint8_t test_data[] = {
		0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
		0xf5,                          /* CBOR major type 7 (simple/float) - represents boolean true */
		0x01,                          /* CBOR unsigned integer 1 */
		0x20,                          /* CBOR negative integer -1 (0x20 = -1 in CBOR) */
		0x63, 0x61, 0x62, 0x63,        /* CBOR string "abc" (0x63 = length 3, followed by ASCII bytes) */
		0xf6                           /* CBOR simple value false (end of packet) */
	};

	LOG_HEXDUMP_DBG(test_data, sizeof(test_data), "Expected test data packet:");

	mock_nrf_rpc_pkt_t expect = {
		.data = test_data,
		.len = sizeof(test_data)
	};
	mock_nrf_rpc_pkt_t response = {
		.data = NULL,
		.len = 0
	};

	mock_nrf_rpc_tr_expect_add(expect, response);
	LOG_DBG("Added expected test data packet");

	/* Initialize context for encoding */
	NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
	LOG_DBG("Initialized CBOR context");

	/* Encode test data */
	nrf_rpc_encode_bool(&ctx, true);
	nrf_rpc_encode_uint(&ctx, 1);
	nrf_rpc_encode_int(&ctx, -1);
	nrf_rpc_encode_str(&ctx, "abc", 3);
	LOG_DBG("Encoded test data");

    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);
	/* Log the actual packet being sent */
	if (ctx.out_packet != NULL) {
		LOG_HEXDUMP_DBG(ctx.out_packet, 32, "Actual packet being sent:");
		LOG_DBG("Packet length: %d", 32);
	}

	mock_nrf_rpc_tr_expect_done();
	LOG_INF("Test completed successfully");
}

ZTEST(serialize_suite, test_encode_null)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0xf6,                          /* CBOR null value */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_null(&ctx);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_undefined)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0xf7,                          /* CBOR undefined value */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_undefined(&ctx);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint64)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x01,                          /* CBOR uint64 value: 1 (minimal encoding) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint64(&ctx, 1);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint64_max)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* CBOR uint64 value: UINT64_MAX */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint64(&ctx, UINT64_MAX);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint64_min)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint64 value: 0 (UINT64_MIN) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint64(&ctx, 0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint64_random)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1b, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,  /* CBOR uint64 value: 0x123456789abcdef0 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint64(&ctx, 0x123456789abcdef0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int64)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x20,                          /* CBOR int64 value: -1 (minimal encoding) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int64(&ctx, -1);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int64_min)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* CBOR int64 value: INT64_MIN */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int64(&ctx, INT64_MIN);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int64_max)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* CBOR int64 value: INT64_MAX */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int64(&ctx, INT64_MAX);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int64_random)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x3b, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,  /* CBOR int64 value: -0x123456789abcdef1 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int64(&ctx, -0x123456789abcdef1);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint value: 0 (minimal encoding) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint(&ctx, 0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint_max)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1a, 0xff, 0xff, 0xff, 0xff,  /* CBOR uint value: UINT32_MAX */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint(&ctx, UINT32_MAX);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint_min)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint value: 0 (minimal encoding) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint(&ctx, 0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_uint_random)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1a, 0x12, 0x34, 0x56, 0x78,  /* CBOR uint value: 0x12345678 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_uint(&ctx, 0x12345678);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x20,                          /* CBOR int value: -1 (minimal encoding) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int(&ctx, -1);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int_min)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x3a, 0x7f, 0xff, 0xff, 0xff,  /* CBOR int value: INT32_MIN */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int(&ctx, INT32_MIN);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int_max)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x1a, 0x7f, 0xff, 0xff, 0xff,  /* CBOR int value: INT32_MAX */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int(&ctx, INT32_MAX);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_int_random)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x3a, 0x12, 0x34, 0x56, 0x78,  /* CBOR int value: -0x12345679 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_int(&ctx, -0x12345679);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_str)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x60,                          /* CBOR string: "" (empty string) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_str(&ctx, "", 0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_str_long)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x78, 0x20,                    /* CBOR string length: 32 */
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,  /* "aaaaaaaa" */
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,  /* "aaaaaaaa" */
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,  /* "aaaaaaaa" */
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,  /* "aaaaaaaa" */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 64);
    const char *str = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    nrf_rpc_encode_str(&ctx, str, 32);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_str_min)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x60,                          /* CBOR string: "" (empty string) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_str(&ctx, "", 0);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_str_random)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x6b, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64,  /* CBOR string: "Hello World" */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_str(&ctx, "Hello World", 11);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite, test_encode_buffer)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x44, 0x01, 0x02, 0x03, 0x04,  /* CBOR bytes: [0x01, 0x02, 0x03, 0x04] */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect = {
        .data = test_data,
        .len = sizeof(test_data)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    nrf_rpc_encode_buffer(&ctx, buffer, sizeof(buffer));
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(serialize_suite_invalid, test_encoder_invalid)
{
    /* Setup */
    LOG_INF("Setting up invalid state test");
    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    zassert_ok(nrf_rpc_init(nrf_rpc_err_handler_invalid));
    LOG_DBG("Initialized nRF RPC for invalid state test");
    
    /* Test */
    struct nrf_rpc_cbor_ctx ctx;
    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    
    /* First encode a value */
    nrf_rpc_encode_uint(&ctx, 42);
    
    /* Make encoder invalid */
    nrf_rpc_encoder_invalid(&ctx);
    
    /* Try to encode more values - they should be ignored */
    nrf_rpc_encode_int(&ctx, -1);
    nrf_rpc_encode_str(&ctx, "test", 4);
    nrf_rpc_encode_bool(&ctx, true);
    
    /* Send the packet - should fail with error -12 */
    int err = nrf_rpc_cbor_evt(&test_group, 0x00, &ctx);
    zassert_equal(err, -12, "Expected error code -12, got %d", err);

    /* Teardown */
    LOG_INF("Tearing down invalid state test");
    mock_nrf_rpc_tr_expect_reset();
    LOG_DBG("Reset mock transport");
}

/* Test suite declarations */
ZTEST_SUITE(serialize_suite, NULL, setup, NULL, NULL, teardown);
ZTEST_SUITE(serialize_suite_decode, NULL, setup_decode, NULL, NULL, teardown_decode);
ZTEST_SUITE(serialize_suite_invalid, NULL, setup_invalid, NULL, NULL, teardown_invalid);

/* Move decode tests to decode suite */
ZTEST(serialize_suite_decode, test_decode_basic_types)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0xf5,                          /* CBOR boolean true */
        0xf6,                          /* CBOR boolean false */
        0x01,                          /* CBOR uint32: 1 (major type 0, value 1) */
        0x1a, 0xff, 0xff, 0xff, 0xff,  /* CBOR uint32: UINT32_MAX (major type 0, value 0xffffffff) */
        0x20,                          /* CBOR int32: -1 (major type 1, value 0) */
        0x3a, 0x7f, 0xff, 0xff, 0xff,  /* CBOR int32: INT32_MIN (major type 1, value 0x7fffffff) */
        0x63, 0x61, 0x62, 0x63,        /* CBOR string: "abc" (major type 3, length 3) */
        0x45, 0x01, 0x02, 0x03, 0x04, 0x05,  /* CBOR bytes: [0x01, 0x02, 0x03, 0x04, 0x05] (major type 2, length 5) */
        0xf6                           /* CBOR simple value false (end of packet) */
    };
    zcbor_state_t zs[2];  /* Need at least 2 states for backup */
    ctx.in_packet = test_data + 5;  /* Skip RPC header */
    zcbor_new_decode_state(zs, 2, ctx.in_packet, sizeof(test_data) - 5, 1, NULL, 0);
    memcpy(&ctx.zs, zs, sizeof(zcbor_state_t));

    /* Decode boolean values */
    bool bool_val = nrf_rpc_decode_bool(&ctx);
    zassert_true(bool_val, "Failed to decode true bool");
    bool_val = nrf_rpc_decode_bool(&ctx);
    zassert_false(bool_val, "Failed to decode false bool");

    /* Decode uint32 values */
    uint32_t uint_val = nrf_rpc_decode_uint(&ctx);
    zassert_equal(uint_val, 1, "Failed to decode uint32 1");
    uint_val = nrf_rpc_decode_uint(&ctx);
    zassert_equal(uint_val, UINT32_MAX, "Failed to decode uint32 max");

    /* Decode int32 values */
    int32_t int_val = nrf_rpc_decode_int(&ctx);
    zassert_equal(int_val, -1, "Failed to decode int32 -1");
    int_val = nrf_rpc_decode_int(&ctx);
    zassert_equal(int_val, INT32_MIN, "Failed to decode int32 min");

    /* Decode string */
    char str_buffer[32];
    char *decoded_str = nrf_rpc_decode_str(&ctx, str_buffer, sizeof(str_buffer));
    zassert_not_null(decoded_str, "Failed to decode string");
    zassert_equal(strcmp(decoded_str, "abc"), 0, "String decode mismatch");

    /* Decode bytes */
    uint8_t data_buffer[32];
    void *decoded_data = nrf_rpc_decode_buffer(&ctx, data_buffer, sizeof(data_buffer));
    zassert_not_null(decoded_data, "Failed to decode buffer");
    uint8_t expected_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    zassert_mem_equal(decoded_data, expected_data, sizeof(expected_data), "Buffer decode mismatch");
}

ZTEST(serialize_suite_decode, test_decode_uint64)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint64: 0 */
        0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* CBOR uint64: UINT64_MAX */
        0x1b, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,  /* CBOR uint64: 0x123456789abcdef0 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };
    zcbor_state_t zs[1];
    ctx.in_packet = test_data + 5;
    zcbor_new_decode_state(zs, 1, ctx.in_packet, sizeof(test_data) - 5, 1, NULL, 0);
    memcpy(&ctx.zs, zs, sizeof(zcbor_state_t));
    uint64_t uint64_val = nrf_rpc_decode_uint64(&ctx);
    zassert_equal(uint64_val, 0, "Failed to decode uint64 0");
    uint64_val = nrf_rpc_decode_uint64(&ctx);
    zassert_equal(uint64_val, UINT64_MAX, "Failed to decode uint64 max");
    uint64_val = nrf_rpc_decode_uint64(&ctx);
    zassert_equal(uint64_val, 0x123456789abcdef0, "Failed to decode uint64 random value");
}

ZTEST(serialize_suite_decode, test_decode_int64)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* CBOR int64: INT64_MIN */
        0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* CBOR int64: INT64_MAX */
        0x3b, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,  /* CBOR int64: -0x123456789abcdef0 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };
    zcbor_state_t zs[1];
    ctx.in_packet = test_data + 5;
    zcbor_new_decode_state(zs, 1, ctx.in_packet, sizeof(test_data) - 5, 1, NULL, 0);
    memcpy(&ctx.zs, zs, sizeof(zcbor_state_t));
    int64_t int64_val = nrf_rpc_decode_int64(&ctx);
    zassert_equal(int64_val, INT64_MIN, "Failed to decode int64 min");
    int64_val = nrf_rpc_decode_int64(&ctx);
    zassert_equal(int64_val, INT64_MAX, "Failed to decode int64 max");
    int64_val = nrf_rpc_decode_int64(&ctx);
    zassert_equal(int64_val, -0x123456789abcdef0, "Failed to decode int64 random value");
}

ZTEST(serialize_suite_decode, test_decode_edge_cases)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0xf6,                          /* CBOR null */
        0xf7,                          /* CBOR undefined */
        0x63, 0x61, 0x62, 0x63,        /* CBOR string: "abc" */
        0x45, 0x01, 0x02, 0x03, 0x04, 0x05,  /* CBOR bytes: [0x01, 0x02, 0x03, 0x04, 0x05] */
        0xf6                           /* CBOR simple value false (end of packet) */
    };
    zcbor_state_t zs[1];
    ctx.in_packet = test_data + 5;
    zcbor_new_decode_state(zs, 1, ctx.in_packet, sizeof(test_data) - 5, 1, NULL, 0);
    memcpy(&ctx.zs, zs, sizeof(zcbor_state_t));
    zassert_true(nrf_rpc_decode_is_null(&ctx), "Failed to decode null");
    zassert_true(nrf_rpc_decode_is_undefined(&ctx), "Failed to decode undefined");
    char str_buffer[2];
    char *decoded_str = nrf_rpc_decode_str(&ctx, str_buffer, sizeof(str_buffer));
    zassert_is_null(decoded_str, "String decode should fail with small buffer");
    uint8_t data_buffer[2];
    void *decoded_data = nrf_rpc_decode_buffer(&ctx, data_buffer, sizeof(data_buffer));
    zassert_is_null(decoded_data, "Buffer decode should fail with small buffer");
} 