#pragma once
/**
 * Here we define the initialization and entrypoint interface for our firmware. For the full
 * sumo-bot firmware, the functions defined in app.c will be used. For integration tests, the
 * functions defined under tests/integration will be used, depending on the build preset specified
 * to cmake.
 *
 * See the README for more information on building and available presets.
 */

/**
 * @brief Initialize the firmware.
 *
 * Initialize the neccessary drivers, as well as the state machine.
 *
 * Alternatively, initialize a subset of drivers for use in integration tests, if an integration
 * test target is built.
 */
void app_init(void);

/**
 * @brief Run the application.
 *
 * The run code that goes in the main while loop, primarily the state machine step.
 */
void app_run(void);