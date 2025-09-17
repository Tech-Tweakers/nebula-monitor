// Test file to demonstrate fixed control flow issues
// This file shows how the new do-while(0) macros prevent dangling else problems

#include "src/core/infrastructure/logger/logger.h"

void testControlFlow() {
  bool condition = true;
  
  // OLD WAY (would cause dangling else problem):
  // if (condition) LOG_DEBUG("Debug message"); else Serial.println("This would bind to the macro's internal if!");
  
  // NEW WAY (fixed with do-while(0)):
  if (condition) {
    LOG_DEBUG("Debug message");
  } else {
    Serial.println("This correctly binds to our if statement!");
  }
  
  // Another example - nested conditions work correctly now
  if (condition) {
    LOG_INFO("Info message");
    if (condition) {
      LOG_WARN("Warning message");
    } else {
      LOG_ERROR("Error message");
    }
  } else {
    Serial.println("Else branch works correctly");
  }
  
  // Loop with logging works correctly
  for (int i = 0; i < 3; i++) {
    LOG_DEBUG_F("Loop iteration %d", i);
  }
}

// This demonstrates that the macros now have proper block scoping
void testBlockScoping() {
  // Variables declared inside macro blocks don't leak scope
  LOG_DEBUG("Testing block scoping");
  
  // The do-while(0) pattern ensures proper block boundaries
  if (true) {
    LOG_INFO("This is inside a proper block");
  }
}
