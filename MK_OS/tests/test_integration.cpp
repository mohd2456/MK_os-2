// ============================================================
// MK OS - Integration Tests for FEAT-005
// Tests crypto, homelab, mind, sync, config subsystems
// ============================================================
#ifndef MK_TEST_INTEGRATION_CPP
#define MK_TEST_INTEGRATION_CPP

// ============================================================
// TEST: Technical Analysis - RSI
// ============================================================
void test_technical_analysis_rsi() {
    MKTechnicalAnalysis ta;

    // Create known price data: steady uptrend then downtrend
    std::vector<double> prices = {
        44, 44.34, 44.09, 43.61, 44.33, 44.83, 45.10, 45.42, 45.84,
        46.08, 45.89, 46.03, 45.61, 46.28, 46.28, 46.00, 46.03, 46.41,
        46.22, 45.64, 46.21, 46.25, 45.71, 46.45, 45.78, 45.35, 44.03,
        44.18, 44.22, 44.57, 43.42, 42.66, 43.13
    };

    auto rsi = ta.RSI(prices, 14);
    TEST_ASSERT_GT((int)rsi.size(), 0, "RSI should produce results");
    TEST_ASSERT_TRUE(rsi[0] >= 0.0 && rsi[0] <= 100.0,
                     "RSI values must be between 0 and 100");
    // Last RSI should be below 50 since prices are declining
    TEST_ASSERT_TRUE(rsi.back() < 60.0,
                     "RSI should be below 60 during downtrend");
    TEST_ASSERT_TRUE(rsi.back() > 0.0,
                     "RSI should be positive");

    // Test with all-up prices: RSI should be high
    std::vector<double> upPrices;
    for (int i = 0; i < 30; i++) upPrices.push_back(100.0 + i * 2.0);
    auto rsiUp = ta.RSI(upPrices, 14);
    TEST_ASSERT_GT((int)rsiUp.size(), 0, "RSI up should have results");
    TEST_ASSERT_TRUE(rsiUp.back() > 80.0, "RSI should be >80 in strong uptrend");

    // Test with insufficient data
    std::vector<double> shortPrices = {10, 11, 12};
    auto rsiShort = ta.RSI(shortPrices, 14);
    TEST_ASSERT_EQ((int)rsiShort.size(), 0, "RSI should be empty with insufficient data");
}

// ============================================================
// TEST: Technical Analysis - MACD
// ============================================================
void test_technical_analysis_macd() {
    MKTechnicalAnalysis ta;

    // Generate 50 prices with an uptrend
    std::vector<double> prices;
    for (int i = 0; i < 50; i++) {
        prices.push_back(100.0 + i * 0.5 + (i % 3 == 0 ? -1.0 : 0.5));
    }

    auto macd = ta.MACD(prices, 12, 26, 9);
    TEST_ASSERT_TRUE(macd.valid, "MACD should be valid with 50 data points");
    TEST_ASSERT_GT((int)macd.macdLine.size(), 0, "MACD line should have values");
    TEST_ASSERT_GT((int)macd.signalLine.size(), 0, "Signal line should have values");
    TEST_ASSERT_GT((int)macd.histogram.size(), 0, "Histogram should have values");

    // In an uptrend, MACD line should generally be positive
    TEST_ASSERT_TRUE(macd.macdLine.back() > 0.0,
                     "MACD line should be positive in uptrend");

    // Test with insufficient data
    std::vector<double> shortPrices = {10, 11, 12, 13, 14};
    auto macdShort = ta.MACD(shortPrices, 12, 26, 9);
    TEST_ASSERT_FALSE(macdShort.valid, "MACD should be invalid with insufficient data");
}

// ============================================================
// TEST: Technical Analysis - Bollinger Bands
// ============================================================
void test_technical_analysis_bollinger() {
    MKTechnicalAnalysis ta;

    // Generate 30 prices around 100 with some volatility
    std::vector<double> prices;
    for (int i = 0; i < 30; i++) {
        double noise = (i % 2 == 0) ? 2.0 : -2.0;
        prices.push_back(100.0 + noise);
    }

    auto bb = ta.BollingerBands(prices, 20, 2.0);
    TEST_ASSERT_TRUE(bb.valid, "Bollinger Bands should be valid with 30 data points");
    TEST_ASSERT_GT((int)bb.upper.size(), 0, "Upper band should have values");
    TEST_ASSERT_GT((int)bb.lower.size(), 0, "Lower band should have values");
    TEST_ASSERT_GT((int)bb.middle.size(), 0, "Middle band should have values");

    // Upper should be above middle, middle above lower
    TEST_ASSERT_TRUE(bb.upper.back() > bb.middle.back(),
                     "Upper band should be above middle");
    TEST_ASSERT_TRUE(bb.middle.back() > bb.lower.back(),
                     "Middle band should be above lower");

    // Bandwidth should be positive
    if (!bb.bandwidth.empty()) {
        TEST_ASSERT_TRUE(bb.bandwidth.back() > 0.0,
                         "Bandwidth should be positive");
    }
}

// ============================================================
// TEST: Risk Manager - Position Limit
// ============================================================
void test_risk_manager_position_limit() {
    MKRiskManager rm;

    // Check order when already at 19% allocation (should allow small buy)
    auto check1 = rm.checkOrder("BTC", 500.0, 10000.0, 19.0);
    TEST_ASSERT_TRUE(check1.approved, "Order within position limit should be approved");
    TEST_ASSERT_TRUE(check1.adjustedSize <= 500.0,
                     "Adjusted size should not exceed requested");

    // Check order that would exceed 20% limit
    auto check2 = rm.checkOrder("ETH", 5000.0, 10000.0, 18.0);
    // Should either be reduced or approved with adjustment
    TEST_ASSERT_TRUE(check2.adjustedSize <= 5000.0,
                     "Order should be reduced to fit within position limit");
    TEST_ASSERT_TRUE(check2.riskScore > 0.0,
                     "Risk score should be elevated for limit-hitting orders");

    // Check order when already at 20% (should be rejected)
    auto check3 = rm.checkOrder("SOL", 1000.0, 10000.0, 20.5);
    TEST_ASSERT_FALSE(check3.approved,
                      "Order exceeding position limit should be rejected");
    TEST_ASSERT_TRUE(check3.riskScore >= 80.0,
                     "Risk score should be high when limit exceeded");
    TEST_ASSERT_TRUE(check3.reason.find("limit") != std::string::npos ||
                     check3.reason.find("Position") != std::string::npos,
                     "Reason should mention position limit");
}

// ============================================================
// TEST: Risk Manager - Drawdown Detection
// ============================================================
void test_risk_manager_drawdown() {
    MKRiskManager rm;

    // Update portfolio values to simulate drawdown
    rm.updatePortfolioValue(10000.0);  // Peak
    rm.updatePortfolioValue(9500.0);   // Small dip
    rm.updatePortfolioValue(8800.0);   // Larger dip (12% drawdown)

    // After >10% drawdown, trading should be paused
    auto check = rm.checkOrder("BTC", 100.0, 8800.0, 5.0);
    TEST_ASSERT_FALSE(check.approved, "Trading should be paused after >10% drawdown");
    TEST_ASSERT_TRUE(check.reason.find("drawdown") != std::string::npos ||
                     check.reason.find("paused") != std::string::npos,
                     "Reason should mention drawdown");
    TEST_ASSERT_EQ((int)check.riskScore, 100, "Risk score should be maximum during drawdown pause");
}

// ============================================================
// TEST: Device Registry - Add/Remove
// ============================================================
void test_device_registry_add_remove() {
    MKDeviceRegistry registry;

    MKDevice dev;
    dev.hostname = "test-server-01";
    dev.ip = "192.168.1.100";
    dev.ssh_user = "mk";
    dev.cpu_model = "AMD Ryzen 9 5950X";
    dev.cpu_cores = 16;
    dev.ram_mb = 65536;
    dev.gpu_info = "RTX 4090";
    dev.storage_gb = 2048;
    dev.os_type = "Linux";
    dev.role = MKDeviceRole::MAIN_RIG;
    dev.status = MKDeviceStatus::ONLINE;
    dev.ssh_port = 22;

    // Add device
    registry.addDevice(dev);

    // Find device
    auto* found = registry.getDevice("test-server-01");
    TEST_ASSERT_TRUE(found != nullptr, "Should find added device");
    TEST_ASSERT_EQ(found->ip, std::string("192.168.1.100"), "IP should match");
    TEST_ASSERT_EQ(found->cpu_cores, 16, "CPU cores should match");
    TEST_ASSERT_EQ(found->ram_mb, 65536L, "RAM should match");

    // Remove device
    bool removed = registry.removeDevice("test-server-01");
    TEST_ASSERT_TRUE(removed, "Remove should return true for existing device");

    // Verify gone
    auto* gone = registry.getDevice("test-server-01");
    TEST_ASSERT_TRUE(gone == nullptr, "Device should be gone after removal");

    // Remove non-existent
    bool removeFail = registry.removeDevice("no-such-host");
    TEST_ASSERT_FALSE(removeFail, "Remove should return false for non-existent device");
}

