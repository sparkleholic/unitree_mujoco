# Shared Agent Instructions

This file is the authoritative instruction source for AI coding agents working on this project. `CLAUDE.md` imports it.
This project connects MuJoCo simulation with the Unitree SDK. Do not hard-code a detailed source tree or a current feature inventory here; maintain the current architecture in `docs/design.md`.

## 1. Starting Work

- Read the user request, this file, applicable nested instructions, and `docs/design.md`.
- Inspect `git status` and relevant diffs. Distinguish existing user changes from your changes and preserve unrelated work.
- Base conclusions on relevant source and actual build configuration. Distinguish header inclusion, library linkage, and executed API calls.
- Instructions from other projects are references. Rules adopted for this project must be stated here.
- Commit, push, merge, or clean up existing user changes only when the user requests it.
- Always write commit messages in English, including the subject and body.
- Limit every commit message line to 78 characters, including the subject,
  body, and footers. Wrap prose at word boundaries.
- Avoid unnecessary whitespace in commit messages: no trailing spaces,
  alignment padding, or repeated spaces. Use blank lines only to separate
  the subject, body paragraphs, and footers.

## 2. Documentation Model

Separate **permanent documentation** from **transient working documents**. Keep the normal reading burden small: `docs/design.md` describes the current system, and completed records provide feature history when needed.

| Document | Lifetime and purpose |
| --- | --- |
| `docs/design.md` | Permanent: the authoritative design of the current implementation. Do not describe planned features as implemented. |
| `docs/req/YYYY-MM-DD-<slug>.md` | Transient: requirements, scope, proposed design, interfaces, and acceptance criteria. |
| `docs/plan/YYYY-MM-DD-<slug>.md` | Transient: implementation plan, affected files, ordered tasks, and verification. |
| `docs/wip/YYYY-MM-DD-<slug>.md` | Transient: actual progress, deviations, decisions, and their reasons. |
| `docs/completed/<NN>_<slug>.md` | Permanent: the final feature record after implementation and verification. |

- Write `AGENTS.md`, `CLAUDE.md`, and all new or updated project documentation in **English**, including req, plan, wip, design, and completed records. Keep API names and identifiers as written in code. Conversation language may follow the user.
- Use the same kebab-case `<slug>` throughout a feature's lifecycle.
- Date prefixes record the document's creation date. Use absolute dates in `YYYY-MM-DD` format.
- Assign completed records the next available sequence number, zero-padded to at least two digits. Never renumber existing records.
- Use repository-relative Markdown links. Specify the directory when linking req and plan files with the same basename.
- Use Mermaid for architecture and sequence diagrams.
- **Only active work belongs in req, plan, and wip. At actual implementation completion, consolidate their useful content into completed, then delete that feature's req, plan, and wip documents. Do not retain them as completed-status stubs or an additional archive.**
- Keep `docs/design.md` and any applicable public interface documentation after completion. The cleanup rule removes feature working documents, not permanent design or usage documentation, and never another active feature's documents.

## 3. Development Lifecycle

1. **Requirements:** Write the purpose, supported and excluded scope, compatibility constraints, proposed design, failure cases, and measurable acceptance criteria in req.
2. **Plan:** Link the plan to req. Specify affected files, testable implementation steps, public contract effects, and runtime verification.
3. **Design review:** Obtain the user's review of req and plan before implementing a new feature or architectural change. Read-only investigation and documentation may continue while review is pending. Do not request approval again for an already reviewed scope. Follow explicit user instructions that change this process.
4. **Implementation:** Establish meaningful failing tests before implementing behavior. Record deviations and their reasons in wip; update req/plan when scope changes materially.
5. **Verification:** Run relevant builds, tests, and runtime checks. Record actual commands, environment, results, and limitations. A successful build does not prove runtime behavior.
6. **Completion:** Once acceptance criteria are met, write the completed record, update the current design and usage/interface documentation, and remove the feature's transient documents as described below. Do not mark unfinished or unverified work complete.

### Mandatory Completion Cleanup

Perform these steps before declaring completion:

1. Select the next completed sequence number.
2. Consolidate requirements, final design, implementation decisions, deviations, and verification evidence from req/plan/wip into `docs/completed/<NN>_<slug>.md`.
3. Update `docs/design.md` and relevant public interface or usage documents to match the implemented behavior.
4. Replace links to the transient documents with links to the permanent completed record where appropriate. The completed record must stand on its own without deleted documents.
5. Delete **all req, plan, and wip documents for the completed slug**, including date variants. Do not delete documents for other work in progress.
6. Verify that no links point to the deleted documents and that the completed record preserves the information needed for future maintenance.

For a completed feature, only its completed record remains from the feature's working-document lifecycle; current design and usage documentation remain permanent. Do not create a completed record or delete working documents merely because a planning or documentation turn has ended.

## 4. Required Contents of a Completed Record

- Original requirements and motivation, final scope, and acceptance criteria with their outcomes.
- Final design with Mermaid diagrams and explicit process, thread, and communication boundaries.
- Main implementation decisions, affected components, deviations from the plan, and reasons.
- Reproducible setup, build, run, and test commands; configuration defaults and compatibility.
- Normal and failure-path results, observed measurements, evidence/log locations, and remaining limitations.
- Actual implementation completion date in `YYYY-MM-DD` format.
- Related implementation commits: short SHA and subject, verified with Git. If uncommitted, state `Uncommitted`; never invent a SHA or commit merely to fill this section.

Treat a subsequent change to a completed feature as new work with a new slug and the same lifecycle. Preserve the previous completed record's identity, link it to the new record, and update the current design. Fold the new work's transient documents into its new completed record and delete them after completion.

## 5. Code, Communication, and Verification

- Preserve existing APIs, configuration defaults, and invocation patterns. Document intentional compatibility changes in req/plan and usage documentation.
- Distinguish DDS topic publication from RPC service implementation, and request acceptance from physical motion completion.
- Identify installed SDK headers, libraries, and actual link targets. Label reconstructed code as reconstructed and compare relevant claims against the original binary when needed.
- Explicitly manage shared state, command ownership, and shutdown lifetimes across service callbacks and physics/control loops.
- Document process, network, and control-mode boundaries. Isolate tests using loopback and a separate DDS domain.
- Verify command effects using state feedback as well as return codes. Cover timeouts, unsupported commands, invalid inputs, and connection loss as relevant.
- Terminate only processes created by the test. Do not disturb existing user sessions or files.
- Investigate failures before changing behavior. Never report tests you did not run or results you did not observe as successful.

## 6. External Skills and Tools

- This file's documentation paths override external skill defaults. Do not create duplicate design authorities or retain a second working-document lifecycle.
- Skill recommendations for commits, publication, or delegation do not expand user authorization.
- Before declaring completion, verify the completed record, updated permanent documentation, transient-document deletion, and repaired links. A skill's branch-finishing workflow does not replace these steps.
