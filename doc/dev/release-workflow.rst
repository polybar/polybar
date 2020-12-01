Release Workflow
================

We try to follow `Semantic Versioning <https://semver.org/>`_ in this project.
Patch releases (e.g. ``3.3.X``) contain only bug fixes. Minor releases (e.g.
``3.X.0``) can have backwards-compatible features. And major releases (
``X.0.0``) can introduce incompatible changes.

.. note::

  This document replaces the "`Release Guidelines
  <https://github.com/polybar/polybar/wiki/Release-Guidelines>`_" on the wiki
  that we used between 3.2.0 and 3.4.3. Starting with 3.5.0, we will follow
  the workflow described here to publish releases.

Polybar uses the `OneFlow
<https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow>`_
branching model for publishing new releases and introducing hotfixes.

The way we accept code from contributors does not change: Contributors fork
polybar, commit their changes to a new branch and open a PR to get that branch
merged.
After reviewing and approving the changes, a maintainer "merges" the PR.
"Merging" is done in the GitHub UI by either rebasing or squashing the
changes.
Regular merging is disabled because we do not want merge a merge commit for
every PR.

This document is mainly concerned with how to properly release a new version of
polybar.
For that reason this might not be of interest to you, if you are not a
maintainer, but feel free to read on anyway.

Drafting a new Release
----------------------

There a two processes for how to draft a new release. The process for major and
minor versions is the same as they both are "regular" releases.
Patch releases are triggered by bugfixes that cannot wait until the next regular
release and have a slightly different workflow.

Regular Releases (Major, Minor)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Regular releases are created once we find that ``master`` is in a stable state
and that there are enough new features to justify a new release.
A release branch ``release/X.Y.0`` is branched off of a commit on ``master``
that contains all the features we want in the release, this branch is pushed to
the official repository.
For example for version ``3.5.0`` the branch ``release/3.5.0`` would be created:

.. code-block:: shell

  git checkout -b release/3.5.0 <commit>

The release branch should typically only exist for at most a few days.

Hotfix Releases (Patch)
~~~~~~~~~~~~~~~~~~~~~~~

A hotfix release is created whenever we receive a fix for a bug that we believe
should be released immediately instead of it only being part of the next regular
release.
Generally any bugfix qualifies, but it is up to the maintainers to decide
whether a hotfix release should be created.

The hotfix release branch ``hotfix/X.Y.Z`` is created by branching off at the
previous release tag (``X.Y.Z-1``).
For example, if the latest version is ``3.5.2``, the next hotfix will be on
branch ``hotfix/3.5.3``:

.. code-block:: shell

  git checkout -b hotfix/3.5.3 3.5.2

Since the PRs for such bugfixes are often not created by maintainers, they will
often not be based on the latest release tag, but just be branched off
``master`` because contributors don't necessarily know about this branching
model and also may well not know whether a hotfix will be created for a certain
bugfix.

.. TODO create contributor page that describes where to branch off. And link to
   that page.

In case a PR containing a bugfix that is destined for a patch release is not
branched off the previous release, a maintainer creates the proper release
branch and cherry-picks the bugfix commits.

.. note::

  Alternatively, the contributor can also ``git rebase --onto`` to base the
  branch off the previous release tag. However, in most cases it makes sense for
  a maintainer to create the release branch since they will also need to add a
  `Release Commit`_ to it.

Once the release branch is created and contains the right commits, the
maintainer should follow `Publishing a new Release`_ to finish this patch
release.

If multiple bugfixes are submitted in close succession, they can all be
cherry-picked onto the same patch release branch to not create many individual
release with only a single fix.
The maintainer can also decide to leave the release branch for this patch
release open for a week in order to possibly combine multiple bugfixes into a
single release.

Publishing a new Release
------------------------

The process for publishing a release is the same for all release types. It goes
as follows:

* A `Release commit`_ is added to the tip of the release branch.
* A draft PR is opened for the release branch. This PR MUST NOT be merged in
  GitHub's interface, it is only here for review, merging happens at the
  commandline.
* After approval, the GitHub release publishing tool is used to publish the
  release and tag the tip of the release branch (the release commit).
* After the tag is created, the release branch is manually merged into
  ``master``.
  Here it is vitally important that the history of the release branch does not
  change and so we use ``git merge``. We do it manually because using ``git
  merge`` is disabled on PRs.

.. code-block:: shell

  git checkout master
  git merge <release-branch>
  git push origin

* After the tag is created, the release branch can be deleted with ``git push
  origin :<release-branch>``.
* Work through the `After-Release Checklist`_.

Here ``<release-branch>`` is either a ``release/X.Y.0`` branch or a
``hotfix/X.Y.Z`` branch.

Release Commit
~~~~~~~~~~~~~~

When merging, a release commit must be at the tip of the release branch.

The release commit needs to update the version number in:

* ``version.txt``

The commit message contains the `Changelog`_ for this release.

Changelog
~~~~~~~~~

Each release should come with a changelog briefly explaining what has changed
for the user. It should generally be separated into 'Deprecations', 'Features',
and 'Fixes', with 'Breaking Changes' listed separately at the top.

See `old releases <https://github.com/polybar/polybar/releases>`_ for how to
format the changelog.

Since major releases generally break backwards compatibility in some way, their
changelog should also prominently feature precisely what breaking changes were
introduced. If suitable, maybe even separate documentation dedicated to the
migration should be written.

After-Release Checklist
~~~~~~~~~~~~~~~~~~~~~~~

* Make sure all the new functionality is documented on the wiki
* Mark deprecated features appropriately (see `Deprecations`_)
* Remove all unreleased notes from the wiki (not for patch releases)
* Inform packagers of new release in `#1971
  <https://github.com/polybar/polybar/issues/1971>`_. Mention any dependency
  changes and any changes to the build workflow. Also mention any new files are
  created by the installation.
* Create a source archive named ``polybar-<version>.tar``.
  The repository contains a script that automates this:

.. code-block:: shell

  ./common/release-archive.sh <version>

* Update the github release with a download section that contains a link to
  ``polybar-<version>.tar`` and its sha256.
* Create a PR that updates the AUR ``PKGBUILD`` files for the ``polybar`` and
  ``polybar-git`` packages (push after the ``.tar`` file was created).


Deprecations
~~~~~~~~~~~~

If any publicly facing part of polybar is being deprecated, it should be marked
as such in the code, through warnings/errors in the log, and by comments in the
wiki. Every deprecated functionality is kept until the next major release and
removed there, unless it has not been deprecated in a minor release before.