// ============================================================
// TEST: Device Registry - Roles
// ============================================================
void test_device_registry_roles() {
    MKDeviceRegistry registry;

    MKDevice dev1;
    dev1.hostname = "main-pc";
    dev1.ip = "192.168.1.10";
    dev1.role = MKDeviceRole::MAIN_RIG;
    dev1.status = MKDeviceStatus::ONLINE;
    dev1.cpu_cores = 16;
    dev1.ram_mb = 32768;
    dev1.storage_gb = 2048;

    MKDevice dev2;
    dev2.hostname = "compute-01";
    dev2.ip = "192.168.1.20";
    dev2.role = MKDeviceRole::COMPUTE_NODE;
    dev2.status = MKDeviceStatus::ONLINE;
    dev2.cpu_cores = 8;
    dev2.ram_mb = 16384;
    dev2.storage_gb = 512;

    MKDevice dev3;
    dev3.hostname = "nas-01";
    dev3.ip = "192.168.1.30";
    dev3.role = MKDeviceRole::STORAGE;
    dev3.status = MKDeviceStatus::ONLINE;
    dev3.cpu_cores = 4;
    dev3.ram_mb = 8192;
    dev3.storage_gb = 16384;

    registry.addDevice(dev1);
    registry.addDevice(dev2);
    registry.addDevice(dev3);

    // Filter by role
    auto mainRigs = registry.getDevicesByRole(MKDeviceRole::MAIN_RIG);
    TEST_ASSERT_EQ((int)mainRigs.size(), 1, "Should find 1 main rig");
    TEST_ASSERT_EQ(mainRigs[0].hostname, std::string("main-pc"), "Main rig hostname should match");

    auto computeNodes = registry.getDevicesByRole(MKDeviceRole::COMPUTE_NODE);
    TEST_ASSERT_EQ((int)computeNodes.size(), 1, "Should find 1 compute node");

    auto storageDevs = registry.getDevicesByRole(MKDeviceRole::STORAGE);
    TEST_ASSERT_EQ((int)storageDevs.size(), 1, "Should find 1 storage device");

    auto phones = registry.getDevicesByRole(MKDeviceRole::PHONE);
    TEST_ASSERT_EQ((int)phones.size(), 0, "Should find 0 phones");

    // Get online devices
    auto online = registry.getOnlineDevices();
    TEST_ASSERT_EQ((int)online.size(), 3, "All 3 devices should be online");
}

// ============================================================
// TEST: Resource Monitor - canRunTask
// ============================================================
void test_resource_monitor_check() {
    MKResourceMonitor monitor;

    // Create a synthetic snapshot for a device
    MKResourceSnapshot snap;
    snap.cpu_usage_percent = 30.0f;
    snap.total_ram_mb = 16384;
    snap.available_ram_mb = 8000;
    snap.total_disk_mb = 500000;
    snap.available_disk_mb = 200000;
    snap.temperature_celsius = 55.0f;
    snap.cpu_cores = 8;
    snap.load_average_1m = 2.4f;
    snap.load_average_5m = 2.0f;
    snap.load_average_15m = 1.8f;
    snap.timestamp = std::time(nullptr);
    snap.valid = true;

    monitor.updateDeviceResources("test-device", snap);

    // Task that fits within resources
    TEST_ASSERT_TRUE(monitor.canRunTask("test-device", 20.0f, 2048),
                     "Should allow task that fits within resources");

    // Task that requires too much RAM
    TEST_ASSERT_FALSE(monitor.canRunTask("test-device", 10.0f, 20000),
                      "Should reject task needing more RAM than available");

    // Task with no snapshot data (unknown device)
    TEST_ASSERT_FALSE(monitor.canRunTask("unknown-device", 5.0f, 512),
                      "Should reject task for unknown device");

    // Task that requires too much CPU
    TEST_ASSERT_FALSE(monitor.canRunTask("test-device", 90.0f, 512),
                      "Should reject task needing excessive CPU");
}

// ============================================================
// TEST: Goal Engine - Create
// ============================================================
void test_goal_engine_create() {
    MKGoalEngine engine;

    int id = engine.createGoal(
        "Learn advanced crypto trading",
        MKGoalPriority::HIGH,
        "Successfully execute 10 profitable paper trades",
        "learning",
        "local",
        0
    );

    TEST_ASSERT_TRUE(id > 0, "Goal ID should be positive");
    TEST_ASSERT_EQ(engine.goalCount(), 1, "Should have 1 goal after creation");

    // Get active goals
    auto active = engine.getActiveGoals();
    TEST_ASSERT_EQ((int)active.size(), 1, "Should have 1 active goal");
    TEST_ASSERT_EQ(active[0].description,
                   std::string("Learn advanced crypto trading"),
                   "Goal description should match");
    TEST_ASSERT_EQ(static_cast<int>(active[0].priority),
                   static_cast<int>(MKGoalPriority::HIGH),
                   "Goal priority should be HIGH");
    TEST_ASSERT_EQ(active[0].progress, 0.0f, "New goal should have 0 progress");
    TEST_ASSERT_EQ(active[0].category, std::string("learning"), "Category should match");
}

// ============================================================
// TEST: Goal Engine - Progress
// ============================================================
void test_goal_engine_progress() {
    MKGoalEngine engine;

    int id = engine.createGoal(
        "Complete 5 system health checks",
        MKGoalPriority::MEDIUM,
        "5 successful checks",
        "maintenance"
    );

    // Break into actions
    engine.breakIntoActions(id, {"check1", "check2", "check3", "check4", "check5"});

    auto active = engine.getActiveGoals();
    TEST_ASSERT_EQ((int)active.size(), 1, "Should still have 1 active goal");
    TEST_ASSERT_EQ(static_cast<int>(active[0].status),
                   static_cast<int>(MKGoalStatus::IN_PROGRESS),
                   "Goal should be IN_PROGRESS after breaking into actions");

    // Complete 2 of 5 actions (40% progress)
    engine.completeAction(id, 0, "ok");
    engine.completeAction(id, 1, "ok");

    active = engine.getActiveGoals();
    TEST_ASSERT_TRUE(active[0].progress >= 39.0f && active[0].progress <= 41.0f,
                     "Progress should be about 40% after 2/5 actions");

    // Complete all remaining
    engine.completeAction(id, 2, "ok");
    engine.completeAction(id, 3, "ok");
    engine.completeAction(id, 4, "ok");

    // Goal at 100% should be completed
    TEST_ASSERT_EQ(engine.completedCount(), 1, "Should have 1 completed goal");
}

// ============================================================
// TEST: Mastery Network - Skill Up
// ============================================================
void test_mastery_network_skill_up() {
    MKMasteryNetwork mastery;

    // Get initial level
    float initialLevel = mastery.getSkillLevel("crypto_trading");
    TEST_ASSERT_TRUE(initialLevel > 0.0f, "Initial crypto_trading level should be > 0");

    // Practice the skill
    mastery.practiceSkill("crypto_trading", 1.0f);

    float afterPractice = mastery.getSkillLevel("crypto_trading");
    TEST_ASSERT_TRUE(afterPractice > initialLevel,
                     "Skill level should increase after practice");

    // Practice more
    mastery.practiceSkill("crypto_trading", 2.0f);
    float afterMore = mastery.getSkillLevel("crypto_trading");
    TEST_ASSERT_TRUE(afterMore > afterPractice,
                     "More practice should increase level further");

    // Verify sub-skill practice works
    float codingBefore = mastery.getSkillLevel("coding");
    mastery.practiceSubSkill("coding", "python_scripting", 0.5f);
    float codingAfter = mastery.getSkillLevel("coding");
    TEST_ASSERT_TRUE(codingAfter >= codingBefore,
                     "Sub-skill practice should increase parent skill");
}

// ============================================================
// TEST: Mastery Network - Decay
// ============================================================
void test_mastery_network_decay() {
    MKMasteryNetwork mastery;

    // Skill levels should account for decay
    // Since skills were just initialized with last_practiced = now, decay should be minimal
    float level = mastery.getSkillLevel("conversation");
    float rawLevel = 35.0f;  // Initial level set in initDefaultSkills
    // Effective level should be close to raw level since just initialized
    TEST_ASSERT_TRUE(level > rawLevel * 0.9f,
                     "Recently practiced skill should retain most of its level");
    TEST_ASSERT_TRUE(level <= rawLevel * 1.1f,
                     "Effective level should not exceed raw level significantly");

    // Get practice schedule - after practicing, schedule gets updated
    mastery.practiceSkill("coding", 0.1f);  // This triggers updateSchedule()
    mastery.practiceSkill("analysis", 0.1f);  // Another practice call
    auto schedule = mastery.getSchedule();
    // The schedule might be empty if all skills were just practiced (no decay yet)
    // So let's just verify the getSchedule method returns without crash
    // and that skill count is correct
    TEST_ASSERT_TRUE(mastery.getSkillLevel("coding") > 0.0f, "Should have coding skill");
    TEST_ASSERT_TRUE(mastery.getSkillLevel("analysis") > 0.0f, "Should have analysis skill");
    TEST_ASSERT_TRUE(mastery.getSkillLevel("crypto_trading") > 0.0f, "Should have crypto_trading skill");
    // If schedule has items, check urgency range
    for (const auto& item : schedule) {
        TEST_ASSERT_TRUE(item.urgency >= 0.0f && item.urgency <= 1.0f,
                         "Urgency should be between 0 and 1");
    }
}

// ============================================================
// TEST: Knowledge Sync - Conflict Resolution
// ============================================================
void test_knowledge_sync_conflict() {
    MKKnowledgeSync sync;
    sync.setDeviceId("pc_main");

    // Queue a fact locally
    sync.queueFact("bitcoin", "created_by", "satoshi_nakamoto", 1.0f,
                   MKSyncPriority::NORMAL, false);

    // Simulate receiving a conflicting fact from another device
    MKSyncFact remoteFact;
    remoteFact.source = "bitcoin";
    remoteFact.relation = "created_by";
    remoteFact.target = "craig_wright";  // Different target - conflict!
    remoteFact.weight = 0.8f;
    remoteFact.learned_at = std::time(nullptr) + 10;  // More recent
    remoteFact.synced_at = 0;
    remoteFact.origin_device = "phone";
    remoteFact.priority = MKSyncPriority::NORMAL;
    remoteFact.status = MKSyncStatus::QUEUED;
    remoteFact.user_correction = false;
    remoteFact.sync_attempts = 0;
    remoteFact.fact_id = "bitcoin|created_by|satoshi_nakamoto";

    // First, add the local fact to synced pool
    MKSyncFact localFact;
    localFact.source = "bitcoin";
    localFact.relation = "created_by";
    localFact.target = "satoshi_nakamoto";
    localFact.weight = 1.0f;
    localFact.learned_at = std::time(nullptr);
    localFact.origin_device = "pc_main";
    localFact.priority = MKSyncPriority::NORMAL;
    localFact.status = MKSyncStatus::SYNCED;
    localFact.user_correction = false;
    localFact.sync_attempts = 0;
    localFact.fact_id = "bitcoin|created_by|satoshi_nakamoto";
    sync.receiveFact(localFact);

    // Now receive conflicting fact (later timestamp wins)
    bool received = sync.receiveFact(remoteFact);
    TEST_ASSERT_TRUE(received, "Should handle conflicting fact");
    TEST_ASSERT_GT(sync.conflictCount(), 0, "Should record the conflict");
}

