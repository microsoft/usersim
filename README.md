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
3. Define the preprocessor symbol USERSIM_DLLMAIN when building one file (typically the one containing your DriverEntry)
  that includes wdf.h.  The sample project does this not in the driver.c source file itself but rather in the sample.vcxproj that
  builds it, but all that is important is defining it before including wdf.h.
4. Add a reference from your DLL project to the usersim project.

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

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
