/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <nrf_rpc.h>
#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_tr.h>
#include <zephyr/logging/log.h>
#include <mock_nrf_rpc_transport.h>

LOG_MODULE_REGISTER(serialize_suite, CONFIG_LOG_DEFAULT_LEVEL);

extern const struct nrf_rpc_tr mock_nrf_rpc_tr;
/* Define a mock group for testing */
NRF_RPC_GROUP_DEFINE(test_group, "test", &mock_nrf_rpc_tr, NULL, NULL, NULL);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

    static uint8_t group_reg[] = {0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 0x74, 0x65, 0x73, 0x74};
    static uint8_t group_reg_response_data[] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x65, 0x73, 0x74};
    mock_nrf_rpc_pkt_t group_reg_pkt = {
        .data = group_reg,
        .len = sizeof(group_reg)
    };
    mock_nrf_rpc_pkt_t group_reg_response = {
        .data = group_reg_response_data,
        .len = sizeof(group_reg_response_data)
    };

static void *setup(void)
{
	LOG_INF("Setting up test suite");

    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    
	/* Initialize the mock transport */
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	LOG_DBG("Initialized nRF RPC");
	return NULL;
}

static void teardown(void *fixture)
{
	LOG_INF("Tearing down test suite");
	mock_nrf_rpc_tr_expect_reset();
	LOG_DBG("Reset mock transport");
}

ZTEST(serialize_suite, test_decode_basic_types)
{
	struct nrf_rpc_cbor_ctx ctx;

	LOG_INF("Starting test_decode_basic_types");

	
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

ZTEST_SUITE(serialize_suite, NULL, setup, NULL, NULL, teardown);

/* New test suite for invalid state tests */
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

ZTEST_SUITE(serialize_suite_invalid, NULL, NULL, NULL, NULL, NULL); 