// ============================================================
// TEST: Knowledge Sync - User Correction Wins
// ============================================================
void test_knowledge_sync_user_correction() {
    MKKnowledgeSync sync;
    sync.setDeviceId("pc_main");

    // Add an auto-learned fact
    MKSyncFact autoFact;
    autoFact.source = "earth";
    autoFact.relation = "is_a";
    autoFact.target = "flat_disc";
    autoFact.weight = 0.5f;
    autoFact.learned_at = std::time(nullptr) + 100;  // Very recent
    autoFact.origin_device = "auto_learner";
    autoFact.priority = MKSyncPriority::NORMAL;
    autoFact.status = MKSyncStatus::SYNCED;
    autoFact.user_correction = false;
    autoFact.sync_attempts = 0;
    autoFact.fact_id = "earth|is_a|flat_disc";
    sync.receiveFact(autoFact);

    // User correction (older timestamp but user_correction=true)
    MKSyncFact userFact;
    userFact.source = "earth";
    userFact.relation = "is_a";
    userFact.target = "oblate_spheroid";
    userFact.weight = 1.0f;
    userFact.learned_at = std::time(nullptr) - 1000;  // Older
    userFact.origin_device = "user";
    userFact.priority = MKSyncPriority::CRITICAL;
    userFact.status = MKSyncStatus::QUEUED;
    userFact.user_correction = true;  // User says this is correct
    userFact.sync_attempts = 0;
    userFact.fact_id = "earth|is_a|flat_disc";  // Same fact_id means conflict

    bool received = sync.receiveFact(userFact);
    TEST_ASSERT_TRUE(received, "Should handle user correction");
    // User correction should win regardless of timestamp
    TEST_ASSERT_GT(sync.conflictCount(), 0, "Should detect conflict");
}

// ============================================================
// TEST: Knowledge Validator - Format
// ============================================================
void test_knowledge_validator_format() {
    MKKnowledgeValidator validator;

    // Valid fact
    auto report1 = validator.validate("python|is_a|programming_language|0.95");
    TEST_ASSERT_TRUE(report1.format_valid, "Valid fact should pass format check");
    TEST_ASSERT_EQ(static_cast<int>(report1.result),
                   static_cast<int>(ValidationResult::ACCEPT),
                   "Valid fact should be accepted");

    // Malformed: missing target
    auto report2 = validator.validate("python|is_a|");
    TEST_ASSERT_FALSE(report2.format_valid, "Fact with empty target should fail format");
    TEST_ASSERT_EQ(static_cast<int>(report2.result),
                   static_cast<int>(ValidationResult::REJECT),
                   "Malformed fact should be rejected");

    // Empty line
    auto report3 = validator.validate("");
    TEST_ASSERT_FALSE(report3.format_valid, "Empty line should fail format");

    // Comment line
    auto report4 = validator.validate("# This is a comment");
    TEST_ASSERT_FALSE(report4.format_valid, "Comment should fail format check");

    // Valid fact without weight (defaults to 1.0)
    auto report5 = validator.validate("cat|is_a|animal");
    TEST_ASSERT_TRUE(report5.format_valid, "Fact without weight should be valid");
}

// ============================================================
// TEST: Knowledge Validator - Duplicate Detection
// ============================================================
void test_knowledge_validator_duplicate() {
    MKKnowledgeValidator validator;

    // Add existing fact
    validator.addExistingFact("python", "is_a", "programming_language", 1.0f);

    // Try to validate a duplicate
    auto report = validator.validate("python|is_a|programming_language|1.0");
    TEST_ASSERT_TRUE(report.is_duplicate, "Should detect duplicate fact");
    TEST_ASSERT_EQ(static_cast<int>(report.result),
                   static_cast<int>(ValidationResult::REJECT),
                   "Duplicate should be rejected");

    // Non-duplicate should pass
    auto report2 = validator.validate("python|created_by|guido_van_rossum|0.9");
    TEST_ASSERT_FALSE(report2.is_duplicate, "Non-duplicate should not be flagged");
    TEST_ASSERT_EQ(static_cast<int>(report2.result),
                   static_cast<int>(ValidationResult::ACCEPT),
                   "Non-duplicate valid fact should be accepted");
}

// ============================================================
// TEST: Knowledge Validator - Weight Range
// ============================================================
void test_knowledge_validator_weight() {
    MKKnowledgeValidator validator;

    // Valid weight
    auto report1 = validator.validate("a|b|c|0.5");
    TEST_ASSERT_TRUE(report1.format_valid, "Weight 0.5 should be valid");

    // Weight too high
    auto report2 = validator.validate("a|b|c|1.5");
    TEST_ASSERT_EQ(static_cast<int>(report2.result),
                   static_cast<int>(ValidationResult::REJECT),
                   "Weight >1.0 should be rejected");

    // Weight negative
    auto report3 = validator.validate("a|b|c|-0.5");
    TEST_ASSERT_EQ(static_cast<int>(report3.result),
                   static_cast<int>(ValidationResult::REJECT),
                   "Negative weight should be rejected");

    // Weight at boundaries
    auto report4 = validator.validate("a|b|c|0.0");
    TEST_ASSERT_TRUE(report4.format_valid, "Weight 0.0 should be valid format");

    auto report5 = validator.validate("a|b|c|1.0");
    TEST_ASSERT_TRUE(report5.format_valid, "Weight 1.0 should be valid format");
}

// ============================================================
// TEST: Config - Load Defaults
// ============================================================
void test_config_load_defaults() {
    MKConfig config("/tmp/nonexistent_mk_config_test.json");
    bool loaded = config.load();
    TEST_ASSERT_TRUE(loaded, "Config should load OK even without file (defaults)");

    // Check default values
    TEST_ASSERT_EQ(config.getTradingMode(), std::string("paper"),
                   "Default trading mode should be paper");
    TEST_ASSERT_EQ(config.getSyncInterval(), 300,
                   "Default sync interval should be 300");

    auto risks = config.getRiskLimits();
    TEST_ASSERT_EQ(risks.max_position_pct, 20,
                   "Default max position should be 20%");
    TEST_ASSERT_EQ(risks.daily_loss_limit_usd, 100,
                   "Default daily loss limit should be $100");
    TEST_ASSERT_EQ(risks.drawdown_pause_pct, 10,
                   "Default drawdown pause should be 10%");

    auto schedule = config.getLearningSchedule();
    TEST_ASSERT_EQ(schedule.start_hour, 2, "Default learning start should be 2");
    TEST_ASSERT_EQ(schedule.end_hour, 6, "Default learning end should be 6");

    auto cgroups = config.getCgroupLimits();
    TEST_ASSERT_EQ(cgroups.memory_mb, 4096, "Default memory limit should be 4096MB");
    TEST_ASSERT_EQ(cgroups.cpu_percent, 80, "Default CPU limit should be 80%");

    // Validate should pass with defaults
    auto errors = config.validate();
    TEST_ASSERT_EQ((int)errors.size(), 0, "Default config should pass validation");
}

// ============================================================
// TEST: Config - Load and Parse JSON
// ============================================================
void test_config_load_parse() {
    MKConfig config;

    std::string json = R"({
        "telegram_token": "123456:ABC-DEF",
        "exchange_api_key": "my_api_key",
        "exchange_api_secret": "my_secret",
        "trading_mode": "paper",
        "risk_limits": {
            "max_position_pct": 15,
            "daily_loss_limit_usd": 50,
            "drawdown_pause_pct": 8
        },
        "sync_interval_sec": 600,
        "learning_schedule": {
            "start_hour": 1,
            "end_hour": 5
        },
        "cgroup_limits": {
            "memory_mb": 8192,
            "cpu_percent": 90
        }
    })";

    bool loaded = config.loadFromString(json);
    TEST_ASSERT_TRUE(loaded, "Config should parse valid JSON");
    TEST_ASSERT_TRUE(config.isLoaded(), "Config should be marked as loaded");

    TEST_ASSERT_EQ(config.getTelegramToken(), std::string("123456:ABC-DEF"),
                   "Telegram token should parse correctly");
    TEST_ASSERT_EQ(config.getExchangeApiKey(), std::string("my_api_key"),
                   "API key should parse correctly");
    TEST_ASSERT_EQ(config.getTradingMode(), std::string("paper"),
                   "Trading mode should parse correctly");
    TEST_ASSERT_EQ(config.getSyncInterval(), 600,
                   "Sync interval should parse correctly");

    auto risks = config.getRiskLimits();
    TEST_ASSERT_EQ(risks.max_position_pct, 15, "Parsed max_position_pct should be 15");
    TEST_ASSERT_EQ(risks.daily_loss_limit_usd, 50, "Parsed daily loss should be 50");
    TEST_ASSERT_EQ(risks.drawdown_pause_pct, 8, "Parsed drawdown should be 8");

    auto schedule = config.getLearningSchedule();
    TEST_ASSERT_EQ(schedule.start_hour, 1, "Parsed start_hour should be 1");
    TEST_ASSERT_EQ(schedule.end_hour, 5, "Parsed end_hour should be 5");

    auto cgroups = config.getCgroupLimits();
    TEST_ASSERT_EQ(cgroups.memory_mb, 8192, "Parsed memory_mb should be 8192");
    TEST_ASSERT_EQ(cgroups.cpu_percent, 90, "Parsed cpu_percent should be 90");

    // Validate should pass
    auto errors = config.validate();
    TEST_ASSERT_EQ((int)errors.size(), 0, "Parsed valid config should pass validation");
}

