#include "target_config.h"
#include "core/infrastructure/memory_manager/memory_manager.h"
#include "core/infrastructure/logger/logger.h"

// Static member definitions
TargetConfig* TargetConfig::instance = nullptr;

TargetConfig& TargetConfig::getInstance() {
  if (!instance) {
    instance = new TargetConfig();
  }
  return *instance;
}

bool TargetConfig::initialize() {
  if (initialized) return true;
  
  Serial.println("[TARGET_CONFIG] Initializing target configuration...");
  
  // Get available memory
  availableMemory = ESP.getFreeHeap();
  
  // Set initial max targets based on available memory
  maxTargets = calculateRecommendedMaxTargets();
  currentTargetCount = 0;
  
  Serial_printf("[TARGET_CONFIG] Initialized: maxTargets=%d, availableMemory=%u bytes\n", 
               maxTargets, availableMemory);
  
  initialized = true;
  return true;
}

void TargetConfig::cleanup() {
  if (initialized) {
    Serial.println("[TARGET_CONFIG] Cleaning up target configuration...");
    initialized = false;
  }
}

bool TargetConfig::setMaxTargets(int count) {
  if (!isValidTargetCount(count)) {
    Serial_printf("[TARGET_CONFIG] ERROR: Invalid target count %d\n", count);
    return false;
  }
  
  if (!isMemorySufficient(count)) {
    Serial_printf("[TARGET_CONFIG] ERROR: Insufficient memory for %d targets\n", count);
    return false;
  }
  
  maxTargets = count;
  Serial_printf("[TARGET_CONFIG] Max targets set to %d\n", maxTargets);
  return true;
}

bool TargetConfig::setCurrentTargetCount(int count) {
  if (count < 0 || count > maxTargets) {
    Serial_printf("[TARGET_CONFIG] ERROR: Current target count %d out of range [0, %d]\n", 
                 count, maxTargets);
    return false;
  }
  
  currentTargetCount = count;
  Serial_printf("[TARGET_CONFIG] Current target count set to %d\n", currentTargetCount);
  return true;
}

bool TargetConfig::isMemorySufficient(int targetCount) const {
  uint32_t requiredMemory = getMemoryEstimate(targetCount);
  uint32_t currentMemory = ESP.getFreeHeap();
  
  // Add safety margin
  uint32_t safetyMargin = MEMORY_SAFETY_MARGIN_BYTES;
  
  return (currentMemory - safetyMargin) >= requiredMemory;
}

int TargetConfig::getRecommendedMaxTargets() const {
  return calculateRecommendedMaxTargets();
}

uint32_t TargetConfig::getMemoryEstimate(int targetCount) const {
  return targetCount * MEMORY_PER_TARGET_BYTES;
}

bool TargetConfig::isValidTargetCount(int count) const {
  return count >= MIN_TARGETS && count <= MAX_TARGETS;
}

bool TargetConfig::canAddTarget() const {
  return currentTargetCount < maxTargets;
}

bool TargetConfig::canRemoveTarget() const {
  return currentTargetCount > 0;
}

void TargetConfig::printConfiguration() const {
  Serial.println("=== TARGET CONFIGURATION ===");
  Serial_printf("Max Targets: %d\n", maxTargets);
  Serial_printf("Current Targets: %d\n", currentTargetCount);
  Serial_printf("Available Memory: %u bytes\n", ESP.getFreeHeap());
  Serial_printf("Memory per Target: %d bytes\n", MEMORY_PER_TARGET_BYTES);
  Serial_printf("Memory Estimate: %u bytes\n", getMemoryEstimate(currentTargetCount));
  Serial_printf("Can Add Target: %s\n", canAddTarget() ? "Yes" : "No");
  Serial_printf("Can Remove Target: %s\n", canRemoveTarget() ? "Yes" : "No");
  Serial.println("============================");
}

void TargetConfig::updateMemoryEstimate() {
  availableMemory = ESP.getFreeHeap();
}

int TargetConfig::calculateRecommendedMaxTargets() const {
  uint32_t currentMemory = ESP.getFreeHeap();
  uint32_t safetyMargin = MEMORY_SAFETY_MARGIN_BYTES;
  uint32_t availableForTargets = currentMemory - safetyMargin;
  
  int recommended = availableForTargets / MEMORY_PER_TARGET_BYTES;
  
  // Apply limits
  if (recommended < MIN_TARGETS) {
    recommended = MIN_TARGETS;
  } else if (recommended > MAX_TARGETS) {
    recommended = MAX_TARGETS;
  }
  
  // Don't exceed display limits
  if (recommended > MAX_DISPLAY_TARGETS) {
    recommended = MAX_DISPLAY_TARGETS;
  }
  
  return recommended;
}
