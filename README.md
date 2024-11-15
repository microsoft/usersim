# User-mode Simulation of Windows Kernel APIs

This project is designed to allow a Windows kernel driver developer to
compile their code as a user-mode DLL for testing purposes, so that it
can be used for code coverage, fuzzers, etc.

## Using

To use this repository from another project:
1. Include this repository as a git submodule from your project.
2. Build your driver files as a DLL.  That is, create a new project or directory, and have it include
   the same source files as your driver does, but set the configuration type to be a Dynamic Library.
   See the "sample" project as an example, which builds the
   [KMDF sample driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver)
   as a DLL (the sample project can be built either using Visual Studio or using cmake).
3. Set the following property on the project: Configuration Properties -> General -> Platform Toolset to
   Visual Studio 2022 (v143).
4. Add a reference from your DLL project to the usersim project and the usersim_dll_skeleton project.
5. Define `_AMD64_;_WIN32_WINNT=0x0a00` in the project properties preprocessor defines.
6. Add to AdditionalIncludeDirectories: `$(WindowsSdkDir)Include\10.0.22621.0\km;$(WindowsSdkDir)Include\10.0.26100.0\km;;$(WindowsSdkDir)Include\wdf\kmdf\1.15`
7. Disable warning 4324 (structure was padded due to alignment) in the project properties, by adding 4324 to
   Configuration Properties -> C/C++ -> Advanced -> Disable Specific Warnings.
8. Make sure you are not linking with any kernel libs, since they aren't usable by DLLs.
9. Give usersim.lib preference over ntdll.lib as follows:
   Under Configuration Properties -> Linker -> Input -> Ignore Specific Default Libraries, add ntdll.lib.
   Under Configuration Properties -> Linker -> Input -> Additional Dependencies, add usersim.lib before ntdll.lib.

### Leak Detection

To detect memory leaks on exit, define the environment variable `CXPLAT_MEMORY_LEAK_DETECTION=true`

### Fault Injection

To use fault injection, define the environment variable `CXPLAT_FAULT_INJECTION_SIMULATION=4`
where the value (4 in this example) is the number of stack frames to use to determine whether a call stack is unique.
Fault injection will cause one call into the UserSim library to fail, for every unique call stack.

The failed call stacks are dumped into a file in the current directory with the name `<exe name>.fault.log`
where `<exe name>` is the name of the original executable.  To reset fault injection for a test executable,
simply delete all `<exe name>.*.log` files from the current directory.

The `.\scripts\Test-FaultInjection.ps1` powershell script can be used to test fault injection for a Catch2-based
test executable by making successive runs that fail each successive path, until the test executable
ends with all tests passing, indicating that no more code paths were found to fail.  This can be done
as follows.

First, navigate into the directory containing the test executable. Then do:

```
powershell <path to script>\Test-FaultInjection.ps1 <dump path> <test timeout> ".\<test exe name>.exe" <depth>
```
where:

* `<path to script>` is the path to the `Test-FaultInjection.ps1` file.
* `<dump path>` is the directory to put any crash dumps in.
* `<test timeout>` is the maximum number of seconds per test iteration.
* `<test exe name>` is the filename of the test executable.
* `<depth>` is the value to use for `CXPLAT_FAULT_INJECTION_SIMULATION`.

Each iteration will result in more stacks being added to the same `<exe name>.fault.log`
file, after a line `# Iteration: <number>` where `<number>` is the iteration number.
If a crash occurs, it can then be reproduced and diagnosed by deleting the last iteration's
content from the file, and then running the test executable under a debugger.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Building this repository

The steps above will build the UserSim project components needed by a dependent project, but not
the tests for UserSim itself.  For contributors wanting to contribute to the UserSim project, it
can be built and tested as follows.

1. As a one-time step, from a Visual Studio Developer Command Prompt, do:
   `cmake -G "Visual Studio 17 2022" -S external\catch2 -B external\catch2\build -DBUILD_TESTING=OFF`
2. Build usersim.sln from the Visual Studio UI or using msbuild.
3. `cxplat_test.exe -d yes` and `usersim_tests.exe -d yes` can then be executed to run the standard tests.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
