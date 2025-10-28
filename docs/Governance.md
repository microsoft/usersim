# Project Governance

## Roles and Personnel

The usersim project currently uses the following roles, listed in order of increasing permission:

* Contributor
* Collaborator
* Maintainer
* Project Admin

### Contributor

The ability to read, clone, and contribute issues or pull requests is open to the public.

**Personnel**: anyone

**Minimum Requirements**: (none)

**Responsibilities**: (none)

### Collaborator

A collaborator is a contributor who can have issues and pull request review requests assigned to them in github. This corresponds to the "Read" role in github.

**Personnel**: To be determined based on project participation

**Minimum Requirements**:
* Has attended a triage meeting
* Has agreed to have a github issue assigned to them to work on
* Is approved by the existing Project Admins

**Responsibilities**:
* Contribute pull requests for any assigned issues

### Maintainer

A maintainer can also merge pull requests. This corresponds to the "Write" role in github. All maintainers must be listed in the [CODEOWNERS file](../.github/CODEOWNERS).

**Personnel**: @Alan-Jowett (and others to be added based on contributions)

**Minimum Requirements**:
* Consistently participates in weekly triage meetings
* Has submitted multiple pull requests across at least 2 months that have been merged
* Has provided feedback on multiple pull requests from others, across at least 2 months
* Has demonstrated an understanding of a particular area of the code such as one or more directories
* Is approved by the existing Project Admins

**Responsibilities**:
* [Review pull requests](#reviewing-pull-requests) from others
* Merge pull requests once tests pass and sufficient approvals exist

### Project Admin

An admin can also assign people to each of these roles, and have full access to all github settings. This corresponds to the "Admin" role in github.

**Personnel**: @Alan-Jowett

**Minimum Requirements**:
* Has acted as a maintainer
* Knows most or all of the maintainers
* Understands the github Admin settings, such as experience from other projects
* Is approved by the existing Project Admins

**Responsibilities**:
* At least annually, review the current set of personnel in each role and update as needed
* Manage github settings such as branch protection rules and repository secrets
* Resolve any disputes among maintainers

## Project Milestones

usersim uses quarterly milestones in the format `YYYY-QN`. So, e.g., the milestone for Q1 2024 will be called 2024-Q1. At any time, there will be one current milestone (for the current quarter) and one or more future milestones, in the project. A `Backlog` milestone exists for backlog issues. In the weekly triage meetings, new issues should be added to a milestone following guidelines in the [Prioritizing Issues](#prioritizing-issues) section.

## Prioritizing Issues

To help prioritize issues, the triage drivers and maintainers MUST use *special labels* to prioritize issues as follows:

### P1
This label represents highest priority. This label should be used for bugs impacting users, new features, or enhancements that are urgently requested by users. `P1` issues MUST be set to the *current milestone*.

### P2
This label is used for the next level of priority. Issues marked as `P2` can be set to the current milestone or to a future milestones (preferred).

### P3
This label is used for low priority issues. Issues marked as `P3` can be set to any future milestone or the `Backlog` milestone.

## Closing Active Issues

* Follow GitHub guidelines on [closing an issue](https://docs.github.com/en/issues/tracking-your-work-with-issues/administering-issues/closing-an-issue).
* Follow GitHub guidelines on [closing duplicate issues](https://github.blog/changelog/2024-12-12-github-issues-projects-close-issue-as-a-duplicate-rest-api-for-sub-issues-and-more/#broom-close-an-issue-as-a-duplicate).
* Issues for bugs that cannot be reproduced can be closed as "not planned" after providing relevant details (such as trace logs etc.).
* Closing an issue for any other reason, including (but not limited to) not fixing the bug, or not completing the task requires discussion in a triage meeting, and should not be done purely due to lack of resources for which case moving to "P3, Help Wanted, Backlog" is preferred. Closing an issue as "*Won't Fix*" is appropriate when it is decided that the proposal is (or no longer is) a good idea to be done at all.

## Milestone Issues Limit

Triage managers need to ensure that the number of issues in a milestone does not exceed the capacity of the contributors actively working on the project. At the end of each triage meeting, the Project Milestones should be checked to see if there aren't too many issues added to any given milestone. The issues limit for a given milestone will depend on number of contributors working on them, the complexities of the issues and other relevant factors as deemed important by the triage drivers. As a rule of thumb, the number of issues in a milestone should not exceed 3 times the number of active contributors. Assuming 3-4 active contributors, there should be no more than 12 issues per milestone.

**Note**: This limit is irrespective of the issue state. Closing out an issue, does not "add room" for more issues into the milestone.

A `P2` or `P3` issues must be added to the first milestone that has not reached its limit. A `P1` issue can only be added to the current milestone (`N`). If adding a new `P1` issue exceeds the milestone limit, one or more open issues must be moved to the future milestones. Open `P2` and `P3` issues in the current milestone must the first candidates for moving. If all the open issues in the current milestone are `P1`, the triage drivers must *downgrade* one or more of them to `P2` and move them to the next (`N+1`) milestone.

If all existing milestones have exceeded their limit issues, a new milestone must be created to accommodate new issues.

## Closing a Milestone

In the first triage meeting of a quarter, the previous milestone (`N`) must be closed.

Ideally, all issues should be addressed by the end of the designated milestones. However, if that does not happen for any reason, triage drivers or administrators must move all overdue issues that do not have an active pull request to a future milestone as follows:
- `P1` issues can be moved to the *next* (`N+1`) milestone. This must be done in a triage meeting, after discussing with all maintainers.
- `P2` issues can be moved to any of the future milestones.
- `P3` issues can be moved to future or `Backlog` milestones.

Moving issues to future milestones should not exceed the per milestone issues limit. If any issue has been moved two times already, then the issue must be re-triaged for setting the appropriate priority and/or the milestone.

## Reviewing Pull Requests

Pull requests need at least two approvals before being eligible for merging. Besides reviewing for technical correctness, reviewers are expected to:

* For any PR that adds functionality, check that the PR includes sufficient tests for that functionality. This is required in [CONTRIBUTING.md](../CONTRIBUTING.md).
* For any PR that adds or changes functionality in a way that is observable by administrators or by authors of applications using usersim, check that documentation has been sufficiently updated. This is required in [CONTRIBUTING.md](../CONTRIBUTING.md).
* Be familiar with, and call out code that does not conform to, the usersim [coding conventions](DevelopmentGuide.md).
* Check that errors are appropriately logged where first detected (not at each level of stack unwind).

When multiple pull requests are approved, maintainers should prioritize merging PRs as follows:

| Pri | Description | Tags | Rationale |
| --- | ----------- | ---- | --------- |
| 1 | Bugs | bug | Affects existing users |
| 2 | Test bugs | tests bug | Problem that affects CI/CD and may mask priority 1 bugs |
| 3 | Additional tests | tests enhancement | Gap that once filled might surface priority 1 bugs |
| 4 | Documentation | documentation | Won't affect CI/CD but may address usability issues |
| 5 | Dependencies | dependencies | Often a large amount of low hanging fruit. Keeping the overall PR count low gives a better impression to newcomers and observers. |
| 6 | New features | enhancement | Adds new functionality requested in a github issue. Although this typically is lower priority than dependencies, such PRs from new contributors should instead be prioritized above dependencies. |
| 7 | Performance optimizations | optimization | Doesn't do anything that isn't already working, but improvements do help users |
| 8 | Code cleanup | cleanup | Good to do but generally doesn't significantly affect any of the above categories |