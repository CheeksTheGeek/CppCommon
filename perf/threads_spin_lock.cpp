//
// Created by Ivan Shynkarenka on 28.01.2016.
//

#include "cppbenchmark.h"

#include "threads/spin_lock.h"

#include <functional>
#include <thread>
#include <vector>

const uint64_t items_to_produce = 10000000;
const int producers_from = 1;
const int producers_to = 32;
const auto settings = CppBenchmark::Settings().ParamRange(producers_from, producers_to, [](int from, int to, int& result) { int r = result; result *= 2; return r; });

void produce(CppBenchmark::Context& context)
{
    const uint64_t producers_count = context.x();
    uint64_t crc = 0;

    // Create spin-lock synchronization primitive
    CppCommon::SpinLock lock;

    // Start producer threads
    std::vector<std::thread> producers;
    for (uint64_t producer = 0; producer < producers_count; ++producer)
    {
        producers.push_back(std::thread([&lock, &crc, producer, producers_count]()
        {
            uint64_t items = (items_to_produce / producers_count);
            for (uint64_t i = 0; i < items; ++i)
            {
                std::lock_guard<CppCommon::SpinLock> guard(lock);
                crc += (producer * items) + i;
            }
        }));
    }

    // Wait for all producers threads
    for (auto& producer : producers)
        producer.join();

    // Update benchmark metrics
    context.metrics().AddIterations(items_to_produce - 1);
    context.metrics().SetCustom("CRC", crc);
}

BENCHMARK("SpinLock", settings)
{
    produce(context);
}

BENCHMARK_MAIN()