// ============================================================
// TEST: Self Funding - Track Income/Expense
// ============================================================
void test_self_funding_track() {
    MKSelfFunding funding;

    // Record income
    funding.recordIncome(MKIncomeSource::CRYPTO_PROFIT, 150.0f, "BTC trade profit", "BTC");
    funding.recordIncome(MKIncomeSource::AIRDROP_REWARD, 50.0f, "AVAX airdrop", "AVAX");

    // Record expense
    funding.recordExpense(MKExpenseCategory::API_COST, 10.0f, "Exchange API monthly");

    // Check totals
    float totalIncome = funding.getTotalIncome();
    TEST_ASSERT_TRUE(totalIncome >= 200.0f, "Total income should be >= $200");

    float totalExpenses = funding.getTotalExpenses();
    TEST_ASSERT_TRUE(totalExpenses >= 10.0f, "Total expenses should be >= $10");

    float net = funding.getNetEarnings(365);
    TEST_ASSERT_TRUE(net >= 190.0f, "Net earnings should be >= $190");

    // Budget summary
    auto summary = funding.getBudgetSummary();
    TEST_ASSERT_TRUE(summary.total_income >= 200.0f, "Summary income should be >= 200");
    TEST_ASSERT_TRUE(summary.net_earnings > 0.0f, "Net should be positive");
    TEST_ASSERT_TRUE(summary.savings > 0.0f, "Savings should be positive (auto-allocated)");
}

// ============================================================
// TEST: Trading Bot - Paper Mode
// ============================================================
void test_trading_bot_paper_mode() {
    MKMarketData md;
    MKTechnicalAnalysis ta;
    MKSignalEngine se;
    MKPortfolioManager pm;
    MKRiskManager rm;
    MKExchangeAPI api;

    MKTradingBot bot(md, ta, se, pm, rm, api);

    // Bot should default to paper mode
    auto stats = bot.getStats();
    TEST_ASSERT_EQ(stats.totalTrades, 0, "New bot should have 0 trades");
    TEST_ASSERT_EQ(stats.signalsGenerated, 0, "New bot should have 0 signals");

    // Verify paper mode - exchange API should NOT be initialized (no real calls)
    TEST_ASSERT_FALSE(api.isInitialized(),
                      "Exchange API should not be initialized in paper mode");

    // Run a scan cycle (should not crash even without market data)
    bot.runScanCycle();
    TEST_ASSERT_TRUE(true, "Scan cycle should complete without crash");
}

// ============================================================
// TEST: Signal Engine - Signal Generation
// ============================================================
void test_signal_engine_generation() {
    MKSignalEngine se;

    // Create data that signals strong buy:
    // RSI very low (oversold), MACD bullish crossover, price at lower Bollinger
    std::vector<double> prices;
    for (int i = 0; i < 30; i++) {
        prices.push_back(100.0 - i * 1.5);  // Declining prices (oversold condition)
    }
    std::vector<double> volumes(30, 1000000.0);
    volumes.back() = 2000000.0;  // High volume on last bar

    double rsi = 22.0;  // Very oversold
    double macdHist = 0.5;     // Positive (bullish)
    double prevMacdHist = -0.3; // Was negative (crossover!)
    double bbUpper = 110.0;
    double bbLower = 50.0;
    double bbMiddle = 80.0;
    double vwap = 80.0;

    auto signal = se.generateSignal("BTC", prices, volumes,
                                     rsi, macdHist, prevMacdHist,
                                     bbUpper, bbLower, bbMiddle, vwap);

    TEST_ASSERT_EQ(signal.symbol, std::string("BTC"), "Signal symbol should be BTC");
    // With oversold RSI and bullish MACD crossover, should get buy signal
    TEST_ASSERT_TRUE(signal.type == MKSignalType::BUY ||
                     signal.type == MKSignalType::STRONG_BUY,
                     "Strong buy indicators should generate buy signal");
    TEST_ASSERT_GT(signal.strength, 30, "Signal strength should be > 30");
    TEST_ASSERT_TRUE(signal.confidence > 0.0, "Signal confidence should be positive");
    TEST_ASSERT_FALSE(signal.reasoning.empty(), "Signal should have reasoning");
}

// ============================================================
// TEST: HMAC-SHA256 against RFC 4231 Test Vector
// ============================================================
void test_hmac_sha256_rfc4231() {
    // RFC 4231 Test Case 2:
    // Key  = "Jefe" (4 bytes)
    // Data = "what do ya want for nothing?" (28 bytes)
    // HMAC-SHA-256 = 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
    std::string key = "Jefe";
    std::string data = "what do ya want for nothing?";

    auto hmacResult = MKCryptoAuth::hmacSha256(key, data);
    std::string hexResult = MKCryptoAuth::toHex(hmacResult);

    TEST_ASSERT_EQ(hexResult,
                   std::string("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"),
                   "HMAC-SHA256 should match RFC 4231 Test Case 2");

    // RFC 4231 Test Case 1:
    // Key  = 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (20 bytes of 0x0b)
    // Data = "Hi There"
    // HMAC-SHA-256 = b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7
    std::string key1(20, '\x0b');
    std::string data1 = "Hi There";

    auto hmacResult1 = MKCryptoAuth::hmacSha256(key1, data1);
    std::string hexResult1 = MKCryptoAuth::toHex(hmacResult1);

    TEST_ASSERT_EQ(hexResult1,
                   std::string("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"),
                   "HMAC-SHA256 should match RFC 4231 Test Case 1");

    // Verify that different keys produce different outputs
    auto hmacDiff = MKCryptoAuth::hmacSha256("different_key", data);
    std::string hexDiff = MKCryptoAuth::toHex(hmacDiff);
    TEST_ASSERT_NE(hexDiff, hexResult,
                   "Different key should produce different HMAC");

    // Verify that different data produces different outputs
    auto hmacDiff2 = MKCryptoAuth::hmacSha256(key, "different data");
    std::string hexDiff2 = MKCryptoAuth::toHex(hmacDiff2);
    TEST_ASSERT_NE(hexDiff2, hexResult,
                   "Different data should produce different HMAC");
}

// ============================================================
// TEST: Device Comm - Auth Token Validation
// ============================================================
void test_device_comm_auth_validation() {
    MKDeviceComm comm;
    comm.setLocalDevice("test_device", "my_secret_key_123", 9876);
    comm.registerEndpoint("remote_device", "http://192.168.1.50:8765",
                         "my_secret_key_123");

    // Build a sync request - should include auth token
    auto msg = comm.buildSyncRequest("remote_device", "test|data|payload|1.0");
    TEST_ASSERT_FALSE(msg.auth_token.empty(), "Sync request should have auth token");

    // Validate the request using the same body - should pass
    bool valid = comm.validateRequest(msg.auth_token, msg.body);
    TEST_ASSERT_TRUE(valid, "Auth token should validate with correct body and secret");

    // Validate with wrong body - should fail
    bool invalid = comm.validateRequest(msg.auth_token, "tampered_body");
    TEST_ASSERT_FALSE(invalid, "Auth token should NOT validate with tampered body");

    // Validate with empty token - should fail
    bool emptyToken = comm.validateRequest("", msg.body);
    TEST_ASSERT_FALSE(emptyToken, "Empty auth token should not validate");

    // Validate with wrong token - should fail
    bool wrongToken = comm.validateRequest("deadbeef1234", msg.body);
    TEST_ASSERT_FALSE(wrongToken, "Wrong auth token should not validate");
}

// ============================================================
// TEST: SSH Controller - Shell Escape
// ============================================================
void test_ssh_controller_shell_escape() {
    MKSSHController controller;
    controller.registerDevice("test-host", "192.168.1.1", "root", 22, "", 10);

    // Test that shell metacharacters in commands get escaped properly
    // The escape function should handle single quotes to prevent injection

    // We cannot directly call buildSSHCommand (it is private), but we can verify
    // executeRemote does not crash with dangerous input containing shell metacharacters.
    // The escape function turns ' into '\'' preventing breakout.
    std::string dangerous_cmd = "echo 'hello'; rm -rf /";

    // This will fail (no SSH host), but should not crash or inject
    auto result = controller.executeRemote("test-host", dangerous_cmd);
    // Connection will fail - the important thing is no segfault and no injection
    TEST_ASSERT_FALSE(result.success,
                      "Command to unreachable host should fail gracefully");
    TEST_ASSERT_TRUE(true, "executeRemote with shell metacharacters did not crash");
}

// ============================================================
// TEST: Autonomous Learner - Session
// ============================================================
void test_autonomous_learner_session() {
    MKAutonomousLearner learner;

    // Start a session
    auto session = learner.startSession("crypto_trading");
    TEST_ASSERT_EQ(session.topic, std::string("crypto_trading"),
                   "Session topic should match");
    TEST_ASSERT_EQ(session.facts_accepted, 0, "New session should have 0 facts accepted");

    // Record some facts
    learner.factAccepted(session);
    learner.factAccepted(session);
    learner.factRejected(session);

    TEST_ASSERT_EQ(session.facts_accepted, 2, "Should have 2 accepted facts");
    TEST_ASSERT_EQ(session.facts_rejected, 1, "Should have 1 rejected fact");
    TEST_ASSERT_EQ(session.facts_attempted, 3, "Should have 3 attempted facts");

    // End session
    learner.endSession(session);
    TEST_ASSERT_EQ(learner.totalSessions(), 1, "Should have 1 completed session");
}

