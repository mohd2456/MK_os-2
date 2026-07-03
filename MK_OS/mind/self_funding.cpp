#ifndef MK_SELF_FUNDING_CPP
#define MK_SELF_FUNDING_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <fstream>

// ===================================================================================
// MK SELF-FUNDING
// Tracks all income: crypto profits, airdrop rewards, any other autonomous earnings.
// Tracks expenses (API costs, compute costs). Calculates net earnings.
// Identifies hardware upgrade targets. Generates upgrade proposals.
// ===================================================================================

enum class MKIncomeSource {
    CRYPTO_PROFIT,
    AIRDROP_REWARD,
    YIELD_FARMING,
    STAKING_REWARD,
    OTHER
};

enum class MKExpenseCategory {
    API_COST,
    COMPUTE_COST,
    STORAGE_COST,
    NETWORK_COST,
    SUBSCRIPTION,
    HARDWARE,
    OTHER
};

struct MKIncomeEntry {
    std::time_t timestamp;
    MKIncomeSource source;
    float amount_usd;
    std::string description;
    std::string related_asset;  // e.g., "ETH", "AVAX"
};

struct MKExpenseEntry {
    std::time_t timestamp;
    MKExpenseCategory category;
    float amount_usd;
    std::string description;
    bool recurring;
    int recurrence_days;  // 0 = one-time, 30 = monthly
};

struct MKUpgradeProposal {
    std::string device_name;
    std::string component;       // RAM, GPU, SSD, CPU
    std::string current_spec;
    std::string proposed_spec;
    float estimated_cost_usd;
    float expected_performance_gain;  // multiplier
    float roi_months;            // Expected months to pay for itself
    std::string justification;
    std::time_t proposed_at;
};

struct MKBudgetSummary {
    float total_income;
    float total_expenses;
    float net_earnings;
    float monthly_income_avg;
    float monthly_expense_avg;
    float savings;
    float upgrade_budget;  // Amount available for hardware upgrades
};

class MKSelfFunding {
private:
    std::vector<MKIncomeEntry> income_log;
    std::vector<MKExpenseEntry> expense_log;
    std::vector<MKUpgradeProposal> proposals;
    float savings_balance;
    float upgrade_reserve_percent;  // % of net income to save for upgrades
    std::string persistence_path;

    float calculatePeriodIncome(int days) const {
        std::time_t cutoff = std::time(nullptr) - (days * 86400);
        float total = 0.0f;
        for (const auto& entry : income_log) {
            if (entry.timestamp >= cutoff) total += entry.amount_usd;
        }
        return total;
    }

    float calculatePeriodExpenses(int days) const {
        std::time_t cutoff = std::time(nullptr) - (days * 86400);
        float total = 0.0f;
        for (const auto& entry : expense_log) {
            if (entry.timestamp >= cutoff) total += entry.amount_usd;
        }
        return total;
    }

public:
    MKSelfFunding() : savings_balance(0.0f), upgrade_reserve_percent(20.0f),
                      persistence_path("mk_funding.dat") {}

    // Record income
    void recordIncome(MKIncomeSource source, float amount_usd,
                     const std::string& description, const std::string& asset = "") {
        MKIncomeEntry entry;
        entry.timestamp = std::time(nullptr);
        entry.source = source;
        entry.amount_usd = amount_usd;
        entry.description = description;
        entry.related_asset = asset;
        income_log.push_back(entry);

        // Auto-allocate to savings/upgrade reserve
        savings_balance += amount_usd * (upgrade_reserve_percent / 100.0f);
    }

    // Record expense
    void recordExpense(MKExpenseCategory category, float amount_usd,
                      const std::string& description, bool recurring = false,
                      int recurrence_days = 0) {
        MKExpenseEntry entry;
        entry.timestamp = std::time(nullptr);
        entry.category = category;
        entry.amount_usd = amount_usd;
        entry.description = description;
        entry.recurring = recurring;
        entry.recurrence_days = recurrence_days;
        expense_log.push_back(entry);
    }

    // Calculate net earnings for a period
    float getNetEarnings(int days = 30) const {
        return calculatePeriodIncome(days) - calculatePeriodExpenses(days);
    }

    // Get total lifetime income
    float getTotalIncome() const {
        float total = 0.0f;
        for (const auto& e : income_log) total += e.amount_usd;
        return total;
    }

    // Get total lifetime expenses
    float getTotalExpenses() const {
        float total = 0.0f;
        for (const auto& e : expense_log) total += e.amount_usd;
        return total;
    }

