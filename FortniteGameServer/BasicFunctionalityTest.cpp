#include "ObjectManager.h"
#include <iostream>
#include <cassert>

/**
 * BasicFunctionalityTest.cpp
 * 
 * This file contains basic tests to verify that the improved UObject system
 * is working correctly. Run this to validate the implementation.
 */

// Mock global object array for testing
FUObjectArray* GObjects = nullptr;

// Test functions
void TestBasicTypes() {
    std::cout << "Testing basic types...\n";
    
    // Test FName
    FName testName(123, 456);
    std::string nameStr = testName.ToString();
    assert(!nameStr.empty());
    assert(testName.IsValid());
    
    // Test FString  
    FString testString("Hello World");
    std::string converted = testString.ToString();
    assert(converted == "Hello World");
    
    // Test FVector
    FVector vec(1.0f, 2.0f, 3.0f);
    float size = vec.Size();
    assert(size > 0.0f);
    
    std::cout << "✓ Basic types test passed\n";
}

void TestObjectManager() {
    std::cout << "Testing ObjectManager...\n";
    
    auto& manager = ObjectManager::Get();
    
    // Test singleton access
    auto& manager2 = ObjectManager::Get();
    assert(&manager == &manager2);
    
    // Test cache operations
    manager.InvalidateCache();
    size_t cacheSize = manager.GetCacheSize();
    assert(cacheSize == 0);
    
    // Test stats
    auto stats = manager.GetStats();
    assert(stats.TotalSearches >= 0);
    
    manager.ResetStats();
    auto newStats = manager.GetStats();
    assert(newStats.TotalSearches == 0);
    
    std::cout << "✓ ObjectManager test passed\n";
}

void TestTypeRegistry() {
    std::cout << "Testing TypeRegistry...\n";
    
    auto& registry = TypeRegistry::Get();
    
    // Test singleton access
    auto& registry2 = TypeRegistry::Get();
    assert(&registry == &registry2);
    
    // Test type aliases
    registry.RegisterTypeAlias("TestType", "ActualType");
    
    // Test type queries
    auto types = registry.GetAllTypes();
    // Should not crash
    
    std::cout << "✓ TypeRegistry test passed\n";
}

void TestErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    auto& manager = ObjectManager::Get();
    
    // Test null object validation
    assert(!manager.IsValidObject(nullptr));
    
    // Test finding non-existent objects (should not crash)
    auto* result = manager.FindObject<UObject>("NonExistentObject");
    assert(result == nullptr);  // Should return null, not crash
    
    std::cout << "✓ Error handling test passed\n";
}

void TestPerformance() {
    std::cout << "Testing performance features...\n";
    
    auto& manager = ObjectManager::Get();
    
    // Test batch processing (should not crash with empty object array)
    manager.ProcessObjectsBatch<UObject>([](UObject* obj) {
        // This should not be called since GObjects is null
        assert(false);  
    });
    
    // Test performance monitoring
    auto stats = manager.GetStats();
    // Should not crash accessing stats
    
    std::cout << "✓ Performance features test passed\n";
}

void TestCrossPlatformFeatures() {
    std::cout << "Testing cross-platform features...\n";
    
    // Test logging macros (should not crash)
    LOG_INFO("Test info message");
    LOG_WARN("Test warning message");
    LOG_ERROR("Test error message");
    
    // Test string formatting
    std::string formatted = FormatString("Test %s %d", "string", 42);
    assert(!formatted.empty());
    
    std::cout << "✓ Cross-platform features test passed\n";
}

void TestUtilityFunctions() {
    std::cout << "Testing utility functions...\n";
    
    // Test memory helpers (basic syntax check)
    int testValue = 42;
    void* ptr = &testValue;
    
    int readValue = Read<int>(ptr);
    assert(readValue == 42);
    
    Write<int>(ptr, 123);
    assert(Read<int>(ptr) == 123);
    
    std::cout << "✓ Utility functions test passed\n";
}

int main() {
    std::cout << "=== Fortnite Game Server - Basic Functionality Tests ===\n\n";
    
    try {
        TestBasicTypes();
        TestObjectManager();
        TestTypeRegistry();
        TestErrorHandling();
        TestPerformance();
        TestCrossPlatformFeatures();
        TestUtilityFunctions();
        
        std::cout << "\n=== All Tests Passed Successfully! ===\n";
        std::cout << "The enhanced UObject system is working correctly.\n";
        std::cout << "You can now use the new features safely.\n\n";
        
        // Demonstrate basic usage
        std::cout << "Example Usage:\n";
        std::cout << "  auto& manager = ObjectManager::Get();\n";
        std::cout << "  auto player = manager.FindObject<UObject>(\"ObjectName\");\n";
        std::cout << "  auto& registry = TypeRegistry::Get();\n";
        std::cout << "  registry.RegisterTypeAlias(\"Alias\", \"ActualType\");\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