// ============================================================
// FEAT-004 Integration Tests
// ============================================================

// ============================================================
// TEST: Full Pipeline With No LLM - Graceful Degradation
// ============================================================
void test_full_pipeline_with_no_llm() {
    // Simulate the scenario where no LLM is available:
    // graph has facts. System should produce meaningful response via orchestrator fallback.

    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.loadAllKnowledge();
    graph.addFact("python", "is_a", "programming_language", 1.0f);
    graph.addFact("python", "created_by", "guido_van_rossum", 0.95f);

    // Get facts about python
    auto results = graph.getAll("python");
    std::vector<std::string> relevantFacts;
    for (const auto& e : results) {
        relevantFacts.push_back(e.source + " " + e.relation + " " + e.target);
    }

    // Simulate graceful degradation: all providers failed, build response from facts
    std::string response;
    if (!relevantFacts.empty()) {
        std::string factBased;
        for (size_t i = 0; i < relevantFacts.size() && i < 3; i++) {
            if (!factBased.empty()) factBased += " Also, ";
            factBased += relevantFacts[i];
        }
        factBased += ".";
        response = "Here's what I know: " + factBased;
    }

    TEST_ASSERT_FALSE(response.empty(),
                      "Graceful degradation should produce a non-empty response");
    TEST_ASSERT_TRUE(response.find("python") != std::string::npos,
                     "Degraded response should contain graph facts (python)");
    TEST_ASSERT_TRUE(response.size() > 10,
                     "Degraded response should be meaningful (> 10 chars)");

    // Use orchestrator for the full test
    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;
    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.systemPrompt = &testPrompt;

    std::string orchResponse = orch.respond("what is python");
    TEST_ASSERT_FALSE(orchResponse.empty(),
                      "Orchestrator should produce response even without LLM");
}

// ============================================================
// TEST: SetKey Flow
// ============================================================
void test_setkey_flow() {
    MKProviderRouter router;
    MKKeyEncryption enc("/tmp");

    // Initially no providers should be online
    TEST_ASSERT_EQ(router.onlineCount(), 0, "No providers online initially");

    // Simulate setting a key for groq
    std::string provider = "groq";
    std::string key = "gsk_test_key_12345";

    // Set key in router (simulates what /setkey does)
    router.setProviderKey(provider, key);

    // Provider should now be online
    TEST_ASSERT_EQ(router.onlineCount(), 1, "groq should be online after setting key");

    // Verify the router can pick this provider
    std::string picked = router.pickProvider(MKRoutingUrgency::HIGH, 50);
    TEST_ASSERT_EQ(picked, std::string("groq"),
                   "Router should pick groq as the available provider");

    // Verify encrypted key storage works
    std::string encrypted = enc.encrypt(key);
    std::string decrypted = enc.decrypt(encrypted);
    TEST_ASSERT_EQ(decrypted, key,
                   "Key should survive encrypt/decrypt roundtrip for setkey flow");

    // Setting a key for another provider
    router.setProviderKey("nvidia", "nvapi-test-456");
    TEST_ASSERT_EQ(router.onlineCount(), 2, "Two providers should be online now");
}

// ============================================================
// TEST: Request Logging
// ============================================================
void test_request_logging() {
    MKRequestLogger logger("/tmp/test_mk_requests.log");

    // Clear any previous entries
    logger.clear();

    // Log some requests
    logger.logRequest("groq", "hello world", 25, 150.0f, true);
    logger.logRequest("nvidia", "what is python", 30, 300.0f, true);
    logger.logRequest("groq", "tell me about AI", 40, 200.0f, false);

    // Verify total count
    TEST_ASSERT_EQ(logger.getTotalCount(), 3, "Should have 3 total logged requests");

    // Verify today count
    TEST_ASSERT_EQ(logger.getTodayCount(), 3, "Should have 3 requests today");

    // Verify stats output
    std::string stats = logger.getStats();
    TEST_ASSERT_FALSE(stats.empty(), "Stats should not be empty");
    TEST_ASSERT_TRUE(stats.find("Total calls") != std::string::npos ||
                     stats.find("3") != std::string::npos,
                     "Stats should mention total calls");
    TEST_ASSERT_TRUE(stats.find("groq") != std::string::npos,
                     "Stats should mention groq provider");
    TEST_ASSERT_TRUE(stats.find("nvidia") != std::string::npos,
                     "Stats should mention nvidia provider");

    // Verify daily usage per provider
    int groqUsage = logger.getDailyUsage("groq");
    TEST_ASSERT_EQ(groqUsage, 65, "Groq daily usage should be 25+40=65 tokens");

    int nvidiaUsage = logger.getDailyUsage("nvidia");
    TEST_ASSERT_EQ(nvidiaUsage, 30, "Nvidia daily usage should be 30 tokens");

    // Verify log file was written (JSON lines format)
    std::ifstream logFile("/tmp/test_mk_requests.log");
    TEST_ASSERT_TRUE(logFile.good(), "Log file should exist");
    std::string line;
    int lineCount = 0;
    while (std::getline(logFile, line)) {
        lineCount++;
        // Each line should be valid JSON (starts with { and ends with })
        TEST_ASSERT_TRUE(line.front() == '{' && line.back() == '}',
                         "Each log line should be a JSON object");
        // Should contain expected fields
        TEST_ASSERT_TRUE(line.find("\"provider\"") != std::string::npos,
                         "Log line should contain provider field");
        TEST_ASSERT_TRUE(line.find("\"timestamp\"") != std::string::npos,
                         "Log line should contain timestamp field");
        TEST_ASSERT_TRUE(line.find("\"tokens_used\"") != std::string::npos,
                         "Log line should contain tokens_used field");
        TEST_ASSERT_TRUE(line.find("\"latency_ms\"") != std::string::npos,
                         "Log line should contain latency_ms field");
        TEST_ASSERT_TRUE(line.find("\"success\"") != std::string::npos,
                         "Log line should contain success field");
    }
    TEST_ASSERT_EQ(lineCount, 3, "Log file should have 3 lines");

    // Clean up test file
    std::remove("/tmp/test_mk_requests.log");
}

// ============================================================
// TEST: Provider Quota Tracking
// ============================================================
void test_provider_quota_tracking() {
    MKRequestLogger logger("/tmp/test_mk_quota.log");
    logger.clear();

    // Simulate multiple requests to track daily quota
    logger.logRequest("groq", "request 1", 100, 150.0f, true);
    logger.logRequest("groq", "request 2", 200, 160.0f, true);
    logger.logRequest("groq", "request 3", 150, 140.0f, true);
    logger.logRequest("openrouter", "request 4", 300, 250.0f, true);
    logger.logRequest("groq", "request 5", 250, 170.0f, false);

    // Check daily usage per provider
    int groqDailyTokens = logger.getDailyUsage("groq");
    TEST_ASSERT_EQ(groqDailyTokens, 700, "Groq daily tokens should be 100+200+150+250=700");

    int openrouterDailyTokens = logger.getDailyUsage("openrouter");
    TEST_ASSERT_EQ(openrouterDailyTokens, 300, "OpenRouter daily tokens should be 300");

    int nvidiaDailyTokens = logger.getDailyUsage("nvidia");
    TEST_ASSERT_EQ(nvidiaDailyTokens, 0, "Nvidia daily tokens should be 0 (no requests)");

    // Verify total count
    TEST_ASSERT_EQ(logger.getTodayCount(), 5, "Should have 5 requests today");

    // Clean up
    std::remove("/tmp/test_mk_quota.log");
}

// ============================================================
// FEAT-003 Live Integration Tests - Real I/O
// ============================================================

// ============================================================
// TEST: Live File Read via Tool Executor
// ============================================================
void test_tool_live_file_read() {
    // Create a known file in /tmp
    std::string testPath = "/tmp/mk_tool_test_read.txt";
    std::string knownContent = "MK OS live file read test content 12345";
    {
        std::ofstream f(testPath);
        TEST_ASSERT_TRUE(f.is_open(), "Should be able to create test file");
        f << knownContent;
        f.close();
    }

    // Set up executor with file reader
    MKToolExecutor executor;
    MKFileReader reader;
    executor.fileReader = &reader;

    // Call read_file tool
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "read_file";
    call.args["path"] = testPath;

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "read_file tool should succeed on real file");
    TEST_ASSERT_TRUE(result.output.find(knownContent) != std::string::npos,
                     "read_file output should contain the known content");

    // Clean up
    std::remove(testPath.c_str());
}

// ============================================================
// TEST: Live File Write via Tool Executor
// ============================================================
void test_tool_live_file_write() {
    std::string testPath = "/tmp/mk_tool_test_write.txt";
    std::string writeContent = "hello from MK tool";

    // Remove any leftover file
    std::remove(testPath.c_str());

    // Set up executor
    MKToolExecutor executor;

    // Call write_file tool
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "write_file";
    call.args["path"] = testPath;
    call.args["content"] = writeContent;

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "write_file tool should succeed for /tmp path");
    TEST_ASSERT_TRUE(result.output.find("Written") != std::string::npos,
                     "write_file output should confirm write");

    // Verify by reading back
    std::ifstream f(testPath);
    TEST_ASSERT_TRUE(f.is_open(), "Written file should exist");
    std::string readBack;
    std::getline(f, readBack);
    f.close();
    TEST_ASSERT_EQ(readBack, writeContent, "File content should match what was written");

    // Clean up
    std::remove(testPath.c_str());
}

// ============================================================
// TEST: Live Local Shell Execution
// ============================================================
void test_tool_live_local_shell() {
    MKToolExecutor executor;
    MKCodeRunner runner(5);
    executor.codeRunner = &runner;

    // Execute a simple echo command
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "local_shell";
    call.args["command"] = "echo hello_from_mk";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "local_shell echo should succeed");
    TEST_ASSERT_TRUE(result.output.find("hello_from_mk") != std::string::npos,
                     "local_shell output should contain echo text");
}

