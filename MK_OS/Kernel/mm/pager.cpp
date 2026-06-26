#ifndef MK_PAGER_CPP
#define MK_PAGER_CPP

#include <iostream>
#include <map>
#include <string>

enum class MKPageProt { READ_ONLY, READ_WRITE, KERNEL_ONLY };

struct MKPage {
    int id;
    std::string desc;
    MKPageProt prot;
    bool pinned;     // Pinned pages cannot be evicted
    int accessCount;
};

class MKPager {
private:
    std::map<int, MKPage> pages;
    int maxPages;
    int nextFreeFrame = 100; // Simulated physical frame start address

    std::string protLabel(MKPageProt p) const {
        switch(p) {
            case MKPageProt::READ_ONLY:   return "R";
            case MKPageProt::READ_WRITE:  return "RW";
            case MKPageProt::KERNEL_ONLY: return "KERN";
        }
        return "?";
    }

public:
    MKPager(int maxPages = 64) : maxPages(maxPages) {
        std::cout << "[KERNEL - MM] Pager online. Max pages=" << maxPages << "\n";
    }

    // Map a virtual page to a physical frame
    bool mapPage(int id, const std::string& desc,
                 MKPageProt prot = MKPageProt::READ_WRITE,
                 bool pinned = false) {
        if((int)pages.size() >= maxPages) {
            std::cout << "[MM] ERROR: Page table full. Cannot map page " << id << "\n";
            return false;
        }
        if(pages.count(id)) {
            std::cout << "[MM] WARNING: Page " << id << " already mapped. Remapping.\n";
        }
        pages[id] = {id, desc, prot, pinned, 0};
        std::cout << "[MM] Mapped page [" << id << "] -> \"" << desc
                  << "\" | Prot=" << protLabel(prot)
                  << " | Frame=0x" << std::hex << (nextFreeFrame++) << std::dec
                  << (pinned ? " | PINNED" : "") << "\n";
        return true;
    }

    // Unmap a page — refuses to unmap pinned pages
    bool unmapPage(int id) {
        auto it = pages.find(id);
        if(it == pages.end()) {
            std::cout << "[MM] WARNING: Cannot unmap page " << id << " — not found.\n";
            return false;
        }
        if(it->second.pinned) {
            std::cout << "[MM] ERROR: Cannot unmap pinned page " << id << ".\n";
            return false;
        }
        pages.erase(it);
        std::cout << "[MM] Unmapped page [" << id << "]\n";
        return true;
    }

    // Access a page — increments access counter
    bool access(int id) {
        auto it = pages.find(id);
        if(it == pages.end()) {
            std::cout << "[MM] PAGE FAULT: Page " << id << " not mapped!\n";
            return false;
        }
        it->second.accessCount++;
        return true;
    }

    // Pin/unpin a page in memory
    void pin(int id)   { if(pages.count(id)) pages[id].pinned = true; }
    void unpin(int id) { if(pages.count(id)) pages[id].pinned = false; }

    int used()  const { return pages.size(); }
    int avail() const { return maxPages - pages.size(); }

    void stats() const {
        std::cout << "\n--- [MM PAGE TABLE] Used=" << pages.size()
                  << "/" << maxPages << " ---\n";
        for(const auto& kv : pages) {
            const auto& p = kv.second;
            std::cout << "  Page[" << p.id << "] \"" << p.desc << "\""
                      << " Prot=" << protLabel(p.prot)
                      << " Access=" << p.accessCount
                      << (p.pinned ? " PINNED" : "") << "\n";
        }
        std::cout << "-----------------------------------\n";
    }
};

#endif // MK_PAGER_CPP