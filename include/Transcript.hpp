#ifndef TRANSCRIPT
#define TRANSCRIPT

#include <atomic>
#include <cmath>
#include "tbb/atomic.h"
#include "SailfishStringUtils.hpp"
#include "SailfishMath.hpp"

class Transcript {
public:
    Transcript(size_t idIn, const char* name, uint32_t len, double alpha = 0.005) :
        id(idIn), RefName(name), RefLength(len), Sequence(nullptr), mass_(std::log(alpha)), sharedCount_(0.0)  {
            uniqueCount_.store(0);
        }

    Transcript(Transcript&& other) {
        id = other.id;
        RefName = std::move(other.RefName);
        RefLength = other.RefLength;
        uniqueCount_.store(other.uniqueCount_);
    }

    Transcript& operator=(Transcript&& other) {
        id = other.id;
        RefName = std::move(other.RefName);
        RefLength = other.RefLength;
        uniqueCount_.store(other.uniqueCount_);
    }


    inline double sharedCount() { return sharedCount_.load(); }
    inline size_t uniqueCount() { return uniqueCount_.load(); }
    inline size_t totalCount() { return totalCount_.load(); }

    inline void addUniqueCount(size_t newCount) { uniqueCount_ += newCount; }
    inline void addTotalCount(size_t newCount) { totalCount_ += newCount; }

    inline uint8_t baseAt(size_t idx,
                          sailfish::stringtools::strand dir = sailfish::stringtools::strand::forward) {
        using sailfish::stringtools::strand;
        using sailfish::stringtools::encodedRevComp;
        size_t byte = idx >> 1;
        size_t nibble = idx & 0x1;

        switch(dir) {
        case strand::forward:
            if (nibble) {
                return Sequence[byte] & 0x0F;
            } else {
                return (Sequence[byte] & 0xF0) >> 4;
            }
            break;
        case strand::reverse:
            if (nibble) {
                return encodedRevComp[Sequence[byte] & 0x0F];
            } else {
                return encodedRevComp[(Sequence[byte] & 0xF0) >> 4];
            }
            break;
        }

    }

    inline void addSharedCount(double sc) {
        double oldMass = sharedCount_;
        double returnedMass = oldMass;
        double newMass{0.0};
        do {
            oldMass = returnedMass;
            newMass = oldMass + sc;
            returnedMass = sharedCount_.compare_and_swap(newMass, oldMass);
        } while (returnedMass != oldMass);
    }

    inline void addMass(double mass) {
        double oldMass = mass_;
        double returnedMass = oldMass;
        double newMass{0.0};
        do {
            oldMass = returnedMass;
            newMass = sailfish::math::logAdd(oldMass, mass);
            returnedMass = mass_.compare_and_swap(newMass, oldMass);
        } while (returnedMass != oldMass);
    }

    inline double mass() { return mass_; }

    std::string RefName;
    uint32_t RefLength;
    uint32_t id;

    double uniqueCounts{0.0};
    double totalCounts{0.0};
    double projectedCounts{0.0};
    double sharedCounts{0.0};

    uint8_t* Sequence;

private:
    std::atomic<size_t> uniqueCount_;
    std::atomic<size_t> totalCount_;
    tbb::atomic<double> mass_;
    tbb::atomic<double> sharedCount_;
};

#endif //TRANSCRIPT