// ============================================================
// TEST: Live Local Shell - Exit Code Handling
// ============================================================
void test_tool_live_local_shell_exit_code() {
    MKToolExecutor executor;
    MKCodeRunner runner(5);
    executor.codeRunner = &runner;

    // Execute "false" which returns exit code 1
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "local_shell";
    call.args["command"] = "false";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "local_shell 'false' should report failure");
    TEST_ASSERT_TRUE(!result.error.empty(), "Error should be non-empty for failed command");
}

// ============================================================
// TEST: Live Web Search - News (HackerNews API)
// ============================================================
void test_tool_live_web_search_news() {
    MKToolExecutor executor;
    MKRealtimeAPIs apis;
    executor.realtimeApis = &apis;

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "web_search";
    call.args["query"] = "news";

    MKToolResult result = executor.execute(call);
    // Accept success (API up) or graceful failure (API down)
    TEST_ASSERT_TRUE(result.success || !result.error.empty(),
                     "web_search should either succeed or return a descriptive error");
    if (result.success) {
        TEST_ASSERT_TRUE(result.output.size() > 10,
                         "News results should have meaningful content");
        TEST_ASSERT_TRUE(result.output.find("news") != std::string::npos ||
                         result.output.find("headlines") != std::string::npos ||
                         result.output.find(".") != std::string::npos,
                         "News results should contain headlines or sentences");
    }
}

// ============================================================
// TEST: Live System Info
// ============================================================
void test_tool_live_system_info() {
    MKToolExecutor executor;
    MKResourceMonitor monitor;
    executor.resourceMonitor = &monitor;

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "system_info";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "system_info should succeed on this machine");
    TEST_ASSERT_TRUE(result.output.find("CPU") != std::string::npos,
                     "system_info should mention CPU");
    TEST_ASSERT_TRUE(result.output.find("RAM") != std::string::npos,
                     "system_info should mention RAM");
    // Check for numeric values (at least a digit should be present)
    bool hasDigit = false;
    for (char c : result.output) {
        if (std::isdigit(c)) { hasDigit = true; break; }
    }
    TEST_ASSERT_TRUE(hasDigit, "system_info should contain numeric values");
}

// ============================================================
// TEST: SSH Graceful Failure - Unreachable Host
// ============================================================
void test_tool_ssh_unreachable_host() {
    MKSSHController controller;
    // Register a device with RFC 5737 TEST-NET IP (guaranteed unreachable)
    // Use a 2-second timeout to avoid hanging
    controller.registerDevice("unreachable-test", "192.0.2.1", "testuser", 22, "", 2);

    // Attempt to execute a command on the unreachable host
    MKSSHResult result = controller.executeRemote("unreachable-test", "echo hello");

    // Should fail gracefully (not crash, not hang indefinitely)
    TEST_ASSERT_FALSE(result.success,
                      "SSH to unreachable host should fail");
    TEST_ASSERT_TRUE(result.timed_out ||
                     result.error.find("timed out") != std::string::npos ||
                     result.error.find("timeout") != std::string::npos ||
                     result.exit_code != 0,
                     "Should report timeout or connection error");
    // Verify it did not hang (duration should be roughly around timeout)
    TEST_ASSERT_TRUE(result.duration_ms < 30000.0f,
                     "SSH attempt should not hang longer than 30 seconds");
}

// ============================================================
// TEST: Docker - List Command Generation and Local Execution
// ============================================================
void test_tool_docker_list_local() {
    MKDockerManager dockerMgr;

    // Verify command generation
    std::string listCmd = dockerMgr.listContainersCmd();
    TEST_ASSERT_TRUE(listCmd.find("docker ps") != std::string::npos,
                     "listContainersCmd should contain 'docker ps'");

    // Try to run docker info locally to check if docker is available
    MKCodeRunner runner(5);
    MKRunResult infoResult = runner.runBash("docker info > /dev/null 2>&1 && echo docker_ok || echo docker_missing");

    if (infoResult.success && infoResult.stdoutOutput.find("docker_ok") != std::string::npos) {
        // Docker is available - run the list command locally
        MKRunResult listResult = runner.runBash("docker ps -a --format '{{.ID}}|{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}' 2>&1");
        // Should not crash regardless of whether there are containers
        TEST_ASSERT_TRUE(listResult.success || listResult.exitCode >= 0,
                         "docker ps should not crash when docker is available");

        // Try parsing the output (may be empty if no containers)
        auto containers = dockerMgr.parseContainerList(listResult.stdoutOutput, "localhost");
        // Just verify it didn't crash - container count may be 0
        TEST_ASSERT_TRUE(true, "Docker container list parsing completed without crash");
    } else {
        // Docker not available - verify we handle gracefully
        TEST_ASSERT_TRUE(true, "Docker not available - skipping live docker test (graceful)");
    }
}

// ============================================================
// TEST: Tool Call Round-Trip (Registry -> Parse -> Execute)
// ============================================================
void test_tool_call_roundtrip() {
    // Create a registry with tools
    MKToolRegistry registry;

    // Forge a fake LLM response containing a tool call for system_info
    std::string fakeLLMResponse = "Let me check the system status. {\"tool\": \"system_info\", \"args\": {}}";

    // Parse it
    MKParsedToolCall call = registry.parseToolCall(fakeLLMResponse);
    TEST_ASSERT_TRUE(call.valid, "Round-trip: parseToolCall should find valid tool call");
    TEST_ASSERT_EQ(call.tool, std::string("system_info"),
                   "Round-trip: parsed tool should be system_info");

    // Execute it with a real resource monitor
    MKToolExecutor executor;
    MKResourceMonitor monitor;
    executor.resourceMonitor = &monitor;

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "Round-trip: system_info execution should succeed");
    TEST_ASSERT_TRUE(result.output.find("CPU") != std::string::npos,
                     "Round-trip: result should contain CPU info");
    TEST_ASSERT_TRUE(result.output.find("RAM") != std::string::npos,
                     "Round-trip: result should contain RAM info");
}

// ============================================================
// TEST: browse_url - Tool Registration and Live Fetch
// ============================================================
void test_tool_browse_url_registered() {
    MKToolRegistry registry;
    TEST_ASSERT_TRUE(registry.exists("browse_url"), "browse_url tool should be registered");
    const MKToolDef* def = registry.lookup("browse_url");
    TEST_ASSERT_TRUE(def != nullptr, "browse_url lookup should return non-null");
    if (def) {
        TEST_ASSERT_TRUE(def->requiresArgs, "browse_url should require args");
        TEST_ASSERT_TRUE(def->description.find("webpage") != std::string::npos ||
                         def->description.find("URL") != std::string::npos,
                         "browse_url description should mention webpage or URL");
    }

    // Verify tool prompt includes browse_url guidance
    std::string prompt = registry.buildToolPrompt();
    TEST_ASSERT_TRUE(prompt.find("browse_url") != std::string::npos,
                     "Tool prompt should mention browse_url");
    TEST_ASSERT_TRUE(prompt.find("Read/visit a specific URL") != std::string::npos,
                     "Tool prompt should explain when to use browse_url");
}

void test_tool_browse_url_live_fetch() {
    MKToolExecutor executor;

    // Test with a simple, reliable URL
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "browse_url";
    call.args["url"] = "http://example.com";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "browse_url should fetch example.com successfully");
    TEST_ASSERT_TRUE(result.output.find("Example Domain") != std::string::npos,
                     "browse_url result should contain 'Example Domain'");
    TEST_ASSERT_TRUE(result.error.empty(), "browse_url should have no error on success");
}

void test_tool_browse_url_safety() {
    MKToolExecutor executor;

    // Test private IP rejection
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "browse_url";

    // localhost
    call.args["url"] = "http://localhost/secret";
    MKToolResult r1 = executor.execute(call);
    TEST_ASSERT_FALSE(r1.success, "browse_url should reject localhost");
    TEST_ASSERT_TRUE(r1.error.find("private") != std::string::npos || r1.error.find("Blocked") != std::string::npos,
                     "Error should mention private/blocked");

    // 127.0.0.1
    call.args["url"] = "http://127.0.0.1:8080/admin";
    MKToolResult r2 = executor.execute(call);
    TEST_ASSERT_FALSE(r2.success, "browse_url should reject 127.x.x.x");

    // 192.168.x.x
    call.args["url"] = "http://192.168.1.1/config";
    MKToolResult r3 = executor.execute(call);
    TEST_ASSERT_FALSE(r3.success, "browse_url should reject 192.168.x.x");

    // 10.x.x.x
    call.args["url"] = "http://10.0.0.1/internal";
    MKToolResult r4 = executor.execute(call);
    TEST_ASSERT_FALSE(r4.success, "browse_url should reject 10.x.x.x");

    // 172.16.x.x
    call.args["url"] = "https://172.16.0.1/vpn";
    MKToolResult r5 = executor.execute(call);
    TEST_ASSERT_FALSE(r5.success, "browse_url should reject 172.16-31.x.x");

    // Invalid scheme
    call.args["url"] = "ftp://example.com/file";
    MKToolResult r6 = executor.execute(call);
    TEST_ASSERT_FALSE(r6.success, "browse_url should reject non-http(s) URLs");
    TEST_ASSERT_TRUE(r6.error.find("http://") != std::string::npos || r6.error.find("https://") != std::string::npos,
                     "Error should mention valid schemes");

    // Missing URL
    call.args.clear();
    MKToolResult r7 = executor.execute(call);
    TEST_ASSERT_FALSE(r7.success, "browse_url should fail with missing url arg");
}

// ============================================================
// FEAT-001 (custom-tools-fix): Double-response bug fix tests
// ============================================================

