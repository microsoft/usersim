// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "catch2/catch_all.hpp"

#include <fstream>
#include <iostream>
#include <windows.h>

/**
 * @brief A Catch2 reporter that logs the name of each test that passes.
 * This is used to generate a list of tests that passed in the last run in a
 * file that can be used to filter the set of tests that are run in the next
 * run. This is consumed by the Test-FaultInjection.ps1 script.
 */
class cxplat_passed_test_log : public Catch::EventListenerBase
{
  public:
    using Catch::EventListenerBase::EventListenerBase;

    // Log passing tests.
    void
    testCaseEnded(Catch::TestCaseStats const& testCaseStats) override
    {
        if (!passed_tests) {
            char process_name[MAX_PATH];
            GetModuleFileNameA(nullptr, process_name, MAX_PATH);
            std::string log_file = process_name;
            log_file += ".passed.log";
            passed_tests = std::make_unique<std::ofstream>(log_file, std::ios::app);
        }
        if (testCaseStats.totals.assertions.failed == 0) {
            *passed_tests << testCaseStats.testInfo->name << std::endl;
            passed_tests->flush();
        }
    }

  private:
    static std::unique_ptr<std::ofstream> passed_tests;
};

std::unique_ptr<std::ofstream> cxplat_passed_test_log::passed_tests;
