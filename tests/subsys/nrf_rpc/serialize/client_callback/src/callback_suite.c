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

LOG_MODULE_REGISTER(callback_suite, CONFIG_LOG_DEFAULT_LEVEL);

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
    LOG_INF("Setting up callback test suite");
    mock_nrf_rpc_tr_expect_add(group_reg_pkt, group_reg_response);
    zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
    LOG_DBG("Initialized nRF RPC for callback tests");
    return NULL;
}

static void teardown(void *fixture)
{
    LOG_INF("Tearing down callback test suite");
    mock_nrf_rpc_tr_expect_reset();
    LOG_DBG("Reset mock transport");
}

/* Test callback function */
static void test_callback(const struct nrf_rpc_cbor_ctx *ctx)
{
    /* This is just a mock callback, we don't need to do anything */
}

/* Test callback function with data */
static void test_callback_with_data(const struct nrf_rpc_cbor_ctx *ctx)
{
    uint32_t value = nrf_rpc_decode_uint((struct nrf_rpc_cbor_ctx *)ctx);
    zassert_equal(value, 42, "Expected value 42, got %d", value);
}

ZTEST(callback_suite, test_encode_callback)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x01,                          /* CBOR uint: 1 (callback slot) */
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
    
    /* Use real function as callback */
    nrf_rpc_encode_callback(&ctx, test_callback);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(callback_suite, test_encode_callback_call)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x02,                          /* CBOR uint: 2 (callback slot to call) */
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
    
    /* Encode callback slot 2 for calling */
    nrf_rpc_encode_callback_call(&ctx, 2);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(callback_suite, test_encode_callback_with_data)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint: 0 (callback slot) */
        0x18, 0x2a,                    /* CBOR uint: 42 */
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
    
    /* Use callback function that expects data */
    nrf_rpc_encode_callback(&ctx, test_callback_with_data);
    nrf_rpc_encode_uint(&ctx, 42);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(callback_suite, test_encode_multiple_callbacks)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x01,                          /* CBOR uint: 1 (first callback slot) */
        0x01,                          /* CBOR uint: 1 (second callback slot) */
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
    
    /* Encode multiple callbacks */
    nrf_rpc_encode_callback(&ctx, test_callback);
    nrf_rpc_encode_callback(&ctx, test_callback);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(callback_suite, test_callback_call_with_data)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x02,                          /* CBOR uint: 2 (callback slot to call) */
        0x18, 0x2a,                    /* CBOR uint: 42 */
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
    
    /* Call callback with data */
    nrf_rpc_encode_callback_call(&ctx, 2);
    nrf_rpc_encode_uint(&ctx, 42);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST(callback_suite, test_callback_lifecycle)
{
    struct nrf_rpc_cbor_ctx ctx;
    uint8_t test_data1[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint: 0 (callback slot) */
        0x18, 0x2a,                    /* CBOR uint: 42 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    uint8_t test_data2[] = {
        0x00, 0x00, 0xff, 0x00, 0x00,  /* RPC packet header */
        0x00,                          /* CBOR uint: 0 (callback slot) */
        0x18, 0x2a,                    /* CBOR uint: 42 */
        0xf6                           /* CBOR simple value false (end of packet) */
    };

    mock_nrf_rpc_pkt_t expect1 = {
        .data = test_data1,
        .len = sizeof(test_data1)
    };
    mock_nrf_rpc_pkt_t expect2 = {
        .data = test_data2,
        .len = sizeof(test_data2)
    };
    mock_nrf_rpc_pkt_t response = {
        .data = NULL,
        .len = 0
    };

    mock_nrf_rpc_tr_expect_add(expect1, response);
    mock_nrf_rpc_tr_expect_add(expect2, response);

    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    
    /* Register callback */
    nrf_rpc_encode_callback(&ctx, test_callback_with_data);
    nrf_rpc_encode_uint(&ctx, 42);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    /* Call the registered callback */
    NRF_RPC_CBOR_ALLOC(&test_group, ctx, 32);
    nrf_rpc_encode_callback_call(&ctx, 0);
    nrf_rpc_encode_uint(&ctx, 42);
    nrf_rpc_cbor_evt_no_err(&test_group, 0x00, &ctx);

    mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(callback_suite, NULL, setup, NULL, NULL, teardown); 