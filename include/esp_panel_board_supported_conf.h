/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file  esp_panel_board_supported_conf.h
 * @brief Configuration file for supported ESP development boards
 *
 * This file contains configuration options for various supported development boards using ESP Panel.
 * Users can select their specific board by uncommenting the corresponding macro definition.
 */

#pragma once

/**
 * @brief Flag to enable supported board configuration (0/1)
 *
 * Set to `1` to enable supported board configuration, `0` to disable
 */
#define ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED       (1)

#if ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
#define BOARD_VIEWE_UEDX80480050E_WB_A
 
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR 2
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_PATCH 0

#endif
