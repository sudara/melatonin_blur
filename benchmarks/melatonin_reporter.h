#pragma once

#include <catch2/catch_test_case_info.hpp>
#include <catch2/interfaces/catch_interfaces_config.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <iomanip>
#include <iostream>

class MelatoninBenchmark : public Catch::StreamingReporterBase
{
public:
    using StreamingReporterBase::StreamingReporterBase;

    void sectionStarting (Catch::SectionInfo const& info) override
    {
        // don't print the damn test case name over and over
        if (indent > 0)
        {
            auto textIndent = std::string (((size_t) indent - 1) * 2, ' ');
            std::cout << textIndent << info.name << '\n';
        }

        // required, otherwise Catch2 just hangs
        StreamingReporterBase::sectionStarting (info);

        // makes nested SECTIONS more understandable
        indent++;

        // the first benchmark is treated as the reference
        benchmarkNumber = 0;
    }

    void sectionEnded (Catch::SectionStats const& stats) override
    {
        indent--;
        StreamingReporterBase::sectionEnded (stats);

        // give us a bit of space
        std::cout << std::endl;
    }

    void benchmarkStarting (Catch::BenchmarkInfo const& info) override
    {
        // not doing anything here but printing a header
        if (!firstBenchmark())
            return;

        // print the header
        std::string meanString = "Mean (n=" + std::to_string (info.samples) + ")";
        std::cout << '\n'
                  << std::left << std::setw (30) << "Name"
                  << std::setw (25) << meanString
                  << std::setw (25) << "Max" << std::endl
                  << std::string (60, '-') << std::endl;
    }

    void benchmarkEnded (Catch::BenchmarkStats<> const& stats) override
    {
        double averageInNS = stats.mean.point.count() / 1'000.0;
        double maxInNS = std::max_element (stats.samples.begin(), stats.samples.end())->count() / 1'000.0;
        std::string averageTime = std::to_string (averageInNS).substr (0, std::to_string (averageInNS).find ('.')) + "µs";
        std::string maxTime = std::to_string (maxInNS).substr (0, std::to_string (maxInNS).find ('.')) + "µs";

//        for (auto& sample : stats.samples)
//        {
//            std::cout << sample.count() << ", ";
//        }

        std::cout << std::endl;

        // the first BENCHMARK sets the baseline average
        if (firstBenchmark())
            baselineAverage = averageInNS;
        else
            averageTime += percentFasterString (baselineAverage, averageInNS);

        // setw doesn't know how to deal with ANSI escape codes
        int benchmarkWidth = firstBenchmark() ? 25 : 34;
        std::cout << std::left << std::setw (30) << stats.info.name
                  << std::setw (benchmarkWidth) << averageTime
                  << std::setw (benchmarkWidth) << maxTime
                  << std::endl;

        benchmarkNumber++;
    }

    static std::string getDescription()
    {
        return "Comparative benchmarking in µs";
    }

private:
    int indent = 0;
    int benchmarkNumber = 0;
    double baselineAverage = 0;

    [[nodiscard]] bool firstBenchmark()
    {
        return benchmarkNumber == 0;
    }

    static std::string percentFasterString (double baseline, double measurement)
    {
        auto threshold = 1.1;
        if ((measurement * threshold) < baseline)
        {
            auto changeFactor = std::to_string (baseline / measurement);
            return " \033[32m(" + changeFactor.substr (0, changeFactor.find ('.') + 2) + "x faster!)\033[0m";
        }
        else if (measurement > (baseline * threshold))
        {
            auto changeFactor = std::to_string (measurement / baseline);
            return " \033[31m(" + changeFactor.substr (0, changeFactor.find ('.') + 2) + "x slower)\033[0m";
        }
        else
            return " \033[37m(similar)\033[0m";
    }
};

CATCH_REGISTER_REPORTER ("melatonin", MelatoninBenchmark)
