#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <functional>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;
class PlayerManager;
class InventoryManager;
class BuildingManager;
class SafezoneManager;
class MatchManager;
class NetworkManager;
class EventManager;

/**
 * GameSystemTestSuite - Comprehensive testing for all game systems
 * 
 * Features:
 * - Unit tests for individual systems
 * - Integration tests for system interactions
 * - Performance benchmarks
 * - Stress testing
 * - Automated test execution
 * - Test reporting and validation
 */

enum class ETestResult : uint8_t {
    NotRun = 0,
    Passed = 1,
    Failed = 2,
    Skipped = 3,
    Error = 4
};

struct FTestCase {
    std::string Name;
    std::string Description;
    std::function<bool()> TestFunction;
    ETestResult Result = ETestResult::NotRun;
    std::string ErrorMessage;
    std::chrono::milliseconds ExecutionTime{0};
    
    FTestCase() = default;
    FTestCase(const std::string& name, const std::string& desc, std::function<bool()> func)
        : Name(name), Description(desc), TestFunction(func) {}
};

struct FTestSuite {
    std::string Name;
    std::string Description;
    std::vector<FTestCase> TestCases;
    bool bEnabled = true;
    
    FTestSuite() = default;
    FTestSuite(const std::string& name, const std::string& desc) 
        : Name(name), Description(desc) {}
    
    void AddTest(const FTestCase& TestCase) {
        TestCases.push_back(TestCase);
    }
    
    size_t GetPassedCount() const {
        return std::count_if(TestCases.begin(), TestCases.end(), 
            [](const FTestCase& test) { return test.Result == ETestResult::Passed; });
    }
    
    size_t GetFailedCount() const {
        return std::count_if(TestCases.begin(), TestCases.end(), 
            [](const FTestCase& test) { return test.Result == ETestResult::Failed; });
    }
    
    bool AllTestsPassed() const {
        return GetFailedCount() == 0 && GetPassedCount() == TestCases.size();
    }
};

struct FBenchmarkResult {
    std::string Name;
    uint32_t Iterations = 0;
    std::chrono::milliseconds TotalTime{0};
    std::chrono::microseconds AverageTime{0};
    std::chrono::microseconds MinTime{999999999};
    std::chrono::microseconds MaxTime{0};
    
    void AddSample(std::chrono::microseconds time) {
        Iterations++;
        TotalTime += std::chrono::duration_cast<std::chrono::milliseconds>(time);
        MinTime = std::min(MinTime, time);
        MaxTime = std::max(MaxTime, time);
        AverageTime = std::chrono::duration_cast<std::chrono::microseconds>(TotalTime) / Iterations;
    }
};

class GameSystemTestSuite {
public:
    static GameSystemTestSuite& Get();
    
    // Test execution
    void RunAllTests();
    void RunTestSuite(const std::string& SuiteName);
    void RunSingleTest(const std::string& SuiteName, const std::string& TestName);
    void RunBenchmarks();
    
    // Test management
    void RegisterTestSuite(const FTestSuite& Suite);
    void AddTestToSuite(const std::string& SuiteName, const FTestCase& TestCase);
    bool HasTestSuite(const std::string& SuiteName) const;
    
    // Results and reporting
    void GenerateReport(const std::string& FilePath = "");
    void PrintResults() const;
    void PrintSummary() const;
    bool AllTestsPassed() const;
    
    // Configuration
    struct Config {
        bool bRunOnStartup = false;
        bool bVerboseOutput = true;
        bool bStopOnFirstFailure = false;
        uint32_t BenchmarkIterations = 1000;
        float TestTimeout = 30.0f;
        bool bRunBenchmarks = true;
    } Settings;
    
    // Initialization
    void Initialize();
    void InitializeTestData();
    void CleanupTestData();
    
private:
    GameSystemTestSuite() = default;
    ~GameSystemTestSuite() = default;
    
    GameSystemTestSuite(const GameSystemTestSuite&) = delete;
    GameSystemTestSuite& operator=(const GameSystemTestSuite&) = delete;
    
