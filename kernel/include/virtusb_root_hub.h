// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "virtusb_hub.h"

/**
 * DOC: VirtUSB root hub model
 *
 * This header defines the functional model of a VirtUSB root hub.
 *
 * A VirtUsbRHub is permanently associated with exactly one VirtUsbHcd and uses
 * the common VirtUSB hub model for its hub and downstream-port state.
 *
 * Root-hub-specific state may be added to this structure as required by later
 * implementation steps.
 */

/**
 * struct virtusb_root_hub - Functional state of a virtual USB root hub
 * @hub: Common virtual USB hub state.
 *
 * This structure extends the common VirtUSB hub model with state specific to
 * VirtUsbRHub.
 */
struct virtusb_root_hub {
   struct virtusb_hub hub;
};

/**
 * virtusb_root_hub_init() - Initialize a virtual USB root hub
 * @root_hub: Root hub to initialize.
 * @port_count: Number of downstream ports.
 *
 * Initializes the common hub state of @root_hub.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_root_hub_init(struct virtusb_root_hub *root_hub,
                          unsigned int port_count);

/**
 * virtusb_root_hub_reset() - Reset the functional root-hub state
 * @root_hub: Root hub to reset.
 *
 * Resets the common hub state while preserving the configured number of
 * downstream ports.
 */
void virtusb_root_hub_reset(struct virtusb_root_hub *root_hub);
