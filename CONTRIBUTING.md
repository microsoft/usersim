# Contributing to User-mode Simulation of Windows Kernel APIs

We'd love your help with usersim! Here are our contribution guidelines.

- [Contributing to User-mode Simulation of Windows Kernel APIs](#contributing-to-user-mode-simulation-of-windows-kernel-apis)
  - [Code of Conduct](#code-of-conduct)
  - [Bugs](#bugs)
    - [Did you find a bug?](#did-you-find-a-bug)
    - [Did you write a patch that fixes a bug?](#did-you-write-a-patch-that-fixes-a-bug)
  - [New Features](#new-features)
  - [Contributor License Agreement](#contributor-license-agreement)
  - [Contributing Code](#contributing-code)
    - [Development Environment](#development-environment)
    - [Testing](#testing)
    - [Code Style](#code-style)

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Microsoft Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with additional questions or comments.

## Bugs

### Did you find a bug?

First, **ensure the bug was not already reported** by searching on GitHub under [Issues](https://github.com/microsoft/usersim/issues).

If you found a non-security related bug, you can help us by [submitting a GitHub Issue](https://github.com/microsoft/usersim/issues/new). The best bug reports provide a detailed description of the issue and step-by-step instructions for reliably reproducing the issue.

We will aim to triage issues in weekly triage meetings. In case we are unable to repro the issue, we will request more information from you, the filer. There will be a waiting period of 2 weeks for the requested information and if there is no response, the issue will be closed. If this happens, please reopen the issue if you do get a repro and collect the requested information.

However, in the best case, we would love it if you can submit a Pull Request with a fix.

If you found a security issue, please **do not open a GitHub Issue**, and instead follow [these instructions](SECURITY.md).

### Did you write a patch that fixes a bug?

Fork the repo and make your changes. Then open a new GitHub pull request with the patch.

* Ensure the PR description clearly describes the problem and solution. Include the relevant issue number if applicable.
* Before submitting, please read the [Development Guide](docs/DevelopmentGuide.md) to know more about coding conventions.

## New Features

You can request a new feature by [submitting a GitHub Issue](https://github.com/microsoft/usersim/issues/new).

If you would like to implement a new feature, please first [submit a GitHub Issue](https://github.com/microsoft/usersim/issues/new) and communicate your proposal so that the community can review and provide feedback. Getting early feedback will help ensure your implementation work is accepted by the community. This will also allow us to better coordinate our efforts and minimize duplicated effort.

## Contributor License Agreement

You will need to complete a Contributor License Agreement (CLA) for any code submissions. Briefly, this agreement testifies that you are granting us permission to use the submitted change according to the terms of the project's license, and that the work being submitted is under appropriate copyright. You only need to do this once. For more information see https://cla.opensource.microsoft.com/.

## Contributing Code

For all but the absolute simplest changes, first [submit a GitHub Issue](https://github.com/microsoft/usersim/issues/new) so that the community can review and provide feedback. Getting early feedback will help ensure your work is accepted by the community. This will also allow us to better coordinate our efforts and minimize duplicated effort.

If you would like to contribute, first identify the scale of what you would like to contribute. If it is small (grammar/spelling or a bug fix) feel free to start working on a fix. If you are submitting a feature or substantial code contribution, please discuss it with the maintainers and ensure it follows the product roadmap. You might also read these two blogs posts on contributing code: [Open Source Contribution Etiquette](http://tirania.org/blog/archive/2010/Dec-31.html) by Miguel de Icaza and [Don't "Push" Your Pull Requests](https://www.igvita.com/2011/12/19/dont-push-your-pull-requests/) by Ilya Grigorik. All code submissions will be rigorously [reviewed](docs/Governance.md) and tested by the maintainers, and only those that meet the bar for both quality and design/roadmap appropriateness will be merged into the source.

For all new Pull Requests the following rules apply:
- Existing tests should continue to pass.
- [Tests](docs/TestingGuide.md) need to be provided for every bug/feature that is completed.
- Documentation needs to be provided for every feature that is end-user visible.
- Code should follow the existing style and conventions used in the project.
- All commits should have meaningful commit messages that describe the changes being made.
- Pull requests should be focused and contain only related changes.

### Development Environment

To set up your development environment:

1. Clone the repository and initialize submodules:
   ```
   git clone https://github.com/microsoft/usersim.git
   git submodule update --init --recursive
   ```

2. Open the solution in Visual Studio 2022 or use CMake to build:
   - Visual Studio: Open `usersim.sln`
   - CMake: Follow the CMake build instructions in the README

3. Ensure you have the Windows SDK and WDK installed as described in the README.

### Testing

Before submitting a pull request, ensure that:
- All existing tests pass
- New tests are added for any new functionality
- Code coverage is maintained or improved where possible

### Code Style

- Follow the existing C/C++ coding style used throughout the project
- Use meaningful variable and function names
- Add comments for complex logic
- Ensure proper error handling and cleanup
- Use the project's existing patterns for memory management and error handling