    // Test suites
    std::unordered_map<std::string, FTestSuite> TestSuites;
    std::vector<FBenchmarkResult> BenchmarkResults;
    
    // Test data
    std::vector<AFortPlayerControllerAthena*> TestPlayers;
    std::unordered_map<std::string, UObject*> TestObjects;
    
    // Setup methods
    void SetupPlayerManagerTests();
    void SetupInventoryManagerTests();
    void SetupBuildingManagerTests();
    void SetupSafezoneManagerTests();
    void SetupMatchManagerTests();
    void SetupNetworkManagerTests();
    void SetupEventManagerTests();
    void SetupIntegrationTests();
    void SetupPerformanceTests();
    
    // PlayerManager tests
    bool TestPlayerJoinLeave();
    bool TestPlayerSpawning();
    bool TestPlayerElimination();
    bool TestTeamManagement();
    bool TestSpectating();
    bool TestPlayerStats();
    
    // InventoryManager tests
    bool TestItemGiving();
    bool TestItemRemoving();
    bool TestQuickbarManagement();
    bool TestWeaponEquipping();
    bool TestMaterialManagement();
    bool TestItemStacking();
    
    // BuildingManager tests
    bool TestStructurePlacement();
    bool TestStructureDestruction();
    bool TestBuildingValidation();
    bool TestStructuralIntegrity();
    bool TestMaterialConsumption();
    bool TestBuildingGrid();
    
    // SafezoneManager tests
    bool TestStormInitialization();
    bool TestStormPhases();
    bool TestPlayerInStorm();
    bool TestStormDamage();
    bool TestSafezoneMovement();
    bool TestStormStatistics();
    
    // MatchManager tests
    bool TestMatchStates();
    bool TestLobbySystem();
    bool TestVictoryConditions();
    bool TestMatchResults();
    bool TestGameModeSettings();
    bool TestMatchTiming();
    
    // NetworkManager tests
    bool TestPlayerConnection();
    bool TestMessageSystem();
    bool TestObjectReplication();
    bool TestRPCSystem();
    bool TestAntiCheat();
    bool TestNetworkStats();
    
    // EventManager tests
    bool TestEventCreation();
    bool TestLootSpawning();
    bool TestSupplyDrops();
    bool TestEventScheduling();
    bool TestEventParticipation();
    bool TestEventRewards();
    
    // Integration tests
    bool TestFullMatchFlow();
    bool TestPlayerInventoryBuilding();
    bool TestStormPlayerInteraction();
    bool TestNetworkReplication();
    bool TestEventPlayerInteraction();
    bool TestSystemCoordination();
    
    // Performance tests
    FBenchmarkResult BenchmarkObjectFinding();
    FBenchmarkResult BenchmarkPlayerOperations();
    FBenchmarkResult BenchmarkInventoryOperations();
    FBenchmarkResult BenchmarkBuildingOperations();
    FBenchmarkResult BenchmarkNetworkOperations();
    FBenchmarkResult BenchmarkEventProcessing();
    
    // Stress tests
    bool StressTestManyPlayers();
    bool StressTestManyStructures();
    bool StressTestManyEvents();
    bool StressTestNetworkTraffic();
    bool StressTestMemoryUsage();
    
    // Utilities
    AFortPlayerControllerAthena* CreateTestPlayer(const std::string& Name);
    void CleanupTestPlayer(AFortPlayerControllerAthena* Player);
    bool ExecuteTestWithTimeout(std::function<bool()> Test, float TimeoutSeconds = 10.0f);
    void LogTestResult(const std::string& TestName, bool bPassed, const std::string& Error = "");
    
    // Helper assertions
    bool Assert(bool Condition, const std::string& Message);
    bool AssertEqual(int32_t Expected, int32_t Actual, const std::string& Message = "");
    bool AssertEqual(float Expected, float Actual, const std::string& Message = "", float Tolerance = 0.001f);
    bool AssertNotNull(void* Pointer, const std::string& Message = "");
    bool AssertNull(void* Pointer, const std::string& Message = "");
};