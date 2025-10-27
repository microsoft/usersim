# Testing Guide

This file details the testing that is done in CI/CD, and that must be maintained and extended as new features are added to usersim.

## Overview

The usersim project uses multiple types of testing to ensure code quality and reliability:

* **Unit tests**: Test individual components in isolation
* **Integration tests**: Test component interactions
* **Sample tests**: Test the sample driver implementation
* **Fault injection tests**: Test behavior under error conditions
* **Memory leak tests**: Detect memory management issues

## Unit Tests

Unit tests test the functionality in the static libraries and core components. Unit tests run entirely in user mode and use mock implementations where necessary. Unit tests are run with address sanitization on to catch memory issues.

Tests in this category currently include:
* `tests.exe`: This unit tests core usersim functionality, including API simulation and memory management.
* `cxplat_test.exe`: This unit tests the cross-platform abstraction layer.

### Running Unit Tests

```sh
# Run all unit tests
ctest -L unit

# Run specific unit test
.\tests\Debug\tests.exe
.\cxplat\cxplat_test\Debug\cxplat_test.exe
```

### Writing Unit Tests

Unit tests should:
* Test a single component or function
* Be independent of other tests
* Use descriptive test names
* Include both positive and negative test cases
* Test edge cases and error conditions

Example test structure:
```cpp
TEST_CASE("Test function name", "[component]")
{
    // Arrange
    setup_test_data();

    // Act
    auto result = function_under_test(input);

    // Assert
    REQUIRE(result == expected_value);

    // Cleanup
    cleanup_test_data();
}
```

## Integration Tests

Integration tests verify that components work correctly together. These tests exercise the actual runtime behavior of usersim when simulating kernel APIs.

Tests in this category include:
* Sample driver tests that verify a complete kernel driver can be compiled and run as a DLL
* API interaction tests that verify proper simulation of Windows kernel APIs
* Memory management integration tests

### Running Integration Tests

```sh
# Run all integration tests
ctest -L integration

# Run sample driver test
.\sample\Debug\sample.exe
```

## Sample Tests

Sample tests use the included sample kernel driver to verify that usersim can successfully simulate a real-world kernel driver scenario.

The sample tests:
* Compile the sample driver as a DLL using usersim
* Execute driver entry points and verify correct behavior
* Test driver unload and cleanup

### Running Sample Tests

```sh
# Build and test the sample driver
cmake --build . --target sample
ctest -R sample
```

## Fault Injection Tests

Fault injection tests inject faults to test behavior under fault conditions, such as memory allocation failures or I/O errors.

Tests in this category include:
* Unit tests run under fault injection conditions
* Memory allocation failure simulation
* Resource exhaustion testing

### Running Fault Injection Tests

```sh
# Enable fault injection and run tests
set CXPLAT_FAULT_INJECTION=true
ctest -L fault_injection
```

## Memory Leak Tests

Memory leak detection helps identify memory management issues. All tests can be run with memory leak detection enabled.

### Running Memory Leak Tests

```sh
# Enable memory leak detection
set CXPLAT_MEMORY_LEAK_DETECTION=true

# Run tests with leak detection
ctest
```

Any memory leaks will be reported when the process exits.

## Test Requirements

All new code must include appropriate tests:

### For Bug Fixes
* Add a test that reproduces the bug
* Verify the test fails before the fix
* Verify the test passes after the fix

### For New Features
* Add unit tests for the new functionality
* Add integration tests if the feature affects component interactions
* Add sample tests if the feature affects the driver simulation
* Ensure test coverage is maintained or improved

### For API Changes
* Update existing tests that use the changed APIs
* Add tests for new API behavior
* Verify backward compatibility where applicable

## Test Organization

Tests are organized in the following directories:

* `tests/`: Main unit and integration tests
* `cxplat/cxplat_test/`: Cross-platform layer tests
* `sample/`: Sample driver tests

Each test file should:
* Have a descriptive filename indicating what is being tested
* Include appropriate test categories/tags
* Be documented with comments explaining complex test scenarios

## Continuous Integration

All tests are run automatically in CI/CD:

* Unit tests run on every pull request
* Integration tests run on every pull request
* Fault injection tests run on scheduled builds
* Memory leak tests run on scheduled builds

Test failures in CI/CD will block pull request merging.

## Best Practices

### Test Design
* Keep tests simple and focused
* Use descriptive test and variable names
* Test one thing at a time
* Make tests independent and idempotent

### Test Data
* Use minimal test data sets
* Avoid dependencies on external resources
* Clean up test data after each test

### Performance
* Keep test execution time reasonable
* Use mocks and stubs to avoid expensive operations
* Parallelize tests where possible

### Maintenance
* Update tests when APIs change
* Remove obsolete tests
* Refactor common test utilities
* Keep test documentation current

## Debugging Test Failures

When tests fail:

1. **Check the test output** for specific error messages
2. **Run the test locally** to reproduce the issue
3. **Use a debugger** to step through the failing test
4. **Check for memory issues** using AddressSanitizer
5. **Verify test isolation** by running the test alone
6. **Check for environmental factors** like file permissions or system state

### Common Test Issues

* **Timing issues**: Use proper synchronization instead of sleep
* **Resource cleanup**: Ensure all resources are properly released
* **Test order dependencies**: Make tests independent
* **Platform differences**: Account for platform-specific behavior
* **Memory corruption**: Use memory debugging tools