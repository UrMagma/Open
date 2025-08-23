#include "GameSystemTestSuite.h"
#include <iostream>
#include <fstream>
#include <sstream>

GameSystemTestSuite& GameSystemTestSuite::Get() {
    static GameSystemTestSuite Instance;
    return Instance;
}

void GameSystemTestSuite::Initialize() {
    LOG_INFO("Initializing Game System Test Suite...");
    
    // Setup all test suites
    SetupPlayerManagerTests();
    SetupInventoryManagerTests();
    SetupBuildingManagerTests();
    SetupSafezoneManagerTests();
    SetupMatchManagerTests();
    SetupNetworkManagerTests();
    SetupEventManagerTests();
    SetupIntegrationTests();
    SetupPerformanceTests();
    
    LOG_INFO("Test Suite initialized with " + std::to_string(TestSuites.size()) + " test suites");
}

void GameSystemTestSuite::RunAllTests() {
    LOG_INFO("=== Starting Full Game System Test Suite ===");
    
    InitializeTestData();
    
    auto startTime = std::chrono::steady_clock::now();
    
    for (auto& [suiteName, suite] : TestSuites) {
        if (suite.bEnabled) {
            RunTestSuite(suiteName);
            
            if (Settings.bStopOnFirstFailure && !suite.AllTestsPassed()) {
                LOG_ERROR("Stopping test execution due to failures in " + suiteName);
                break;
            }
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    LOG_INFO("=== Test Suite Completed in " + std::to_string(totalTime.count()) + "ms ===");
    
    if (Settings.bRunBenchmarks) {
        RunBenchmarks();
    }
    
    PrintSummary();
    GenerateReport();
    
    CleanupTestData();
}

void GameSystemTestSuite::RunTestSuite(const std::string& SuiteName) {
    auto it = TestSuites.find(SuiteName);
    if (it == TestSuites.end()) {
        LOG_ERROR("Test suite not found: " + SuiteName);
        return;
    }
    
    FTestSuite& suite = it->second;
    LOG_INFO("Running test suite: " + SuiteName);
    
    for (auto& testCase : suite.TestCases) {
        auto startTime = std::chrono::steady_clock::now();
        
        try {
            bool result = testCase.TestFunction();
            testCase.Result = result ? ETestResult::Passed : ETestResult::Failed;
            
            if (Settings.bVerboseOutput) {
                std::string status = result ? "PASSED" : "FAILED";
                LOG_INFO("  " + testCase.Name + ": " + status);
            }
            
        } catch (const std::exception& e) {
            testCase.Result = ETestResult::Error;
            testCase.ErrorMessage = e.what();
            LOG_ERROR("  " + testCase.Name + ": ERROR - " + std::string(e.what()));
        }
        
        auto endTime = std::chrono::steady_clock::now();
        testCase.ExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    }
    
    LOG_INFO("Suite " + SuiteName + " completed: " + 
             std::to_string(suite.GetPassedCount()) + "/" + 
             std::to_string(suite.TestCases.size()) + " tests passed");
}

// Example test implementations
void GameSystemTestSuite::SetupPlayerManagerTests() {
    FTestSuite suite("PlayerManager", "Tests for player management system");
    
    suite.AddTest(FTestCase("PlayerJoinLeave", "Test player joining and leaving", 
        [this]() { return TestPlayerJoinLeave(); }));
    
    suite.AddTest(FTestCase("PlayerSpawning", "Test player spawn system", 
        [this]() { return TestPlayerSpawning(); }));
    
    suite.AddTest(FTestCase("PlayerElimination", "Test player elimination", 
        [this]() { return TestPlayerElimination(); }));
    
    suite.AddTest(FTestCase("TeamManagement", "Test team assignment and management", 
        [this]() { return TestTeamManagement(); }));
    
    TestSuites["PlayerManager"] = suite;
}

bool GameSystemTestSuite::TestPlayerJoinLeave() {
    auto& playerManager = PlayerManager::Get();
    
    // Create test player
    AFortPlayerControllerAthena* testPlayer = CreateTestPlayer("TestPlayer");
    if (!Assert(testPlayer != nullptr, "Failed to create test player")) {
        return false;
    }
    
    // Test joining
    playerManager.OnPlayerJoin(testPlayer);
    auto allPlayers = playerManager.GetAllPlayers();
    
    if (!Assert(std::find(allPlayers.begin(), allPlayers.end(), testPlayer) != allPlayers.end(), 
                "Player not found after joining")) {
        return false;
    }
    
    // Test leaving
    playerManager.OnPlayerLeave(testPlayer);
    
    // Cleanup
    CleanupTestPlayer(testPlayer);
    
    return true;
}

bool GameSystemTestSuite::TestPlayerSpawning() {
    auto& playerManager = PlayerManager::Get();
    
    AFortPlayerControllerAthena* testPlayer = CreateTestPlayer("SpawnTestPlayer");
    if (!testPlayer) return false;
    
    // Add to player manager
    playerManager.OnPlayerJoin(testPlayer);
    
    // Test spawning
    FSpawnInfo spawnInfo;
    spawnInfo.Location = FVector(0, 0, 1000);
    
    bool spawnResult = playerManager.SpawnPlayer(testPlayer, spawnInfo);
    if (!Assert(spawnResult, "Failed to spawn player")) {
        CleanupTestPlayer(testPlayer);
        return false;
    }
    
    // Check if player is alive
    bool isAlive = playerManager.IsPlayerAlive(testPlayer);
    Assert(isAlive, "Player should be alive after spawning");
    
    CleanupTestPlayer(testPlayer);
    return isAlive;
}

void GameSystemTestSuite::PrintSummary() const {
    std::cout << "\n=== TEST SUITE SUMMARY ===\n";
    
    size_t totalTests = 0;
    size_t totalPassed = 0;
    size_t totalFailed = 0;
    
    for (const auto& [suiteName, suite] : TestSuites) {
        size_t passed = suite.GetPassedCount();
        size_t failed = suite.GetFailedCount();
        
        totalTests += suite.TestCases.size();
        totalPassed += passed;
        totalFailed += failed;
        
        std::cout << suiteName << ": " << passed << "/" << suite.TestCases.size() 
                  << " passed";
        
        if (failed > 0) {
            std::cout << " (" << failed << " FAILED)";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nOVERALL: " << totalPassed << "/" << totalTests << " tests passed";
    
    if (totalFailed > 0) {
        std::cout << " (" << totalFailed << " FAILED)";
    }
    
    std::cout << "\nSUCCESS RATE: " 
              << (totalTests > 0 ? (totalPassed * 100 / totalTests) : 0) 
              << "%\n\n";
}

void GameSystemTestSuite::GenerateReport(const std::string& FilePath) {
    std::string filename = FilePath.empty() ? "test_report.html" : FilePath;
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        LOG_ERROR("Failed to create test report file: " + filename);
        return;
    }
    
    // Generate HTML report
    file << "<!DOCTYPE html>\n<html>\n<head>\n";
    file << "<title>Game System Test Report</title>\n";
    file << "<style>\n";
    file << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    file << ".passed { color: green; }\n";
    file << ".failed { color: red; }\n";
    file << ".error { color: orange; }\n";
    file << "table { border-collapse: collapse; width: 100%; }\n";
    file << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    file << "th { background-color: #f2f2f2; }\n";
    file << "</style>\n</head>\n<body>\n";
    
    file << "<h1>Game System Test Report</h1>\n";
    file << "<p>Generated on: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "</p>\n";
    
    for (const auto& [suiteName, suite] : TestSuites) {
        file << "<h2>" << suiteName << "</h2>\n";
        file << "<p>" << suite.Description << "</p>\n";
        file << "<table>\n";
        file << "<tr><th>Test Name</th><th>Result</th><th>Time (ms)</th><th>Error</th></tr>\n";
        
        for (const auto& test : suite.TestCases) {
            file << "<tr>\n";
            file << "<td>" << test.Name << "</td>\n";
            
            std::string resultClass;
            std::string resultText;
            
            switch (test.Result) {
                case ETestResult::Passed:
                    resultClass = "passed";
                    resultText = "PASSED";
                    break;
                case ETestResult::Failed:
                    resultClass = "failed";
                    resultText = "FAILED";
                    break;
                case ETestResult::Error:
                    resultClass = "error";
                    resultText = "ERROR";
                    break;
                default:
                    resultClass = "";
                    resultText = "NOT RUN";
                    break;
            }
            
            file << "<td class=\"" << resultClass << "\">" << resultText << "</td>\n";
            file << "<td>" << test.ExecutionTime.count() << "</td>\n";
            file << "<td>" << test.ErrorMessage << "</td>\n";
            file << "</tr>\n";
        }
        
        file << "</table>\n";
    }
    
    file << "</body>\n</html>\n";
    file.close();
    
    LOG_INFO("Test report generated: " + filename);
}

// Helper implementations
AFortPlayerControllerAthena* GameSystemTestSuite::CreateTestPlayer(const std::string& Name) {
    // In a real implementation, you'd create a proper player controller
    // For testing purposes, we'll create a mock or simplified version
    
    // This is a placeholder - in the real implementation you'd:
    // 1. Create a proper AFortPlayerControllerAthena instance
    // 2. Set up necessary properties
    // 3. Register with appropriate systems
    
    return nullptr; // Placeholder
}

void GameSystemTestSuite::CleanupTestPlayer(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    // Cleanup player from all systems
    auto& playerManager = PlayerManager::Get();
    playerManager.OnPlayerLeave(Player);
    
    // Additional cleanup...
}

bool GameSystemTestSuite::Assert(bool Condition, const std::string& Message) {
    if (!Condition && Settings.bVerboseOutput) {
        LOG_ERROR("ASSERTION FAILED: " + Message);
    }
    return Condition;
}