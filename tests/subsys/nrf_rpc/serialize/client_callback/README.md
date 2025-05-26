# nRF RPC Callback Serialization Tests

## Overview

The nRF RPC callback feature enables remote procedure calls (RPC) to include function pointers (callbacks) that can be executed on the remote side. This is particularly useful in scenarios where:

- A client needs to register a callback that will be called by the server when certain events occur
- Asynchronous operations need to notify the client about their completion
- Event-driven architectures need to handle notifications across different devices
- Bidirectional communication is required between client and server

The callback feature works by:
1. Serializing the callback function into a CBOR format
2. Transmitting it to the remote side
3. Storing it in a callback proxy
4. Allowing the remote side to call the callback when needed

This test suite verifies the functionality of the nRF RPC callback serialization feature. It tests the encoding and decoding of callbacks in CBOR format, as well as the callback proxy functionality.

## Test Cases

### Basic Callback Tests

1. `test_encode_callback`
   - Tests basic callback encoding
   - Verifies that a callback function is properly encoded into a CBOR slot
   - Checks the correct packet format and slot number

2. `test_encode_callback_with_data`
   - Tests callback encoding with additional data
   - Verifies that a callback function and its associated data are properly encoded
   - Ensures the data is correctly serialized in the CBOR format

3. `test_encode_multiple_callbacks`
   - Tests encoding multiple callbacks in sequence
   - Verifies that multiple callbacks are assigned correct slot numbers
   - Ensures proper serialization of multiple callbacks in a single packet

### Callback Call Tests

4. `test_encode_callback_call`
   - Tests encoding a callback call
   - Verifies that a callback slot can be properly encoded for calling
   - Checks the correct packet format for callback calls

5. `test_callback_call_with_data`
   - Tests calling a callback with data
   - Verifies that data can be passed to a callback when calling it
   - Ensures proper serialization of both the callback call and its data

### Lifecycle Tests

6. `test_callback_lifecycle`
   - Tests the complete callback lifecycle
   - Verifies callback registration
   - Tests callback calling with data
   - Ensures proper interaction between registration and calling

## Building and Running

To build and run the tests:

```bash
west build -b native_sim -p && ./build/client_callback/zephyr/zephyr.exe
```

## Configuration

The test suite requires the following configuration:
- `CONFIG_NRF_RPC=y`
- `CONFIG_NRF_RPC_CBOR=y`
- `CONFIG_NRF_RPC_CALLBACK_PROXY=y`
- `CONFIG_NRF_RPC_CBKPROXY_OUT_SLOTS=0` (for native_sim)
- `CONFIG_MOCK_NRF_RPC=y`
- `CONFIG_MOCK_NRF_RPC_TRANSPORT=y` 