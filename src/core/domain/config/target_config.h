#pragma once
#include "constants.h"
#include <Arduino.h>

/**
 * @brief Target Configuration Manager
 * 
 * This class manages the dynamic configuration of targets,
 * including memory allocation, validation, and limits.
 */
class TargetConfig {
private:
  static TargetConfig* instance;
  
public:
  // Singleton pattern
  static TargetConfig& getInstance();
  
  // Configuration management
  bool initialize();
  void cleanup();
  
  // Target count management
  int getMaxTargets() const { return maxTargets; }
  int getCurrentTargetCount() const { return currentTargetCount; }
  bool setMaxTargets(int count);
  bool setCurrentTargetCount(int count);
  
  // Memory management
  bool isMemorySufficient(int targetCount) const;
  int getRecommendedMaxTargets() const;
  uint32_t getMemoryEstimate(int targetCount) const;
  
  // Validation
  bool isValidTargetCount(int count) const;
  bool canAddTarget() const;
  bool canRemoveTarget() const;
  
  // Status
  bool isInitialized() const { return initialized; }
  void printConfiguration() const;
  
private:
  TargetConfig() = default;
  ~TargetConfig() = default;
  TargetConfig(const TargetConfig&) = delete;
  TargetConfig& operator=(const TargetConfig&) = delete;
  
  // Internal state
  bool initialized;
  int maxTargets;
  int currentTargetCount;
  uint32_t availableMemory;
  
  // Internal methods
  void updateMemoryEstimate();
  int calculateRecommendedMaxTargets() const;
};