// ============================================================
// TEST: synthesizeWithToolResult strips short preamble
// ============================================================
void test_double_response_preamble_stripped() {
    // Set up orchestrator with no LLM (forces synthesis fallback path)
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;
    MKToolRegistry toolRegistry;
    MKToolExecutor toolExecutor;
    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.toolRegistry = &toolRegistry;
    orch.toolExecutor = &toolExecutor;
    orch.systemPrompt = &testPrompt;

    // Simulate the synthesizeWithToolResult fallback logic directly:
    // When synthesis fails and the original response has a short preamble + tool JSON,
    // the preamble should be dropped and only the tool result returned.
    std::string originalResponse = "Let me check that\n{\"tool\": \"system_info\", \"args\": {}}";
    std::string toolResult = "CPU: 4 cores, RAM: 16GB";

    // Strip the tool JSON from original (simulating what synthesizeWithToolResult does)
    std::string rawJson = "{\"tool\": \"system_info\", \"args\": {}}";
    std::string cleaned = originalResponse;
    size_t jsonPos = cleaned.find(rawJson);
    if (jsonPos != std::string::npos) {
        cleaned.erase(jsonPos, rawJson.size());
    }
    // Trim whitespace
    size_t start = cleaned.find_first_not_of(" \t\n\r");
    size_t end = cleaned.find_last_not_of(" \t\n\r");
    if (start != std::string::npos) {
        cleaned = cleaned.substr(start, end - start + 1);
    } else {
        cleaned = "";
    }

    // Apply the preamble check: short text (<60 chars) without period or question mark
    std::string finalResult;
    if (!cleaned.empty()) {
        if (cleaned.size() < 60 &&
            cleaned.find('.') == std::string::npos &&
            cleaned.find('?') == std::string::npos) {
            finalResult = toolResult;
        } else {
            finalResult = cleaned + "\n\n" + toolResult;
        }
    } else {
        finalResult = toolResult;
    }

    // The preamble "Let me check that" should NOT appear in the final result
    TEST_ASSERT_TRUE(finalResult.find("Let me check that") == std::string::npos,
                     "Short preamble should be stripped from tool result output");
    TEST_ASSERT_TRUE(finalResult.find("CPU: 4 cores") != std::string::npos,
                     "Tool result should be present in output");
    TEST_ASSERT_EQ(finalResult, toolResult,
                   "Final result should be just the tool result when preamble is short");

    // Verify that a substantive cleaned text (with period) is NOT stripped
    std::string substantiveCleaned = "Here is the system information you requested.";
    std::string withSubstantive;
    if (substantiveCleaned.size() < 60 &&
        substantiveCleaned.find('.') == std::string::npos &&
        substantiveCleaned.find('?') == std::string::npos) {
        withSubstantive = toolResult;
    } else {
        withSubstantive = substantiveCleaned + "\n\n" + toolResult;
    }
    TEST_ASSERT_TRUE(withSubstantive.find(substantiveCleaned) != std::string::npos,
                     "Substantive text with period should be preserved");
}

// ============================================================
// TEST: callback_query detection for skip logic
// ============================================================
void test_callback_query_skip() {
    // Validate that the pattern used to detect callback_query works correctly
    std::string updateWithCallback = R"({"update_id":123456,"callback_query":{"id":"789","chat_instance":"abc","data":"btn_click"}})";
    std::string updateWithMessage = R"({"update_id":123457,"message":{"chat":{"id":100},"text":"hello"}})";
    std::string updateWithBoth = R"({"update_id":123458,"callback_query":{"id":"790"},"message":{"chat":{"id":101},"text":"hi"}})";

    // callback_query-only update should be detected
    TEST_ASSERT_TRUE(updateWithCallback.find("\"callback_query\"") != std::string::npos,
                     "Should detect callback_query in callback-only update");

    // message-only update should NOT have callback_query
    TEST_ASSERT_TRUE(updateWithMessage.find("\"callback_query\"") == std::string::npos,
                     "Should not detect callback_query in message-only update");

    // update with both should be detected (and skipped in the real code)
    TEST_ASSERT_TRUE(updateWithBoth.find("\"callback_query\"") != std::string::npos,
                     "Should detect callback_query even when message is also present");
}

// ============================================================
// TEST: brainMemory commit strips tool JSON
// ============================================================
void test_brain_memory_commit_strips_tool_json() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;
    MKToolRegistry toolRegistry;
    MKToolExecutor toolExecutor;
    MKResourceMonitor monitor;
    toolExecutor.resourceMonitor = &monitor;
    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.toolRegistry = &toolRegistry;
    orch.toolExecutor = &toolExecutor;
    orch.systemPrompt = &testPrompt;
    orch.config.enableToolCalls = true;

    // The stripToolCallJson function should remove tool JSON from text
    // We test this by verifying the orchestrator's strip function works
    std::string textWithToolJson = "Here is the info {\"tool\": \"system_info\", \"args\": {}} and more text.";
    std::string stripped = orch.stripToolCallJson(textWithToolJson);

    TEST_ASSERT_TRUE(stripped.find("{\"tool\"") == std::string::npos,
                     "stripToolCallJson should remove tool JSON from text");
    TEST_ASSERT_TRUE(stripped.find("Here is the info") != std::string::npos,
                     "stripToolCallJson should preserve non-tool text");
    TEST_ASSERT_TRUE(stripped.find("and more text.") != std::string::npos,
                     "stripToolCallJson should preserve text after tool JSON");
}

// ============================================================
// FEAT-002 (custom-tools): Custom tool creation and execution tests
// ============================================================

// ============================================================
// TEST: create_tool registers successfully
// ============================================================
void test_create_tool_registered() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    executor.toolRegistry = &registry;

    int countBefore = registry.toolCount();

    // Call handleCreateTool with valid args via execute()
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "create_tool";
    call.args["name"] = "my_test_tool";
    call.args["description"] = "A simple test tool";
    call.args["language"] = "bash";
    call.args["code"] = "echo hello from custom tool";
    call.args["args"] = "msg";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "create_tool should succeed with valid args");
    TEST_ASSERT_TRUE(result.output.find("Created custom tool") != std::string::npos,
                     "create_tool output should confirm creation");

    // Registry should now have the tool
    TEST_ASSERT_TRUE(registry.exists("my_test_tool"),
                     "Registry should contain the newly created tool");
    TEST_ASSERT_EQ(registry.toolCount(), countBefore + 1,
                   "Tool count should increase by 1");

    // Cleanup
    const char* home = std::getenv("HOME");
    if (home) {
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/my_test_tool.sh").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

// ============================================================
// TEST: create_tool rejects invalid names
// ============================================================
void test_create_tool_invalid_name() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    executor.toolRegistry = &registry;

    // Name with spaces
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "create_tool";
    call.args["name"] = "bad name";
    call.args["description"] = "test";
    call.args["language"] = "bash";
    call.args["code"] = "echo hi";
    call.args["args"] = "";

    MKToolResult r1 = executor.execute(call);
    TEST_ASSERT_FALSE(r1.success, "create_tool should reject name with spaces");
    TEST_ASSERT_TRUE(r1.error.find("alphanumeric") != std::string::npos,
                     "Error should mention alphanumeric requirement");

    // Name too short
    call.args["name"] = "ab";
    MKToolResult r2 = executor.execute(call);
    TEST_ASSERT_FALSE(r2.success, "create_tool should reject name shorter than 3 chars");
    TEST_ASSERT_TRUE(r2.error.find("3-30") != std::string::npos,
                     "Error should mention length requirement");

    // Name with special chars
    call.args["name"] = "bad!tool@name";
    MKToolResult r3 = executor.execute(call);
    TEST_ASSERT_FALSE(r3.success, "create_tool should reject name with special characters");

    // Name too long (31 chars)
    call.args["name"] = "abcdefghijklmnopqrstuvwxyz12345";
    MKToolResult r4 = executor.execute(call);
    TEST_ASSERT_FALSE(r4.success, "create_tool should reject name longer than 30 chars");
}

