#pragma once
#include <Arduino.h>

/**
 * @brief Configuration constants for the ESP32 TFT Network Monitor
 * 
 * This file contains all the configurable constants that control
 * system behavior, memory allocation, and feature limits.
 */

// ===========================================
// Target Configuration
// ===========================================
// Maximum number of targets that can be monitored
// Can be adjusted based on available memory and requirements
// Recommended values:
// - 4-6 targets: Low memory usage, good for basic monitoring
// - 8-10 targets: Medium memory usage, balanced performance
// - 12-15 targets: High memory usage, comprehensive monitoring
// - 20+ targets: Maximum monitoring, requires careful memory management

#ifndef MAX_TARGETS
#define MAX_TARGETS 18  // Default maximum targets (3 tabs x 6 targets)
#endif

// Minimum number of targets (always allow at least 1)
#define MIN_TARGETS 1

// Maximum targets for memory-constrained environments
#define MAX_TARGETS_LOW_MEMORY 6

// Maximum targets for high-memory environments
#define MAX_TARGETS_HIGH_MEMORY 18

// Tab configuration
#define TARGETS_PER_TAB 6
#define MAX_TABS 3
#define MAX_DISPLAY_TARGETS (TARGETS_PER_TAB * MAX_TABS)  // 18 targets total

// ===========================================
// Memory Configuration
// ===========================================
// Memory thresholds for different target counts
#define MEMORY_PER_TARGET_BYTES 300  // Estimated memory per target (bytes)
#define MEMORY_SAFETY_MARGIN_BYTES 10000  // Safety margin for system stability

// ===========================================
// Display Configuration
// ===========================================
// Maximum targets that can be displayed on screen
// Limited by screen size and UI layout
// Now supports tabbed interface with 6 targets per tab

// ===========================================
// Validation
// ===========================================
// Ensure MAX_TARGETS is within reasonable bounds
#if MAX_TARGETS < MIN_TARGETS
#error "MAX_TARGETS must be at least MIN_TARGETS"
#endif

#if MAX_TARGETS > MAX_TARGETS_HIGH_MEMORY
#error "MAX_TARGETS exceeds maximum recommended value"
#endif

// ===========================================
// Helper Macros
// ===========================================
// Get the actual maximum targets based on available memory
#define GET_ACTUAL_MAX_TARGETS() min(MAX_TARGETS, MAX_DISPLAY_TARGETS)

// Check if target count is valid
#define IS_VALID_TARGET_COUNT(count) ((count) >= MIN_TARGETS && (count) <= MAX_TARGETS)

// Get memory estimate for target count
#define GET_MEMORY_ESTIMATE(count) ((count) * MEMORY_PER_TARGET_BYTES + MEMORY_SAFETY_MARGIN_BYTES)
