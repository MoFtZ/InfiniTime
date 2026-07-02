# Develop-branch CI + rolling release — design

**Repo:** fork `MoFtZ/InfiniTime` of InfiniTimeOrg/InfiniTime, branch `develop` cut off the `v1.16` (1.16.0) tag.

## Goal

Make CI build off `develop` and publish a single, always-current "rolling" pre-release carrying the flashable firmware files. The fork should stay lean and diverge from upstream as little as possible, so future merges from upstream remain clean.

## Decisions

The build triggers move from `main` to `develop`, for both pushes and pull requests. Every push to `develop` refreshes one rolling pre-release, `develop-latest`, whose assets always reflect the newest commit and live at a stable URL. Pull requests into `develop` still build, but they do not publish a release.

The firmware build and the simulator build are both kept. The lint/format checks and the pull-request build-size comments are dropped. The release is published by the `gh` CLI running in its own job on the host runner, rather than a third-party action, because `gh` is pre-installed on the runner and the firmware build container does not include it.

The overriding principle is minimal, merge-clean divergence. Workflow files that already target `main` and therefore never fire while work happens on `develop` are left exactly as upstream ships them, so they produce no merge conflicts later. Only the main CI workflow is edited, because it is the one file that genuinely has to change — it is the custom build — and even there the edits are kept as small and additive as possible.

## Release assets

Each refresh of `develop-latest` attaches three files, copied directly from the build output directory: the DFU package (for over-the-air or companion-app flashing), the MCUBoot application image, and the external-flash resources archive holding fonts and images.

These come straight from the build output rather than from the existing per-run artifacts. The existing DFU artifact is uploaded unzipped so it is pleasant to browse from the Actions UI, but a release needs the original archive so it can be flashed directly, so the release path takes its own copy and leaves the existing artifact-upload steps unchanged.

## Changes to the main CI workflow

This is the only workflow that is edited. Its triggers are retargeted from `main` to `develop` on both push and pull-request events, keeping the existing path-ignore filters that skip documentation-only changes.

The two pull-request build-size jobs are removed entirely. They only ran on pull requests and fed the now-dropped size comparison; left in place they would have run an expensive base-reference build on every develop pull request for no benefit. The build-firmware job's size-output step and its job outputs are left as-is — harmless once unused — to keep the diff small.

The build-firmware job gains a small, push-only addition at the end that gathers the three flashable files into a staging folder and uploads them as a single bundle artifact for the release job to consume. This is purely additive and does not touch the existing artifact uploads.

A new release job is added. It runs only on pushes to `develop`, depends on the firmware build, and runs on the host runner because it needs the `gh` CLI. It is granted write access to repository contents, scoped to that job alone so the build jobs keep a read-only token, and it is serialized through a concurrency group so that two quick pushes cannot race on the tag or the release, without cancelling a publish already in flight. The job downloads the bundle, force-moves the `develop-latest` tag onto the current commit (creating it on the first run), then creates the pre-release if it does not yet exist or updates its title and notes if it does, and finally uploads the three assets, replacing any existing ones of the same name.

The simulator build job is kept as-is.

## Deliberately not changed

A few workflow files are left byte-identical to upstream because they target `main` and never fire during develop work, so they sit dormant while staying conflict-free on future merges: the lint/format workflow, the pull-request size-comment workflow, and the build-image (Docker Hub) workflow. The last also needs Docker Hub secrets the fork does not have, which is a further reason to leave it untouched.

Two small fixes were considered and deliberately skipped. The simulator artifact currently ends up with an empty name suffix because the branch-name variable is only set in the firmware job; naming it properly is cosmetic and would add divergence, so it is left alone unless wanted. The main workflow also hardcodes a container work-directory path; because this fork happens to be named `InfiniTime`, that path already resolves correctly, so generalizing it is unnecessary and would only add divergence — worth revisiting only if the repository is ever renamed.

## Operational notes

The release job needs the repository's Actions workflow permissions set to read and write so its token can move the tag and publish the release; this is the default for a personal fork. On the first run the tag and release do not exist yet and are created. The asset filenames embed the version string, which is stable on `develop`, so re-uploading cleanly overwrites the previous assets; if the version string is ever customized and changes, stale-named assets could accumulate and this should be revisited. The simulator job continues to clone the simulator project from its own upstream.

## Out of scope

Customizing the on-watch version string and the resulting asset filenames (for example, replacing `1.16.0` with a custom brand or version) is a separate change to the build configuration, to be done whenever custom branding is wanted. No firmware feature changes are part of this work; it covers only the CI and release plumbing.