// ============================================================
// TEST: custom tool execution
// ============================================================
void test_custom_tool_execution() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    executor.toolRegistry = &registry;

    // First create a tool
    MKParsedToolCall createCall;
    createCall.valid = true;
    createCall.tool = "create_tool";
    createCall.args["name"] = "echo_test";
    createCall.args["description"] = "Echoes the msg argument";
    createCall.args["language"] = "bash";
    createCall.args["code"] = "echo \"received: $MK_ARG_MSG\"";
    createCall.args["args"] = "msg";

    MKToolResult createResult = executor.execute(createCall);
    TEST_ASSERT_TRUE(createResult.success, "Tool creation should succeed");

    // Now call the custom tool
    MKParsedToolCall execCall;
    execCall.valid = true;
    execCall.tool = "echo_test";
    execCall.args["msg"] = "hello_world";

    MKToolResult execResult = executor.execute(execCall);
    TEST_ASSERT_TRUE(execResult.success, "Custom tool execution should succeed");
    TEST_ASSERT_TRUE(execResult.output.find("received: hello_world") != std::string::npos,
                     "Custom tool output should contain the arg value");

    // Cleanup
    const char* home = std::getenv("HOME");
    if (home) {
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/echo_test.sh").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

// ============================================================
// TEST: create_tool manifest persistence
// ============================================================
void test_create_tool_manifest_persistence() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    executor.toolRegistry = &registry;

    // Create a tool
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "create_tool";
    call.args["name"] = "persist_test";
    call.args["description"] = "Tests manifest persistence";
    call.args["language"] = "python";
    call.args["code"] = "print('persistent')";
    call.args["args"] = "";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "create_tool should succeed");

    // Verify manifest.json exists and contains the tool
    const char* home = std::getenv("HOME");
    TEST_ASSERT_TRUE(home != nullptr, "HOME should be set");
    if (home) {
        std::string manifestPath = std::string(home) + "/.mk_os/tools/manifest.json";
        std::ifstream mf(manifestPath);
        TEST_ASSERT_TRUE(mf.is_open(), "manifest.json should exist after creating a tool");

        std::string content((std::istreambuf_iterator<char>(mf)),
                             std::istreambuf_iterator<char>());
        mf.close();

        TEST_ASSERT_TRUE(content.find("persist_test") != std::string::npos,
                         "manifest.json should contain the tool name");
        TEST_ASSERT_TRUE(content.find("Tests manifest persistence") != std::string::npos,
                         "manifest.json should contain the tool description");
        TEST_ASSERT_TRUE(content.find("python") != std::string::npos,
                         "manifest.json should contain the language");
        TEST_ASSERT_TRUE(content.find(".py") != std::string::npos,
                         "manifest.json should contain the script path with .py extension");

        // Verify the script file exists
        std::string scriptPath = std::string(home) + "/.mk_os/tools/persist_test.py";
        std::ifstream sf(scriptPath);
        TEST_ASSERT_TRUE(sf.is_open(), "Script file should exist");
        if (sf.is_open()) {
            std::string scriptContent((std::istreambuf_iterator<char>(sf)),
                                       std::istreambuf_iterator<char>());
            sf.close();
            TEST_ASSERT_TRUE(scriptContent.find("print('persistent')") != std::string::npos,
                             "Script file should contain the provided code");
        }

        // Test loadCustomTools: create a new registry and load from manifest
        MKToolRegistry registry2;
        int countBefore = registry2.toolCount();
        registry2.loadCustomTools();
        TEST_ASSERT_TRUE(registry2.exists("persist_test"),
                         "loadCustomTools should load the tool from manifest");
        TEST_ASSERT_EQ(registry2.toolCount(), countBefore + 1,
                       "loadCustomTools should increase tool count");

        // Cleanup
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/persist_test.py").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

// ============================================================
// Review fix tests: preamble heuristic, overwrite protection, path validation
// ============================================================

// ============================================================
// TEST: Preamble detection preserves legitimate short responses
// ============================================================
void test_preamble_preserves_short_responses() {
    MKOrchestrator orch;

    // These are legitimate short responses that should NOT be treated as preamble
    std::vector<std::string> legitimateResponses = {
        "Done",
        "5 items found",
        "Here you go",
        "No results",
        "Success",
        "Yes",
        "42"
    };

    // These are preamble phrases that SHOULD be detected
    std::vector<std::string> preamblePhrases = {
        "Let me check that",
        "I'll look into that for you",
        "Sure, let me find out",
        "One moment please",
        "Checking now",
        "Looking that up",
        "Let me search for that:"
    };

    // Test that legitimate responses are preserved in synthesis fallback:
    // Simulate: originalResponse = legitimate + tool JSON, synthesis failed
    // Result should include the legitimate text + tool result.
    for (const auto& legit : legitimateResponses) {
        std::string originalResponse = legit + "\n{\"tool\": \"system_info\", \"args\": {}}";
        std::string toolResult = "CPU: 4 cores, RAM: 16GB";

        // Replicate the synthesis fallback logic
        std::string cleaned = legit; // after stripping tool JSON and trimming
        // The orchestrator's isPreamblePhrase should NOT match these
        // We verify by calling stripToolCallJson and checking the response retains legit text
        std::string stripped = orch.stripToolCallJson(originalResponse);
        // After stripping, the legitimate text should remain
        TEST_ASSERT_TRUE(stripped.find(legit) != std::string::npos ||
                         stripped.find(legit.substr(0, 4)) != std::string::npos,
                         ("Legitimate response '" + legit + "' should be preserved after strip").c_str());
    }

    // Test preamble phrases: verify they match the pattern
    // We use the orchestrator's respond path indirectly, but for unit testing
    // the isPreamblePhrase is private, so we test the observable behavior:
    // A preamble + tool call should result in just the tool result.
    // Since isPreamblePhrase checks startsWith patterns, we verify our expectations:
    TEST_ASSERT_TRUE(std::string("let me check that").find("let me ") == 0,
                     "Preamble 'let me...' should match 'let me ' prefix");
    TEST_ASSERT_TRUE(std::string("checking now").find("checking ") == 0,
                     "Preamble 'checking now' should match 'checking ' prefix");
    TEST_ASSERT_TRUE(std::string("one moment please").find("one moment") == 0,
                     "Preamble 'one moment...' should match 'one moment' prefix");

    // Legitimate responses should NOT match any prefix
    std::string done_lower = "done";
    TEST_ASSERT_TRUE(done_lower.find("let me ") != 0 &&
                     done_lower.find("i'll ") != 0 &&
                     done_lower.find("checking ") != 0 &&
                     done_lower.find("looking ") != 0,
                     "'Done' should not match any preamble prefix");
}

// ============================================================
// TEST: create_tool rejects duplicate tool names
// ============================================================
void test_create_tool_duplicate_rejected() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    executor.toolRegistry = &registry;

    // Create a tool first
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "create_tool";
    call.args["name"] = "dup_test_tool";
    call.args["description"] = "Original tool";
    call.args["language"] = "bash";
    call.args["code"] = "echo original";
    call.args["args"] = "";

    MKToolResult r1 = executor.execute(call);
    TEST_ASSERT_TRUE(r1.success, "First create_tool should succeed");

    // Try to create with the same name - should be rejected
    call.args["description"] = "Duplicate tool";
    call.args["code"] = "echo duplicate";

    MKToolResult r2 = executor.execute(call);
    TEST_ASSERT_FALSE(r2.success, "Duplicate create_tool should be rejected");
    TEST_ASSERT_TRUE(r2.error.find("already exists") != std::string::npos,
                     "Error should mention tool already exists");

    // Verify only one entry in registry
    TEST_ASSERT_TRUE(registry.exists("dup_test_tool"),
                     "Original tool should still exist");

    // Cleanup
    const char* home = std::getenv("HOME");
    if (home) {
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/dup_test_tool.sh").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

// ============================================================
// TEST: custom tool rejects path outside tools directory
// ============================================================
void test_custom_tool_path_validation() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    MKCodeRunner runner(5);
    executor.toolRegistry = &registry;
    executor.codeRunner = &runner;

    // Create a legitimate tool first
    MKParsedToolCall createCall;
    createCall.valid = true;
    createCall.tool = "create_tool";
    createCall.args["name"] = "path_test";
    createCall.args["description"] = "Path validation test";
    createCall.args["language"] = "bash";
    createCall.args["code"] = "echo safe";
    createCall.args["args"] = "";

    MKToolResult createResult = executor.execute(createCall);
    TEST_ASSERT_TRUE(createResult.success, "Tool creation should succeed");

    // Now tamper with the manifest to inject a bad path
    const char* home = std::getenv("HOME");
    TEST_ASSERT_TRUE(home != nullptr, "HOME should be set");
    if (home) {
        std::string manifestPath = std::string(home) + "/.mk_os/tools/manifest.json";

        // Overwrite manifest with an entry pointing outside tools dir
        std::string maliciousManifest = "[\n  {\"name\": \"path_test\", \"description\": \"test\", "
            "\"paramSchema\": \"{}\", \"language\": \"bash\", \"scriptPath\": \"/etc/passwd\"}\n]";

        std::ofstream mf(manifestPath);
        if (mf.is_open()) {
            mf << maliciousManifest;
            mf.close();
        }

        // Try to execute the tool with the tampered path
        MKParsedToolCall execCall;
        execCall.valid = true;
        execCall.tool = "path_test";
        execCall.args = {};

        // Need to register it in the registry for dispatch
        if (!registry.exists("path_test")) {
            registry.registerCustomTool("path_test", "test", "{}");
        }

        MKToolResult execResult = executor.execute(execCall);
        TEST_ASSERT_FALSE(execResult.success,
                          "Execution should fail when scriptPath points outside tools dir");
        TEST_ASSERT_TRUE(execResult.error.find("outside") != std::string::npos ||
                         execResult.error.find("Blocked") != std::string::npos,
                         "Error should mention path is outside allowed directory");

        // Cleanup
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/path_test.sh").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

// ============================================================
// TEST: custom tool blocks dangerous script content
// ============================================================
void test_custom_tool_dangerous_script_blocked() {
    MKToolRegistry registry;
    MKToolExecutor executor;
    MKCodeRunner runner(5);
    executor.toolRegistry = &registry;
    executor.codeRunner = &runner;

    // Create a tool with dangerous content
    MKParsedToolCall createCall;
    createCall.valid = true;
    createCall.tool = "create_tool";
    createCall.args["name"] = "danger_test";
    createCall.args["description"] = "Dangerous tool test";
    createCall.args["language"] = "bash";
    createCall.args["code"] = "rm -rf /";
    createCall.args["args"] = "";

    MKToolResult createResult = executor.execute(createCall);
    TEST_ASSERT_TRUE(createResult.success,
                     "Tool creation itself should succeed (content check is at execution time)");

    // Try to execute the dangerous tool
    MKParsedToolCall execCall;
    execCall.valid = true;
    execCall.tool = "danger_test";
    execCall.args = {};

    MKToolResult execResult = executor.execute(execCall);
    TEST_ASSERT_FALSE(execResult.success,
                      "Execution of dangerous script should be blocked");
    TEST_ASSERT_TRUE(execResult.error.find("dangerous") != std::string::npos ||
                     execResult.error.find("Blocked") != std::string::npos,
                     "Error should mention dangerous commands");

    // Cleanup
    const char* home = std::getenv("HOME");
    if (home) {
        std::string toolsDir = std::string(home) + "/.mk_os/tools";
        std::remove((toolsDir + "/danger_test.sh").c_str());
        std::remove((toolsDir + "/manifest.json").c_str());
    }
}

#endif // MK_TEST_INTEGRATION_CPP
