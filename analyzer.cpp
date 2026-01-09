#include "analyzer.h"

#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <cctype>

using namespace std;

// ===================== COMPARATORS  =====================

struct ZoneCountMinHeapCmp {
    bool operator()(const ZoneCount& a, const ZoneCount& b) const {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    }
};

struct SlotCountMinHeapCmp {
    bool operator()(const SlotCount& a, const SlotCount& b) const {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    }
};

struct ZoneCountSortCmp {
    bool operator()(const ZoneCount& a, const ZoneCount& b) const {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    }
};

struct SlotCountSortCmp {
    bool operator()(const SlotCount& a, const SlotCount& b) const {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    }
};

// ===================== HELPERS  =====================

static void trimStr(string& s) {
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    size_t end   = s.find_last_not_of(" \t\n\r\f\v");
    if (start == string::npos || end == string::npos) s.clear();
    else s = s.substr(start, end - start + 1);
}

static int extractHour(const string& datetime) {
    if (datetime.empty()) return -1;

    size_t pos = datetime.find(' ');
    if (pos == string::npos) pos = datetime.find('T');

    if (pos == string::npos || pos + 1 >= datetime.size()) return -1;

    pos++;
    string hourStr;
    while (pos < datetime.size() && isdigit((unsigned char)datetime[pos])) {
        hourStr += datetime[pos++];
        if (hourStr.size() == 2) break;
    }

    if (hourStr.empty()) return -1;

    int hour = stoi(hourStr);
    if (hour < 0 || hour > 23) return -1;
    return hour;
}

// ===================== IMPLEMENTATION  =====================

void TripAnalyzer::ingestFile(const string& csvPath) {
    zoneCounts.clear();
    slotCounts.clear();

    ifstream fin(csvPath);
    if (!fin.is_open()) return;

    string line;


    while (getline(fin, line)) {
        string tripID, zone, dropoff, pickupTime, dist, fare;
        stringstream ss(line);

        if (!getline(ss, tripID, ',')) continue;
        if (!getline(ss, zone, ',')) continue;
        if (!getline(ss, dropoff, ',')) continue;
        if (!getline(ss, pickupTime, ',')) continue;
        if (!getline(ss, dist, ',')) continue;
        if (!getline(ss, fare)) continue;

        trimStr(tripID);
        trimStr(zone);
        trimStr(pickupTime);

        if (tripID.empty() || zone.empty() || pickupTime.empty()) continue;

        int hour = extractHour(pickupTime);
        if (hour == -1) continue;

        zoneCounts[zone]++;
        slotCounts[zone][hour]++;
    }
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    priority_queue<ZoneCount, vector<ZoneCount>, ZoneCountMinHeapCmp> pq;

    for (const auto& p : zoneCounts) {
        ZoneCount z{p.first, p.second};
        if ((int)pq.size() < k) {
            pq.push(z);
        } else if (ZoneCountMinHeapCmp()(z, pq.top())) {
            pq.pop();
            pq.push(z);
        }
    }

    vector<ZoneCount> res;
    while (!pq.empty()) {
        res.push_back(pq.top());
        pq.pop();
    }

    sort(res.begin(), res.end(), ZoneCountSortCmp());
    return res;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    priority_queue<SlotCount, vector<SlotCount>, SlotCountMinHeapCmp> pq;

    for (const auto& z : slotCounts) {
        for (const auto& h : z.second) {
            SlotCount s{z.first, h.first, h.second};
            if ((int)pq.size() < k) {
                pq.push(s);
            } else if (SlotCountMinHeapCmp()(s, pq.top())) {
                pq.pop();
                pq.push(s);
            }
        }
    }

    vector<SlotCount> res;
    while (!pq.empty()) {
        res.push_back(pq.top());
        pq.pop();
    }

    sort(res.begin(), res.end(), SlotCountSortCmp());
    return res;
}