    // Get budget summary
    MKBudgetSummary getBudgetSummary() const {
        MKBudgetSummary summary;
        summary.total_income = getTotalIncome();
        summary.total_expenses = getTotalExpenses();
        summary.net_earnings = summary.total_income - summary.total_expenses;
        summary.monthly_income_avg = calculatePeriodIncome(30);
        summary.monthly_expense_avg = calculatePeriodExpenses(30);
        summary.savings = savings_balance;
        summary.upgrade_budget = savings_balance;
        return summary;
    }

    // Generate an upgrade proposal
    void proposeUpgrade(const std::string& device, const std::string& component,
                       const std::string& current_spec, const std::string& proposed_spec,
                       float cost_usd, float performance_gain) {
        MKUpgradeProposal proposal;
        proposal.device_name = device;
        proposal.component = component;
        proposal.current_spec = current_spec;
        proposal.proposed_spec = proposed_spec;
        proposal.estimated_cost_usd = cost_usd;
        proposal.expected_performance_gain = performance_gain;
        proposal.proposed_at = std::time(nullptr);

        // Calculate ROI based on monthly earnings improvement
        float monthly_net = getNetEarnings(30);
        float improvement = monthly_net * (performance_gain - 1.0f);
        proposal.roi_months = (improvement > 0.0f) ? (cost_usd / improvement) : 999.0f;

        proposal.justification = "Upgrade " + component + " on " + device +
                                " from " + current_spec + " to " + proposed_spec +
                                ". Cost: $" + std::to_string((int)cost_usd) +
                                ", ROI: " + std::to_string((int)proposal.roi_months) + " months.";

        proposals.push_back(proposal);
    }

    // Get affordable proposals (within budget)
    std::vector<MKUpgradeProposal> getAffordableUpgrades() const {
        std::vector<MKUpgradeProposal> affordable;
        for (const auto& p : proposals) {
            if (p.estimated_cost_usd <= savings_balance) {
                affordable.push_back(p);
            }
        }
        // Sort by ROI (shortest payback first)
        std::sort(affordable.begin(), affordable.end(),
                  [](const MKUpgradeProposal& a, const MKUpgradeProposal& b) {
                      return a.roi_months < b.roi_months;
                  });
        return affordable;
    }

    // Get all proposals
    std::vector<MKUpgradeProposal> getAllProposals() const { return proposals; }

    // Get income breakdown by source
    std::map<std::string, float> getIncomeBreakdown() const {
        std::map<std::string, float> breakdown;
        for (const auto& entry : income_log) {
            std::string source_name;
            switch (entry.source) {
                case MKIncomeSource::CRYPTO_PROFIT: source_name = "crypto_profit"; break;
                case MKIncomeSource::AIRDROP_REWARD: source_name = "airdrop_reward"; break;
                case MKIncomeSource::YIELD_FARMING: source_name = "yield_farming"; break;
                case MKIncomeSource::STAKING_REWARD: source_name = "staking_reward"; break;
                case MKIncomeSource::OTHER: source_name = "other"; break;
            }
            breakdown[source_name] += entry.amount_usd;
        }
        return breakdown;
    }

    // Get savings balance
    float getSavingsBalance() const { return savings_balance; }

    // Save state
    void save() const {
        std::ofstream out(persistence_path);
        if (!out.is_open()) return;
        out << savings_balance << "\n";
        out << income_log.size() << "\n";
        for (const auto& e : income_log) {
            out << e.timestamp << "|" << (int)e.source << "|"
                << e.amount_usd << "|" << e.description << "\n";
        }
        out.close();
    }

    // Load state
    void load() {
        std::ifstream in(persistence_path);
        if (!in.is_open()) return;
        std::string line;
        if (std::getline(in, line)) {
            try { savings_balance = std::stof(line); } catch (...) {}
        }
        in.close();
    }

    // Get summary
    std::string getSummary() const {
        auto summary = getBudgetSummary();
        std::ostringstream ss;
        ss << "=== Self-Funding ===\n";
        ss << "  Total income: $" << (int)summary.total_income << "\n";
        ss << "  Total expenses: $" << (int)summary.total_expenses << "\n";
        ss << "  Net earnings: $" << (int)summary.net_earnings << "\n";
        ss << "  Monthly income (30d): $" << (int)summary.monthly_income_avg << "\n";
        ss << "  Monthly expenses (30d): $" << (int)summary.monthly_expense_avg << "\n";
        ss << "  Savings/Upgrade fund: $" << (int)summary.savings << "\n";
        ss << "  Pending proposals: " << proposals.size() << "\n";
        return ss.str();
    }
};

#endif // MK_SELF_FUNDING_CPP
